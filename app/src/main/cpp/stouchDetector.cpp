#include <libfreenect/libfreenect_registration.h>
#include <algorithm>
#include <pthread.h>

#include "stouchDetector.h"
#include "stouchJNI.h"
#include "stouchFreenect.h"

#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

using namespace std;
using namespace cv;

namespace STouchDetectorConst {
    const unsigned int BackgroundTrainFrameMax = 1;
    const unsigned int ROITrainFrameMax = 30;
    const unsigned short TouchDepthMin = 2;
    const unsigned short TouchDepthMax = 15;
    const unsigned int ContourAreaThreshold = 50;
    const unsigned int CannyThresh = 50;
    const unsigned int CannyThreshLevelsAmount = 5;
    const int ThreasholdOfStabilityDistance = 15;
    const unsigned int CalibrationUnstableFramesCount = 1;
    const unsigned int CalibrationStableFramesCount = 1;
}

STouchDetector::STouchDetector() {
    // Pre-allocate the memory
    depthBack.reserve(STouchFreenectDeviceConst::DepthBufferSize);
    depthMid.reserve(STouchFreenectDeviceConst::DepthBufferSize);
    depthFront.reserve(STouchFreenectDeviceConst::DepthBufferSize);
    rgbBack.reserve(STouchFreenectDeviceConst::VideoBufferSizeRGB);
    rgbMid.reserve(STouchFreenectDeviceConst::VideoBufferSizeRGB);
    rgbFront.reserve(STouchFreenectDeviceConst::VideoBufferSizeRGB);

    // At the beginning our ROI is equal to the whole screen
    roi = Rect(0, 0, FREENECT_IR_FRAME_W, FREENECT_IR_FRAME_H);
    backBufMutex = PTHREAD_MUTEX_INITIALIZER;
    frameCond = PTHREAD_COND_INITIALIZER;
    gotDepth = 0;
    gotVideo = 0;
    processDataThreadDie = false;
    fps = 0.0;
    framesCounter = 0;
    measurementMutex = PTHREAD_MUTEX_INITIALIZER;
    rgbFrontMutex = PTHREAD_MUTEX_INITIALIZER;
    argbFrontMutex = PTHREAD_MUTEX_INITIALIZER;
    depthFrontMutex = PTHREAD_MUTEX_INITIALIZER;
    calibrationMutex = PTHREAD_MUTEX_INITIALIZER;
    touchInProgressMutex = PTHREAD_MUTEX_INITIALIZER;

	depth = Mat1s(FREENECT_IR_FRAME_H, FREENECT_IR_FRAME_W); // 16 bit depth (in millimeters)
	rgb = Mat3b(FREENECT_FRAME_H, FREENECT_FRAME_W); // 24 bit RGB
    argb = Mat4b(FREENECT_FRAME_H, FREENECT_FRAME_W); // 32 bit ARGB
	foreground = Mat1i(FREENECT_FRAME_W, FREENECT_IR_FRAME_H);
	touch = Mat1b(FREENECT_FRAME_W, FREENECT_IR_FRAME_H); // touch mask
	background = Mat1i(FREENECT_IR_FRAME_H, FREENECT_FRAME_W);
    trainedBackground = Mat1i(FREENECT_IR_FRAME_H, FREENECT_FRAME_W);

    // Reset calibration parameters
    roi = Rect(0, 0, FREENECT_FRAME_W, FREENECT_FRAME_H);
    refRGB = vector<uint8_t>(STouchFreenectDeviceConst::VideoBufferSizeRGB);
    isTouchInProgress = false;
    calibrationState = COMPLETED;
}

STouchDetector::~STouchDetector() {
    // Signal and wait until the process thread stops
    processDataThreadDie = true;
    pthread_join(processDataThread, nullptr);
}

void STouchDetector::processDepth(const vector<uint16_t>& depthData) {
    pthread_mutex_lock(&backBufMutex);
    // Swap depth buffers
    depthBack.swap(depthMid);
    deviceDelegate->setDepthBuffer(depthBack);
    depthMid = vector<uint16_t>(depthData.begin(), depthData.end());
    gotDepth++;
    pthread_cond_signal(&frameCond);
    pthread_mutex_unlock(&backBufMutex);
}

void STouchDetector::processVideo(const vector<uint8_t>& videoData) {
    pthread_mutex_lock(&backBufMutex);
    // Swap video buffers
    rgbBack.swap(rgbMid);
    deviceDelegate->setVideoBuffer(rgbBack);
    rgbMid = vector<uint8_t>(videoData.begin(), videoData.end());
    gotVideo++;
    pthread_cond_signal(&frameCond);
    pthread_mutex_unlock(&backBufMutex);
}

void STouchDetector::start() {
    int res = pthread_create(&processDataThread,
                             nullptr, STouchDetector::processData, this);
    assert(!res);
    gettimeofday(&lastTimeMeasurement, nullptr);
}


void STouchDetector::convertToARGB() {
    pthread_mutex_lock(&argbFrontMutex);
    cvtColor(rgb, rgba, CV_RGB2RGBA);
    // R G B A
    // 0 1 2 3
    // A R G B
    // 0 1 2 3
    int from_to[] = { 0, 1, 1, 2, 2, 3, 3, 0 };
    mixChannels(&rgba, 1, &argb, 1, from_to, 4);
    pthread_mutex_unlock(&argbFrontMutex);
}

void STouchDetector::measureTime(int gotDepth, int gotVideo) {
    int totalFrames = gotDepth + gotVideo;
    // Time measurement
    struct timeval currentTimeMeasurement;
    gettimeofday(&currentTimeMeasurement, nullptr);
    struct timeval tvalResult;
    timersub(&currentTimeMeasurement, &lastTimeMeasurement, &tvalResult);
    framesCounter += totalFrames;
    int fps = this->fps;
    if (tvalResult.tv_sec > 0) {
        fps = framesCounter;
        framesCounter = 0;
        gettimeofday(&lastTimeMeasurement, nullptr);
    }
    pthread_mutex_lock(&measurementMutex);
    this->fps = fps;
    pthread_mutex_unlock(&measurementMutex);
}

void STouchDetector::copyBuffers(int& gotDepth, int& gotVideo) {
    gotDepth = this->gotDepth;
    gotVideo = this->gotVideo;
    // Copy buffers
    if (gotDepth) {
        pthread_mutex_lock(&depthFrontMutex);
        depthFront.swap(depthMid);
        pthread_mutex_unlock(&depthFrontMutex);
        this->gotDepth = 0;
    }
    if (gotVideo) {
        pthread_mutex_lock(&rgbFrontMutex);
        rgbFront.swap(rgbMid);
        pthread_mutex_unlock(&rgbFrontMutex);
        this->gotVideo = 0;
    }
}

void* STouchDetector::processData(void *arg) {
    STouchDetector &thiz = *(STouchDetector*)arg;

    while(!thiz.processDataThreadDie) {
        int gotDepth = 0, gotVideo = 0;
        // Acquire an access to a shared adata
        pthread_mutex_lock(&thiz.backBufMutex);
        // Wait for the next frame
        while (!thiz.gotDepth || !thiz.gotVideo)
            pthread_cond_wait(&thiz.frameCond, &thiz.backBufMutex);
        // Copy buffers
        thiz.copyBuffers(gotDepth, gotVideo);
        pthread_mutex_unlock(&thiz.backBufMutex);
        // Process video and depth data
        thiz.processFrame();
        thiz.convertToARGB();
        // Let the waiters know that the frame is ready
        STouchJNI::instance().videoDataFrameReady();
        // Measure time spent
        thiz.measureTime(gotDepth, gotVideo);
    }
    return nullptr;
}

// A helper function:
// Finds a cosine of angle between vectors
// From pt0->pt1 and from pt0->pt2
double STouchDetector::angle(Point pt1, Point pt2, Point pt0) {
    double dx1 = pt1.x - pt0.x;
    double dy1 = pt1.y - pt0.y;
    double dx2 = pt2.x - pt0.x;
    double dy2 = pt2.y - pt0.y;
    return (dx1*dx2 + dy1*dy2) / sqrt((dx1*dx1 + dy1*dy1) * (dx2*dx2 + dy2*dy2) + 1e-10);
}

// Returns sequence of squares detected on the image.
// The sequence is stored in the specified memory storage
void STouchDetector::findSquares(const Mat& image,
                                 vector<vector<Point> >& squares) {
    squares.clear();
    // Blur will enhance edge detection
    Mat timg(image);
    medianBlur(image, timg, 9);
    Mat gray0(timg.size(), CV_8U), gray;

    vector<vector<Point> > contours;

    // Take into account a red channel only. The input image which comes from the Kinect is in RGB
    // Therefore the red channel has an index 0
    int ch[] = {0, 0};
    mixChannels(&timg, 1, &gray0, 1, ch, 1);

    // Try several threshold levels
    for (int l = 0; l < STouchDetectorConst::CannyThreshLevelsAmount; l++) {
        // A hack: use Canny instead of zero threshold level.
        // Canny helps to catch squares with gradient shading
        if (l == 0) {
            // Apply Canny. Take the upper threshold from slider
            // And set the lower to 0 (which forces edges merging)
            Canny(gray0, gray, 5, STouchDetectorConst::CannyThresh, 5);
            // Dilate canny output to remove potential
            // Holes between edge segments
            dilate(gray, gray, Mat(), Point(-1, -1));
        } else {
            // Apply threshold if l!=0:
            // tgray(x,y) = gray(x,y) < (l + 1) * 255 / N ? 255 : 0
            gray = gray0 >= (l + 1) * 255 / STouchDetectorConst::CannyThreshLevelsAmount;
        }
        // find contours and store them all as a list
        findContours(gray, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

        vector<Point> approx;

        // Test each contour
        for (size_t i = 0; i < contours.size(); i++) {
            // Approximate contour with accuracy proportional
            // To the contour perimeter
            approxPolyDP(Mat(contours[i]), approx, arcLength(Mat(contours[i]), true) * 0.02,
                         true);

            // Square contours should have 4 vertices after approximation
            // Relatively large area (to filter out noisy contours)
            // And be convex.
            // Note: absolute value of an area is used because
            // Area may be positive or negative - in accordance with the
            // Contour orientation
            if (approx.size() == 4 &&
                fabs(contourArea(Mat(approx))) > 1000 &&
                isContourConvex(Mat(approx))) {
                double maxCosine = 0;

                for (int j = 2; j < 5; j++) {
                    // find the maximum cosine of the angle between joint edges
                    double cosine = fabs(angle(approx[j % 4], approx[j - 2], approx[j - 1]));
                    maxCosine = MAX(maxCosine, cosine);
                }

                // If cosines of all angles are small
                // (all angles are ~90 degree) then write quandrange
                // vertices to resultant sequence
                if (maxCosine < 0.3)
                    squares.push_back(approx);
            }
        }
    }
}

// The function draws all the squares in the image
void STouchDetector::drawSquares( Mat& image, const vector<vector<Point> >& squares ) {
    for(size_t i = 0; i < squares.size(); i++) {
        const Point* p = &squares[i][0];
        int n = (int)squares[i].size();
        // Dont detect the border
        if (p->x > 3 && p->y > 3)
            polylines(image, &p, &n, 1, true, Scalar(0, 255, 0), 3, LINE_AA);
    }
}

void STouchDetector::getVideoData(std::vector<uint8_t>& videoData) {
    pthread_mutex_lock(&argbFrontMutex);
    std::vector<uint8_t> argbFront(argb.data,
                                   argb.data + STouchFreenectDeviceConst::VideoBufferSizeARGB);

    argbFront.swap(videoData);
    pthread_mutex_unlock(&argbFrontMutex);
}

void STouchDetector::calculateBackgroundTrain() {
    // Calculate a background
    Scalar bmeanVal = mean(trainedBackground(roi));
    double bminVal = 0.0, bmaxVal = 0.0;
    minMaxLoc(trainedBackground(roi), &bminVal, &bmaxVal);

    LOGD("Background extraction completed. Average depth is %f min %f max %f",
         bmeanVal.val[0], bminVal, bmaxVal);
}

bool STouchDetector::getTouchInProgress() {
    bool ret = false;
    pthread_mutex_lock(&touchInProgressMutex);
    ret = isTouchInProgress;
    pthread_mutex_unlock(&touchInProgressMutex);
    return ret;
}

void STouchDetector::setTouchInProgress(bool isInProgress) {
    pthread_mutex_lock(&touchInProgressMutex);
    isTouchInProgress = isInProgress;
    pthread_mutex_unlock(&touchInProgressMutex);
}

void STouchDetector::startCalibration(bool start) {
    pthread_mutex_lock(&calibrationMutex);
    if (start && calibrationState == COMPLETED) {
        calibrationState = UNSTABLE;
        calibrationFrmCount = 0;
        roiCalibration = vector<Point>(4);
        background = Mat1i(FREENECT_IR_FRAME_H, FREENECT_FRAME_W);
        setTouchInProgress(false);
    } else if (!start && calibrationState != COMPLETED) {
        calibrationState = COMPLETED;
        calculateBackgroundTrain();
    }
    pthread_mutex_unlock(&calibrationMutex);
}

void STouchDetector::startTouch(bool start) {
    if (start) {
        startCalibration(false);
    }
    setTouchInProgress(start);
}

int STouchDetector::getDistance(vector<Point> v1, vector<Point> v2) {

    assert(v1.size() == v2.size());

    int maxDist = 0;
    for(auto aIt = v1.begin(), bIt = v2.begin(); aIt != v1.end(); aIt++, bIt++) {
        int dist = (int)cv::norm(*aIt - *bIt);
        if (maxDist < dist) {
            maxDist = dist;
        }
    }
    return maxDist;
}
void STouchDetector::processFrame() {

    // If simulating touch events is in progress do it
    // Go to calibration otherwise
    if (getTouchInProgress()) {
        processTouch();
    } else {
        // Get the registered RGB data (that matches the depth IR sensor view area)
        // Keep in mind that 11BIT does not have registration info
        deviceDelegate->mapRGBToDepth(depthFront, rgbFront, refRGB);
        // The image is in RGB
        rgb.data = reinterpret_cast<uchar *>(const_cast<uint8_t *>(refRGB.data()));

        vector<vector<Point> > rects;
        findSquares(rgb, rects);
        drawSquares(rgb, rects);

        pthread_mutex_lock(&calibrationMutex);
        // If calibration is in progress
        if (calibrationState != COMPLETED) {

            if (rects.size() > 0) {

                calibrationFrmCount++;

                // Find biggest rect
                int biggestContourIdx = 0;
                float biggestContourArea = 0;
                for (int i = 0; i < rects.size(); i++) {
                    float ctArea = contourArea(rects[i]);
                    if (ctArea > biggestContourArea) {
                        biggestContourArea = ctArea;
                        biggestContourIdx = i;
                    }
                }
                // Switch to an unstable state if the distance exceed the threshold value
                if (calibrationState != BACKGROUND_TRAINING &&
                    getDistance(roiCalibration, rects[biggestContourIdx]) >
                    STouchDetectorConst::ThreasholdOfStabilityDistance) {
                    calibrationState = UNSTABLE;
                    calibrationFrmCount = 0;
                }

                roiCalibration = rects[biggestContourIdx];

                // If it is stable enough amount of time
                if (calibrationState == UNSTABLE &&
                    calibrationFrmCount > STouchDetectorConst::CalibrationUnstableFramesCount) {
                    calibrationFrmCount = 0;
                    calibrationState = ROI_STABLE_TRANSITION_DELAY;
                } else if (calibrationState == ROI_STABLE_TRANSITION_DELAY &&
                           calibrationFrmCount >
                           STouchDetectorConst::CalibrationStableFramesCount) {
                    calibrationState = ROI_STABLE;
                    calibrationFrmCount = 0;
                    // Update ROI using the biggest rect
                    Point2f topLeft = roiCalibration[0];
                    Point2f bottomRight = roiCalibration[2];
                    // Save ROI for the future use (background extraction)
                    roi = Rect(topLeft, bottomRight + Point2f(1, 1));
                    STouchJNI::instance().virtualROIUpdated(topLeft.x, topLeft.y,
                                                            roi.width, roi.height);
                    LOGD("ROI finding completed. ROI top left x: %f y: %f width: %d height: %d",
                         topLeft.x, topLeft.y, roi.width, roi.height);

                } else if (calibrationState == ROI_STABLE) {
                    calibrationState = BACKGROUND_TRAINING;
                    calibrationFrmCount = 0;
                }
            }

            if (calibrationState == BACKGROUND_TRAINING) {
                if (calibrationFrmCount < STouchDetectorConst::BackgroundTrainFrameMax) {
                    depth.data = reinterpret_cast<uchar *>(const_cast<ushort *>(depthFront.data()));
                    // Accumulate depth data
                    background += depth;
                } else {
                    trainedBackground = background / calibrationFrmCount;
                    calculateBackgroundTrain();
                    calibrationState = COMPLETED;
                    calibrationFrmCount = 0;
                }
            }
        }
        pthread_mutex_unlock(&calibrationMutex);
    }
}

void STouchDetector::processTouch() {
    // Updating 16 bit depth matrix
    depth.data = reinterpret_cast<uchar *>(const_cast<ushort *>(depthFront.data()));
    // Extract foreground by simple subtraction of very basic background model
    foreground = trainedBackground - depth;
    // Find touch mask by a threshold (points that are close to background = touch points)
    touch = (foreground > STouchDetectorConst::TouchDepthMin) &
            (foreground < STouchDetectorConst::TouchDepthMax);
    // Applying ROI
    Mat touchRoi = touch(roi);
    // Finding touch points
    vector< vector<Point2i> > contours;
    vector<Point2i> touchPoints;
    findContours(touchRoi, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, roi.tl());
    // Extracting the big enough areas
    for(int i = 0; i < contours.size(); i++) {
        Mat contourMat(contours[i]);
        // Find touch points with an area which is more than a threshold value
        if ( contourArea(contourMat) > STouchDetectorConst::ContourAreaThreshold ) {
            Scalar center = mean(contourMat);
            Point2i touchPoint(center[0], center[1]);
            touchPoints.push_back(touchPoint);
        }
    }
    // Sending simulated touch events
    for(int i = 0; i < touchPoints.size(); i++) {
        auto touchPoint = touchPoints[i];
        LOGD("Touch detected %d %d", touchPoint.x, touchPoint.y);
        // TODO improve tracking (don't move cursors away, that might be close to another touch point)
        STouchJNI::instance().sendEvent(touchPoint.x, touchPoint.y);
    }
}
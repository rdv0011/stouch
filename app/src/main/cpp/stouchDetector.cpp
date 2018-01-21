#include <libfreenect/libfreenect_registration.h>
#include <algorithm>
#include <pthread.h>

#include "stouchDetector.h"
#include "stouchFreenect.h"

#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

using namespace std;
using namespace cv;

const unsigned int BackgroundTrainFrameMax = 30;
const unsigned int ROITrainFrameMax = 30;
const unsigned short TouchDepthMin = 2;
const unsigned short TouchDepthMax = 15;
const unsigned int ContourAreaThreshold = 50;

//////// Depth gamma
uint16_t t_gamma[2048];
uint8_t *depth_mid;
/////////////

extern void virtualROIUpdated(int xVirtualOffset, int yVirtualOffset,
								int virtualWidth, int virtualHeight);
extern void sendEvent(int x, int y);


///////////////////////////////////
void convertDepth(void *v_depth) {
    uint16_t *depth = (uint16_t*)v_depth;
    for (int i=0; i<640*480; i++) {
        int pval = t_gamma[depth[i]];
        int lb = pval & 0xff;
        switch (pval>>8) {
            case 0:
                depth_mid[3*i+0] = 255;
                depth_mid[3*i+1] = 255-lb;
                depth_mid[3*i+2] = 255-lb;
                break;
            case 1:
                depth_mid[3*i+0] = 255;
                depth_mid[3*i+1] = lb;
                depth_mid[3*i+2] = 0;
                break;
            case 2:
                depth_mid[3*i+0] = 255-lb;
                depth_mid[3*i+1] = 255;
                depth_mid[3*i+2] = 0;
                break;
            case 3:
                depth_mid[3*i+0] = 0;
                depth_mid[3*i+1] = 255;
                depth_mid[3*i+2] = lb;
                break;
            case 4:
                depth_mid[3*i+0] = 0;
                depth_mid[3*i+1] = 255-lb;
                depth_mid[3*i+2] = 255;
                break;
            case 5:
                depth_mid[3*i+0] = 0;
                depth_mid[3*i+1] = 0;
                depth_mid[3*i+2] = 255-lb;
                break;
            default:
                depth_mid[3*i+0] = 0;
                depth_mid[3*i+1] = 0;
                depth_mid[3*i+2] = 0;
                break;
        }
    }
}
////////////////////////////

STouchDetector::STouchDetector() {

    // Pre-allocate the memory
    depthBack.reserve(STouchFreenectDevice::DepthBufferSize);
    depthMid.reserve(STouchFreenectDevice::DepthBufferSize);
    depthFront.reserve(STouchFreenectDevice::DepthBufferSize);
    rgbBack.reserve(STouchFreenectDevice::VideoBufferSize);
    rgbMid.reserve(STouchFreenectDevice::VideoBufferSize);
    rgbFront.reserve(STouchFreenectDevice::VideoBufferSize);

    // At the beginning our ROI is equal to the whole screen
    roi = Rect(0, 0, FREENECT_IR_FRAME_W, FREENECT_IR_FRAME_H);
    backBufMutex = PTHREAD_MUTEX_INITIALIZER;
    frameCond = PTHREAD_COND_INITIALIZER;
    gotDepth = 0;
    gotRGB = 0;
    processDataThreadDie = false;

    /*
	depth = Mat1s(FREENECT_IR_FRAME_H, FREENECT_IR_FRAME_W); // 16 bit depth (in millimeters)
	rgb = Mat3b(FREENECT_FRAME_H, FREENECT_FRAME_W); // 24 bit rgb
	foreground = Mat1i(FREENECT_FRAME_W, FREENECT_IR_FRAME_H);
	touch = Mat1b(FREENECT_FRAME_W, FREENECT_IR_FRAME_H); // touch mask
	background = Mat1i(FREENECT_IR_FRAME_H, FREENECT_FRAME_W);

    // Four points of ROI
    roiBuffer = vector<Point2f>(4);

	frmCount = 0;
    // Let's do a background train
    roiTrain = false;
    roi = Rect(0, 0, FREENECT_FRAME_W, FREENECT_FRAME_H);

    /////////////////////////////////////
    depth_mid = (uint8_t*)malloc(640*480*3);

    for (int i=0; i<2048; i++) {
        float v = i/2048.0;
        v = powf(v, 3)* 6;
        t_gamma[i] = v*6*256;
    }
    ///////////////////////
     */
}

STouchDetector::~STouchDetector() {
    // Signal and wait untill the process thread stops
    processDataThreadDie = true;
    pthread_join(processDataThread, NULL);
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
    gotRGB++;
    pthread_cond_signal(&frameCond);
    pthread_mutex_unlock(&backBufMutex);
}

void STouchDetector::start() {
    int res = pthread_create(&processDataThread,
                             NULL, STouchDetector::processData, this);
    assert(res);
}

void* STouchDetector::processData(void *arg) {
    STouchDetector &ref = *(STouchDetector*)arg;
    while(!ref.processDataThreadDie) {
        // Acquire an access to a shared adata
        pthread_mutex_lock(&ref.backBufMutex);
        while (!ref.gotDepth || !ref.gotRGB)
            pthread_cond_wait(&ref.frameCond, &ref.backBufMutex);
        // Copy buffers
        if (ref.gotDepth) {
            ref.depthFront.swap(ref.depthMid);
            ref.gotDepth = 0;
        }
        if (ref.gotRGB) {
            ref.rgbFront.swap(ref.rgbMid);
            ref.gotRGB = 0;
        }
        pthread_mutex_unlock(&ref.backBufMutex);
        // Process video and depth data
    }
    return nullptr;
}

/*
void videoFunc() {

    if (!roiTrain) return;

    frmCount++;

    if (frmCount < ROITrainFrameMax) {
        vector<uint8_t> refRGB(STouchFreenectDevice::VideoBufferSize);
        // Get the registered RGB data (that matches the depth IR sensor view area)
        // Keep in mind that 11BIT does not have registration info
        //mapRGBToDepth(depthBuffer, videoData, refRGB);

        //rgb.data = reinterpret_cast<uchar*>(const_cast<uint8_t*>(refRGB.data()));
        rgb.data = reinterpret_cast<uchar*>(const_cast<uint8_t*>(videoData.data()));

        Mat mask;

        Mat rgbInv = ~rgb;
        Mat hsvInv;
        cvtColor(rgbInv, hsvInv, CV_RGB2HSV);
        inRange(hsvInv, Scalar(90, 113, 81), Scalar(255, 255, 255), mask);

        //Mat gray;
        //cvtColor(rgb, gray, CV_RGB2GRAY);
        //adaptiveThreshold(gray, mask, 255, ADAPTIVE_THRESH_GAUSSIAN_C, THRESH_BINARY, 11, 2);

        //////////// saving the image
        cvtColor(rgb, hsvInv, CV_BGR2RGB);
        //bool ret = imwrite("/data/data/com.github.rdv0011.stouch/stouch-video.jpg", hsvInv);
        //////////////////////////

        vector<vector<Point>> contours;
        findContours(mask, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);

        int biggestContourIdx = -1;
        float biggestContourArea = 0;
        for( int i = 0; i < contours.size(); i++ ) {
            float ctArea= contourArea(contours[i]);
            if(ctArea > biggestContourArea) {
                biggestContourArea = ctArea;
                biggestContourIdx = i;
            }
        }
        // If no contour found
        if(biggestContourIdx < 0) {
            LOGD("No contour found. Let's step back and try again.");
            // Let's step back and do it again
            frmCount -= 1;
            return;
        }
        // Compute the rotated bounding rect of the biggest contour! (this is the part that does what you want/need)
        RotatedRect boundingBox = minAreaRect(contours[biggestContourIdx]);
        vector<Point2f> corners = vector<Point2f>(4);
        // The order is bottomLeft, topLeft, topRight, bottomRight
        boundingBox.points(corners.data());
        // Accumulate bounding box coordinates to calcualte an average later
        std::transform(roiBuffer.begin(), roiBuffer.end(),
                       corners.begin(), roiBuffer.begin(),
                       std::plus<Point2f>());
    } else {
        // Calculate an average and set ROI
        Point2f topLeft = roiBuffer[2] / (float)ROITrainFrameMax;
        Point2f bottomRight = roiBuffer[0] / (float)ROITrainFrameMax;
        roi = Rect(topLeft, bottomRight + Point2f(1, 1));
        virtualROIUpdated(topLeft.x, topLeft.y, roi.width, roi.height);
        LOGD("ROI finding completed. ROI top left x: %f y: %f width: %d height: %d",
             topLeft.x, topLeft.y, roi.width, roi.height);
        // Stop ROI training
        roiTrain = false;
        // At this step the background training has already been done
        // Thus let's force it to pass with doing nothing
        frmCount = BackgroundTrainFrameMax;
    }
}
*/

/*
//    if (depthBuffer.size() == 0) {
//        depthBuffer = depthData;
//    }

    if (roiTrain) return;

    frmCount++;

	// Create background model (average depth)
	if (frmCount < BackgroundTrainFrameMax) {
        // Despite it looks ugly this cast should be safe in this particular case
        depth.data = reinterpret_cast<uchar*>(const_cast<ushort*>(depthData.data()));
        background += depth;

        ///////////////////////////////////////////
        //convertDepth((void*)depthData.data());
        //Mat3b convertedDepth = Mat3b(FREENECT_FRAME_H, FREENECT_FRAME_W);
        //convertedDepth.data = depth_mid;
        //bool ret = imwrite("/data/data/com.github.rdv0011.stouch/stouch-depth.jpg", convertedDepth);
        /////////////////////////////

	} else {

		if (frmCount == BackgroundTrainFrameMax) {
			background /= BackgroundTrainFrameMax;
			Scalar bmeanVal = mean(background(roi));
	        double bminVal = 0.0, bmaxVal = 0.0;
	        minMaxLoc(background(roi), &bminVal, &bmaxVal);
	        LOGD("Background extraction completed. Average depth is %f min %f max %f",
                 bmeanVal.val[0], bminVal, bmaxVal);
            // Let's continue to do ROI training
            roiTrain = true;
            frmCount = 0;
            return;
		}

		// update 16 bit depth matrix
		depth.data = (uchar*)(&depthData);
		// extract foreground by simple subtraction of very basic background model
		foreground = background - depth;

		// find touch mask by a threshold (points that are close to background = touch points)
		touch = (foreground > TouchDepthMin) & (foreground < TouchDepthMax);

		// Extract ROI
		Mat touchRoi = touch(roi);

		// Find touch points
		vector< vector<Point2i> > contours;
		vector<Point2i> touchPoints;
        findContours(touchRoi, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, roi.tl());

        for(int i = 0; i < contours.size(); i++) {
            Mat contourMat(contours[i]);
            // Find touch points with an area which is more than a threshold value
            if ( contourArea(contourMat) > ContourAreaThreshold ) {
                Scalar center = mean(contourMat);
                Point2i touchPoint(center[0], center[1]);
                touchPoints.push_back(touchPoint);
            }
        }

        for(int i = 0; i < touchPoints.size(); i++) {
            auto touchPoint = touchPoints[i];
			LOGD("Touch detected %d %d", touchPoint.x, touchPoint.y);
			// TODO improve tracking (don't move cursors away, that might be close to another touch point)
            sendEvent(touchPoint.x, touchPoint.y);
		}
	}

    // Stop the frame counter on this value
    // It was needed for the training only (finding a ROI and a background)
    frmCount = frmCount > BackgroundTrainFrameMax + 1 ? BackgroundTrainFrameMax + 1: frmCount;
}
*/

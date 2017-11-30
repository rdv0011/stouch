#include <libfreenect/libfreenect_registration.h>
#include <algorithm>
#include "stouchDetector.h"
#include "stouchFreenect.hpp"

#include "opencv2/imgproc.hpp"

using namespace std;
using namespace cv;

const unsigned int BackgroundTrainFrameMax = 30;
const unsigned int ROITrainFrameMax = 30;
const unsigned short TouchDepthMin = 2;
const unsigned short TouchDepthMax = 15;
const unsigned int ContourAreaThreshold = 50;

extern void virtualROIUpdated(int xVirtualOffset, int yVirtualOffset,
								int virtualWidth, int virtualHeight);
extern void sendEvent(int x, int y);

STouchDetector::STouchDetector() {
    // At the beginning our ROI is equal to the whole screen
	xMin = 0;
	xMax = FREENECT_IR_FRAME_W - 1;
	yMin = 0;
	yMax = FREENECT_IR_FRAME_H - 1;

	depth = Mat1s(FREENECT_IR_FRAME_H, FREENECT_IR_FRAME_W); // 16 bit depth (in millimeters)
	rgb = Mat3b(FREENECT_FRAME_H, FREENECT_FRAME_W); // 8 bit depth
	foreground = Mat1i(FREENECT_FRAME_W, FREENECT_IR_FRAME_H);
	touch = Mat1b(FREENECT_FRAME_W, FREENECT_IR_FRAME_H); // touch mask
	background = Mat1i(FREENECT_IR_FRAME_H, FREENECT_FRAME_W);

    // Four points of ROI
    roiBuffer = vector<Point2f>(4);

	frmCount = 0;
    // Let's do a background train
    roiTrain = false;
}

STouchDetector::~STouchDetector() {
}

void STouchDetector::processVideo(const vector<uint8_t>& videoData) {

    if (!roiTrain) return;

    frmCount++;

    if (frmCount < ROITrainFrameMax) {
        //rgb.data = static_cast<uchar*>(videoData);

        // Get the registered RGB data (that matches the depth IR sensor view area)
        uint16_t* ptrBckgr = (uint16_t*)background.data;
        vector<uint16_t> refBackground(ptrBckgr, ptrBckgr + FREENECT_VIDEO_IR_10BIT_SIZE);
        uint8_t* ptrRGB = (uint8_t*)rgb.data;
        vector<uint8_t> refRGB(ptrRGB, ptrRGB + FREENECT_VIDEO_RGB_SIZE);
        mapRGBToDepth(refBackground, videoData, refRGB);

        Mat gray;
        cvtColor(rgb, gray, CV_RGB2GRAY);
        Mat mask;
        threshold(gray, mask, 0, 255, CV_THRESH_BINARY_INV | CV_THRESH_OTSU);
        vector<vector<Point>> contours;
        vector<Vec4i> hierarchy;
        findContours(mask,contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE);
        int biggestContourIdx = -1;
        float biggestContourArea = 0;
        for( int i = 0; i < contours.size(); i++ ) {
            float ctArea= contourArea(contours[i]);
            if(ctArea > biggestContourArea) {
                biggestContourArea = ctArea;
                biggestContourIdx = i;
            }
        }
        // if no contour found
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
                       corners.begin(), corners.begin(),
                       std::plus<Point2f>());
    } else {
        // Calculate an average and set ROI
        Point2f topLeft = roiBuffer[1] / (float)ROITrainFrameMax;
        Point2f bottomRight = roiBuffer[3] / (float)ROITrainFrameMax;
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

void STouchDetector::processDepth(const vector<uint16_t>& depthData) {

    if (roiTrain) return;

    frmCount++;

	// create background model (average depth)
	if (frmCount < BackgroundTrainFrameMax) {

        // Despite it looks ugly this cast should be safe in this particular case
        depth.data = reinterpret_cast<uchar*>(const_cast<ushort*>(depthData.data()));
        background += depth;
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
        findContours(touchRoi, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, Point2i(xMin, yMin));

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
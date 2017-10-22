#include "stouchDetector.h"
#include "stouchFreenect.hpp"
#include "stouchEventInjector.h"

#include "opencv2/imgproc.hpp"

using namespace std;
using namespace TUIO;
using namespace cv;

const unsigned int BackgroundTrain = 30;
const unsigned short TouchDepthMin = 2;
const unsigned short TouchDepthMax = 15;
const unsigned int ContourAreaThreshold = 50;

STouchDetector::STouchDetector() {
	xMin = 140;
	xMax = 532;
	yMin = 50;
	yMax = 297;

	depth = Mat1s(480, 640); // 16 bit depth (in millimeters)
	depth8 = Mat1b(480, 640); // 8 bit depth
	rgb = Mat3b(480, 640); // 8 bit depth
	debug = Mat3b(480, 640); // debug visualization
	foreground = Mat1s(640, 480);
	touch = Mat1b(640, 480); // touch mask
	background = Mat1s(480, 640);
	buffer = vector<Mat1s>(BackgroundTrain);
	roi = Rect(xMin, yMin, xMax - xMin, yMax - yMin);
	// TUIO server object
	tuio = nullptr;
	eventInjector = new STouchEventInjector(xMax - xMin, yMax - yMin);
	frmCount = 0;
}

STouchDetector::~STouchDetector() {
	delete eventInjector;
}

void STouchDetector::average(vector<Mat1s>& frames, Mat1s& mean) {
    Mat1d acc(mean.size());
    Mat1d frame(mean.size());
    for (unsigned int i=0; i<frames.size(); i++) {
        frames[i].convertTo(frame, CV_64FC1);
        acc = acc + frame;
    }
    acc = acc / frames.size();
    acc.convertTo(mean, CV_16SC1);
}

void STouchDetector::process(const uint16_t& depthData) {
	frmCount++;
	// create background model (average depth)
	if (frmCount < BackgroundTrain) {
		depth.data = (uchar*)(&depthData);
		buffer[frmCount] = depth;
	}
	else {
		if (frmCount == BackgroundTrain) {
			average(buffer, background);
			Scalar bmeanVal = mean(background(roi));
	        double bminVal = 0.0, bmaxVal = 0.0;
	        minMaxLoc(background(roi), &bminVal, &bmaxVal);
	        LOGD("Background extraction completed. Average depth is %f min %f max %f", bmeanVal.val[0], bminVal, bmaxVal);
		}

		if (!tuio) {
			try {
				// Connect to local TUIO server
				//tuio = new TuioServer("192.168.0.2",3333,false);
				tuio = new TuioServer();
				LOGD("TUIO server was started.");
			}
			catch(std::exception &e) {
				LOGD("Couldn't create TUIO server.");
			}
		}
		// update 16 bit depth matrix
		depth.data = (uchar*)(&depthData);
		// extract foreground by simple subtraction of very basic background model
		foreground = background - depth;

		// find touch mask by thresholding (points that are close to background = touch points)
		touch = (foreground > TouchDepthMin) & (foreground < TouchDepthMax);

		// extract ROI
		Mat touchRoi = touch(roi);

		// find touch points
		vector< vector<Point2i> > contours;
		vector<Point2f> touchPoints;
        findContours(touchRoi, contours, CV_RETR_LIST, CV_CHAIN_APPROX_SIMPLE, Point2i(xMin, yMin));

        for (unsigned int i=0; i < contours.size(); i++) {
			Mat contourMat(contours[i]);
			// find touch points by area thresholding
            if ( contourArea(contourMat) > ContourAreaThreshold ) {
				Scalar center = mean(contourMat);
				Point2i touchPoint(center[0], center[1]);
				touchPoints.push_back(touchPoint);
			}
		}

		// send TUIO cursors
        tuioTime = TuioTime::getSessionTime();
        tuio->initFrame(tuioTime);

		for (unsigned int i=0; i < touchPoints.size(); i++) { // touch points
			float cursorX = (touchPoints[i].x - xMin) / (xMax - xMin);
            float cursorY = 1 - (touchPoints[i].y - yMin) / (yMax - yMin);
			TuioCursor* cursor = tuio->getClosestTuioCursor(cursorX,cursorY);

			LOGD("Touch detected %d %d", (int)touchPoints[i].x, (int)touchPoints[i].y);

			// TODO improve tracking (don't move cursors away, that might be close to another touch point)
            if (cursor == nullptr || cursor->getTuioTime() == tuioTime) {
				tuio->addTuioCursor(cursorX,cursorY);
				eventInjector->sendEventToTouchDevice((int)(touchPoints[i].x - xMin),
														(int)(touchPoints[i].y - yMin));
				LOGD("TUIO cursor was added at %d %d", (int)touchPoints[i].x, (int)touchPoints[i].y);
			} else {
				tuio->updateTuioCursor(cursor, cursorX, cursorY);
			}
		}

		tuio->stopUntouchedMovingCursors();
		tuio->removeUntouchedStoppedCursors();
		tuio->commitFrame();
	}
}

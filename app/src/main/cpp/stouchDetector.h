#ifndef _STOUCH_DETECTOR_H_
#define _STOUCH_DETECTOR_H_

#include <vector>
// openCV
#include <opencv2/core.hpp>
#include <opencv/highgui.h>
// TUIO
#include "TuioServer.h"

class STouchEventInjector;

class STouchDetector {
public:
	STouchDetector();
	~STouchDetector();
	void process(const uint16_t& depthData);

protected:
	void average(std::vector<cv::Mat1s>& frames, cv::Mat1s& mean);
	
private:
	int xMin, xMax, yMin, yMax;
	cv::Mat1s depth; // 16 bit depth (in millimeters)
	cv::Mat1b depth8; // 8 bit depth
	cv::Mat3b rgb; // 8 bit depth
	cv::Mat3b debug; // debug visualization
	cv::Mat1s foreground;
	cv::Mat1b touch; // touch mask
	cv::Mat1s background;
	std::vector<cv::Mat1s> buffer;
	cv::Rect roi;
	// TUIO server object
	TUIO::TuioServer* tuio;
	TUIO::TuioTime tuioTime;
	STouchEventInjector* eventInjector;
	int64_t frmCount;
};
#endif //_STOUCH_DETECTOR_H_

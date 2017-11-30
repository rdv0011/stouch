#ifndef _STOUCH_DETECTOR_H_
#define _STOUCH_DETECTOR_H_

#include <vector>
// openCV
#include <opencv2/core.hpp>
#include <opencv/highgui.h>

class STouchDetector {
public:
	STouchDetector();
	~STouchDetector();
	void processVideo(const std::vector<uint8_t>& videoData);
	void processDepth(const std::vector<uint16_t>& depthData);
    std::function<void(const std::vector<uint16_t>&,
                       const std::vector<uint8_t>&,
                       const std::vector<uint8_t>&)> mapRGBToDepth;

protected:
	void average(std::vector<cv::Mat1s>& frames, cv::Mat1s& mean);
	void findRect();
	
private:
	int xMin, xMax, yMin, yMax;
	cv::Mat1s depth; // 16 bit depth (in millimeters)
	cv::Mat3b rgb; // rgb data
	cv::Mat1i foreground;
	cv::Mat1b touch; // touch mask
	cv::Mat1i background;
    std::vector<cv::Point2f> roiBuffer;
	cv::Rect roi;
	int64_t frmCount;
    bool roiTrain;
};
#endif //_STOUCH_DETECTOR_H_
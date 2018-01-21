#ifndef _STOUCH_DETECTOR_H_
#define _STOUCH_DETECTOR_H_

#include <vector>
#include <opencv2/core.hpp>

class STouchFreenectDeviceProtocol;

class STouchDataProcessProtocol {
public:
    virtual void processVideo(const std::vector<uint8_t>&) = 0;
    virtual void processDepth(const std::vector<uint16_t>&) = 0;
};

class STouchDetector: public STouchDataProcessProtocol {
public:
	STouchDetector();
	~STouchDetector();
    virtual void processVideo(const std::vector<uint8_t>& videoData);
    virtual void processDepth(const std::vector<uint16_t>& depthData);
    void start();

protected:
	void average(std::vector<cv::Mat1s>& frames, cv::Mat1s& mean);
	void findRect();
    static void* processData(void *arg);
	
public:
    STouchFreenectDeviceProtocol *deviceDelegate;

protected:
	std::vector<uint8_t> rgbBack, rgbMid, rgbFront;
	std::vector<uint16_t> depthBack, depthMid, depthFront;
	pthread_mutex_t backBufMutex;
    pthread_cond_t frameCond;
    int gotRGB;
    int gotDepth;
    pthread_t processDataThread;
    volatile bool processDataThreadDie;

    cv::Rect roi;
    int64_t frmCount;
    bool roiTrain;

	cv::Mat1s depth; // 16 bit depth (in millimeters)
	cv::Mat3b rgb; // rgb data
	cv::Mat1i foreground;
	cv::Mat1b touch; // touch mask
	cv::Mat1i background;
    std::vector<cv::Point2f> roiBuffer;
};
#endif //_STOUCH_DETECTOR_H_
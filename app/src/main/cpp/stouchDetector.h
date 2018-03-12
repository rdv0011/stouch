#pragma once

#include <vector>
#include <opencv2/core.hpp>

class STouchFreenectDeviceProtocol;

class STouchDataProcessProtocol {
public:
    virtual void processVideo(const std::vector<uint8_t>&) = 0;
    virtual void processDepth(const std::vector<uint16_t>&) = 0;
};

enum STouchDetectorCalibrationState {
    UNSTABLE,
    ROI_STABLE_TRANSITION_DELAY,
    ROI_STABLE,
    BACKGROUND_TRAINING,
    COMPLETED
};

class STouchDetector: public STouchDataProcessProtocol {
public:
	STouchDetector();
	~STouchDetector();
    virtual void processVideo(const std::vector<uint8_t>& videoData);
    virtual void processDepth(const std::vector<uint16_t>& depthData);
    void start();
    int getFPS() {
        int ret;
        pthread_mutex_lock(&measurementMutex);
        ret = fps;
        pthread_mutex_unlock(&measurementMutex);
        return ret;
    }
    void getVideoData(std::vector<uint8_t>& videoData);
	void startCalibration(bool start = true);
    STouchDetectorCalibrationState getCalibrationState() {
        STouchDetectorCalibrationState ret;
        pthread_mutex_lock(&calibrationMutex);
        ret = calibrationState;
        pthread_mutex_unlock(&calibrationMutex);
        return ret;
    }
    int getCalibrationFrameCount() {
        int frm = 0;
        pthread_mutex_lock(&calibrationMutex);
        frm = calibrationFrmCount;
        pthread_mutex_unlock(&calibrationMutex);
        return frm;
    }
	void startTouch(bool start = true);

private:
	void average(std::vector<cv::Mat1s>& frames, cv::Mat1s& mean);
	void findRect();
    static void* processData(void *arg);
	void convertToARGB();
    void measureTime(int gotDepth, int gotVideo);
    void copyBuffers(int& gotDepth, int& gotVideo);
    void processFrame();
    void drawSquares(cv::Mat& image, const std::vector<std::vector<cv::Point> >& squares);
    void findSquares(const cv::Mat& image, std::vector<std::vector<cv::Point> >& squares);
    double angle(cv::Point pt1, cv::Point pt2, cv::Point pt0);
	void calculateBackgroundTrain();
    int getDistance(std::vector<cv::Point> v1, std::vector<cv::Point> v2);
    void processTouch();
    bool getTouchInProgress();
    void setTouchInProgress(bool);

public:
    STouchFreenectDeviceProtocol *deviceDelegate;

private:
    // Frame processing
    cv::Mat1s depth; // 16 bit depth (in millimeters)
    cv::Mat3b rgb; // RGB data
    cv::Mat4b rgba; // RGBA data
    cv::Mat4b argb;
	std::vector<uint8_t> rgbBack;
    std::vector<uint8_t> rgbMid;
    std::vector<uint8_t>  rgbFront;
	std::vector<uint16_t> depthBack;
    std::vector<uint16_t> depthMid;
    std::vector<uint16_t> depthFront;
	pthread_mutex_t backBufMutex;
    pthread_cond_t frameCond;
    pthread_mutex_t rgbFrontMutex;
	pthread_mutex_t argbFrontMutex;
    pthread_mutex_t depthFrontMutex;
    int gotVideo;
    int gotDepth;
    pthread_t processDataThread;
    volatile bool processDataThreadDie;
    int framesCounter;
    int fps;
    struct timeval lastTimeMeasurement;
    pthread_mutex_t measurementMutex;

    // Calibration
    std::vector<uint8_t> refRGB;
    cv::Rect roi;
    int64_t calibrationFrmCount;
    STouchDetectorCalibrationState calibrationState;
    pthread_mutex_t calibrationMutex;
    std::vector<cv::Point> roiCalibration;

    // Touch detection and simulation
	volatile bool isTouchInProgress;
	cv::Mat1i foreground;
	cv::Mat1b touch; // touch mask
	cv::Mat1i background;
	cv::Mat1i trainedBackground;
    pthread_mutex_t touchInProgressMutex;
};
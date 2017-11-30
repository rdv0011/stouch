// Kinect driver
#include <list>

#include "libfreenect.hpp"
#include "stouchDetector.h"

using namespace std::placeholders;

#if defined(ANDROID)
#include <android/log.h>
#  define  LOGD(x...)  __android_log_print(ANDROID_LOG_INFO,"STouch",x)
#  define  LOGE(x...)  __android_log_print(ANDROID_LOG_ERROR,"STouch",x)
#else
#  define  LOGD(...)  do {} while (0)
#  define  LOGE(...)  do {} while (0)
#endif

class STouchFreenectDevice: public Freenect::FreenectDevice {
public:
	STouchFreenectDevice(freenect_context *_ctx, int _index):FreenectDevice(_ctx, _index) {
        detector.mapRGBToDepth = std::bind(&Freenect::FreenectDevice::mapRGBToDepth, this, _1, _2, _3);
	}

	virtual void VideoCallback(void *video, uint32_t timestamp) {
        uint8_t *ptrVideo = static_cast<uint8_t*>(video);
        std::vector<uint8_t> refVideo(ptrVideo, ptrVideo + FREENECT_VIDEO_RGB_SIZE);
        detector.processVideo(refVideo);
	}

	virtual void DepthCallback(void *depth, uint32_t timestamp) {
		uint16_t *ptrDepth = static_cast<uint16_t*>(depth);
        std::vector<uint16_t> refDepth(ptrDepth, ptrDepth + FREENECT_VIDEO_IR_10BIT_SIZE);
		detector.processDepth(refDepth);
	}
private:
	STouchDetector detector;
};

class STouchFreenect: public Freenect::Freenect {
public:
	STouchFreenect() {}
	~STouchFreenect() {}

	bool init() {
	    bool success = true;
	    try {
	    	if (deviceCount() < 1) {
	    		LOGE("Can't find freenect device");
	    		success = false;
	    	}
	    	else {
	    		STouchFreenectDevice& device = createDevice<STouchFreenectDevice>(0);
	    		device.setDepthFormat(FREENECT_DEPTH_MM);
				device.setVideoFormat(FREENECT_VIDEO_RGB);
	    		start();
	    		LOGD("Freenect successfully initialized");
	    	}
	    }
	    catch(std::runtime_error& err) {
	    	success = false;
	    	LOGE("Freenect initialization error %s", err.what());
	    }
	    return success;
	}
};

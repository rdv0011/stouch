// Kinect driver
#include "libfreenect.hpp"
#include "stouchDetector.h"

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
	}

	virtual void VideoCallback(void *video, uint32_t timestamp) {
	}

	virtual void DepthCallback(void *depth, uint32_t timestamp) {
		uint16_t *refDepth = static_cast<uint16_t*>(depth);
		detector.process(*refDepth);
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

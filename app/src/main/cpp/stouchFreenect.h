#pragma once

// Kinect driver
#include <list>

#include "libfreenect.hpp"
#include "stouchDetector.h"

extern void freenect_map_rgb_to_depth(freenect_device* dev, uint16_t* depth_mm, uint8_t* rgb_raw, uint8_t* rgb_registered);

using namespace std::placeholders;

#if defined(ANDROID)
#include <android/log.h>

#  define  LOGD(x...)  __android_log_print(ANDROID_LOG_INFO,"STouch",x)
#  define  LOGE(x...)  __android_log_print(ANDROID_LOG_ERROR,"STouch",x)
#else
#  define  LOGD(...)  do {} while (0)
#  define  LOGE(...)  do {} while (0)
#endif

namespace STouchFreenectDeviceConst {
    static const int VideoBufferSizeRGB = FREENECT_VIDEO_RGB_SIZE;
    static const int VideoBufferSizeARGB = FREENECT_FRAME_PIX * 4; // 4 bytes per pixel
    static const int DepthBufferSize = FREENECT_DEPTH_11BIT_SIZE;
    static const freenect_video_format VideoFormat = FREENECT_VIDEO_RGB;
    static const freenect_depth_format DepthFormat = FREENECT_DEPTH_MM;
}

class STouchFreenectDeviceProtocol {
public:
	virtual void setVideoBuffer(const std::vector<uint8_t>&) = 0;
	virtual void setDepthBuffer(const std::vector<uint16_t>&) = 0;
    virtual void mapRGBToDepth(const std::vector<uint16_t>&,
                               const std::vector<uint8_t>&,
                               const std::vector<uint8_t>&) = 0;
};

class STouchFreenectDevice: public Freenect::FreenectDevice, public STouchFreenectDeviceProtocol {
public:
    friend class STouchFreenect;

public:
    STouchFreenectDevice(freenect_context *_ctx, int _index);

	virtual void VideoCallback(void *video, uint32_t timestamp) {
        uint8_t *ptrVideo = static_cast<uint8_t*>(video);
        std::vector<uint8_t> refVideo(ptrVideo, ptrVideo + STouchFreenectDeviceConst::VideoBufferSizeRGB);
        dataProcessDelegate->processVideo(refVideo);
	}

	virtual void DepthCallback(void *depth, uint32_t timestamp) {
		uint16_t *ptrDepth = static_cast<uint16_t*>(depth);
        std::vector<uint16_t> refDepth(ptrDepth, ptrDepth + STouchFreenectDeviceConst::DepthBufferSize);
        dataProcessDelegate->processDepth(refDepth);
	}

	virtual void setVideoBuffer(const std::vector<uint8_t>& videoData) {
        freenect_set_video_buffer(m_dev, const_cast<uint8_t*>(videoData.data()));
	}

	virtual void setDepthBuffer(const std::vector<uint16_t>& depthData) {
        freenect_set_depth_buffer(m_dev, const_cast<uint16_t*>(depthData.data()));
	}

    virtual void mapRGBToDepth(const std::vector<uint16_t>& depth_mm,
                               const std::vector<uint8_t>& rgb_raw,
                               const std::vector<uint8_t>& rgb_registered) {
        freenect_map_rgb_to_depth(m_dev, const_cast<uint16_t*>(depth_mm.data()),
                                  const_cast<uint8_t*>(rgb_raw.data()),
                                  const_cast<uint8_t*>(rgb_registered.data()));
    }
private:
    STouchDataProcessProtocol *dataProcessDelegate;
};

class STouchFreenect: public Freenect::Freenect {
public:
	STouchFreenect() {}
	~STouchFreenect() {}

	bool init(STouchDataProcessProtocol *delegate, STouchFreenectDevice*& device) {
	    bool success = true;
	    try {
	    	if (deviceCount() < 1) {
	    		LOGE("Can't find freenect device");
	    		success = false;
	    	}
	    	else {
	    		device = &createDevice<STouchFreenectDevice>(0);
                device->dataProcessDelegate = delegate;
	    		LOGD("Freenect successfully initialized");
	    	}
	    }
	    catch(std::runtime_error& err) {
	    	success = false;
	    	LOGE("Freenect initialization error %s", err.what());
	    }
	    return success;
	}

    void start(STouchFreenectDevice& device) {
        device.setDepthFormat(STouchFreenectDeviceConst::DepthFormat);
        device.setVideoFormat(STouchFreenectDeviceConst::VideoFormat);
        Freenect::Freenect::start();
    }
};

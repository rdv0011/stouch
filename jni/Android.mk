LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

#opencv libraries - static links
OPENCV_LIB_TYPE := STATIC
include $(OPENCV)/sdk/native/jni/OpenCV.mk

LOCAL_C_INCLUDES += $(LOCAL_PATH)/libusb $(LOCAL_PATH)/libfreenect \
	$(LOCAL_PATH)/TUIO $(LOCAL_PATH)/oscpack

#libusb files
LOCAL_SRC_FILES := libusb/core.c libusb/descriptor.c libusb/hotplug.c \
					libusb/io.c libusb/sync.c libusb/strerror.c \
					libusb/os/linux_usbfs.c libusb/os/poll_posix.c \
					libusb/os/threads_posix.c libusb/os/linux_netlink.c
#libfreenect files
LOCAL_SRC_FILES += libfreenect/cameras.c libfreenect/core.c libfreenect/loader.c \
				libfreenect/tilt.c libfreenect/usb_libusb10.c \
				libfreenect/registration.c libfreenect/audio.c libfreenect/flags.c
#TUIO server
LOCAL_SRC_FILES += TUIO/TuioClient.cpp TUIO/TuioServer.cpp TUIO/TuioTime.cpp

#osc
LOCAL_SRC_FILES += oscpack/ip/IpEndpointName.cpp oscpack/ip/posix/NetworkingUtils.cpp \
	oscpack/ip/posix/UdpSocket.cpp oscpack/osc/OscOutboundPacketStream.cpp oscpack/osc/OscPrintReceivedElements.cpp \
	oscpack/osc/OscReceivedElements.cpp oscpack/osc/OscTypes.cpp
				
LOCAL_SRC_FILES += stouchJNI.cpp stouchDetector.cpp stouchEventInjector.cpp

LOCAL_LDLIBS += -lstdc++ -lc -lm -llog

LOCAL_CFLAGS += -O3

LOCAL_CPPFLAGS  += -std=c++11

APP_OPTIM := debug

LOCAL_MODULE    := stouch2

include $(BUILD_SHARED_LIBRARY)

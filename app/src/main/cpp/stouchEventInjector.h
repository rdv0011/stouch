//
//  STouchEventInjector.h

#ifndef __STouchEventInjector__
#define __STouchEventInjector__

#include <string>

class STouchEventInjector {
public:
	// Parameters width and height defines the size of virtual touch surface in pixels
	STouchEventInjector(int width, int height);
	~STouchEventInjector();
	// Inject event to /dev/input/evetX. local coordinates of virtual touch surface
	bool sendEventToTouchDevice(int x, int y);
protected:
	// Open device
	int openTouchDevice(int width, int height);
	// Close device file i.e. /dev/input/eventX
	void closeTouchDevice();
	std::string execShellCommand(std::string cmd);
	int openDevice();
	bool intSendEvent(int fd, uint16_t type, uint16_t code, int32_t value);
	bool sendTouchDownAbs(int fd, int x, int y);
private:
	double scaleX;
	double scaleY;
	int touchDeviceFileDesciptor;
};

#endif /* defined(__STouchEventInjector__) */

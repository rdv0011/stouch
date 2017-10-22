//  stouchEventInjector.cpp
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <dirent.h>
#include <time.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/inotify.h>
#include <sys/limits.h>
#include <sys/poll.h>
#include <linux/fb.h>
#include <linux/kd.h>
#include <linux/input.h>
#include <iostream>
#include <string>

#include "stouchEventInjector.h"
#include "stouchFreenect.hpp"

// Event system constants
//const int ABS_MT_TOUCH_MAJOR = 0x30;/* Major axis of touching ellipse */
//const int ABS_MT_POSITION_X = 0x35;	/* Center X ellipse position */
//const int ABS_MT_POSITION_Y = 0x36;	/* Center Y ellipse position */
// Shell command to show event details
const char EventCommandFormat[] = "getevent -lp /dev/input/event%d";
const char DevicePathFormat[] = "/dev/input/event%d";
// Touch screen driver we are looking for
const char Sun4iTouchScreenDeviceName[] = "sun4i-ts";
// sun4i touch screen specific constants
const int TouchSurfaceWidth = 4095;
const int TouchSurfaceHeight = 4095;
const int TouchPressure = 0x68;
const int MaxInputDeviceObservable = 19;

struct uinput_event {
    struct timeval time;
    uint16_t type;
    uint16_t code;
    int32_t value;
};

STouchEventInjector::STouchEventInjector(int width, int height) {
	scaleX = 1.0f;
	scaleY = 1.0f;
	touchDeviceFileDesciptor = openTouchDevice(width, height);
}

STouchEventInjector::~STouchEventInjector() {
	closeTouchDevice();
}

int STouchEventInjector::openTouchDevice(int width, int height) {
    touchDeviceFileDesciptor = openDevice();
    if (touchDeviceFileDesciptor >= 0) {
        scaleX = TouchSurfaceWidth / (double)width;
        scaleY = TouchSurfaceHeight / (double)height;
        LOGD("Touch device successfully opened size [%d, %d] scale [%f,%f]", width, height, scaleX, scaleY);
    }
    return touchDeviceFileDesciptor;
}

void STouchEventInjector::closeTouchDevice() {
    close(touchDeviceFileDesciptor);
}

std::string STouchEventInjector::execShellCommand(std::string cmd) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "ERROR";
    char buffer[128];
    std::string result = "";
    while(!feof(pipe)) {
        if(fgets(buffer, 128, pipe) != NULL)
            result += buffer;
    }
    pclose(pipe);
    return result;
}

int STouchEventInjector::openDevice() {
    char devicePath[PATH_MAX];
    char name[PATH_MAX];
    int fd = -1;
    for(int i = 0; i < MaxInputDeviceObservable; i++) {
        sprintf(devicePath, DevicePathFormat, i);
        fd = open(devicePath, O_RDWR);
        if (fd < 0) {
            LOGD("Could not open %s, %s\n", devicePath, strerror(errno));
            // possible only if we have root permissions
            // set new permissions
            name[sizeof(name) - 1] = '\0';
            sprintf(name, "chmod 666 %s", devicePath);
            system(name);
            // reopen
            fd = open(devicePath, O_RDWR);
        }

        if (fd >= 0) {
            name[sizeof(name) - 1] = '\0';
            if (ioctl(fd, EVIOCGNAME(sizeof(name) - 1), &name) < 1) {
                LOGD("Could not get device name for %s, %s\n", devicePath, strerror(errno));
                name[0] = '\0';
            }

            if (strcmp(name, Sun4iTouchScreenDeviceName) == 0) {
                LOGD("Touch device event found at path %s\nName %s\n", devicePath, name);
                sprintf(name, EventCommandFormat, i);
                std::string output = execShellCommand(name);
                // To do: parse  ABS_MT_POSITION_X     : value 0, min 0, max 4095
                // max - min = touchSurfaceWidth
                //ABS_MT_POSITION_Y     : value 0, min 0, max 4095
                // max - min = touchSurfaceHeight
                break;
            }
        }
    }

    if (fd < 0) {
    	LOGE("Couldn't open touch device to inject event!");
    }

    return fd;
}

bool STouchEventInjector::intSendEvent(int fd, uint16_t type, uint16_t code, int32_t value) {
    if (fd < 0) return false;
    struct uinput_event event;
    memset(&event, 0, sizeof(event));
    event.type = type;
    event.code = code;
    event.value = value;
    int len = write(fd, &event, sizeof(event));
    //LOGD("send event %d %d %d", type, code, value);
    return len > 0;
}
// Mouse click
/*
int sendClickToMouse(int fd) {
	intSendEvent(fd, 0x4, 0x4, 0x90001);
	intSendEvent(fd, 0x1, 0x110, 0x1);
	intSendEvent(fd, 0x0, 0x0, 0x0);
	intSendEvent(fd, 0x4, 0x4, 0x90001);
	intSendEvent(fd, 0x1, 0x110, 0x0);
	intSendEvent(fd, 0x0, 0x0, 0x0);
	return 1;
}
*/
// Touch event
bool STouchEventInjector::sendTouchDownAbs(int fd, int x, int y) {
    return intSendEvent(fd, EV_ABS, ABS_MT_POSITION_X, x) &&
    		intSendEvent(fd, EV_ABS, ABS_MT_POSITION_Y, y) &&
    		intSendEvent(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, TouchPressure) &&
    		intSendEvent(fd, EV_KEY, BTN_TOUCH, 1) &&
    		intSendEvent(fd, EV_SYN, SYN_REPORT, 0) &&
    		intSendEvent(fd, EV_ABS, ABS_MT_TOUCH_MAJOR, 1) &&
    		intSendEvent(fd, EV_KEY, BTN_TOUCH, 0) &&
    		intSendEvent(fd, EV_SYN, SYN_REPORT, 0);
}

bool STouchEventInjector::sendEventToTouchDevice(int x, int y) {
    int touchX = x * scaleX;
    int touchY = y * scaleY;
    return sendTouchDownAbs(touchDeviceFileDesciptor, touchX, touchY);
}


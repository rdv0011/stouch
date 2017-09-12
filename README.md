# Project Stouch

This software is used in the project which creates an interactive surface on the floor. An interactivity is realised by using Microsoft Kinect sensor.

This software is supposed to be run on a modified version of the Android system which allows an access to USB devices from a native code.

# How to prepare the Android source code:
To build Android it is possible to use the latest version of Ubuntu in my case the version was 14.10. Source code is taken from here Cubieboard A10 Android and unpack. We need to change two files:

* android / device / softwinner / apollo-cubieboard / init.sun4i.rc
* android / frameworks / base / data / etc / platform.xml

In the init.sun4i.rc file, you need to uncomment the insmod /system/vendor/modules/sun4i-ts.ko line. In the platform.xml file, add the usb, input, and shell groups to the INTERNET section:

* \<group gid = "usb" /\>
* \<group gid = "input" /\>
* \<group gid = "shell" /\>

Besides the above the permission for USB should be changed. To do this in the file /ueventd.rc find and change the following line:
/dev/bus/usb/*            0660   root       usb       ====>      /dev/bus/usb/*            0666   root       usb

After making all above changes, run the assembly with the following command:

$./build.sh -p sun4i_crane -k 3.0

To build the version of Android ICS, you need a GCC compiler version 4.6 and 'make' version 3.81. If the version of the compiler and 'make' differs from the required one, you can change it by the commands:

* $sudo update-alternatives --install / usr / bin / gcc gcc /usr/bin/gcc-4.6 60 --slave / usr / bin / g ++ g ++ /usr/bin/g++-4.6
* $sudo update-alternatives --install / usr / bin / gcc gcc /usr/bin/gcc-4.9 40 --slave / usr / bin / g ++ g ++ /usr/bin/g++-4.9
* $sudo update-alternatives --config gcc
* $sudo mv / usr / bin / make / usr / bin / make40
* $sudo update-alternatives --install / usr / bin / make make / usr / local / bin / make 60
* $sudo update-alternatives --install / usr / bin / make make / usr / bin / make40 40
* $sudo update-alternatives --config make

Then follow the instructions on the Cubieboard A10 Android page. Compilation errors may occur during the build process. Tips for fixing errors can be found in the Fix building issues section in the fix_android_firmware.readme file in the source code repository.
To connect the board to the PC, you need to add the rules for connecting the device via USB. To achieve this do the following:
* $sudo vim /etc/udev/rules.d/51-android.rules

Add next line to the file:
´<SUBSYSTEM=="usb", ATTRS{idVendor}=="18d1", ATTRS{idProduct}=="0003",MODE="0666">´

* $sudo chmod a+rx /etc/udev/rules.d/51-android.rules
* $sudo service udev restart

* $sudo ./adb kill-server
* $./adb devices
* $./adb root

# Building problems

Fix building issues
If compilation fails due to existing links, remove them under the next folders:
* lichee/linux-3.0/modules/wifi/bcm40181
* lichee/linux-3.0/modules/wifi/bcm40183
* lichee/linux-3.0/modules/wifi/usi-bcm4329

If compilation fails due to undefined structure setrlimit, just add next line to include headers:
```c++
#include <sys/time.h>
#include <sys/resource.h>
```

Compilation may fail because Stream.pm perl package is not installed. To fix just install it.

# How to fix a problem with the USB permissions:
## Approaches
The default system configuration on most Android device will not allow access to USB devices. There are several options for changing this.

If you have control of the system image then you can modify the ueventd.rc used in the image to change the permissions on /dev/bus/usb//.
If using this approach then it is advisable to create a new Android permission to protect access to these files. It is not advisable to give all applications read and write permissions to these files.

For rooted devices the code using libusb could be executed as root using the "su" command.
An alternative would be to use the "su" command to change the permissions on the appropriate /dev/bus/usb/ files.

Users have reported success in using android.hardware.usb.UsbManager to request permission to use the UsbDevice and then opening the device.
The difficulties in this method is that there is no guarantee that it will continue to work in the future Android versions, it requires invoking Java APIs and running code to match each android.hardware.usb.UsbDevice to a libusb_device.

## Using USBManager. A 'permanent' approach which means you should not do any action when the system is rebooted.

1. Search for the UsbDevice [1] you are interested in.
2. Extract the usbfs path [2] from the UsbDevice.
3. Build a libbox0_device from the path
    using libusb_get_device2(context, path) [3]
4. open the UsbDevice [4], you will get UsbDeviceConnection [5].
5. from the UsbDeviceConnection, extract the fd[6]
6. now, from the fd and the previously
    created libusb_device, build a libusb_device_handle
    by calling libusb_get_device2(dev, handle, fd)[7].
7. and that all you need.

* [1] http://developer.android.com/reference/android/hardware/usb/UsbDevice.html
* [2] http://developer.android.com/reference/android/hardware/usb/UsbDevice.html#getDeviceName%28%29
* [3] https://github.com/kuldeepdhaka/libusb/blob/android-open2/libusb/core.c#L1492
* [4] http://developer.android.com/reference/android/hardware/usb/UsbManager.html#openDevice%28android.hardware.usb.UsbDevice%29
* [5] http://developer.android.com/reference/android/hardware/usb/UsbDeviceConnection.html
* [6] http://developer.android.com/reference/android/hardware/usb/UsbDeviceConnection.html#getFileDescriptor%28%29
* [7] https://github.com/kuldeepdhaka/libusb/blob/android-open2/libusb/core.c#L1267

## A sample using USBManager. Does not require a root access
A sample how to find and open a USB device might be found below:
https://github.com/mik3y/usb-serial-for-android
In this approach the USD device descriptor (fd) should be created at a Java level and then passed to a JNI C level (libfreenect/libusb)

## An alternative way by using ADB(*has not been checked*). The below approach is not permanent which means you should repeat it on each reboot.
After each reboot do the following:
	$adb shell mount -o devmode=0666 -t usbfs none /proc/bus/usb

## How to check whether an approach works. The below approach is not permanent which means you should repeat it on each reboot.

* $adb shell mount -o remount rw /system
* $adb pull /system/etc/permissions/platform.xml platform.xml
* $vi platform.xml -- make all relevant changes which are described above
* $adb pull /ueventd.rc ueventd.rc
* $vi ueventd.rc -- make all relevant changes which are described above
* $adb push platform.xml /system/etc/permissions/
* $adb push ueventd.rc /
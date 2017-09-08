# stouch

This software is used to create a simulated surface on the floor. This surface supports an intercation by using Microsoft Kinect sensor.

TODO:
Runtime Permissions:

The default system configuration on most Android device will not allow access to USB devices. There are several options for changing this.

If you have control of the system image then you can modify the ueventd.rc used in the image to change the permissions on /dev/bus/usb//. If using this approach then it is advisable to create a new Android permission to protect access to these files. It is not advisable to give all applications read and write permissions to these files.

For rooted devices the code using libusb could be executed as root using the "su" command. An alternative would be to use the "su" command to change the permissions on the appropriate /dev/bus/usb/ files.

Users have reported success in using android.hardware.usb.UsbManager to request permission to use the UsbDevice and then opening the device. The difficulties in this method is that there is no guarantee that it will continue to work in the future Android versions, it requires invoking Java APIs and running code to match each android.hardware.usb.UsbDevice to a libusb_device.

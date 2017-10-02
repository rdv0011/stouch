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
* &adb shell "ps aux  |  grep -i csp_build  |  awk '{print $2}'  |  xargs sudo kill -9"

## One more alternative approach is to substitute a system package

There's a service that Android uses internally that allows to manage USB devices and accessories. This service is hidden from thrid party developers and is not documented.
If you check the source code for UsbPermissionActivity you'll be able to figure out how that service is called. In order to call the service IUsbManager interface and ServiceManager class are employed.
These are both hidden too, so you can't use them directly. But what you can do is to create their stubs with exactly the same names and in corresponding namespaces (packages).
Then you'll be able to compile that code, while the runtime environment will use the real things.
The only requirement is that your application has to be a system one - that is it has to be located in /system/app/ directory. Since your device is rooted that shouldn't be a problem.
So you will have to add a package to your project: "android.hardware.usb" and put a file in it named "IUsbManager.java" with the following content:

```java
package android.hardware.usb;

public interface IUsbManager extends android.os.IInterface
{
    /** Local-side IPC implementation stub class. */
    public static abstract class Stub extends android.os.Binder implements android.hardware.usb.IUsbManager
    {
        /** Construct the stub at attach it to the interface. */
        public Stub()
        {
            throw new RuntimeException( "Stub!" );
        }
        /**
         * Cast an IBinder object into an android.hardware.usb.IUsbManager interface,
         * generating a proxy if needed.
         */
        public static android.hardware.usb.IUsbManager asInterface( android.os.IBinder obj )
        {
            throw new RuntimeException( "Stub!" );
        }

        public android.os.IBinder asBinder()
        {
            throw new RuntimeException( "Stub!" );
        }

        public boolean onTransact( int code, android.os.Parcel data, android.os.Parcel reply, int flags ) throws android.os.RemoteException
        {
            throw new RuntimeException( "Stub!" );
        }

        static final int TRANSACTION_getDeviceList = (android.os.IBinder.FIRST_CALL_TRANSACTION + 0);
        static final int TRANSACTION_openDevice = (android.os.IBinder.FIRST_CALL_TRANSACTION + 1);
        static final int TRANSACTION_getCurrentAccessory = (android.os.IBinder.FIRST_CALL_TRANSACTION + 2);
        static final int TRANSACTION_openAccessory = (android.os.IBinder.FIRST_CALL_TRANSACTION + 3);
        static final int TRANSACTION_setDevicePackage = (android.os.IBinder.FIRST_CALL_TRANSACTION + 4);
        static final int TRANSACTION_setAccessoryPackage = (android.os.IBinder.FIRST_CALL_TRANSACTION + 5);
        static final int TRANSACTION_hasDevicePermission = (android.os.IBinder.FIRST_CALL_TRANSACTION + 6);
        static final int TRANSACTION_hasAccessoryPermission = (android.os.IBinder.FIRST_CALL_TRANSACTION + 7);
        static final int TRANSACTION_requestDevicePermission = (android.os.IBinder.FIRST_CALL_TRANSACTION + 8);
        static final int TRANSACTION_requestAccessoryPermission = (android.os.IBinder.FIRST_CALL_TRANSACTION + 9);
        static final int TRANSACTION_grantDevicePermission = (android.os.IBinder.FIRST_CALL_TRANSACTION + 10);
        static final int TRANSACTION_grantAccessoryPermission = (android.os.IBinder.FIRST_CALL_TRANSACTION + 11);
        static final int TRANSACTION_hasDefaults = (android.os.IBinder.FIRST_CALL_TRANSACTION + 12);
        static final int TRANSACTION_clearDefaults = (android.os.IBinder.FIRST_CALL_TRANSACTION + 13);
        static final int TRANSACTION_setCurrentFunction = (android.os.IBinder.FIRST_CALL_TRANSACTION + 14);
        static final int TRANSACTION_setMassStorageBackingFile = (android.os.IBinder.FIRST_CALL_TRANSACTION + 15);
    }

    /* Returns a list of all currently attached USB devices */
    public void getDeviceList( android.os.Bundle devices ) throws android.os.RemoteException;
    /* Returns a file descriptor for communicating with the USB device.
         * The native fd can be passed to usb_device_new() in libusbhost.
         */
    public android.os.ParcelFileDescriptor openDevice( java.lang.String deviceName ) throws android.os.RemoteException;
    /* Returns the currently attached USB accessory */
    public android.hardware.usb.UsbAccessory getCurrentAccessory() throws android.os.RemoteException;
    /* Returns a file descriptor for communicating with the USB accessory.
         * This file descriptor can be used with standard Java file operations.
         */
    public android.os.ParcelFileDescriptor openAccessory( android.hardware.usb.UsbAccessory accessory ) throws android.os.RemoteException;
    /* Sets the default package for a USB device
         * (or clears it if the package name is null)
         */
    public void setDevicePackage( android.hardware.usb.UsbDevice device, java.lang.String packageName ) throws android.os.RemoteException;
    /* Sets the default package for a USB accessory
         * (or clears it if the package name is null)
         */
    public void setAccessoryPackage( android.hardware.usb.UsbAccessory accessory, java.lang.String packageName ) throws android.os.RemoteException;
    /* Returns true if the caller has permission to access the device. */
    public boolean hasDevicePermission(android.hardware.usb.UsbDevice device) throws android.os.RemoteException;
    /* Returns true if the caller has permission to access the accessory. */
    public boolean hasAccessoryPermission( android.hardware.usb.UsbAccessory accessory ) throws android.os.RemoteException;
    /* Requests permission for the given package to access the device.
         * Will display a system dialog to query the user if permission
         * had not already been given.
         */
    public void requestDevicePermission( android.hardware.usb.UsbDevice device, java.lang.String packageName, android.app.PendingIntent pi ) throws android.os.RemoteException;
    /* Requests permission for the given package to access the accessory.
         * Will display a system dialog to query the user if permission
         * had not already been given. Result is returned via pi.
         */
    public void requestAccessoryPermission( android.hardware.usb.UsbAccessory accessory, java.lang.String packageName, android.app.PendingIntent pi ) throws android.os.RemoteException;
    /* Grants permission for the given UID to access the device */
    public void grantDevicePermission( android.hardware.usb.UsbDevice device, int uid ) throws android.os.RemoteException;
    /* Grants permission for the given UID to access the accessory */
    public void grantAccessoryPermission( android.hardware.usb.UsbAccessory accessory, int uid ) throws android.os.RemoteException;
    /* Returns true if the USB manager has default preferences or permissions for the package */
    public boolean hasDefaults( java.lang.String packageName ) throws android.os.RemoteException;
    /* Clears default preferences and permissions for the package */
    public void clearDefaults( java.lang.String packageName ) throws android.os.RemoteException;
    /* Sets the current USB function. */
    public void setCurrentFunction( java.lang.String function, boolean makeDefault ) throws android.os.RemoteException;
    /* Sets the file path for USB mass storage backing file. */
    public void setMassStorageBackingFile( java.lang.String path ) throws android.os.RemoteException;
}
```

Then another package: "android.os" with "ServiceManager.java":
```java
package android.os;

import java.util.Map;

public final class ServiceManager
{
    public static IBinder getService( String name )
    {
        throw new RuntimeException( "Stub!" );
    }

    /**
     * Place a new @a service called @a name into the service
     * manager.
     * 
     * @param name the name of the new service
     * @param service the service object
     */
    public static void addService( String name, IBinder service )
    {
        throw new RuntimeException( "Stub!" );
    }

    /**
     * Retrieve an existing service called @a name from the
     * service manager.  Non-blocking.
     */
    public static IBinder checkService( String name )
    {
        throw new RuntimeException( "Stub!" );
    }

    public static String[] listServices() throws RemoteException
    {
        throw new RuntimeException( "Stub!" );
    }

    /**
     * This is only intended to be called when the process is first being brought
     * up and bound by the activity manager. There is only one thread in the process
     * at that time, so no locking is done.
     * 
     * @param cache the cache of service references
     * @hide
     */
    public static void initServiceCache( Map<String, IBinder> cache )
    {
        throw new RuntimeException( "Stub!" );
    }
}
```
Note that interfaces of these classes may change depending on the version of Android. In my case the version is 4.0.3.
So if you have another version of Android and this code doesn't work you will have to check the source code for your particular version of OS.

Here's an example of using the service to grant permissions to all FTDI devices:
```java
import java.util.HashMap;
import java.util.Iterator;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.hardware.usb.IUsbManager;
import android.hardware.usb.UsbDevice;
import android.hardware.usb.UsbManager;
import android.os.IBinder;
import android.os.ServiceManager;

public class LaunchReceiver extends BroadcastReceiver
{
    public void onReceive( Context context, Intent intent )
    {
        String action = intent.getAction();
        if( action != null && action.equals( Intent.ACTION_BOOT_COMPLETED ) )
        {
            try
            {
                PackageManager pm = context.getPackageManager();
                ApplicationInfo ai = pm.getApplicationInfo( YOUR_APP_PACKAGE_NAMESPACE, 0 );
                if( ai != null )
                {
                    UsbManager manager = (UsbManager) context.getSystemService( Context.USB_SERVICE );
                    IBinder b = ServiceManager.getService( Context.USB_SERVICE );
                    IUsbManager service = IUsbManager.Stub.asInterface( b );

                    HashMap<String, UsbDevice> deviceList = manager.getDeviceList();
                    Iterator<UsbDevice> deviceIterator = deviceList.values().iterator();
                    while( deviceIterator.hasNext() )
                    {
                            UsbDevice device = deviceIterator.next();
                            if( device.getVendorId() == 0x0403 )
                            {
                                service.grantDevicePermission( device, ai.uid );
                                service.setDevicePackage( device, YOUR_APP_PACKAGE_NAMESPACE );
                            }
                    }
                }
            }
            catch( Exception e )
            {
                trace( e.toString() );
            }
        }
    }
}
```
One more thing - you will have to add the following permission to your manifest (Lint might not like it but you can always change severity level in your project's properties):
```xml
<uses-permission android:name="android.permission.MANAGE_USB" />
```
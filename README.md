# Project Stouch

This software is used in the project which creates an interactive surface on the floor. An interactivity is realised by using Microsoft Kinect sensor.

This software is supposed to be run on a modified version of the Android system which allows an access to USB devices from a native code.

# How to prepare the Android source code:
To build Android it is possible to use the latest version of Ubuntu in my case the version was 14.10. Source code is taken from here Cubieboard A10 Android and unpack. We need to change two files:

* android / device / softwinner / apollo-cubieboard / init.sun4i.rc
* android / frameworks / base / data / etc / platform.xml

In the init.sun4i.rc file, you need to uncomment the insmod /system/vendor/modules/sun4i-ts.ko line.

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

# Creating java key store

openssl pkcs8 -in platform.pk8 -inform DER -outform PEM -out platform.priv.pem -nocrypt

openssl pkcs12 -export -in platform.x509.pem -inkey platform.priv.pem -out platform.pk12 -name android

keytool -importkeystore -destkeystore platform.jks -srckeystore platform.pk12 -srcstoretype PKCS12 -srcstorepass android -alias android


## Useful ADB commands

To install the app
``` bash
adb push /Users/dmitry/Documents/stouch/app/build/outputs/apk/app-debug.apk /data/local/tmp/com.github.rdv0011.stouch
adb shell pm install -r com.github.rdv0011.stouch
```

To start the app
``` bash
adb shell am start -n "com.github.rdv0011.stouch/com.github.rdv0011.stouch.STouchActivity" -a android.intent.action.MAIN -c android.intent.category.LAUNCHER -D

adb shell am start -n "com.stouch.rdv0011.testapp/com.stouch.rdv0011.testapp.Test" -a android.intent.action.MAIN -c android.intent.category.LAUNCHER
```

Stop the app
``` bash
adb shell am force-stop com.github.rdv0011.stouch
```

To uninstall the app from the system
``` bash
adb uninstall com.github.rdv0011.stouch
```

To check the details of the installed package
``` bash
adb shell dumpsys package com.github.rdv0011.stouch
```

To send the app to the background
``` bash
adb shell am start -n "com.stouch.rdv0011.testapp/com.stouch.rdv0011.testapp.Test" -a android.intent.action.MAIN -c android.intent.category.LAUNCHER
adb shell dumpsys window windows | grep -E 'mFocusedApp'
```

To send mouse event
``` bash
adb shell sendevent /dev/input/mice 3 0 x
```
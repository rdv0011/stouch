adb push ueventd.rc /ueventd.rc
adb shell kill $(adb shell ps | grep 'ueventd' | awk '{print $2}')
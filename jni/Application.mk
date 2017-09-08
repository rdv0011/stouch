# The ARMv7 is significanly faster due to the use of the hardware FPU
APP_ABI := armeabi-v7a
APP_PLATFORM := android-15
#APP_STL := stlport_static
APP_CPPFLAGS := -frtti -fexceptions
APP_STL := gnustl_static
#APP_OPTIM := debug
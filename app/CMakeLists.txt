# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

# Set a path where OpenCVConfig.cmake is located
# Usually it is located inside the Android SDK. You can make a soft link /usr/local/share/OpenCV pointing to a real location
# A sample is: ln -s ~/OpenCV-android-sdk /usr/local/share/OpenCV-Android
set(OpenCV_DIR "/usr/local/share/OpenCV-Android/sdk/native/jni")
set(OpenCV_STATIC ON)
find_package(OpenCV 3 REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

# Try to add the whole src folder using one line
file(GLOB_RECURSE SRC_FILES "src/main/cpp/libfreenect/*.c")
file(GLOB H_FILES "src/main/cpp" "src/main/cpp/*/" "src/main/cpp/*/*/" "src/main/cpp/*/*/*/")

set(LIBUSB_SRC_REL "src/main/cpp/libusb")
set(LIBUSB_SRC "${LIBUSB_SRC_REL}/core.c" "${LIBUSB_SRC_REL}/descriptor.c" "${LIBUSB_SRC_REL}/hotplug.c"
                "${LIBUSB_SRC_REL}/io.c" "${LIBUSB_SRC_REL}/sync.c" "${LIBUSB_SRC_REL}/strerror.c"
				"${LIBUSB_SRC_REL}/os/linux_usbfs.c" "${LIBUSB_SRC_REL}/os/poll_posix.c"
				"${LIBUSB_SRC_REL}/os/threads_posix.c" "${LIBUSB_SRC_REL}/os/linux_netlink.c")
set(MAIN_SRC_REL "src/main/cpp/")
set(MAIN_SRC "${MAIN_SRC_REL}/stouchDetector.cpp" "${MAIN_SRC_REL}/stouchJNI.cpp"
             "${MAIN_SRC_REL}/stouchJNIEntry.cpp" "${MAIN_SRC_REL}/stouchFreenect.cpp"
             "${MAIN_SRC_REL}/stouchUtility.cpp")
set(SRC_FILES ${SRC_FILES} ${LIBUSB_SRC})
set(SRC_FILES ${SRC_FILES}   ${MAIN_SRC})

include_directories(${H_FILES})

add_library( # Sets the name of the library.
             stouch

             # Sets the library as a shared library.
             SHARED

             # Provides a relative path to your source file(s).
             ${SRC_FILES} )

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

find_library( # Sets the name of the path variable.
              log-lib

              # Specifies the name of the NDK library that
              # you want CMake to locate.
              log )

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

target_link_libraries( # Specifies the target library.
                       stouch

                       # Links the target library to the log library
                       # included in the NDK.
                       ${log-lib}
                       ${OpenCV_LIBS} )
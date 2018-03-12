#include "stouchJNI.h"
#include <sstream>
#include "freenect_internal.h"
#include "stouchFreenect.h"

using namespace std;

const int STouchJNI::videoFrameSize = STouchFreenectDeviceConst::VideoBufferSizeARGB;

// Native functions which will be exposed to the Java env
jboolean init(JNIEnv *env, jobject activityObj) {
    return STouchJNI::instance().initDetector(env, activityObj) == JNI_OK;
}

void cleanup(JNIEnv *env) {
    STouchJNI::instance().cleanupEnv(env);
}

int fps(JNIEnv *env) {
    return STouchJNI::instance().getFPS(env);
}

int calibrationState(JNIEnv *env) {
    return STouchJNI::instance().getCalibrationState();
}

int calibrationFrameCount(JNIEnv *env) {
    return STouchJNI::instance().getCalibrationFrameCount();
}

void startCalibration(JNIEnv *env, jboolean start) {
    STouchJNI::instance().startCalibrationImpl(start);
}

void startTouch(JNIEnv *env, jboolean start) {
    STouchJNI::instance().startTouchImpl(start);
}

STouchJNI::STouchJNI() {
    videoDataMutex = PTHREAD_MUTEX_INITIALIZER;
    frameCond = PTHREAD_COND_INITIALIZER;
    videoData.reserve(videoFrameSize);
    videoDataHandlerMethod = nullptr;
    virtualROIUpdatedMethod = nullptr;
    sendEventMethod = nullptr;
    javaVideoFrameDataObj = nullptr;
}

// Native functions which will be exposed to Java
int STouchJNI::initEnv(JNIEnv *env) {
    // Getting an environment
    int ret = env->GetJavaVM(&jvm);
    if( ret != JNI_OK ) {
        LOGD("Could not get JavaVM: %d", ret);
        throw;
    }

    LOGD("Native functions initialization...\n");
    registerNativeFunctions(env);

    return ret;
}

int STouchJNI::initDetector(JNIEnv *env, jobject activityObj) {

    try {
        LOGD("Getting references to Java methods...\n");

        const char* videoDataHandler[] = {"videoDataHandler", "([B)V"};
        const char* virtualROIUpdated[] = {"virtualROIUpdated", "(IIII)V"};
        const char* sendEvent[] = {"sendEvent", "(II)V"};

        jclass javaClass = env->GetObjectClass(activityObj);
        videoDataHandlerMethod = env->GetMethodID(javaClass, videoDataHandler[0], videoDataHandler[1]);
        if(!videoDataHandlerMethod) {
            LOGE("Failed to get method %s", videoDataHandler[0]);
            throw;
        }
        virtualROIUpdatedMethod = env->GetMethodID(javaClass, virtualROIUpdated[0], virtualROIUpdated[1]);
        if(!virtualROIUpdatedMethod) {
            LOGE("Failed to get method %s", virtualROIUpdated[0]);
            throw;
        }
        sendEventMethod = env->GetMethodID(javaClass, sendEvent[0], sendEvent[1]);
        if(!sendEventMethod) {
            LOGE("Failed to get method %s", sendEvent[0]);
            throw;
        }

        javaVideoFrameDataObj = env->NewByteArray(videoFrameSize);
        javaVideoFrameDataObj = (jbyteArray)env->NewGlobalRef(javaVideoFrameDataObj);

        activityObjRef = env->NewGlobalRef(activityObj);

        if (javaVideoFrameDataObj == nullptr || activityObjRef == nullptr ||
            videoDataHandlerMethod == nullptr || virtualROIUpdatedMethod == nullptr ||
            sendEventMethod == nullptr) {

            LOGE("Failed to get a reference");
            throw;
        }

        LOGD("Freenect initialization...\n");

        freenectDriver = new STouchFreenect;
        touchDetector = new STouchDetector;
        STouchFreenectDevice *device = nullptr;
        int ret = (jboolean) (freenectDriver->init(touchDetector, device) ? JNI_OK: JNI_ERR);
        if (ret == JNI_OK) {
            touchDetector->deviceDelegate = device;
            freenectDriver->start(*device);
            touchDetector->start();
        }
    }
    catch (std::runtime_error &err) {
        LOGE("%s", err.what());
        return JNI_ERR;
    }
    return JNI_OK;
}

void STouchJNI::cleanupEnv(JNIEnv *env) {
    LOGD("Delete freenect driver");
    try {
        // Signal and wait untill the process thread stops
        delete freenectDriver;
        freenectDriver = nullptr;
        delete touchDetector;
        touchDetector = nullptr;
        processDataThreadDie = true;
        //pthread_join(thiz->processDataThread, NULL);
        env->DeleteGlobalRef(javaVideoFrameDataObj);
        env->DeleteGlobalRef(activityObjRef);
    }
    catch (std::runtime_error &err) {
        LOGE("Freenect cleanup error %s", err.what());
    }
}

STouchJNI& STouchJNI::instance() {
    static STouchJNI inst;
    return inst;
}

void STouchJNI::registerNativeFunctions(JNIEnv * env) {

    JNINativeMethod activityMethods[] = {
            {
                    "fps",
                    "()I",
                    (void *) fps
            },
            {
                    "init",
                    "(Ljava/lang/Object;)Z",
                    (void *) init
            },
            {
                    "cleanup",
                    "()V",
                    (void *) cleanup
            },
            {
                    "startCalibration",
                    "(Z)V",
                    (void *) startCalibration
            },
            {
                    "calibrationState",
                    "()I",
                    (void *) calibrationState
            },
            {
                    "calibrationFrameCount",
                    "()I",
                    (void *) calibrationFrameCount
            },
            {
                    "startTouch",
                    "(Z)V",
                    (void *) startTouch
            }
    };

    const char* fullClassName = "com/github/rdv0011/stouch/STouchActivity";
    jclass activityDataClass = env->FindClass(fullClassName);
    if (!activityDataClass) {
        LOGE("Native registration unable to find class '%s'", fullClassName);
        throw;
    }

    jint res = (env)->RegisterNatives(activityDataClass, activityMethods,
                                 sizeof(activityMethods) / sizeof(activityMethods[0]));
    if (res) {
        LOGE("Failed to register natives '%d'", res);
        throw;
    }
}

JNIEnv * STouchJNI::getEnv() {
    JNIEnv * env;
    // Get the JVM environment
    int ret = jvm->GetEnv((void **)&env, JNI_VERSION_1_6);
    if (ret == JNI_OK)
        return env;

    if (ret == JNI_EDETACHED) {
        LOGD("GetEnv: thread is not attached, trying to attach");
        ret = jvm->AttachCurrentThread(&env, nullptr);
        if( ret == JNI_OK ) {
            LOGD("GetEnv: attach successful");
            return env;
        }
        LOGD("Cannot attach JNI to current thread: %d", ret);
        return nullptr;
    }

    LOGD("could not get JNI ENV: %d", ret);
    return nullptr;
}

void STouchJNI::videoDataFrameReady() {
    JNIEnv * env = getEnv();

    //pthread_mutex_lock(&videoDataMutex);

    touchDetector->getVideoData(videoData);

    //gotVideoFrame = true;
    //pthread_cond_signal(&frameCond);
    //pthread_mutex_unlock(&videoDataMutex);

    env->SetByteArrayRegion(javaVideoFrameDataObj, 0, videoFrameSize,
                            (const jbyte *)videoData.data());
    //pthread_mutex_unlock(&videoDataMutex);
    // Send the video frame to the Java side
    env->CallVoidMethod(activityObjRef, videoDataHandlerMethod, javaVideoFrameDataObj);
}

/*
void* STouchJNI::processData(void *arg) {
    STouchJNI *thiz = (STouchJNI*)arg;
    JNIEnv * env = thiz->getEnv();
    jmethodID methodRef;
    jobject obj = thiz->methodClassHelper(env, thiz->getClassPath(kinectPreviewClassName).data(),
                                    "videoDataHandler", "([S)V", &methodRef);
    jbyteArray javaByteVideoData = env->NewByteArray(STouchFreenectDevice::VideoBufferSize);

    assert(javaByteVideoData != nullptr);

    while(!thiz->processDataThreadDie) {
        pthread_mutex_lock(&thiz->videoDataMutex);
        while (!thiz->gotVideoFrame)
            pthread_cond_wait(&thiz->frameCond, &thiz->videoDataMutex);

        thiz->gotVideoFrame = 0;

        env->SetByteArrayRegion(javaByteVideoData, 0, STouchFreenectDevice::VideoBufferSize,
                                      (const jbyte*)thiz->videoData.data());
        pthread_mutex_unlock(&videoDataMutex);
        // Send the video frame to the Java side
        env->CallVoidMethod(obj, methodRef, javaByteVideoData);
    }

    return nullptr;
}
*/

// Function wrappers to call their analog in Java
void STouchJNI::virtualROIUpdated(int xVirtualOffset, int yVirtualOffset,
                       int virtualWidth, int virtualHeight) {
    JNIEnv* env = getEnv();
    env->CallVoidMethod(activityObjRef, virtualROIUpdatedMethod,
                        xVirtualOffset, yVirtualOffset,
                        virtualWidth, virtualHeight);
}

void STouchJNI::sendEvent(int x, int y) {
    JNIEnv *env = getEnv();
    env->CallVoidMethod(activityObjRef, sendEventMethod, x, y);
}

int STouchJNI::getFPS(JNIEnv *env) {
    if (touchDetector)
        return touchDetector->getFPS();
    return 0;
}

void STouchJNI::startCalibrationImpl(bool start) {
    if (touchDetector) {
        touchDetector->startCalibration(start);
    }
}

void STouchJNI::startTouchImpl(bool start) {
    if (touchDetector) {
        touchDetector->startTouch(start);
    }
}

int STouchJNI::getCalibrationState() {
    if (touchDetector) {
        return (int)touchDetector->getCalibrationState();
    }
    return COMPLETED;
}

int STouchJNI::getCalibrationFrameCount() {
    if (touchDetector) {
        return touchDetector->getCalibrationFrameCount();
    }
    return 0;
}
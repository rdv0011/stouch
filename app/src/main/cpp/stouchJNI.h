//
// Created by Dmitry Rybakov on 2018-02-02.
//

#pragma once

#include <jni.h>
#include <string>
#include <vector>
#include <string>

class STouchFreenect;
class STouchDetector;

class STouchJNI {
    static const int videoFrameSize;
public:
    static STouchJNI& instance();
    void videoDataFrameReady();
    void virtualROIUpdated(int xVirtualOffset, int yVirtualOffset,
                           int virtualWidth, int virtualHeight);
    void sendEvent(int x, int y);
private:
    int initEnv(JNIEnv *env);
    int initDetector(JNIEnv *env, jobject activityObj);
    void registerNativeFunctions(JNIEnv * env);
    void cleanupEnv(JNIEnv *env);
    // Function wrappers to call their analog in Java
    int getFPS(JNIEnv *env);
    void startCalibrationImpl(bool start = true);
    void startTouchImpl(bool start = true);
    int getCalibrationState();
    int getCalibrationFrameCount();

    friend jboolean init(JNIEnv *env, jobject activityObj);
    friend void cleanup(JNIEnv *env);
    friend int fps(JNIEnv *env);
    friend void startCalibration(JNIEnv *env, jboolean start);
    friend int calibrationState(JNIEnv *env);
    friend int calibrationFrameCount(JNIEnv *env);
    friend void startTouch(JNIEnv *env, jboolean start);
    friend jint JNI_OnLoad(JavaVM* vm, void* reserved);

private:
    STouchJNI();
    JNIEnv * getEnv();
    static void* processData(void *arg);

private:
    JavaVM *jvm;
    STouchFreenect *freenectDriver;
    STouchDetector *touchDetector;
    pthread_mutex_t videoDataMutex;
    pthread_cond_t frameCond;
    bool gotVideoFrame = false;
    pthread_t processDataThread;
    volatile bool processDataThreadDie = false;

    std::vector<uint8_t> videoData;
    jmethodID videoDataHandlerMethod;
    jmethodID virtualROIUpdatedMethod;
    jmethodID sendEventMethod;
    jobject activityObjRef;
    jbyteArray javaVideoFrameDataObj;
};

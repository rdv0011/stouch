//
// Created by Dmitry Rybakov on 2018-02-02.
//
#include <jni.h>
#include "stouchFreenect.h"
#include "stouchJNI.h"

jint JNI_OnLoad(JavaVM* vm, void* reserved) {

    LOGD("Enter JNI_OnLoad()");

    JNIEnv * env = nullptr;
    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("Failed to get the environment using GetEnv()");
        return -1;
    }

    if (STouchJNI::instance().initEnv(env) != JNI_OK) {
        LOGE("Failed to init environment in STouchJNI");
    }

    LOGD("JNI_OnLoad() complete!");

    return JNI_VERSION_1_4;
}
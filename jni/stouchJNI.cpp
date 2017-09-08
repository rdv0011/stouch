#include <jni.h>
#include "stouchFreenect.hpp"

STouchFreenect *freenectDriver = nullptr;

jboolean init(JNIEnv*  env, jobject  thiz) {

	jboolean ret = JNI_FALSE;
    LOGD("Freenect initialization...\n");
    try {
    	freenectDriver = new STouchFreenect;
    	ret = freenectDriver->init()?JNI_TRUE:JNI_FALSE;
	}
	catch(std::runtime_error& err) {
		LOGE("%s", err.what());
	}
    return ret;
}

void cleanup(JNIEnv*  env) {
    LOGD("Delete freenect driver");
    try {
    	if (freenectDriver) {
    		delete freenectDriver;
    		freenectDriver = nullptr;
    	}
    }
    catch(std::runtime_error& err) {
    	LOGE("Freenect cleanup error %s", err.what());
    }
}

jint JNI_OnLoad(JavaVM* vm, void* reserved) {
    JNIEnv *env;

    LOGD("Enter JNI_OnLoad()");

    if (vm->GetEnv((void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        LOGE("Failed to get the environment using GetEnv()");
        return -1;
    }

    JNINativeMethod methods[] = {
        {
            "init",
            "()Z",
            (void *) init
        },
        {
            "cleanup",
            "()V",
            (void *) cleanup
        }
    };

    jclass k;
    const char* className = "com/example/stouch2/STouchService";
    k = (env)->FindClass (className);
    if (!k) {
        LOGE("Native registration unable to find class '%s'", className);
        return -1;
    }

    (env)->RegisterNatives(k, methods, sizeof(methods) / sizeof(methods[0]));

    LOGD("JNI_OnLoad() complete!");

    return JNI_VERSION_1_4;
}

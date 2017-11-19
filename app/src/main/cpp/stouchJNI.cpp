#include <jni.h>
#include "freenect_internal.h"
#include "stouchFreenect.hpp"

JNIEnv *env;
jclass javaActivityClassRef;
jmethodID javaVirtualMetricsUpdatedRef;
jmethodID javaSendEventRef;

/////// USB TESTING
#include "usb_libusb10.h"

#include <sys/types.h>
#include <grp.h>
#include <pwd.h>

ssize_t libusb_get_device_list(libusb_context *ctx, libusb_device ***list);

STouchFreenect *freenectDriver = nullptr;

///////////////////////////

jboolean init(JNIEnv*  env, jobject  thiz) {

	jboolean ret = JNI_FALSE;
    LOGD("Freenect initialization...\n");

    /////// USB TESTING

    register struct passwd *pw;
    register uid_t uid;
    int c;
    register gid_t gid;
    register struct group *grp;
    grp = getgrgid(gid);

    uid = geteuid();
    pw = getpwuid(uid);


    if (pw) {
        LOGD("UID: %d, GID: %d, %s, %s", uid, gid, pw->pw_name, grp->gr_name);
    } else {
        printf("failed\n");
    }

    fnusb_ctx *ctx = (fnusb_ctx*)malloc(sizeof(fnusb_ctx));
    memset(ctx, 0, sizeof(fnusb_ctx));
    int res = libusb_init(&ctx->ctx);
    if (res == 0) {
        libusb_device **devs;
        ssize_t count = libusb_get_device_list(NULL, &devs);
        if (count > 0)
        {
            for (int i = 0; i < count; i++) {
                libusb_device *usb_device = devs[i];
                struct libusb_device_descriptor desc;
                int res = libusb_get_device_descriptor (usb_device, &desc);
                if (res < 0)
                {
                    continue;
                }

                LOGD("===TESTING USB==== vendor 0x%04x, product 0x%04x, serial %d", desc.idVendor, desc.idProduct, desc.iSerialNumber);

                libusb_device_handle *device_handle;
                res = libusb_open(usb_device, &device_handle);
                if (res < 0)
                {
                    LOGE("===TESTING USB== Failed to open USB device: %d", res);
                    continue;
                }
                LOGD("Succeed to open USB device 0x%04x, product 0x%04x, serial %d", desc.idVendor, desc.idProduct, desc.iSerialNumber);
                libusb_close(device_handle);
            }
        }
    }
    ///////////////////////////////////////

    try {
    	freenectDriver = new STouchFreenect;
    	ret = freenectDriver->init() ? JNI_TRUE: JNI_FALSE;
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
    const char* serviceClassName = "com/github/rdv0011/stouch/STouchService";
    k = (env)->FindClass (serviceClassName);
    if (!k) {
        LOGE("Native registration unable to find class '%s'", serviceClassName);
        return -1;
    }

    (env)->RegisterNatives(k, methods, sizeof(methods) / sizeof(methods[0]));

    const char* activityClassName = "com/github/rdv0011/stouch/STouchActivity";
    jclass activityDataClass = env->FindClass(activityClassName);
    if (!activityDataClass) {
        LOGE("Native registration unable to find class '%s'", activityClassName);
        return -1;
    }
    javaActivityClassRef = (jclass) env->NewGlobalRef(activityDataClass);
    javaVirtualMetricsUpdatedRef = env->GetMethodID(javaActivityClassRef, "virtualMetricsUpdated", "(IIII)V");
    javaSendEventRef = env->GetMethodID(javaActivityClassRef, "sendEvent", "(II)V");

    LOGD("JNI_OnLoad() complete!");

    return JNI_VERSION_1_4;
}

void virtualMetricsUpdated(int xVirtualOffset, int yVirtualOffset,
                         int virtualWidth, int virtualHeight) {
    jobject javaObjectRef = env->NewObject(javaActivityClassRef, javaVirtualMetricsUpdatedRef);
    env->CallVoidMethod(javaObjectRef, javaVirtualMetricsUpdatedRef,
                        xVirtualOffset, yVirtualOffset,
                        virtualWidth, virtualHeight);
}

void sendEvent(int x, int y) {
    jobject javaObjectRef = env->NewObject(javaActivityClassRef, javaSendEventRef);
    env->CallVoidMethod(javaObjectRef, javaSendEventRef,
                        x, y);
}
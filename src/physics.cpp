C++
#include <jni.h>
#include <android/log.h>

#define TAG "SBARealisticPhysics"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

extern "C"
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved) {
    LOGI("SBA Realistic Physics loaded!");
    LOGI("Waiting for game runtime...");
    return JNI_VERSION_1_6;
}

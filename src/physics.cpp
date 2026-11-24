#include <jni.h>
#include <android/log.h>

#include "unity_map_runtime.h"

#define TAG "SBACostumeMapMaker"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, TAG, __VA_ARGS__)

extern "C" JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM*, void*) {
    LOGI("Costume Map Maker loaded");
    LOGI("Waiting for IL2CPP before loading the saved map");
    return JNI_VERSION_1_6;
}

extern "C" __attribute__((visibility("default"))) void sba_il2cpp_ready() {
    LOGI("IL2CPP is ready");
    sba::costume_map::load_into_game();
}

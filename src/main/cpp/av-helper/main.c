#include <jni.h>
#include <android/log.h>
#include <stdlib.h>
#include <string.h>

#include "av-helper.h"

JNIEXPORT jint JNICALL
Java_io_github_tobelogin_FormatConverter_00024Companion_nativeList2Webp(JNIEnv *env, jobject thiz,
                                                                        jbyteArray list_path,
                                                                        jbyteArray webp_path)
{
    jbyte *tmp=(*env)->GetByteArrayElements(env, list_path, NULL);
    jsize len=(*env)->GetArrayLength(env, list_path);
    char *list_path_utf=malloc(len+1);
    if (list_path_utf == NULL)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "nativeList2webp", "Unable to allocate list file path buffer.\n");
        (*env)->ReleaseByteArrayElements(env, list_path, tmp, JNI_ABORT);
        return -1;
    }
    strncpy(list_path_utf, (const char *)tmp, len);
    list_path_utf[len]=0; // 如果 src 更长则 strncpy 不会自动加 null
    (*env)->ReleaseByteArrayElements(env, list_path, tmp, JNI_ABORT);

    tmp=(*env)->GetByteArrayElements(env, webp_path, NULL);
    len=(*env)->GetArrayLength(env, webp_path);
    char *webp_path_utf=malloc(len+1);
    if (webp_path_utf == NULL)
    {
        __android_log_print(ANDROID_LOG_DEBUG, "nativeList2webp", "Unable to allocate list file path buffer.\n");
        (*env)->ReleaseByteArrayElements(env, webp_path, tmp, JNI_ABORT);
        return -1;
    }
    strncpy(webp_path_utf, (const char *)tmp, len);
    webp_path_utf[len]=0;
    (*env)->ReleaseByteArrayElements(env, webp_path, tmp, JNI_ABORT);

    __android_log_print(ANDROID_LOG_DEBUG, "nativeList2webp", "List path is %s, webp path is %s\n", list_path_utf, webp_path_utf);
    int ret = list2webp((const char*)list_path_utf, (const char*)webp_path_utf);
    free(list_path_utf);
    free(webp_path_utf);
    return ret;
}
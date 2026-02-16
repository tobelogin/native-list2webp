#include <jni.h>
#include <android/log.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

#include "frame_queue.h"

// TODO 保留原队列中的内容，或者直接对原队列中的帧进行操作
int color_converse_batch(FrameQueue *origin, FrameQueue *dst, enum AVPixelFormat dst_fmt)
{
    AVFrame *ori_frame = framequeue_pop(origin);
    AVFrame *tmp_frame = av_frame_alloc();
    if (tmp_frame == NULL)
    {
        fprintf(stderr, "Unable to alloc tmporary frame.\n");
        return -1;
    }
    int width = ori_frame->width, height = ori_frame->height;
    enum AVPixelFormat format = ori_frame->format;
    struct SwsContext *context = sws_getContext(width, height, format,
        width, height, dst_fmt, SWS_BICUBIC, NULL, NULL, NULL);
    if (context == NULL)
    {
        fprintf(stderr,
            "Error when allocating sws context.\n");
        exit(1);
    }

    int ret = sws_scale_frame(context, tmp_frame, ori_frame);
    while (ret >= 0)
    {
        tmp_frame->time_base = ori_frame->time_base;
        tmp_frame->duration = ori_frame->duration;
        tmp_frame->pts = ori_frame->pts;
        framequeue_add(dst, tmp_frame);
        tmp_frame = ori_frame;
        av_frame_unref(tmp_frame);
        ori_frame = framequeue_pop(origin);
        if (ori_frame == NULL)
        {
            break;
        }
        ret = sws_scale_frame(context, tmp_frame, ori_frame);
    }
    if (ret < 0)
    {
        fprintf(stderr,
            "Error when conversing pixel format: %s.\n", av_err2str(ret));
        return -1;
    }
    av_frame_free(&tmp_frame);

    sws_freeContext(context);
    return 0;
}

// 解码 concat 获取到的内容
int get_frames_from_list(const char *list_path, FrameQueue *queue)
{
    AVFormatContext *in_ctx = NULL;
    const AVInputFormat *in_fmt = av_find_input_format("concat");
    int ret=0;
    if (in_fmt == NULL)
    {
        fprintf(stderr, "Unable to find concat demuxer.\n");
        return -1;
    }

    AVDictionary *in_options = NULL;
    ret = av_dict_set(&in_options, "safe", "0", 0); // 相当于命令行选项 -update 1
    if (ret < 0)
    {
        fprintf(stderr, "Failed to set input context options!\n");
        return -1;
    }

    ret = avformat_open_input(&in_ctx, list_path, in_fmt, &in_options);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to open list file!\n");
        return -1;
    }

    av_dict_free(&in_options);

    const AVCodec *decoder=avcodec_find_decoder(in_ctx->streams[0]->codecpar->codec_id); // 需要选对解码器
    AVCodecContext *dec_context = avcodec_alloc_context3(decoder); // 选对了解码器 libavcodec 才能猜测到视频流的像素格式
    if (!dec_context) {
        fprintf(stderr, "Could not alloc an decoding context\n");
        exit(1);
    }
    if(avformat_find_stream_info(in_ctx, NULL) < 0) // 需要先获取流信息
    {
        fprintf(stderr, "Error when reading input stream info.\n");
        exit(1);
    }
    if(avcodec_parameters_to_context(dec_context,
        in_ctx->streams[0]->codecpar) < 0) // 才能将解码器需要的信息读取出来
    {
        fprintf(stderr, "Error when filling dec_context.\n");
        exit(1);
    }
    ret = avcodec_open2(dec_context, decoder, NULL);
    if (ret < 0) {
        fprintf(stderr,
            "Could not open video codec: %s\n", av_err2str(ret));
        exit(1);
    }

    AVPacket *packet=av_packet_alloc();
    if (packet == NULL)
    {
        fprintf(stderr, "Unable to allocate packet.\n");
        return -1;
    }
    ret = av_read_frame(in_ctx, packet);
    while (ret >= 0)
    {
        AVFrame *dec_frame = av_frame_alloc();
        if (dec_frame == NULL)
        {
            fprintf(stderr, "Unable to allocate frame for decoding.\n");
            return -1;
        }
        ret = avcodec_send_packet(dec_context, packet);
        if(ret < 0)
        {
            fprintf(stderr,
                "Error when sending packet: %s.\n", av_err2str(ret));
            return -1;
        }
        av_packet_unref(packet);
        ret = avcodec_receive_frame(dec_context, dec_frame);
        if (ret < 0)
        {
            fprintf(stderr,
                "Error when recving frame: %s.\n", av_err2str(ret));
            return -1;
        }
        dec_frame->time_base = in_ctx->streams[0]->time_base;
        dec_frame->duration = in_ctx->streams[0]->duration;
        framequeue_add(queue, dec_frame);
        ret = av_read_frame(in_ctx, packet);
    }
    if(ret != AVERROR_EOF)
    {
        fprintf(stderr, "Unable to read picture\n");
        avformat_close_input(&in_ctx);
        return -1;
    }
    avcodec_send_packet(dec_context, NULL);
    av_packet_free(&packet);

    avcodec_free_context(&dec_context);
    avformat_close_input(&in_ctx);
    return 0;
}

int frames2webp(FrameQueue *queue, const char *out_path)
{
    AVFormatContext *output_context = NULL;

    const AVOutputFormat *output_format = av_guess_format("image2", out_path, NULL);
    if(avformat_alloc_output_context2(&output_context, output_format, "image2", out_path) < 0)
    {
        fprintf(stderr, "Unable to open output file\n");
        return -1;
    }
    const AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_WEBP);
    if(!encoder)
    {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(output_format->video_codec));
        return 1;
    }
    AVCodecContext *enc_context = avcodec_alloc_context3(encoder);
    if (!enc_context) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        return 1;
    }

    AVFrame *frame = framequeue_pop(queue);

    enc_context->codec_id = AV_CODEC_ID_WEBP;
    enc_context->width = frame->width;
    enc_context->height = frame->height;
    enc_context->time_base = frame->time_base;
    enc_context->pix_fmt = frame->format;

    int ret = avcodec_open2(enc_context, encoder, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
        return -1;
    }

    AVStream *output_stream = avformat_new_stream(output_context, NULL); // 第二个参数未使用
    if (!output_stream)
    {
        fprintf(stderr, "Could not allocate stream\n");
        exit(1);
    }
    if (avcodec_parameters_from_context(output_stream->codecpar, enc_context) < 0)
    {
        fprintf(stderr, "Could not set stream info\n");
        exit(1);
    }
    output_stream->id=output_context->nb_streams-1;
    output_stream->duration=frame->duration;
    // output_stream->nb_frames=1;

    while (frame != NULL && ret >= 0)
    {
        ret = avcodec_send_frame(enc_context, frame); // 这里似乎后来的帧会覆盖之前的帧
        av_frame_free(&frame);
        frame = framequeue_pop(queue);
    }
    if (ret < 0)
    {
        fprintf(stderr, "Unable to reencode frames.\n");
        return -1;
    }
    avcodec_send_frame(enc_context, NULL);

    AVPacket *packet = av_packet_alloc();
    if(packet == NULL)
    {
        fprintf(stderr, "Failed to allocate packet!\n");
        return -1;
    }

    AVDictionary *out_options = NULL;
    ret = av_dict_set(&out_options, "update", "1", 0); // 相当于命令行选项 -update 1
    (ret >= 0) && (ret = avformat_init_output(output_context, &out_options));
    if (ret < 0) {
        fprintf(stderr, "Could not set output context options.\n");
        return -1;
    }
    av_dict_free(&out_options);

    ret = avformat_write_header(output_context, NULL);
    if(ret < 0)
    {
        fprintf(stderr, "Error when writing header.\n");
        av_packet_unref(packet);
        av_packet_free(&packet);
        avformat_free_context(output_context);
        return 1;
    }
    ret = avcodec_receive_packet(enc_context, packet);
    while (ret >= 0)
    {
        ret = av_write_frame(output_context, packet);
        if (ret < 0)
        {
            fprintf(stderr, "Unable to write packet.\n");
            return -1;
        }
        av_packet_unref(packet);
        ret = avcodec_receive_packet(enc_context, packet);
    }
    if (ret != AVERROR_EOF)
    {
        fprintf(stderr, "Unable to read frames from decoder.\n");
    }
    av_write_frame(output_context, NULL);

    av_packet_free(&packet);
    avformat_free_context(output_context);
    return 0;
}

int list2webp(const char *list_path, const char *out_img)
{
    FrameQueue *ori_frames = framequeue_alloc();
    int ret = 0;
    if(ori_frames == NULL)
    {
        return -1;
    }
    ret = get_frames_from_list(list_path, ori_frames);
    if (ret < 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, "nativeList2webp", "Error when reading input frames.\n");
        return -1;
    }

    FrameQueue *yuv_frames = framequeue_alloc();
    if(yuv_frames == NULL)
    {
        return -1;
    }
    ret = color_converse_batch(ori_frames, yuv_frames, AV_PIX_FMT_YUV420P);
    if (ret < 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, "nativeList2webp", "Error when conversing pixel format.\n");
        return -1;
    }
    framequeue_free(ori_frames);
    ori_frames = NULL;

    ret = frames2webp(yuv_frames, out_img);
    if (ret < 0)
    {
        __android_log_print(ANDROID_LOG_ERROR, "nativeList2webp", "Error when encoding frames.\n");
        return -1;
    }
    framequeue_free(yuv_frames);

    return 0;
}

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

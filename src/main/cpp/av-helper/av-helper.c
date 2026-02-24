#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>

typedef struct {
    AVFormatContext *in_ctx, *out_ctx;
    AVCodecContext *dec_ctx, *enc_ctx;
    struct SwsContext *sws_ctx;
} LibavContext;

int init_in_ctx(AVFormatContext **in_ctx, const char *list_path)
{
    const AVInputFormat *in_fmt = av_find_input_format("concat");
    int ret=0;
    if (in_fmt == NULL)
    {
        fprintf(stderr, "Unable to find concat demuxer.\n");
        return -1;
    }

    AVDictionary *in_options = NULL;
    ret = av_dict_set(&in_options, "safe", "0", 0); // 相当于命令行选项 -safe 0
    if (ret < 0)
    {
        fprintf(stderr, "Failed to set input context options!\n");
        return -1;
    }

    ret = avformat_open_input(in_ctx, list_path, in_fmt, &in_options);
    if (ret < 0)
    {
        fprintf(stderr, "Failed to open list file!\n");
        return -1;
    }

    av_dict_free(&in_options);
    return 0;
}

int init_out_ctx(AVFormatContext **out_ctx,
    const AVCodecContext *enc_ctx, const char *webp_path)
{
    const AVOutputFormat *output_format = av_guess_format("image2", webp_path, NULL);
    if(avformat_alloc_output_context2(out_ctx, output_format, "image2", webp_path) < 0)
    {
        fprintf(stderr, "Unable to open output file\n");
        return -1;
    }

    AVStream *out_stream = avformat_new_stream(*out_ctx, NULL); // 第二个参数未使用
    if (!out_stream)
    {
        fprintf(stderr, "Could not allocate stream\n");
        return 1;
    }
    int ret = avcodec_parameters_from_context(out_stream->codecpar, enc_ctx);
    if (ret < 0)
    {
        fprintf(stderr, "Could not set stream info\n");
        return 1;
    }

    AVDictionary *out_options = NULL;
    ret = av_dict_set(&out_options, "update", "1", 0); // 相当于命令行选项 -update 1
    (ret >= 0) && (ret = avformat_init_output(*out_ctx, &out_options));
    if (ret < 0) {
        fprintf(stderr, "Could not set output context options.\n");
        return -1;
    }
    av_dict_free(&out_options);

    return 0;
}

int init_dec_ctx(AVCodecContext **dec_ctx, AVFormatContext *in_ctx)
{
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
    int ret = avcodec_open2(dec_context, decoder, NULL);
    if (ret < 0) {
        fprintf(stderr,
                "Could not open video codec: %s\n", av_err2str(ret));
        exit(1);
    }

    *dec_ctx = dec_context;
    return 0;
}

int init_enc_ctx(AVCodecContext **enc_ctx, const AVFormatContext *in_ctx,
        enum AVCodecID codec_id, enum AVPixelFormat pix_fmt)
{
    if (in_ctx->streams[0]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO)
    {
        fprintf(stderr, "Stream 0 is not a video stream.\n");
        return 1;
    }
    const AVCodec *encoder = avcodec_find_encoder(codec_id);
    if (!encoder)
    {
        fprintf(stderr, "Could not find encoder for '%s'\n",
                avcodec_get_name(codec_id));
        return 1;
    }
    *enc_ctx = avcodec_alloc_context3(encoder);
    if (!enc_ctx) {
        fprintf(stderr, "Could not alloc an encoding context\n");
        return 1;
    }
    AVCodecContext *enc_context = *enc_ctx;
    enc_context->codec_id = codec_id;
    enc_context->width = in_ctx->streams[0]->codecpar->width;
    enc_context->height = in_ctx->streams[0]->codecpar->height;
    enc_context->pix_fmt = pix_fmt;
    enc_context->time_base = in_ctx->streams[0]->time_base;

    int ret = avcodec_open2(*enc_ctx, encoder, NULL);
    if (ret < 0) {
        fprintf(stderr, "Could not open video codec: %s\n", av_err2str(ret));
        return -1;
    }

    return 0;
}

int init_sws_ctx(struct SwsContext **sws_ctx, const AVCodecContext *enc_ctx,
    const AVCodecContext *dec_ctx)
{
    *sws_ctx = sws_getContext(
        dec_ctx->width, dec_ctx->height, dec_ctx->pix_fmt,
        enc_ctx->width, enc_ctx->height, enc_ctx->pix_fmt,
        SWS_BICUBIC, NULL, NULL, NULL);
    if (*sws_ctx == NULL)
    {
        fprintf(stderr,
            "Error when allocating sws context.\n");
        return 1;
    }
    return 0;
}

int list2webp(const char *list_path, const char *webp_path)
{
    LibavContext av_ctx;
    memset(&av_ctx, 0, sizeof(av_ctx));
    int ret = 0;
    ret = init_in_ctx(&av_ctx.in_ctx, list_path);
    (ret >= 0) && (ret = init_dec_ctx(&av_ctx.dec_ctx, av_ctx.in_ctx));
    (ret >= 0) && (ret = init_enc_ctx(&av_ctx.enc_ctx, av_ctx.in_ctx,
        AV_CODEC_ID_WEBP, AV_PIX_FMT_YUV420P));
    (ret >= 0) && (ret = init_out_ctx(&av_ctx.out_ctx, av_ctx.enc_ctx, webp_path));
    (ret >= 0) && (ret = init_sws_ctx(&av_ctx.sws_ctx, av_ctx.enc_ctx, av_ctx.dec_ctx));
    if (ret < 0)
    {
        fprintf(stderr, "Initialize failed!\n");
        return -1;
    }


    AVPacket *in_packet=av_packet_alloc();
    AVPacket *out_packet=av_packet_alloc();
    if (in_packet == NULL || out_packet == NULL)
    {
        fprintf(stderr, "Unable to allocate packets.\n");
        av_packet_free(&in_packet);
        av_packet_free(&out_packet);
        return -1;
    }
    AVFrame *in_frame = av_frame_alloc();
    AVFrame *out_frame = av_frame_alloc();
    if (in_frame == NULL || out_frame == NULL)
    {
        fprintf(stderr, "Unable to allocate frames.\n");
        av_frame_free(&in_frame);
        av_frame_free(&out_frame);
        return -1;
    }

    ret = avformat_write_header(av_ctx.out_ctx, NULL);
    if (ret < 0)
    {
        fprintf(stderr, "Error when writing header.\n"); // TODO 释放内存
        return 1;
    }

    ret = av_read_frame(av_ctx.in_ctx, in_packet);
    // decode and encode a frame
    while (ret >= 0)
    {
        ret = avcodec_send_packet(av_ctx.dec_ctx, in_packet);
        if(ret < 0)
        {
            fprintf(stderr,
                "Error when sending packet: %s.\n", av_err2str(ret));
            return -1;
        }
        ret = avcodec_receive_frame(av_ctx.dec_ctx, in_frame);
        if (ret < 0)
        {
            fprintf(stderr,
                "Error when recving frame: %s.\n", av_err2str(ret));
            return -1;
        }

        // color conversion
        // out_frame->format = av_ctx.enc_ctx->pix_fmt;
        ret = sws_scale_frame(av_ctx.sws_ctx, out_frame, in_frame);
        if (ret < 0)
        {
            fprintf(stderr,
                "Error when conversing pixel format: %s.\n", av_err2str(ret));
            return -1;
        }

        // encode a frame
        out_frame->time_base = in_frame->time_base;
        out_frame->pts = in_frame->pts;
        ret = avcodec_send_frame(av_ctx.enc_ctx, out_frame);
        if (ret < 0)
        {
            fprintf(stderr,
                "Error when sending frame to encoder: %s.\n", av_err2str(ret));
            return -1;
        }
        av_packet_unref(in_packet);
        av_frame_unref(in_frame);
        av_frame_unref(out_frame);

        ret = av_read_frame(av_ctx.in_ctx, in_packet);
    }
    if (ret != AVERROR_EOF)
    {
        fprintf(stderr, "Unable to read input packet.\n");
    }
    else
    {
        ret = 0;
    }
    avcodec_send_packet(av_ctx.dec_ctx, NULL);
    avcodec_send_frame(av_ctx.enc_ctx, NULL);

    // 必须要先 send NULL 才能 receive packet
    // 但其实下面的循环也只执行了一次
    ret = avcodec_receive_packet(av_ctx.enc_ctx, out_packet);
    while (ret == 0)
    {
        ret = av_write_frame(av_ctx.out_ctx, out_packet);
        if (ret < 0)
        {
            fprintf(stderr, "Unable to write packet: %s.\n", av_err2str(ret));
            return -1;
        }
        av_packet_unref(out_packet);
        ret = avcodec_receive_packet(av_ctx.enc_ctx, out_packet);
    }
    if (ret != AVERROR_EOF)
    {
        fprintf(stderr, "Unable to read packet from encoder: %s.\n", av_err2str(ret));
        return -1;
    }
    else
    {
        ret = 0;
    }

    av_write_frame(av_ctx.out_ctx, NULL);
    av_write_trailer(av_ctx.out_ctx);

    av_packet_free(&in_packet);
    av_frame_free(&in_frame);
    av_frame_free(&out_frame);
    av_packet_free(&out_packet);

    sws_freeContext(av_ctx.sws_ctx);
    avcodec_free_context(&av_ctx.dec_ctx);
    avformat_close_input(&av_ctx.in_ctx);
    avcodec_free_context(&av_ctx.enc_ctx);
    avformat_free_context(av_ctx.out_ctx);

    return ret >= 0 ? 0 : ret;
}

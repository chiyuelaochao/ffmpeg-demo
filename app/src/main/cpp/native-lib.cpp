#include <jni.h>
#include <string>
#include <android/log.h>

#define MAX_AUDIO_FRME_SIZE 48000 * 4

extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
//重采样
#include "libswresample/swresample.h"

#include <android/native_window_jni.h>
#include <unistd.h>
}
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"jason",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"jason",FORMAT,##__VA_ARGS__);

extern "C" {
JNIEXPORT void JNICALL
Java_com_caiwei_ffmpeg_FFmpegUtils_sound(JNIEnv *env, jclass type, jstring input_, jstring output_) {
    const char *input = env->GetStringUTFChars(input_, 0);
    const char *output = env->GetStringUTFChars(output_, 0);
    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //打开音频文件
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0) {
        LOGI("%s", "无法打开音频文件");
        return;
    }
    //获取输入文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGI("%s", "无法获取输入文件信息");
        return;
    }
    //获取音频流索引位置
    int i = 0, audio_stream_idx = -1;
    for (; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }
//获取解码器
    AVCodecContext *codecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);

    //打开解码器
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        LOGI("%s", "无法打开解码器");
        return;
    }
    //压缩数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //解压缩数据
    AVFrame *frame = av_frame_alloc();
    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    SwrContext *swrContext = swr_alloc();
//    音频格式  重采样设置参数
    AVSampleFormat in_sample = codecCtx->sample_fmt;
//    输出采样格式
    AVSampleFormat out_sample = AV_SAMPLE_FMT_S16;
// 输入采样率
    int in_sample_rate = codecCtx->sample_rate;
//    输出采样
    int out_sample_rate = 44100;

//    输入声道布局
    uint64_t in_ch_layout = codecCtx->channel_layout;
//    输出声道布局
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    /**
     * struct SwrContext *swr_alloc_set_opts(struct SwrContext *s,
      int64_t out_ch_layout, enum AVSampleFormat out_sample_fmt, int out_sample_rate,
      int64_t  in_ch_layout, enum AVSampleFormat  in_sample_fmt, int  in_sample_rate,
      int log_offset, void *log_ctx);
     */
    swr_alloc_set_opts(swrContext, out_ch_layout, out_sample, out_sample_rate, in_ch_layout, in_sample, in_sample_rate,
                       0, NULL);
    swr_init(swrContext);
    int got_frame = 0;
    int ret;
    int out_channerl_nb = av_get_channel_layout_nb_channels(out_ch_layout);
    LOGE("声道数量%d ", out_channerl_nb);
    int count = 0;
//    设置音频缓冲区间 16bit   44100  PCM数据
    uint8_t *out_buffer = (uint8_t *) av_malloc(2 * 44100);
    FILE *fp_pcm = fopen(output, "wb");
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        ret = avcodec_decode_audio4(codecCtx, frame, &got_frame, packet);
        LOGE("正在解码%d", count++);
        if (ret < 0) {
            LOGE("解码完成");
        }
//        解码一帧
        if (got_frame > 0) {
            /**
             * int swr_convert(struct SwrContext *s, uint8_t **out, int out_count, const uint8_t **in , int in_count);
             */
            swr_convert(swrContext, &out_buffer, 2 * 44100, (const uint8_t **) frame->data, frame->nb_samples);
            /**
             * int av_samples_get_buffer_size(int *linesize, int nb_channels, int nb_samples, enum AVSampleFormat
             * sample_fmt, int align);
             */
            int out_buffer_size = av_samples_get_buffer_size(NULL, out_channerl_nb, frame->nb_samples, out_sample, 1);
            fwrite(out_buffer, 1, out_buffer_size, fp_pcm);
        }
    }
    fclose(fp_pcm);
    av_frame_free(&frame);
    av_free(out_buffer);
    swr_free(&swrContext);
    avcodec_close(codecCtx);
    avformat_close_input(&pFormatCtx);
    env->ReleaseStringUTFChars(input_, input);
    env->ReleaseStringUTFChars(output_, output);
}

JNIEXPORT void JNICALL
Java_com_caiwei_ffmpeg_FFmpegUtils_render(JNIEnv *env, jclass type, jstring input_, jobject surface) {
    const char *input = env->GetStringUTFChars(input_, false);
    av_register_all();

    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //第四个参数是 可以传一个 字典   是一个入参出参对象
    if (avformat_open_input(&pFormatCtx, input, NULL, NULL) != 0) {
        LOGE("%s", "打开输入视频文件失败");
    }
    //3.获取视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("%s", "获取视频信息失败");
        return;
    }

    int video_stream_idx = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            LOGE("  找到视频id %d", pFormatCtx->streams[i]->codec->codec_type);
            video_stream_idx = i;
            break;
        }
    }

    //    获取视频编解码器
    AVCodecContext *pCodecCtx = pFormatCtx->streams[video_stream_idx]->codec;
    LOGE("获取视频编码器上下文 %p  ", pCodecCtx);
    //    加密的用不了
    AVCodec *pCodex = avcodec_find_decoder(pCodecCtx->codec_id);
    LOGE("获取视频编码 %p", pCodex);
    //版本升级了
    if (avcodec_open2(pCodecCtx, pCodex, NULL) < 0) {


    }
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //    av_init_packet(packet);
    //    像素数据
    AVFrame *frame;
    frame = av_frame_alloc();
    //    RGB
    AVFrame *rgb_frame = av_frame_alloc();
    //    给缓冲区分配内存
    //只有指定了AVFrame的像素格式、画面大小才能真正分配内存
    //缓冲区分配内存
    uint8_t *out_buffer = (uint8_t *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height));
    LOGE("宽  %d,  高  %d  ", pCodecCtx->width, pCodecCtx->height);
    //设置yuvFrame的缓冲区，像素格式
    int re = avpicture_fill((AVPicture *) rgb_frame, out_buffer, AV_PIX_FMT_RGBA, pCodecCtx->width, pCodecCtx->height);
    LOGE("申请内存%d   ", re);

    //    输出需要改变
    int length = 0;
    int got_frame;
    //    输出文件
    int frameCount = 0;
    SwsContext *swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_RGBA, SWS_BICUBIC, NULL,
                                            NULL, NULL
    );
    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);
    //    视频缓冲区
    ANativeWindow_Buffer outBuffer;
    //    ANativeWindow
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        //        AvFrame
        if (packet->stream_index == video_stream_idx) {
            length = avcodec_decode_video2(pCodecCtx, frame, &got_frame, packet);
            LOGE(" 获得长度   %d ", length);

            //非零   正在解码
            if (got_frame) {
                //            绘制之前   配置一些信息  比如宽高   格式
                ANativeWindow_setBuffersGeometry(nativeWindow, pCodecCtx->width, pCodecCtx->height,
                                                 WINDOW_FORMAT_RGBA_8888);
                //            绘制
                ANativeWindow_lock(nativeWindow, &outBuffer, NULL);
                //     h 264   ----yuv          RGBA
                LOGI("解码%d帧", frameCount++);
                //转为指定的YUV420P
                sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0, pCodecCtx->height,
                          rgb_frame->data,
                          rgb_frame->linesize);
                //rgb_frame是有画面数据
                uint8_t *dst = (uint8_t *) outBuffer.bits;
                //            拿到一行有多少个字节 RGBA
                int destStride = outBuffer.stride * 4;
                //像素数据的首地址
                uint8_t *src = (uint8_t *) rgb_frame->data[0];
                //            实际内存一行数量
                int srcStride = rgb_frame->linesize[0];
                int i = 0;
                for (int i = 0; i < pCodecCtx->height; ++i) {
//                memcpy(void *dest, const void *src, size_t n)
                    memcpy(dst + i * destStride, src + i * srcStride, srcStride);
                }
                ANativeWindow_unlockAndPost(nativeWindow);
                usleep(1000 * 16);
            }
        }
        av_free_packet(packet);
    }
    ANativeWindow_release(nativeWindow);
    av_frame_free(&frame);
    avcodec_close(pCodecCtx);
    avformat_free_context(pFormatCtx);
    env->ReleaseStringUTFChars(input_, input);
}

JNIEXPORT void JNICALL
Java_com_caiwei_ffmpeg_FFmpegUtils_open(JNIEnv *env, jclass type, jstring inputStr_, jstring outStr_) {
    const char *inputStr = env->GetStringUTFChars(inputStr_, 0);
    const char *outStr = env->GetStringUTFChars(outStr_, 0);

    //    注册各大组件
    av_register_all();

    AVFormatContext *pContext = avformat_alloc_context();


    if (avformat_open_input(&pContext, inputStr, NULL, NULL) < 0) {
        LOGE("打开失败");
        return;
    }
    if (avformat_find_stream_info(pContext, NULL) < 0) {
        LOGE("获取信息失败");
        return;
    }

    int vedio_stream_idx = -1;
//    找到视频流
    for (int i = 0; i < pContext->nb_streams; ++i) {
        LOGE("循环  %d", i);
//      codec 每一个流 对应的解码上下文   codec_type 流的类型
        if (pContext->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            vedio_stream_idx = i;
        }
    }

//    获取到解码器上下文
    AVCodecContext *pCodecCtx = pContext->streams[vedio_stream_idx]->codec;

//    解码器
    AVCodec *pCodex = avcodec_find_decoder(pCodecCtx->codec_id);
//    ffempg版本升级
    if (avcodec_open2(pCodecCtx, pCodex, NULL) < 0) {
        LOGE("解码失败");
        return;
    }
//    分配内存   malloc  AVPacket   1   2
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
//    初始化结构体
    av_init_packet(packet);
    //还是不够
    AVFrame *frame = av_frame_alloc();
//    声明一个yuvframe
    AVFrame *yuvFrame = av_frame_alloc();
//    给yuvframe  的缓冲区 初始化

    uint8_t *out_buffer = (uint8_t *) av_malloc(
            avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));

    int re = avpicture_fill((AVPicture *) yuvFrame, out_buffer, AV_PIX_FMT_YUV420P, pCodecCtx->width,
                            pCodecCtx->height);
    LOGE("宽 %d  高 %d", pCodecCtx->width, pCodecCtx->height);
//    mp4   的上下文pCodecCtx->pix_fmt
    SwsContext *swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height, pCodecCtx->pix_fmt,
                                            pCodecCtx->width, pCodecCtx->height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, NULL,
                                            NULL, NULL
    );
    int frameCount = 0;
    FILE *fp_yuv = fopen(outStr, "wb");

//packet入参 出参对象  转换上下文
    int got_frame;
    while (av_read_frame(pContext, packet) >= 0) {
//        节封装

//        根据frame 进行原生绘制    bitmap  window
        avcodec_decode_video2(pCodecCtx, frame, &got_frame, packet);
//   frame  的数据拿到   视频像素数据 yuv   三个rgb    r   g  b   数据量大   三个通道
//        r  g  b  1824年    yuv 1970
        LOGE("解码%d  ", frameCount++);
        if (got_frame > 0) {
            sws_scale(swsContext, (const uint8_t *const *) frame->data, frame->linesize, 0, frame->height,
                      yuvFrame->data,
                      yuvFrame->linesize
            );
            int y_size = pCodecCtx->width * pCodecCtx->height;
//        y 亮度信息写完了
            fwrite(yuvFrame->data[0], 1, y_size, fp_yuv);
            fwrite(yuvFrame->data[1], 1, y_size / 4, fp_yuv);
            fwrite(yuvFrame->data[2], 1, y_size / 4, fp_yuv);
        }
        av_free_packet(packet);
    }
    fclose(fp_yuv);
    av_frame_free(&frame);
    av_frame_free(&yuvFrame);
    avcodec_close(pCodecCtx);
    avformat_free_context(pContext);

    env->ReleaseStringUTFChars(inputStr_, inputStr);
    env->ReleaseStringUTFChars(outStr_, outStr);
}
}
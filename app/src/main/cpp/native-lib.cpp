#include <jni.h>
#include <string>
#include <android/log.h>

extern "C" {
//编码
#include "libavcodec/avcodec.h"
//封装格式处理
#include "libavformat/avformat.h"
//像素处理
#include "libswscale/swscale.h"
}
#define LOGI(FORMAT, ...) __android_log_print(ANDROID_LOG_INFO,"jason",FORMAT,##__VA_ARGS__);
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"jason",FORMAT,##__VA_ARGS__);

extern "C"
JNIEXPORT void JNICALL
Java_com_caiwei_ffmpeg_TranslateActivity_open(JNIEnv *env, jobject instance, jstring inputStr_, jstring outStr_) {
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
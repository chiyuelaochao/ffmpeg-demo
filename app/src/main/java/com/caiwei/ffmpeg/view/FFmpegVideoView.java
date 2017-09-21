package com.caiwei.ffmpeg.view;

import android.content.Context;
import android.graphics.PixelFormat;
import android.util.AttributeSet;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import com.caiwei.ffmpeg.FFmpegUtils;

/**
 * Created by wei.cai on 2017/9/21.
 */

public class FFmpegVideoView extends SurfaceView {

    public FFmpegVideoView(Context context) {
        super(context);
    }

    public FFmpegVideoView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    private void init() {
        SurfaceHolder holder = getHolder();
        holder.setFormat(PixelFormat.RGBA_8888);
    }

    public FFmpegVideoView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public void player(final String input) {
        new Thread(new Runnable() {
            @Override
            public void run() {
                // 绘制功能 不需要交给SurfaveView,VideoView.this.getHolder().getSurface()
                FFmpegUtils.render(input, FFmpegVideoView.this.getHolder().getSurface());
            }
        }).start();
    }
}
package com.caiwei.ffmpeg;

import android.view.Surface;

/**
 * Created by wei.cai on 2017/9/21.
 */

public class FFmpegUtils {
    public static native void open(String inputStr, String outStr);

    public static native void render(String input, Surface surface);
}

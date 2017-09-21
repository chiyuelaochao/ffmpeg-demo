package com.caiwei.ffmpeg;

import android.os.Bundle;
import android.os.Environment;
import android.support.v7.app.AppCompatActivity;
import android.view.View;

import java.io.File;

public class PlayAudioActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_play_audio);
    }

    public void transcode(View view) {
        String input = new File(Environment.getExternalStorageDirectory(), "input.mp3").getAbsolutePath();
        String output = new File(Environment.getExternalStorageDirectory(), "output.pcm").getAbsolutePath();
        FFmpegUtils.sound(input, output);
    }
}

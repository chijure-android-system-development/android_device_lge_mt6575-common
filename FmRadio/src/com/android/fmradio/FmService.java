/*
 * Copyright (C) 2026 The CyanogenMod Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.fmradio;

import android.app.Notification;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.PowerManager;
import android.util.Log;

import com.mediatek.FMRadio.FMRadioNative;

import java.util.ArrayList;
import java.util.List;

/**
 * Drives the MT6620 FM receiver the way LG's stock FM app does:
 *
 *  - the chip through libfmjni (/dev/fm): opendev, powerup(MHz),
 *    switchAntenna(0) (the wired headset is the antenna), rdsset;
 *  - the audio through the stock audio HAL's own parameters:
 *    "fm_audio=active" opens the analog FM path to the headset,
 *    "fm_speakerphone=on|off" moves it to the speaker, "SetFmVolume=<x>"
 *    sets the chip volume (stock sends the music volume index / 100),
 *    "fm_audio=inactive" closes it.
 *
 * Stock LG/MTK frameworks mirror the music volume onto a STREAM_FM; CM has
 * no such stream, so the service follows the music volume itself.
 */
public class FmService extends Service {
    private static final String TAG = "FmService";

    public static final float MIN_FREQ = 87.5f;
    public static final float MAX_FREQ = 108.0f;
    public static final float STEP = 0.1f;
    private static final float DEFAULT_FREQ = 98.1f;

    public static final String ACTION_POWER_OFF = "com.android.fmradio.POWER_OFF";

    private static final int NOTIFICATION_ID = 1;
    private static final long RDS_POLL_MS = 500;

    /* libfmjni readrds() event bits */
    private static final int RDS_EVENT_PROGRAMNAME = 0x0008;

    private static final String VOLUME_CHANGED_ACTION =
            "android.media.VOLUME_CHANGED_ACTION";
    private static final String EXTRA_VOLUME_STREAM_TYPE =
            "android.media.EXTRA_VOLUME_STREAM_TYPE";

    public interface Listener {
        void onStateChanged();
    }

    public class LocalBinder extends Binder {
        public FmService getService() {
            return FmService.this;
        }
    }

    private final IBinder mBinder = new LocalBinder();
    private final Handler mHandler = new Handler();
    private final List<Listener> mListeners = new ArrayList<Listener>();

    private AudioManager mAudioManager;
    private PowerManager.WakeLock mWakeLock;
    private SharedPreferences mPrefs;

    private boolean mDeviceOpen;
    private volatile boolean mPoweredOn;
    private volatile boolean mBusy;
    private Thread mRdsThread;
    private boolean mSpeaker;
    private boolean mFocusMuted;
    private float mFrequency;
    private String mStationName = "";

    @Override
    public void onCreate() {
        super.onCreate();
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        PowerManager pm = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);
        mPrefs = getSharedPreferences("fmradio", MODE_PRIVATE);
        mFrequency = mPrefs.getFloat("frequency", DEFAULT_FREQ);

        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_HEADSET_PLUG);
        filter.addAction(AudioManager.ACTION_AUDIO_BECOMING_NOISY);
        filter.addAction(VOLUME_CHANGED_ACTION);
        registerReceiver(mReceiver, filter);
    }

    @Override
    public void onDestroy() {
        powerOff();
        unregisterReceiver(mReceiver);
        super.onDestroy();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (intent != null && ACTION_POWER_OFF.equals(intent.getAction())) {
            powerOff();
        }
        return START_NOT_STICKY;
    }

    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }

    public void addListener(Listener l) {
        mListeners.add(l);
    }

    public void removeListener(Listener l) {
        mListeners.remove(l);
    }

    private void notifyListeners() {
        for (Listener l : new ArrayList<Listener>(mListeners)) {
            l.onStateChanged();
        }
        if (mPoweredOn) {
            startForeground(NOTIFICATION_ID, buildNotification());
        }
    }

    public boolean isPoweredOn() {
        return mPoweredOn;
    }

    public boolean isBusy() {
        return mBusy;
    }

    public boolean isSpeaker() {
        return mSpeaker;
    }

    public float getFrequency() {
        return mFrequency;
    }

    public String getStationName() {
        return mStationName;
    }

    public boolean isAntennaPlugged() {
        return mAudioManager.isWiredHeadsetOn();
    }

    /** Turns the receiver on at the last frequency. */
    public boolean powerOn() {
        if (mPoweredOn) {
            return true;
        }
        if (!isAntennaPlugged()) {
            Log.w(TAG, "no headset, no antenna");
            return false;
        }
        if (mAudioManager.requestAudioFocus(mFocusListener,
                AudioManager.STREAM_MUSIC, AudioManager.AUDIOFOCUS_GAIN)
                != AudioManager.AUDIOFOCUS_REQUEST_GRANTED) {
            Log.w(TAG, "audio focus denied");
            return false;
        }
        if (!mDeviceOpen) {
            mDeviceOpen = FMRadioNative.opendev();
            if (!mDeviceOpen) {
                Log.e(TAG, "opendev failed");
                mAudioManager.abandonAudioFocus(mFocusListener);
                return false;
            }
        }
        if (!FMRadioNative.powerup(mFrequency)) {
            Log.e(TAG, "powerup " + mFrequency + " failed");
            mAudioManager.abandonAudioFocus(mFocusListener);
            return false;
        }
        FMRadioNative.switchAntenna(0);
        FMRadioNative.rdsset(true);

        mSpeaker = false;
        mAudioManager.setParameters("fm_audio=active;fm_speakerphone=off;fm_mute=loud");
        applyVolume();

        mPoweredOn = true;
        mFocusMuted = false;
        mStationName = "";
        mWakeLock.acquire();
        startService(new Intent(this, FmService.class));
        mRdsThread = new Thread(mRdsLoop, "FmRds");
        mRdsThread.start();
        notifyListeners();
        return true;
    }

    public void powerOff() {
        if (!mPoweredOn) {
            return;
        }
        mPoweredOn = false;
        if (mRdsThread != null) {
            mRdsThread.interrupt();
            mRdsThread = null;
        }
        mAudioManager.setParameters("fm_audio=inactive");
        FMRadioNative.rdsset(false);
        FMRadioNative.powerdown(0);
        if (mDeviceOpen) {
            FMRadioNative.closedev();
            mDeviceOpen = false;
        }
        mAudioManager.abandonAudioFocus(mFocusListener);
        mStationName = "";
        if (mWakeLock.isHeld()) {
            mWakeLock.release();
        }
        stopForeground(true);
        stopSelf();
        notifyListeners();
    }

    public void setSpeaker(boolean on) {
        if (!mPoweredOn) {
            return;
        }
        mSpeaker = on;
        mAudioManager.setParameters("fm_speakerphone=" + (on ? "on" : "off"));
        notifyListeners();
    }

    /** Tunes to a frequency in MHz, clamped to the band. */
    public void tune(float frequency) {
        frequency = clamp(frequency);
        if (!mPoweredOn) {
            setFrequency(frequency);
            notifyListeners();
            return;
        }
        if (FMRadioNative.tune(frequency)) {
            setFrequency(frequency);
        }
        notifyListeners();
    }

    /** Seeks to the next station; runs in the background. */
    public void seek(final boolean up) {
        if (!mPoweredOn || mBusy) {
            return;
        }
        mBusy = true;
        notifyListeners();
        new Thread(new Runnable() {
            public void run() {
                final float found = FMRadioNative.seek(mFrequency, up);
                mHandler.post(new Runnable() {
                    public void run() {
                        mBusy = false;
                        if (found >= MIN_FREQ && found <= MAX_FREQ) {
                            FMRadioNative.tune(found);
                            setFrequency(found);
                        }
                        notifyListeners();
                    }
                });
            }
        }).start();
    }

    public interface ScanCallback {
        void onScanFinished(float[] stations);
    }

    /** Scans the whole band; the callback gets the stations in MHz. */
    public void scan(final ScanCallback callback) {
        if (!mPoweredOn || mBusy) {
            return;
        }
        mBusy = true;
        notifyListeners();
        new Thread(new Runnable() {
            public void run() {
                short[] raw = FMRadioNative.autoscan();
                final List<Float> list = new ArrayList<Float>();
                if (raw != null) {
                    for (short s : raw) {
                        float f = s > 2000 ? s / 100f : s / 10f;
                        if (f >= MIN_FREQ && f <= MAX_FREQ && !list.contains(f)) {
                            list.add(f);
                        }
                    }
                }
                mHandler.post(new Runnable() {
                    public void run() {
                        mBusy = false;
                        // autoscan leaves the chip off the station; tune back.
                        FMRadioNative.tune(mFrequency);
                        float[] result = new float[list.size()];
                        for (int i = 0; i < result.length; i++) {
                            result[i] = list.get(i);
                        }
                        notifyListeners();
                        callback.onScanFinished(result);
                    }
                });
            }
        }).start();
    }

    private void setFrequency(float frequency) {
        mFrequency = Math.round(frequency * 10) / 10f;
        mStationName = "";
        mPrefs.edit().putFloat("frequency", mFrequency).apply();
    }

    private static float clamp(float f) {
        if (f < MIN_FREQ) {
            return MAX_FREQ;
        }
        if (f > MAX_FREQ) {
            return MIN_FREQ;
        }
        return f;
    }

    /* The stock HAL takes the music volume index divided by 100. */
    private void applyVolume() {
        int index = mAudioManager.getStreamVolume(AudioManager.STREAM_MUSIC);
        mAudioManager.setParameters("SetFmVolume=" + (index / 100f));
    }

    /*
     * readrds() can block until the chip has RDS data, so it runs on its own
     * thread (LG's app does the same), never on the main one. It pauses
     * while a seek or scan uses the chip.
     */
    private final Runnable mRdsLoop = new Runnable() {
        public void run() {
            while (mPoweredOn && !Thread.currentThread().isInterrupted()) {
                if (!mBusy) {
                    short events = FMRadioNative.readrds();
                    if ((events & RDS_EVENT_PROGRAMNAME) != 0) {
                        byte[] ps = FMRadioNative.getPS();
                        if (ps != null) {
                            final String name = new String(ps).trim();
                            mHandler.post(new Runnable() {
                                public void run() {
                                    if (mPoweredOn && !name.equals(mStationName)) {
                                        mStationName = name;
                                        notifyListeners();
                                    }
                                }
                            });
                        }
                    }
                }
                try {
                    Thread.sleep(RDS_POLL_MS);
                } catch (InterruptedException e) {
                    return;
                }
            }
        }
    };

    private final AudioManager.OnAudioFocusChangeListener mFocusListener =
            new AudioManager.OnAudioFocusChangeListener() {
        public void onAudioFocusChange(int focusChange) {
            switch (focusChange) {
                case AudioManager.AUDIOFOCUS_LOSS:
                    powerOff();
                    break;
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT:
                case AudioManager.AUDIOFOCUS_LOSS_TRANSIENT_CAN_DUCK:
                    if (mPoweredOn && !mFocusMuted) {
                        FMRadioNative.setmute(true);
                        mFocusMuted = true;
                    }
                    break;
                case AudioManager.AUDIOFOCUS_GAIN:
                    if (mPoweredOn && mFocusMuted) {
                        FMRadioNative.setmute(false);
                        mFocusMuted = false;
                    }
                    break;
            }
        }
    };

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (Intent.ACTION_HEADSET_PLUG.equals(action)) {
                if (intent.getIntExtra("state", 0) == 0) {
                    powerOff();
                }
                notifyListeners();
            } else if (AudioManager.ACTION_AUDIO_BECOMING_NOISY.equals(action)) {
                powerOff();
            } else if (VOLUME_CHANGED_ACTION.equals(action)) {
                if (mPoweredOn && intent.getIntExtra(EXTRA_VOLUME_STREAM_TYPE, -1)
                        == AudioManager.STREAM_MUSIC) {
                    applyVolume();
                }
            }
        }
    };

    private Notification buildNotification() {
        PendingIntent content = PendingIntent.getActivity(this, 0,
                new Intent(this, FmRadioActivity.class), 0);
        PendingIntent off = PendingIntent.getService(this, 0,
                new Intent(this, FmService.class).setAction(ACTION_POWER_OFF), 0);
        String title = FmRadioActivity.formatFrequency(mFrequency);
        String text = mStationName.length() > 0 ? mStationName
                : getString(R.string.app_name);
        return new Notification.Builder(this)
                .setSmallIcon(R.drawable.ic_stat_fm)
                .setContentTitle(title)
                .setContentText(text)
                .setContentIntent(content)
                .setOngoing(true)
                .addAction(R.drawable.ic_stat_power, getString(R.string.power_off), off)
                .build();
    }
}

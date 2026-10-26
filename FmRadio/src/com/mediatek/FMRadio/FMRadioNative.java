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

package com.mediatek.FMRadio;

/**
 * Natives of the stock libfmjni.so (MediaTek FM receiver). The library
 * registers all of them on this class name when it loads, so every one has
 * to exist here even if the app does not call it.
 *
 * Frequencies are in MHz (98.1f), except autoscan(), which returns them in
 * 100 kHz units (981).
 */
public final class FMRadioNative {
    static {
        System.loadLibrary("fmjni");
    }

    private FMRadioNative() {
    }

    public static native boolean opendev();
    public static native boolean closedev();
    public static native boolean powerup(float frequency);
    public static native boolean powerdown(int type);
    public static native boolean tune(float frequency);
    public static native float seek(float frequency, boolean isUp);
    public static native short[] autoscan();
    public static native boolean stopscan();
    public static native int readRssi();
    public static native int rdsset(boolean rdson);
    public static native short readrds();
    public static native short getPI();
    public static native byte getPTY();
    public static native byte[] getPS();
    public static native byte[] getLRText();
    public static native short activeAF();
    public static native short[] getAFList();
    public static native short activeTA();
    public static native short deactiveTA();
    public static native int setmute(boolean mute);
    public static native int getchipid();
    public static native int isRDSsupport();
    public static native int isFMPoweredUp();
    public static native boolean setFMViaBTController(boolean enable);
    public static native int switchAntenna(int antenna);
    public static native boolean stereoMono();
    public static native short readCapArray();
    public static native short readRdsBler();
    public static native boolean setStereoMono(boolean isMono);
    public static native int[] getHardwareVersion();
}

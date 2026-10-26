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

import android.app.Activity;
import android.app.AlertDialog;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.IBinder;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.BaseAdapter;
import android.widget.GridView;
import android.widget.ImageButton;
import android.widget.TextView;
import android.widget.Toast;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

/**
 * Main screen, with the layout and icons of AOSP's FM Radio app (Android
 * 5.1, Apache 2.0) rebuilt with JB widgets: the frequency on a blue-grey
 * header, prev/-/favorite/+/next, a floating play button, and the
 * favorites as cards. The radio itself is FmService.
 */
public class FmRadioActivity extends Activity implements FmService.Listener {
    private FmService mService;

    private TextView mFrequencyView;
    private TextView mStationView;
    private ImageButton mPlayButton;
    private ImageButton mSpeakerButton;
    private ImageButton mFavoriteButton;
    private View mNoHeadset;
    private View[] mRadioButtons;
    private ProgressDialog mScanDialog;

    private SharedPreferences mPrefs;
    private final List<Float> mFavorites = new ArrayList<Float>();
    private final List<Float> mStations = new ArrayList<Float>();
    private FavoritesAdapter mAdapter;

    static String formatFrequency(float mhz) {
        return String.format(Locale.US, "%.1f", mhz);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        setVolumeControlStream(AudioManager.STREAM_MUSIC);
        mPrefs = getSharedPreferences("fmradio", MODE_PRIVATE);

        mFrequencyView = (TextView) findViewById(R.id.station_value);
        mStationView = (TextView) findViewById(R.id.station_name);
        mPlayButton = (ImageButton) findViewById(R.id.play_button);
        mSpeakerButton = (ImageButton) findViewById(R.id.button_speaker);
        mFavoriteButton = (ImageButton) findViewById(R.id.button_add_to_favorite);
        mNoHeadset = findViewById(R.id.no_headset);
        mRadioButtons = new View[] {
            findViewById(R.id.button_prevstation), findViewById(R.id.button_decrease),
            findViewById(R.id.button_increase), findViewById(R.id.button_nextstation),
            mSpeakerButton, findViewById(R.id.button_list),
        };

        mPlayButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService == null) {
                    return;
                }
                if (mService.isPoweredOn()) {
                    mService.powerOff();
                } else if (!mService.powerOn()) {
                    Toast.makeText(FmRadioActivity.this, R.string.power_on_failed,
                            Toast.LENGTH_LONG).show();
                }
            }
        });
        findViewById(R.id.button_prevstation).setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService != null) mService.seek(false);
            }
        });
        findViewById(R.id.button_nextstation).setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService != null) mService.seek(true);
            }
        });
        findViewById(R.id.button_decrease).setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService != null) {
                    mService.tune(mService.getFrequency() - FmService.STEP);
                }
            }
        });
        findViewById(R.id.button_increase).setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService != null) {
                    mService.tune(mService.getFrequency() + FmService.STEP);
                }
            }
        });
        mSpeakerButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService != null) mService.setSpeaker(!mService.isSpeaker());
            }
        });
        mFavoriteButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService == null) {
                    return;
                }
                Float f = mService.getFrequency();
                if (mFavorites.contains(f)) {
                    mFavorites.remove(f);
                } else {
                    mFavorites.add(f);
                    Collections.sort(mFavorites);
                }
                saveList("favorites", mFavorites);
                onStateChanged();
            }
        });
        findViewById(R.id.button_list).setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                showStationList();
            }
        });

        GridView grid = (GridView) findViewById(R.id.favorites);
        mAdapter = new FavoritesAdapter();
        grid.setAdapter(mAdapter);
        grid.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int pos, long id) {
                tuneTo(mFavorites.get(pos));
            }
        });
        grid.setOnItemLongClickListener(new AdapterView.OnItemLongClickListener() {
            public boolean onItemLongClick(AdapterView<?> parent, View view, int pos, long id) {
                confirmRemoveFavorite(mFavorites.get(pos));
                return true;
            }
        });

        loadList("favorites", mFavorites);
        loadList("stations", mStations);

        Intent intent = new Intent(this, FmService.class);
        startService(intent);
        bindService(intent, mConnection, Context.BIND_AUTO_CREATE);
    }

    @Override
    protected void onDestroy() {
        if (mService != null) {
            mService.removeListener(this);
        }
        unbindService(mConnection);
        super.onDestroy();
    }

    private final ServiceConnection mConnection = new ServiceConnection() {
        public void onServiceConnected(ComponentName name, IBinder binder) {
            mService = ((FmService.LocalBinder) binder).getService();
            mService.addListener(FmRadioActivity.this);
            onStateChanged();
        }

        public void onServiceDisconnected(ComponentName name) {
            mService = null;
        }
    };

    public void onStateChanged() {
        if (mService == null) {
            return;
        }
        boolean on = mService.isPoweredOn();
        boolean busy = mService.isBusy();
        float freq = mService.getFrequency();

        mNoHeadset.setVisibility(!on && !mService.isAntennaPlugged()
                ? View.VISIBLE : View.GONE);
        mFrequencyView.setText(formatFrequency(freq));
        mStationView.setText(busy ? getString(R.string.station_searching_tips)
                : mService.getStationName());
        mPlayButton.setImageResource(on ? R.drawable.btn_fm_stop_selector
                : R.drawable.btn_fm_start_selector);
        mSpeakerButton.setImageResource(mService.isSpeaker()
                ? R.drawable.btn_fm_speaker_selector : R.drawable.btn_fm_headset_selector);
        mSpeakerButton.setContentDescription(getString(mService.isSpeaker()
                ? R.string.optmenu_earphone : R.string.optmenu_speaker));
        mFavoriteButton.setImageResource(mFavorites.contains(freq)
                ? R.drawable.btn_fm_favorite_on_selector
                : R.drawable.btn_fm_favorite_off_selector);
        for (View b : mRadioButtons) {
            b.setEnabled(on && !busy);
        }
        mPlayButton.setEnabled(!busy);
        mAdapter.notifyDataSetChanged();
    }

    private void tuneTo(float freq) {
        if (mService != null) {
            mService.tune(freq);
        }
    }

    private void confirmRemoveFavorite(final float freq) {
        new AlertDialog.Builder(this)
                .setTitle(formatFrequency(freq))
                .setItems(new CharSequence[] { getString(R.string.remove_favorite) },
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface d, int which) {
                                mFavorites.remove(Float.valueOf(freq));
                                saveList("favorites", mFavorites);
                                onStateChanged();
                            }
                        })
                .show();
    }

    private void showStationList() {
        if (mStations.isEmpty()) {
            scanStations();
            return;
        }
        CharSequence[] items = new CharSequence[mStations.size()];
        for (int i = 0; i < items.length; i++) {
            items[i] = formatFrequency(mStations.get(i)) + " MHz";
        }
        new AlertDialog.Builder(this)
                .setTitle(R.string.station_title)
                .setItems(items, new DialogInterface.OnClickListener() {
                    public void onClick(DialogInterface d, int which) {
                        tuneTo(mStations.get(which));
                    }
                })
                .setPositiveButton(R.string.fm_station_list_menu_refresh,
                        new DialogInterface.OnClickListener() {
                            public void onClick(DialogInterface d, int which) {
                                scanStations();
                            }
                        })
                .show();
    }

    private void scanStations() {
        if (mService == null || !mService.isPoweredOn()) {
            return;
        }
        mScanDialog = ProgressDialog.show(this, getString(R.string.station_title),
                getString(R.string.station_searching_tips), true, false);
        mService.scan(new FmService.ScanCallback() {
            public void onScanFinished(float[] found) {
                if (mScanDialog != null) {
                    mScanDialog.dismiss();
                    mScanDialog = null;
                }
                mStations.clear();
                for (float f : found) {
                    mStations.add(f);
                }
                saveList("stations", mStations);
                if (found.length == 0) {
                    Toast.makeText(FmRadioActivity.this, R.string.station_empty_text,
                            Toast.LENGTH_SHORT).show();
                } else {
                    Toast.makeText(FmRadioActivity.this,
                            getString(R.string.scan_found, found.length),
                            Toast.LENGTH_SHORT).show();
                    showStationList();
                }
            }
        });
    }

    private void loadList(String key, List<Float> list) {
        list.clear();
        for (String s : mPrefs.getString(key, "").split(",")) {
            if (s.length() > 0) {
                try {
                    list.add(Float.parseFloat(s));
                } catch (NumberFormatException e) {
                    // skip
                }
            }
        }
    }

    private void saveList(String key, List<Float> list) {
        StringBuilder sb = new StringBuilder();
        for (Float f : list) {
            if (sb.length() > 0) {
                sb.append(',');
            }
            sb.append(f);
        }
        mPrefs.edit().putString(key, sb.toString()).apply();
    }

    private class FavoritesAdapter extends BaseAdapter {
        public int getCount() {
            return mFavorites.size();
        }

        public Object getItem(int pos) {
            return mFavorites.get(pos);
        }

        public long getItemId(int pos) {
            return pos;
        }

        public View getView(int pos, View view, ViewGroup parent) {
            if (view == null) {
                view = LayoutInflater.from(FmRadioActivity.this)
                        .inflate(R.layout.favorite_item, parent, false);
            }
            float freq = mFavorites.get(pos);
            boolean current = mService != null && mService.isPoweredOn()
                    && mService.getFrequency() == freq;
            ((TextView) view.findViewById(R.id.station_freq)).setText(formatFrequency(freq));
            ((TextView) view.findViewById(R.id.station_name)).setText(
                    current ? mService.getStationName() : "");
            view.findViewById(R.id.playing).setVisibility(
                    current ? View.VISIBLE : View.INVISIBLE);
            return view;
        }
    }
}

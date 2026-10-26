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
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.SharedPreferences;
import android.media.AudioManager;
import android.os.Bundle;
import android.os.IBinder;
import android.view.View;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Locale;

public class FmRadioActivity extends Activity implements FmService.Listener {
    private FmService mService;

    private TextView mFrequencyView;
    private TextView mStationView;
    private TextView mStatusView;
    private Button mPowerButton;
    private ToggleButton mSpeakerButton;
    private Button mScanButton;
    private Button mFavoriteButton;
    private View[] mTuneButtons;

    private SharedPreferences mPrefs;
    private final List<Float> mStations = new ArrayList<Float>();
    private ArrayAdapter<String> mAdapter;

    static String formatFrequency(float mhz) {
        return String.format(Locale.US, "%.1f MHz", mhz);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.main);
        setVolumeControlStream(AudioManager.STREAM_MUSIC);
        mPrefs = getSharedPreferences("fmradio", MODE_PRIVATE);

        mFrequencyView = (TextView) findViewById(R.id.frequency);
        mStationView = (TextView) findViewById(R.id.station);
        mStatusView = (TextView) findViewById(R.id.status);
        mPowerButton = (Button) findViewById(R.id.power);
        mSpeakerButton = (ToggleButton) findViewById(R.id.speaker);
        mScanButton = (Button) findViewById(R.id.scan);
        mFavoriteButton = (Button) findViewById(R.id.favorite);
        mTuneButtons = new View[] {
            findViewById(R.id.seek_down), findViewById(R.id.step_down),
            findViewById(R.id.step_up), findViewById(R.id.seek_up),
        };

        mPowerButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService == null) {
                    return;
                }
                if (mService.isPoweredOn()) {
                    mService.powerOff();
                } else if (!mService.isAntennaPlugged()) {
                    Toast.makeText(FmRadioActivity.this, R.string.plug_headset,
                            Toast.LENGTH_LONG).show();
                } else if (!mService.powerOn()) {
                    Toast.makeText(FmRadioActivity.this, R.string.power_on_failed,
                            Toast.LENGTH_LONG).show();
                }
            }
        });
        findViewById(R.id.seek_down).setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService != null) mService.seek(false);
            }
        });
        findViewById(R.id.seek_up).setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService != null) mService.seek(true);
            }
        });
        findViewById(R.id.step_down).setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService != null) {
                    mService.tune(mService.getFrequency() - FmService.STEP);
                }
            }
        });
        findViewById(R.id.step_up).setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService != null) {
                    mService.tune(mService.getFrequency() + FmService.STEP);
                }
            }
        });
        mSpeakerButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService != null) mService.setSpeaker(mSpeakerButton.isChecked());
            }
        });
        mScanButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService == null) {
                    return;
                }
                mService.scan(new FmService.ScanCallback() {
                    public void onScanFinished(float[] found) {
                        mStations.clear();
                        for (float f : found) {
                            mStations.add(f);
                        }
                        saveStations();
                        Toast.makeText(FmRadioActivity.this,
                                getString(R.string.scan_found, found.length),
                                Toast.LENGTH_SHORT).show();
                    }
                });
            }
        });
        mFavoriteButton.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
                if (mService == null) {
                    return;
                }
                Float f = mService.getFrequency();
                if (mStations.contains(f)) {
                    mStations.remove(f);
                } else {
                    mStations.add(f);
                    Collections.sort(mStations);
                }
                saveStations();
            }
        });

        ListView list = (ListView) findViewById(R.id.stations);
        mAdapter = new ArrayAdapter<String>(this, android.R.layout.simple_list_item_1);
        list.setAdapter(mAdapter);
        list.setOnItemClickListener(new AdapterView.OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int pos, long id) {
                if (mService != null) mService.tune(mStations.get(pos));
            }
        });
        loadStations();

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

        mFrequencyView.setText(formatFrequency(mService.getFrequency()));
        mStationView.setText(mService.getStationName());
        if (busy) {
            mStatusView.setText(R.string.searching);
        } else if (!on && !mService.isAntennaPlugged()) {
            mStatusView.setText(R.string.plug_headset);
        } else {
            mStatusView.setText("");
        }
        mPowerButton.setText(on ? R.string.power_off : R.string.power_on);
        mSpeakerButton.setChecked(mService.isSpeaker());
        mSpeakerButton.setEnabled(on && !busy);
        mScanButton.setEnabled(on && !busy);
        for (View b : mTuneButtons) {
            b.setEnabled(!busy);
        }
        mFavoriteButton.setText(mStations.contains(mService.getFrequency())
                ? R.string.remove_station : R.string.add_station);
    }

    private void loadStations() {
        mStations.clear();
        String saved = mPrefs.getString("stations", "");
        for (String s : saved.split(",")) {
            if (s.length() > 0) {
                try {
                    mStations.add(Float.parseFloat(s));
                } catch (NumberFormatException e) {
                    // skip
                }
            }
        }
        refreshList();
    }

    private void saveStations() {
        StringBuilder sb = new StringBuilder();
        for (Float f : mStations) {
            if (sb.length() > 0) {
                sb.append(',');
            }
            sb.append(f);
        }
        mPrefs.edit().putString("stations", sb.toString()).apply();
        refreshList();
        onStateChanged();
    }

    private void refreshList() {
        mAdapter.clear();
        for (Float f : mStations) {
            mAdapter.add(formatFrequency(f));
        }
        mAdapter.notifyDataSetChanged();
    }
}

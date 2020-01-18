package com.example.chariotapp;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattServer;
import android.bluetooth.BluetoothGattServerCallback;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.bluetooth.BluetoothProfile;
import android.bluetooth.le.AdvertiseCallback;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertiseSettings;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.ColorStateList;
import android.graphics.Color;
import android.os.ParcelUuid;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.text.SimpleDateFormat;
import java.util.Calendar;
import java.util.Locale;
import java.util.UUID;

public class MainActivity extends AppCompatActivity {
    private static final String LOG_TAG = "CharIoT_App";
    private BluetoothManager mBluetoothManager;
    private BluetoothAdapter mBluetoothAdapter;
    private BluetoothLeAdvertiser mBluetoothLeAdvertiser;
    private BluetoothGattServer mGattServer;
    private TextView textOutput;
    private TextView dev_1_conn_text;
    private TextView dev_2_conn_text;
    private TextView dev_3_conn_text;
    private BluetoothDevice dev_1 = null;
    private BluetoothDevice dev_2 = null;
    private BluetoothDevice dev_3 = null;
    private String buttonValue;
    private int autoColorValue;
    private Button button_red;
    private Button button_green;
    private Button button_blue;
    private Button button_auto;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        autoColorValue = getResources().getColor(R.color.color_button_text_blue);
        mBluetoothManager = (BluetoothManager) getSystemService(BLUETOOTH_SERVICE);
        mBluetoothAdapter = mBluetoothManager.getAdapter();
        textOutput = findViewById(R.id.textOutput);
        dev_1_conn_text = findViewById(R.id.connected_text_1);
        dev_2_conn_text = findViewById(R.id.connected_text_2);
        dev_3_conn_text = findViewById(R.id.connected_text_3);
        button_red = findViewById(R.id.button_1);
        button_green = findViewById(R.id.button_2);
        button_blue = findViewById(R.id.button_3);
        button_auto = findViewById(R.id.button_4);
        onClickButtonAuto(null);
        autoColorValue = Color.rgb(0, 0, 255);
    }

    public void onClickButtonRed(View v) {
        button_red.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_dark)));
        button_green.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_normal)));
        button_blue.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_normal)));
        button_auto.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_normal)));
        buttonValue = "red";
    }
    public void onClickButtonGreen(View v) {
        button_red.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_normal)));
        button_green.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_dark)));
        button_blue.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_normal)));
        button_auto.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_normal)));
        buttonValue = "green";
    }
    public void onClickButtonBlue(View v) {
        button_red.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_normal)));
        button_green.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_normal)));
        button_blue.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_dark)));
        button_auto.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_normal)));
        buttonValue = "blue";
    }
    public void onClickButtonAuto(View v) {
        button_red.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_normal)));
        button_green.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_normal)));
        button_blue.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_normal)));
        button_auto.setBackgroundTintList(ColorStateList.valueOf(getResources().getColor(R.color.color_button_dark)));
        buttonValue = "auto";
    }

    private void prependOutput(String s) {
        final int LINE_LIMIT = 35;
        String t = s + "\n" + textOutput.getText();
        String[] ts = t.split("\n", LINE_LIMIT + 1);
        String x = ts[0];
        int limit = ts.length >= LINE_LIMIT ? LINE_LIMIT : ts.length;
        for (int i = 1; i < limit; i++) {
            x = x + "\n" + ts[i];
        }
        textOutput.setText(x);
    }

    @Override
    protected void onPause() {
        super.onPause();
        stopAdvertising();
        stopServer();
    }
    private void stopServer() {
        if (mGattServer != null) {
            mGattServer.close();
        }
    }
    private void stopAdvertising() {
        if (mBluetoothLeAdvertiser != null) {
            mBluetoothLeAdvertiser.stopAdvertising(mAdvertiseCallback);
        }
    }

    @Override
    protected void onResume() {
        super.onResume();
        if (mBluetoothAdapter == null || !mBluetoothAdapter.isEnabled()) {
            Intent enableBtIntent = new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
            startActivity(enableBtIntent);
            finish();
            return;
        }
        if (!getPackageManager().hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            finish();
            return;
        }
        if (!mBluetoothAdapter.isMultipleAdvertisementSupported()) {
            finish();
            return;
        }
        mBluetoothLeAdvertiser = mBluetoothAdapter.getBluetoothLeAdvertiser();
        GattServerCallback gattServerCallback = new GattServerCallback();
        mGattServer = mBluetoothManager.openGattServer(this, gattServerCallback);
        setupServer();
        startAdvertising();
    }
    private void setupServer() {
        BluetoothGattService service = new BluetoothGattService(UUID.fromString( getString( R.string.ble_uuid ) ),
                BluetoothGattService.SERVICE_TYPE_PRIMARY);
        BluetoothGattCharacteristic txChar = new BluetoothGattCharacteristic(UUID.fromString( getString( R.string.tx_uuid )),
                BluetoothGattCharacteristic.PROPERTY_WRITE |
                BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE,
                BluetoothGattCharacteristic.PERMISSION_WRITE);
        BluetoothGattCharacteristic rxChar = new BluetoothGattCharacteristic(UUID.fromString(getString(R.string.rx_uuid)),
                BluetoothGattCharacteristic.PROPERTY_READ |
                BluetoothGattCharacteristic.PROPERTY_NOTIFY,
                BluetoothGattCharacteristic.PERMISSION_READ);
        service.addCharacteristic(txChar);
        service.addCharacteristic(rxChar);
        mGattServer.addService(service);
    }
    private void startAdvertising() {
        if (mBluetoothLeAdvertiser == null) {
            return;
        }
        AdvertiseSettings settings = new AdvertiseSettings.Builder()
                .setAdvertiseMode( AdvertiseSettings.ADVERTISE_MODE_LOW_LATENCY )
                .setTxPowerLevel( AdvertiseSettings.ADVERTISE_TX_POWER_HIGH )
                .setConnectable(true)
                .build();
        ParcelUuid parcelUuid = new ParcelUuid(UUID.fromString( getString( R.string.ble_uuid )));
        AdvertiseData data = new AdvertiseData.Builder()
                .setIncludeTxPowerLevel(true)
                .setIncludeDeviceName(true)
                .addServiceUuid(parcelUuid)
                .build();
        mBluetoothLeAdvertiser.startAdvertising(settings, data, mAdvertiseCallback);
    }
    private AdvertiseCallback mAdvertiseCallback = new AdvertiseCallback() {
        @Override
        public void onStartSuccess(AdvertiseSettings settingsInEffect) {
            Log.d(LOG_TAG, "Peripheral advertising started.");
        }

        @Override
        public void onStartFailure(int errorCode) {
            Log.d(LOG_TAG, "Peripheral advertising failed: " + errorCode);
        }
    };
    private class GattServerCallback extends BluetoothGattServerCallback {
        @Override
        public void onConnectionStateChange(final BluetoothDevice device, int status, int newState) {
            super.onConnectionStateChange(device, status, newState);
            if (newState == BluetoothProfile.STATE_CONNECTED) {
                Log.d(LOG_TAG, "Device '" + device.getName() + "' (" + device.getAddress() + ") connected.");
            } else if (newState == BluetoothProfile.STATE_DISCONNECTED) {
                Log.d(LOG_TAG, "Device '" + device.getName() + "' (" + device.getAddress() + ") disconnected.");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        if (device.equals(dev_1)) {
                            dev_1 = null;
                            dev_1_conn_text.setText(getString(R.string.dev_1_disconnected));
                            dev_1_conn_text.setTextColor(0xFF000000);
                        }
                        if (device.equals(dev_2)) {
                            dev_2 = null;
                            dev_2_conn_text.setText(getString(R.string.dev_2_disconnected));
                            dev_2_conn_text.setTextColor(0xFF000000);
                        }
                        if (device.equals(dev_3)) {
                            dev_3 = null;
                            dev_3_conn_text.setText(getString(R.string.dev_3_disconnected));
                            dev_3_conn_text.setTextColor(0xFF000000);
                        }
                    }
                });
            }
        }
        @Override
        public void onCharacteristicReadRequest(BluetoothDevice device, int requestId,
                                                int offset, BluetoothGattCharacteristic characteristic) {
            Log.d(LOG_TAG, "Read request by '" + device.getName() + "' (" + device.getAddress() + "), value: [" + buttonValue + "].");
            byte[] data = {0x00, 0x00, 0x00};
            switch (buttonValue) {
                case "red": {
                    data[0] = (byte)0xFF;
                    break;
                }
                case "green": {
                    data[1] = (byte)0xFF;
                    break;
                }
                case "blue": {
                    data[2] = (byte)0xFF;
                    break;
                }
                default: {
                    data[0] = (byte)Color.red(autoColorValue);
                    data[1] = (byte)Color.green(autoColorValue);
                    data[2] = (byte)Color.blue(autoColorValue);
                    break;
                }
            }
            mGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, 0, data);
        }
        @Override
        public void onCharacteristicWriteRequest(final BluetoothDevice device, int requestId,
                                                 BluetoothGattCharacteristic characteristic,
                                                 boolean preparedWrite, boolean responseNeeded,
                                                 int offset, final byte[] value) {
            final String s_val = new String(value);
            Log.d(LOG_TAG, "Write request by '" + device.getName() + "' (" + device.getAddress() + "), [" + s_val + "].");
            runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    if (s_val.length() >= 4 && s_val.substring(0, 4).equalsIgnoreCase("cmd:")) {
                        if (s_val.equalsIgnoreCase("cmd:reg:temp")) {
                            dev_1 = device;
                            dev_1_conn_text.setText(getString(R.string.dev_1_connected));
                            dev_1_conn_text.setTextColor(0xFF00FF00);
                        } else if (s_val.equalsIgnoreCase("cmd:reg:accel")) {
                            dev_2 = device;
                            dev_2_conn_text.setText(getString(R.string.dev_2_connected));
                            dev_2_conn_text.setTextColor(0xFF00FF00);
                        } else if (s_val.equalsIgnoreCase("cmd:reg:led")) {
                            dev_3 = device;
                            dev_3_conn_text.setText(getString(R.string.dev_3_connected));
                            dev_3_conn_text.setTextColor(0xFF00FF00);
                        }
                    } else if (s_val.length() == 16 && s_val.substring(0, 10).equalsIgnoreCase("data:temp:")) {
                        dev_1 = device;
                        dev_1_conn_text.setText(getString(R.string.dev_1_connected));
                        dev_1_conn_text.setTextColor(0xFF00FF00);
                        int[] ints = {0,0,0,0,0,0};
                        for (int i = 0; i < 6; i++) {
                            ints[i] = (int)value[10 + i];
                            if (ints[i] < 0) {
                                ints[i] = ints[i] + 256;
                            }
                        }
                        int temp_ints = (ints[0] * 256) + ints[1];
                        int hum_ints = (ints[2] * 256) + ints[3];
                        int volt = (ints[5] * 256) + ints[4];
                        float temp = (((float)temp_ints) * 165 / 65536) - 40;
                        float hum = (((float)hum_ints) * 100 / 65536);
                        Calendar calendar = Calendar.getInstance();
                        SimpleDateFormat format = new SimpleDateFormat("h:mm:ss a", Locale.US);
                        prependOutput(String.format(Locale.US, "%s: Battery Volt: %.3f V, Temp: %.1f C, Humidity: %.1f %%", format.format(calendar.getTime()), volt / 1000.0, temp, hum));
                        if (temp < 20.0) {
                            autoColorValue = Color.rgb(0, 0, 255);
                        } else if (temp > 40.0) {
                            autoColorValue = Color.rgb(255, 0, 0);
                        } else {
                            int val = Math.round((temp - 20) / 20 * 255);
                            autoColorValue = Color.rgb(val, 0, 255 - val);
                        }
                    } else if (s_val.length() == 13 && s_val.substring(0, 11).equalsIgnoreCase("data:accel:")) {
                        dev_2 = device;
                        dev_2_conn_text.setText(getString(R.string.dev_2_connected));
                        dev_2_conn_text.setTextColor(0xFF00FF00);
                        int volt_low = (int)value[11];
                        int volt_high = (int)value[12];
                        int volt = (volt_high * 256) + volt_low;
                        Calendar calendar = Calendar.getInstance();
                        SimpleDateFormat format = new SimpleDateFormat("h:mm:ss a", Locale.US);
                        prependOutput(String.format(Locale.US, "%s: Batt. Volt: %.1f V, Accelerometer Node has awoken.", format.format(calendar.getTime()), volt / 1000.0));
                    } else if (!(s_val.length() >= 6 && s_val.substring(0, 6).equalsIgnoreCase("error:"))) {
                        Calendar calendar = Calendar.getInstance();
                        SimpleDateFormat format = new SimpleDateFormat("h:mm:ss a", Locale.US);
                        prependOutput(format.format(calendar.getTime()) + ": " + s_val);
                    }
                }
            });
            if (responseNeeded) {
                mGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, 0, value);
            }
        }
    }
}
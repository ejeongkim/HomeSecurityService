package com.example.ejeong.smarthome;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.content.DialogInterface;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.UUID;


public class MainActivity extends Activity implements View.OnClickListener {
    public static final String TAG = "MainActivity";

    private static final int REQUEST_ENABLE_BT = 10;
    private BluetoothAdapter mBluetoothAdapter;
    private int mPairedDeviceCount = 0;
    private Set<BluetoothDevice> mDevices;
    private BluetoothDevice mRemoteDevice;
    private BluetoothSocket mSocket = null;
    private OutputStream mOutputStream = null;
    private InputStream mInputStream = null;

    private Thread mWorkerThread = null;
    private String mStrDelimiter = "\n";
    private char mCharDelimiter = '\n';
    private byte[] readBuffer;
    private int readBufferPosition;
    private char[] mLedData = {'0', '0', '0', '0'};

    private static final int[] ID_BTN_LED = {R.id.btnLed1, R.id.btnLed2, R.id.btnLed3, R.id.btnLed4};
    private Button[] mArrBtnLed = new Button[ID_BTN_LED.length];
    private Button mBtnReload;
    private TextView mTvTemp, mTvHumi, mTvGas, mTvUltra, mTvWater, mTvEarth;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        for (int i = 0; i < ID_BTN_LED.length; i++) {
            mArrBtnLed[i] = (Button) findViewById(ID_BTN_LED[i]);
            mArrBtnLed[i].setOnClickListener(this);
            mArrBtnLed[i].setTag(false);
        }

        mBtnReload = (Button) findViewById(R.id.btnReload);
        mBtnReload.setOnClickListener(this);

        mTvTemp = (TextView) findViewById(R.id.tvTemp);
        mTvHumi = (TextView) findViewById(R.id.tvHumi);
        mTvGas = (TextView) findViewById(R.id.tvGas);
        mTvUltra = (TextView) findViewById(R.id.tvUltra);
        mTvWater = (TextView) findViewById(R.id.tvWater);
        mTvEarth = (TextView) findViewById(R.id.tvEarth);

        checkBluetooth();
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        switch (requestCode) {
            case REQUEST_ENABLE_BT:
                if (resultCode == RESULT_OK) {
                    // 블루투스가 활성 상태로 변경됨
                    selectDevice();
                } else if (resultCode == RESULT_CANCELED) {
                    // 블루투스가 비활성 상태임
                    finish();    // 어플리케이션 종료
                }
                break;
        }
        super.onActivityResult(requestCode, resultCode, data);
    }

    void checkBluetooth() {
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        if (mBluetoothAdapter == null) {
            // 장치가 블루투스를 지원하지 않는 경우
            finish();    // 어플리케이션 종료
        } else {
            // 장치가 블루투스를 지원하는 경우
            if (!mBluetoothAdapter.isEnabled()) {
                // 블루투스를 지원하지만 비활성 상태인 경우
                // 블루투스를 활성 상태로 바꾸기 위해 사용자 동의 요청
                Intent enableBtIntent =
                        new Intent(BluetoothAdapter.ACTION_REQUEST_ENABLE);
                startActivityForResult(enableBtIntent, REQUEST_ENABLE_BT);
            } else {
                // 블루투스를 지원하며 활성 상태인 경우
                // 페어링 된 기기 목록을 보여주고 연결할 장치를 선택
                selectDevice();
            }
        }
    }

    void selectDevice() {
        mDevices = mBluetoothAdapter.getBondedDevices();
        mPairedDeviceCount = mDevices.size();

        if (mPairedDeviceCount == 0) {
            // 페어링 된 장치가 없는 경우
            finish();        // 어플리케이션 종료
        }

        AlertDialog.Builder builder = new AlertDialog.Builder(this);
        builder.setTitle("블루투스 장치 선택");

        // 페어링 된 블루투스 장치의 이름 목록 작성
        List<String> listItems = new ArrayList<String>();
        for (BluetoothDevice device : mDevices) {
            listItems.add(device.getName());
        }
        listItems.add("취소");        // 취소 항목 추가

        final CharSequence[] items =
                listItems.toArray(new CharSequence[listItems.size()]);

        builder.setItems(items, new DialogInterface.OnClickListener() {
            public void onClick(DialogInterface dialog, int item) {
                if (item == mPairedDeviceCount) {
                    // 연결할 장치를 선택하지 않고 ‘취소’를 누른 경우
                    finish();
                } else {
                    // 연결할 장치를 선택한 경우
                    // 선택한 장치와 연결을 시도함
                    connectToSelectedDevice(items[item].toString());
                }
            }
        });

        builder.setCancelable(false);    // 뒤로 가기 버튼 사용 금지
        AlertDialog alert = builder.create();
        alert.show();
    }

    void beginListenForData() {
        final Handler handler = new Handler();

        readBuffer = new byte[1024];    // 수신 버퍼
        readBufferPosition = 0;        // 버퍼 내 수신 문자 저장 위치

        // 문자열 수신 쓰레드
        mWorkerThread = new Thread(new Runnable() {
            public void run() {
                while (!Thread.currentThread().isInterrupted()) {
                    try {
                        int bytesAvailable = mInputStream.available();    // 수신 데이터 확인
                        if (bytesAvailable > 0) {        // 데이터가 수신된 경우
                            byte[] packetBytes = new byte[bytesAvailable];
                            mInputStream.read(packetBytes);
                            for (int i = 0; i < bytesAvailable; i++) {
                                byte b = packetBytes[i];
                                if (b == mCharDelimiter) {
                                    byte[] encodedBytes = new byte[readBufferPosition];
                                    System.arraycopy(readBuffer, 0, encodedBytes, 0, encodedBytes.length);
                                    final String data = new String(encodedBytes, "US-ASCII");
                                    readBufferPosition = 0;

                                    Log.d(TAG, "read good, byte b : " + b + ", data : " + data);

                                    handler.post(new Runnable() {
                                        public void run() {
                                            // 수신된 문자열 데이터에 대한 처리 작업
                                            analyzeData(data);
                                        }
                                    });
                                } else {
                                    //debug code
                                    byte[] encodedBytes = new byte[readBufferPosition];
                                    System.arraycopy(readBuffer, 0, encodedBytes, 0, encodedBytes.length);
                                    final String data = new String(encodedBytes, "US-ASCII");
                                    Log.d(TAG, "read bad, byte b : " + b + ", data : " + data);
                                    //debug code

                                    readBuffer[readBufferPosition++] = b;
                                }
                            }
                        }
                    } catch (IOException ex) {
                        // 데이터 수신 중 오류 발생
                        finish();
                    }
                }
            }
        });

        mWorkerThread.start();
    }

    void sendData(String msg) {
        msg += mStrDelimiter;    // 문자열 종료 표시
        try {
            mOutputStream.write(msg.getBytes());        // 문자열 전송
        } catch (Exception e) {
            // 문자열 전송 도중 오류가 발생한 경우
            finish();        // 어플리케이션 종료
        }
    }

    void analyzeData(String data) {
        if (data != null) {
            char[] chData = data.toCharArray();
            if (chData.length == 12) {
                //led 0~3
                for (int i = 0; i < 4; i++) {
                    mLedData[i] = chData[i];
                    mArrBtnLed[i].setTag(chData[i] == '1' ? true : false);
                    mArrBtnLed[i].setText(chData[i] == '1' ? "On" : "Off");
                }
                //gas 4
                if (mTvGas.getText().toString().equals("No problem") && chData[4] == '1') {
                    pushNotificationBar("가스가 새고 있어요!");
                }
                mTvGas.setText(chData[4] == '1' ? "Warning" : "No problem");
                //ultra 5
                if (mTvUltra.getText().toString().equals("No problem") && chData[5] == '1') {
                    pushNotificationBar("도둑이 들었어요!");
                }
                mTvUltra.setText(chData[5] == '1' ? "Warning" : "No problem");
                //water 6
                if (mTvWater.getText().toString().equals("No problem") && chData[6] == '1') {
                    pushNotificationBar("집이 침수됬어요!");
                }
                mTvWater.setText(chData[6] == '1' ? "Warning" : "No problem");
                //earth 7
                if (mTvEarth.getText().toString().equals("No problem") && chData[7] == '1') {
                    pushNotificationBar("지진이 났어요!");
                }
                mTvEarth.setText(chData[7] == '1' ? "Warning" : "No problem");
                //temperature 8~9
                String temperature = String.valueOf(chData[8]) + String.valueOf(chData[9]);
                mTvTemp.setText(temperature + " C");
                //humidity 10~11
                String humidity = String.valueOf(chData[10]) + String.valueOf(chData[11]);
                mTvHumi.setText(humidity + " %");
            } else {
                Log.d(TAG, "received data's length is not 12");
            }
        } else {
            Log.d(TAG, "received data is null");
        }
    }

    void pushNotificationBar(String text) {
        NotificationManager notificationManager = (NotificationManager) MainActivity.this.getSystemService(MainActivity.this.NOTIFICATION_SERVICE);
        Intent intent1 = new Intent(MainActivity.this.getApplicationContext(), MainActivity.class); //인텐트 생성.

        Notification.Builder builder = new Notification.Builder(getApplicationContext());
        intent1.addFlags(Intent.FLAG_ACTIVITY_SINGLE_TOP | Intent.FLAG_ACTIVITY_CLEAR_TOP);

        PendingIntent pendingNotificationIntent = PendingIntent.getActivity(MainActivity.this, 0, intent1, PendingIntent.FLAG_UPDATE_CURRENT);

        builder.setSmallIcon(R.drawable.icon_warning).setTicker("집에 난리가 났어요!").setWhen(System.currentTimeMillis())
                .setNumber(1).setContentTitle("경고! 경고!").setContentText(text)
                .setDefaults(Notification.DEFAULT_SOUND | Notification.DEFAULT_VIBRATE).setContentIntent(pendingNotificationIntent).setAutoCancel(true).setOngoing(true);
        notificationManager.notify(1, builder.build()); // Notification send
    }

    BluetoothDevice getDeviceFromBondedList(String name) {
        BluetoothDevice selectedDevice = null;

        for (BluetoothDevice device : mDevices) {
            if (name.equals(device.getName())) {
                selectedDevice = device;
                break;
            }
        }

        return selectedDevice;
    }

    @Override
    protected void onDestroy() {
        try {
            mWorkerThread.interrupt();    // 데이터 수신 쓰레드 종료
            mInputStream.close();
            mOutputStream.close();
            mSocket.close();
        } catch (Exception e) {
        }

        super.onDestroy();
    }

    void connectToSelectedDevice(String selectedDeviceName) {
        mRemoteDevice = getDeviceFromBondedList(selectedDeviceName);
        UUID uuid = UUID.fromString("00001101-0000-1000-8000-00805f9b34fb");

        try {
            // 소켓 생성
            mSocket = mRemoteDevice.createRfcommSocketToServiceRecord(uuid);
            // RFCOMM 채널을 통한 연결
            mSocket.connect();

            // 데이터 송수신을 위한 스트림 얻기
            mOutputStream = mSocket.getOutputStream();
            mInputStream = mSocket.getInputStream();

            // 데이터 수신 준비
            beginListenForData();
        } catch (Exception e) {
            // 블루투스 연결 중 오류 발생
            finish();        // 어플리케이션 종료
        }
    }

    @Override
    public void onClick(View v) {
        int id = v.getId();

        switch (id) {
            case R.id.btnLed1: {
                boolean prevTag = (boolean) mArrBtnLed[0].getTag();
                mLedData[0] = prevTag ? '0' : '1';
                mArrBtnLed[0].setText(prevTag ? "Off" : "On");
                mArrBtnLed[0].setTag(!prevTag);
                sendData(String.valueOf(mLedData));
                break;
            }
            case R.id.btnLed2: {
                boolean prevTag = (boolean) mArrBtnLed[1].getTag();
                mLedData[1] = prevTag ? '0' : '1';
                mArrBtnLed[1].setText(prevTag ? "Off" : "On");
                mArrBtnLed[1].setTag(!prevTag);
                sendData(String.valueOf(mLedData));
                break;
            }
            case R.id.btnLed3: {
                boolean prevTag = (boolean) mArrBtnLed[2].getTag();
                mLedData[2] = prevTag ? '0' : '1';
                mArrBtnLed[2].setText(prevTag ? "Off" : "On");
                mArrBtnLed[2].setTag(!prevTag);
                sendData(String.valueOf(mLedData));
                break;
            }
            case R.id.btnLed4: {
                boolean prevTag = (boolean) mArrBtnLed[3].getTag();
                mLedData[3] = prevTag ? '0' : '1';
                mArrBtnLed[3].setText(prevTag ? "Off" : "On");
                mArrBtnLed[3].setTag(!prevTag);
                sendData(String.valueOf(mLedData));
                break;
            }
            case R.id.btnReload: {
                sendData(String.valueOf(mLedData));
                break;
            }
            default: {

                break;
            }
        }
    }
}
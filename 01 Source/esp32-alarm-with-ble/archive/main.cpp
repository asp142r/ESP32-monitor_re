//参考　https://www.e-tecinc.co.jp/blog/2021/04/09/esp32blestep1/
#include "main.h"
#include <Arduino.h>

#include <BLEDevice.h>

#define DEF_BLE_ADDR "05:B3:EC:xx:xx:xx"

//BLEScan *g_pBLEScan;

int ser2mcu;
//int nCnt;
//BLEScanResults res;
//char buf[1024];

//void scanDevice(void);
//void showBLEaddr();

bleScan bScan = bleScan();

bleScan::bleScan(void)
{
    BLEDevice::init("");               // BLEデバイス初期化
    g_pBLEScan = BLEDevice::getScan(); // Scanオブジェクト取得
    g_pBLEScan->setActiveScan(false);  // パッシブスキャンに設定
}

void bleScan::scanDevice(void)
{
    sprintf(buf, "");
    BLEScanResults res = g_pBLEScan->start(1); // スキャン時間10秒
    nCnt = res.getCount();
    if (nCnt > 20)
    { // 上限20件まで検出
        nCnt = 20;
    }
}

void bleScan::showAddr()
{
    // 検出したBLE件数分ループ
    for (int i = 0; i < nCnt; i++)
    {
        BLEAdvertisedDevice bledev = res.getDevice(i);
        BLEAddress ble_addr = bledev.getAddress(); // BLEのMACアドレス取得
        std::string ble_name = bledev.getName();   // BLEの名称取得
        // MACアドレスが一致している場合
        if (DEF_BLE_ADDR == ble_addr.toString())
        {
            // ログ出力
            sprintf(buf, "name=[%s]; addr=[%s]", ble_name.c_str(), ble_addr.toString().c_str());
            Serial.println(buf);
            break;
        }
    }
}

void setup()
{
    Serial.begin(115200); // ログ出力準備
    Serial.println("=============================");
    //BLEDevice::init("");               // BLEデバイス初期化
    //g_pBLEScan = BLEDevice::getScan(); // Scanオブジェクト取得
    //g_pBLEScan->setActiveScan(false);  // パッシブスキャンに設定
}

void loop()
{
    Serial.println("==== choose mode ====");
    Serial.println("[1]   scanDevice");
    Serial.println("[2]   showBLEaddr");
    while (Serial.available() == 0){}
    ser2mcu = Serial.read();
    Serial.print("Received: ");
    Serial.println(ser2mcu);
    switch (ser2mcu)
    {
        case '1':
            Serial.println("Do task 1");
            bScan.scanDevice();
            //scanDevice();
            break;

        case '2':
            Serial.println("Do task 2");
            bScan.showAddr();
            //showBLEaddr();
            break;

        default:
            Serial.println("Command is not found!");
            break;
    }
    Serial.println("=====================");
}

/*
void scanDevice()
{
    sprintf(buf, "");
    BLEScanResults res = g_pBLEScan->start(1); // スキャン時間10秒
    nCnt = res.getCount();
    if (nCnt > 20)
    { // 上限20件まで検出
        nCnt = 20;
    }
}

void showBLEaddr()
{
    // 検出したBLE件数分ループ
    for (int i = 0; i < nCnt; i++)
    {
        BLEAdvertisedDevice bledev = res.getDevice(i);
        BLEAddress ble_addr = bledev.getAddress(); // BLEのMACアドレス取得
        std::string ble_name = bledev.getName();   // BLEの名称取得
        // MACアドレスが一致している場合
        if (DEF_BLE_ADDR == ble_addr.toString())
        {
            // ログ出力
            sprintf(buf, "name=[%s]; addr=[%s]", ble_name.c_str(), ble_addr.toString().c_str());
            Serial.println(buf);
            break;
        }
    }
}
*/
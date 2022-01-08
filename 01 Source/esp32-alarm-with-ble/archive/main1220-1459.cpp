/*
    esp32-alarm-with-ble
    2021.12 M.Hayashi
    自作基板

*/
#include <Arduino.h>
#include <WiFi.h>
#include <TFT_eSPI.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
//#include "DFRobotDFPlayerMini.h"

//#define demo

#define TOUCH_INT 15
#define MP3_STATUS 11

//===== 変数群 ======================================================================
// const char *ssid = " -- Your SSID -- "; // SSID
// const char *pass = " -- Your WiFi PASS -- ";  // PASS
const char *ssid = " --Your SSID -- ";      // SSID
const char *pass = " -- Your WiFi PASS -- "; // PASS

int syncSec = 0;     //液晶更新した秒
int lastNTPdate = 0; //最後にNTP同期した日付の下桁

struct tm timeInfo; //時刻を格納するオブジェクト
char s[20];         //文字格納用

bool playFlag = 0; //再生しているかのフラグ

// DFRobotDFPlayerMini mp3;

BLEScan *g_pBLEScan;

typedef struct bluetooth_scanstore // Bluetoothで読み取ったデータの構造体
{
    BLEAddress ble_addr; // BLEアドレス
    // std::string ble_name;
    // char *ble_name; // BLE機器の名称
    int ble_rssi;
    // float ble_rssi; // BLE機器の電波強度(RSSI)
} BLE_Scandetail;

BLE_Scandetail *s_start;
BLE_Scandetail *sp;

unsigned int nCnt; // BLEで読み取ったデバイスの数

BLEScanResults res;
BLEAdvertisedDevice bledev;

TFT_eSPI tft = TFT_eSPI();
unsigned long prevTime_lcd = 0;
bool LCD_colorChanged = false;

volatile bool device1_Exist = false;
bool mutexOKorNOT = false;

portMUX_TYPE mutex = portMUX_INITIALIZER_UNLOCKED; //ミューテックス処理用

#define DEF_BLETAG1_ADDR "30:ae:a4:99:98:7a" // BLEタグのMACアドレス
#define DEF_BLEband_ADDR "e8:ee:d2:93:3b:aa" // 活動量計のMACアドレス

//===== 圧電ブザー関連 =============================================================
#define BUZZER_PIN 32 // ブザーを鳴らすためのピン

#define LEDC_CHANNEL 0 // チャンネル
#define LEDC_TIMER_BIT 8
#define LEDC_BASE_FREQ 5000

#define stop_sound 0    //音停止
#define do_sound 523    //ド
#define dos_sound 554   //ド♯
#define re_sound 587    //レ
#define res_sound 622   //レ♯
#define mi_sound 659    //ミ
#define fa_sound 698    //ファ
#define fas_sound 740   //ファ♯
#define so_sound 784    //ソ
#define sos_sound 831   //ソ♯
#define ra_sound 880    //ラ
#define ras_sound 932   //ラ♯
#define si_sound 988    //シ
#define doh_sound 1046  //ド高
#define dosh_sound 1108 //ド♯高
#define reh_sound 1174  //レ高

#define yobu 100  // 4分音符
#define nibu 200  // 2分音符
#define zenbu 300 //全音符

bool soundFlag = false;       // 音を鳴らすかどうかのフラグ
unsigned long playedTime = 0; //前回の処理時間
int sequence = 0;             //再生済み管理用
#define gakufu_kazu 27
int Hotaru_no_Hikari_gakufu[] =
    {
        do_sound,
        do_sound,
        fa_sound,
        fa_sound,
        fa_sound,
        fa_sound,
        fa_sound,
        fa_sound,
        ra_sound,
        ra_sound,
        so_sound,
        so_sound,
        so_sound,
        fa_sound,
        so_sound,
        so_sound,
        ra_sound,
        ra_sound,
        fa_sound,
        fa_sound,
        fa_sound,
        fa_sound,
        ra_sound,
        ra_sound,
        doh_sound,
        doh_sound,
        reh_sound,
        reh_sound,
        reh_sound,
        reh_sound,
        reh_sound,
        reh_sound,
        reh_sound,
        reh_sound,
        doh_sound,
        doh_sound,
        doh_sound,
        ra_sound,
        ra_sound,
        ra_sound,
        fa_sound,
        fa_sound,
        so_sound,
        so_sound,
        so_sound,
        fa_sound,
        so_sound,
        so_sound,
        ra_sound,
        ra_sound,
        fa_sound,
        fa_sound,
        fa_sound,
        re_sound,
        re_sound,
        re_sound,
        do_sound,
        do_sound,
        fa_sound,
        fa_sound,
        fa_sound,
        fa_sound,
};

void Hotaru_no_hikari();

//===== プロトタイプ宣言 =============================================================
void LCD(void *pvParameters);
void TOUCH_INTERRUPT();
void IO0_INTERRUPT();

//===== プログラム実体 ===============================================================
void setup()
{
    Serial2.begin(9600);
    Serial.begin(115200);
    delay(100);

    // 液晶設定
    tft.init();         //液晶初期化
    tft.setRotation(1); //画面回転
    // tft.setRotation(3); //画面回転
    tft.fillScreen(TFT_BLACK); //液晶を黒くする

    // MP3設定
    /*
    Serial.print("MP3 initializing...");
    if (!mp3.begin(Serial2))
    {
        Serial.println(" Failed! Rebooting...");
        delay(1000);
        ESP.restart();
    }
    mp3.setTimeOut(500);
    mp3.volume(10);
    Serial.println(" done");
    delay(100);
*/

    // 圧電ブザーセットアップ
    ledcSetup(LEDC_CHANNEL, LEDC_BASE_FREQ, LEDC_TIMER_BIT);
    ledcAttachPin(BUZZER_PIN, LEDC_CHANNEL);

    // BLE設定
    Serial.print("BLE initializing...");
    tft.print("BLE initializing...");
    BLEDevice::init("");               // BLEデバイス初期化
    g_pBLEScan = BLEDevice::getScan(); // Scanオブジェクト取得
    g_pBLEScan->setActiveScan(false);  // パッシブスキャンに設定
    // g_pBLEScan->setActiveScan(true); // アクティブスキャンに設定
    // g_pBLEScan->setInterval(100);
    // g_pBLEScan->setWindow(99); // less or equal setInterval value
    Serial.println(" done");
    tft.println(" done");
    delay(100);
#ifdef demo
    Serial.println("Demo mode ENABLED");
    tft.println("Demo mode ENABLED");
    timeInfo.tm_year = 121;
    timeInfo.tm_mon = 11;
    timeInfo.tm_mday = 20;
    timeInfo.tm_hour = 19;
    timeInfo.tm_min = 59;
    timeInfo.tm_sec = 55;
#endif
#ifndef demo
    // WiFi設定
    Serial.print("Setup WiFi...");
    tft.print("Setup WiFi...");
    if (WiFi.begin(ssid, pass) != WL_DISCONNECTED) // WiFi接続できなければ再起動
    {
        Serial.println(" Failed! Rebooting...");
        tft.println(" Failed! Rebooting...");
        delay(1000);
        ESP.restart();
    }
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(1000);
    }
    Serial.println(" done");
    tft.println(" done");
    delay(100);

    // NTP設定
    Serial.println("Receiving Time...");
    tft.println("Receiving Time...");
    configTime(9 * 3600L, 0, "ntp.nict.jp", "10.64.187.15", "ntp.jst.mfeed.ad.jp"); // NTPの設定
#endif
    Serial.println("Starts in 5 second...");
    tft.println("Starts in 5 second...");

    enableCore1WDT();
    delay(5000);

    tft.setCursor(0, 0);
    tft.fillScreen(TFT_BLACK);
    attachInterrupt(0, IO0_INTERRUPT, FALLING);
    xTaskCreatePinnedToCore(LCD, "LCD", 4096, NULL, 1, NULL, 0); //セカンダリタスク(液晶設定)の起動
    // mp3.play(1);
}

//プライマリタスク(BLE検出)
void loop()
{
    delay(1);
    char buf[1024];
    sprintf(buf, "");
    // BLEScanResults res = g_pBLEScan->start(10); // スキャン時間10秒
    Serial.print("10sec Scan start...");
    // res = g_pBLEScan->start(10);
    res = g_pBLEScan->start(20);
    nCnt = res.getCount();
    if (nCnt > 100)
    { // 上限20件まで検出
        nCnt = 100;
    }
    Serial.println("Finish");
    sprintf(buf, "Get [%d] devices", nCnt);
    Serial.println(buf);

    // s_start = (BLE_Scandetail*)malloc(sizeof(BLE_Scandetail) * nCnt);//メモリの動的確保
    s_start = (BLE_Scandetail *)pvPortMalloc(sizeof(BLE_Scandetail) * nCnt); //メモリの動的確保
    if (s_start == NULL)                                                     //メモリ確保に失敗するとNULLが帰ってくるので、そうなったら強制終了する。
    {
        Serial.println("Failed to allocate memory, system stops...");
        while (1)
        {
        }
    }

    // 検出したBLE件数分ループ
    // for (int j = 0; j < nCnt; j++) {
    Serial.print("Data Correction begin...");
    int j;
    for (j = 0, sp = s_start; j < nCnt; j++, sp++)
    {
        // BLEAdvertisedDevice bledev = res.getDevice(i);
        bledev = res.getDevice(j);
        // BLEAddress ble_addr = bledev.getAddress();  // BLEのMACアドレス取得
        // std::string ble_name = bledev.getName();    // BLEの名称取得
        // int ble_rssi = bledev.getRSSI();  //BLEのRSSI取得

        sp->ble_addr = bledev.getAddress(); // BLEのMACアドレス取得
        // sp->ble_name = bledev.getName();    // BLEの名称取得
        sp->ble_rssi = float(bledev.getRSSI()); // BLEのRSSI取得

        // sprintf(buf, "No.%d name=[%s]; addr=[%s]; rssi=[%d]", i, ble_name.c_str(), ble_addr.toString().c_str(), ble_rssi);
        // Serial.println(buf);
        break;
    }
    Serial.println("End");
    //収集データの出力
    for (int i = 0; i < nCnt; i++)
    {
        // sprintf(buf, "No.%d name=[%s]; addr=[%s]; rssi=[%d]", i, sp[i].ble_name.c_str(), sp[i].ble_addr.toString().c_str(), sp[i].ble_rssi);
        // sprintf(buf, "No.%d name=[%s]; addr=[%s]; rssi=[%d]", i, sp[i].ble_name, sp[i].ble_addr.toString().c_str(), sp[i].ble_rssi);
        /*
        if (DEF_BLETAG1_ADDR == sp[i].ble_addr.toString()) // BLEタグ見つかった
        {
            Serial.println("TAG1 Found");
            if (!mutexOKorNOT) //共有変数が未編集であることを確認(falseなら編集)
            {
                portENTER_CRITICAL(&mutex);
                device1_Exist = true;
                portEXIT_CRITICAL(&mutex);
                mutexOKorNOT = true;
                Serial.println("device1_Exist = true");
            }
        }
        else // BLEタグがいない
        {
            Serial.println("TAG1 Not Found");
            if (mutexOKorNOT) //共有変数が未編集(trueなら編集)
            {
                portENTER_CRITICAL(&mutex);
                device1_Exist = false;
                portEXIT_CRITICAL(&mutex);
                mutexOKorNOT = false;
                Serial.println("device1_Exist = false");
            }
        }
        */
        sprintf(buf, "No.%d addr=[%s]; rssi=[%d]", i, sp[i].ble_addr.toString().c_str(), sp[i].ble_rssi);
        Serial.println(buf);
    }
    //データ処理
    for (int k = 0; k < nCnt; k++)
    {
        if (DEF_BLETAG1_ADDR == sp[k].ble_addr.toString()) // BLEタグ見つかった
        {
            Serial.println("TAG1 Found");
            if (!mutexOKorNOT) //共有変数が未編集であることを確認(falseなら編集)
            {
                portENTER_CRITICAL(&mutex);
                device1_Exist = true;
                portEXIT_CRITICAL(&mutex);
                mutexOKorNOT = true;
                Serial.println("device1_Exist = true");
                break;
            }
            break;
        }
        else // BLEタグがいない
        {
            // Serial.println("TAG1 Not Found");
            if (mutexOKorNOT) //共有変数が未編集(trueなら編集)
            {
                portENTER_CRITICAL(&mutex);
                device1_Exist = false;
                portEXIT_CRITICAL(&mutex);
                mutexOKorNOT = false;
                Serial.println("device1_Exist = false");
            }
        }
    }
    Serial.println("Scan finished");
    // free(sp);              //使用した動的確保領域の開放
    vPortFree(sp); //使用した動的確保領域の開放
    delay(100);    // 1秒wait
}

//セカンダリタスク(液晶描画, DFPlayer制御)
void LCD(void *pvParameters)
{
    while (1)
    {
        delay(1);
#ifndef demo
        getLocalTime(&timeInfo); // tmオブジェクトのtimeInfoに現在時刻を入れ込む
#endif
        // sprintf(s, " %04d/%02d/%02d %02d:%02d:%02d",
        //         timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
        //         timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec); //人間が読める形式に変換
        // delay(1000);
        // Serial.println(s); //時間をシリアルモニタへ出力

        //===== アラーム設定 ======================================================
        if (timeInfo.tm_hour == 20 && timeInfo.tm_min == 0 && timeInfo.tm_sec == 0)
        {
            if (!playFlag)
            {
                Serial.println("Play 1");
                // mp3.play(1);
                playFlag = true;
            }
        }
        if (timeInfo.tm_hour == 0)
        {
            if (timeInfo.tm_sec == 0)
            {
                if (timeInfo.tm_min == 0 || timeInfo.tm_min == 30 || timeInfo.tm_min == 50)
                {
                    portENTER_CRITICAL(&mutex);
                    volatile bool device1existCopy_for_lcd = device1_Exist;
                    portEXIT_CRITICAL(&mutex);
                    if (device1existCopy_for_lcd)
                    {
                        if (!playFlag)
                        {
                            Serial.println("Play 1");
                            // mp3.play(1);
                            playFlag = true;
                        }
                    }
                }
            }
        }

        if (timeInfo.tm_min == 0 || timeInfo.tm_min == 30 || timeInfo.tm_min == 50)
        {
            portENTER_CRITICAL(&mutex);
            volatile bool device1existCopy_for_lcd = device1_Exist;
            portEXIT_CRITICAL(&mutex);
            if (device1existCopy_for_lcd)
            {
                if (!playFlag)
                {
                    Serial.println("Play 1");
                    // mp3.play(1);
                    playFlag = true;
                }
            }
        }

        //===== 画面更新 ==========================================================
        if (LCD_colorChanged != playFlag) //一回だけ背景色を変える
        {
            LCD_colorChanged = playFlag;
            if (LCD_colorChanged)
                tft.fillScreen(TFT_RED);
            else
                tft.fillScreen(TFT_BLACK);
        }

        if (timeInfo.tm_sec != syncSec) // 1秒ごとに画面更新
        {
#ifdef demo
            timeInfo.tm_sec++;
#endif
            // tft.fillScreen(TFT_BLACK);
            tft.setCursor(20, 20);
            // tft.setFreeFont(*FreeSansBold18pt7b);
            sprintf(s, " %04d/%02d/%02d %02d:%02d:%02d",
                    timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
                    timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec); //人間が読める形式に変換
            syncSec = timeInfo.tm_sec;
            tft.println(s);
        }
        Hotaru_no_hikari(); //蛍の光流すか否か
    }
}

void IO0_INTERRUPT()
{
    Serial.println("Touched");
    playFlag = 0;
    // mp3.stop();

    // if(digitalRead(MP3_STATUS))
    //{
    // mp3.stop();
    //}
}

void Hotaru_no_hikari()
{
    unsigned long nowTime = millis();
    if (playFlag)
    {
        if (nowTime - playedTime >= zenbu) //一定間隔経過したら次の音に移る
        {
            playedTime = nowTime;
            ledcWriteTone(LEDC_CHANNEL, Hotaru_no_Hikari_gakufu[sequence]);
            sequence++;
            if (sequence > sizeof(Hotaru_no_Hikari_gakufu) / sizeof(int))
                sequence = 0;
        }
    }
    else
    {
        ledcWrite(LEDC_CHANNEL, 0);
        playedTime = 0;
        sequence = 0;
    }
}

#include <Arduino.h>
#include <WiFi.h>
#include "DFRobotDFPlayerMini.h"

const char *ssid = " -- Your SSID -- ";
const char *pass = " -- Your WiFi PASS -- ";

struct tm timeInfo; //時刻を格納するオブジェクト
char s[20];         //文字格納用

DFRobotDFPlayerMini mp3;

void setup()
{
    Serial2.begin(9600);
    Serial.begin(115200);
    delay(100);
    if (!mp3.begin(Serial2))
    {
        Serial.println("Failed to connect MP3!");
        delay(500);
        ESP.restart();
    }
    Serial.println("MP3 connected");
    mp3.setTimeOut(500);
    mp3.volume(10);
    delay(100);
    Serial.print("Setup WiFi...");
    if (WiFi.begin(ssid, pass) != WL_DISCONNECTED)
        ESP.restart();
    while (WiFi.status() != WL_CONNECTED)
        delay(1000);
    configTime(9 * 3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");//NTPの設定
    Serial.println("done");
    Serial.print("Ready");
}

void loop()
{
    getLocalTime(&timeInfo); // tmオブジェクトのtimeInfoに現在時刻を入れ込む
    sprintf(s, " %04d/%02d/%02d %02d:%02d:%02d",
            timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
            timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec); //人間が読める形式に変換
    delay(1000);
    Serial.println(s); //時間をシリアルモニタへ出力
    if (timeInfo.tm_min == 10 || timeInfo.tm_min == 30 || timeInfo.tm_min == 58)
    {
        Serial.println("Play 1");
        mp3.play(1);
    }
}
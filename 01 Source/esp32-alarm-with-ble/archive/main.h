#ifndef MAIN_H_INCLUDE
#define MAIN_H_INCLUDE
#include <Arduino.h>
#include <BLEDevice.h>
BLEScan *g_pBLEScan;
class bleScan
{
private:
    char* Addr;
    int nCnt;
    BLEScanResults res;
    char buf[1024];
public:
    bleScan();
    void scanDevice();   //スキャンする
    void showAddr();    //アドレスを表示
};
#endif
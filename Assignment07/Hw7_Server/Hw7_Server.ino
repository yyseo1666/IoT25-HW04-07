// BLE Server (advertiser)
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

void setup() {
  Serial.begin(115200);

  BLEDevice::init("IoT_TeamF");  // 이름은 자유롭게 변경 가능

  BLEServer *pServer = BLEDevice::createServer();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->setScanResponse(false);
  pAdvertising->setMinPreferred(0x06);  // optional
  pAdvertising->setMinPreferred(0x12);  // optional
  BLEDevice::startAdvertising();
  
  Serial.println("BLE advertising started...");
}

void loop() {
  delay(1000);
}

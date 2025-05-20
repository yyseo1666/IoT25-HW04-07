#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// BLE UUIDs (server와 동일하게 맞춰야 함)
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define TEMP_CHAR_UUID      "12345678-1234-1234-1234-1234567890ac"
#define HUM_CHAR_UUID       "12345678-1234-1234-1234-1234567890ad"

static BLEAddress *pServerAddress;
static BLERemoteCharacteristic* pTempCharacteristic;
static BLERemoteCharacteristic* pHumCharacteristic;
bool doConnect = false;
bool connected = false;

float temp = 0;
float hum = 0;

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    Serial.println("✅ BLE 서버 연결 완료");
  }

  void onDisconnect(BLEClient* pclient) {
    Serial.println("❌ BLE 서버 연결 끊김");
    connected = false;
  }
};

void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic,
                    uint8_t* pData, size_t length, bool isNotify) {
  if (pBLERemoteCharacteristic->getUUID().toString() == TEMP_CHAR_UUID) {
    memcpy(&temp, pData, length);
  } else if (pBLERemoteCharacteristic->getUUID().toString() == HUM_CHAR_UUID) {
    memcpy(&hum, pData, length);
  }
  
  // OLED 업데이트
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.print("Temp: "); display.print(temp); display.println(" C");
  display.print("Hum : "); display.print(hum); display.println(" %");
  display.display();
}

bool connectToServer() {
  BLEClient* pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  if (!pClient->connect(*pServerAddress)) {
    Serial.println("❌ 연결 실패");
    return false;
  }

  BLERemoteService* pRemoteService = pClient->getService(SERVICE_UUID);
  if (pRemoteService == nullptr) {
    Serial.println("❌ 서비스 찾을 수 없음");
    return false;
  }

  pTempCharacteristic = pRemoteService->getCharacteristic(TEMP_CHAR_UUID);
  pHumCharacteristic  = pRemoteService->getCharacteristic(HUM_CHAR_UUID);

  if (pTempCharacteristic && pTempCharacteristic->canNotify()) {
    pTempCharacteristic->registerForNotify(notifyCallback);
  }
  if (pHumCharacteristic && pHumCharacteristic->canNotify()) {
    pHumCharacteristic->registerForNotify(notifyCallback);
  }

  connected = true;
  return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == "ESP32_DHT11") {
      advertisedDevice.getScan()->stop();
      pServerAddress = new BLEAddress(advertisedDevice.getAddress());
      doConnect = true;
      Serial.println("🔍 서버 발견! 연결 시도 중...");
    }
  }
};

void setup() {
  Serial.begin(115200);

  // OLED 초기화 (SDA: 26, SCL: 25)
  Wire.begin(26, 25);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("❌ OLED 초기화 실패");
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("BLE Client Ready");
  display.display();

  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect && !connected) {
    connectToServer();
    doConnect = false;
  }
  delay(1000);
}

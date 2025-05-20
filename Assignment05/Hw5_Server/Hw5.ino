#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <DHT.h>

// =========================
// DHT 설정
// =========================
#define DHTPIN 26          // DHT11 센서가 연결된 핀
#define DHTTYPE DHT11      // 센서 종류
DHT dht(DHTPIN, DHTTYPE);

// =========================
// BLE 설정
// =========================
#define SERVICE_UUID        "12345678-1234-1234-1234-1234567890ab"
#define TEMP_CHAR_UUID      "12345678-1234-1234-1234-1234567890ac"
#define HUM_CHAR_UUID       "12345678-1234-1234-1234-1234567890ad"

BLECharacteristic tempChar(TEMP_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);
BLECharacteristic humChar(HUM_CHAR_UUID, BLECharacteristic::PROPERTY_NOTIFY);

bool deviceConnected = false;

// =========================
// 콜백 정의
// =========================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
    Serial.println("📱 BLE 클라이언트 연결됨");
  }
  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
    Serial.println("📴 BLE 클라이언트 연결 끊김");
  }
};

void setup() {
  Serial.begin(115200);
  Serial.println("🔧 DHT11 초기화 중...");
  dht.begin();

  // BLE 초기화
  BLEDevice::init("ESP32_DHT11");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  // 특성 추가
  pService->addCharacteristic(&tempChar);
  pService->addCharacteristic(&humChar);

  tempChar.addDescriptor(new BLE2902());
  humChar.addDescriptor(new BLE2902());

  pService->start();

  // 광고 시작
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("📡 BLE Advertising 시작됨...");
}

void loop() {
  if (deviceConnected) {
    float temp = dht.readTemperature();
    float hum = dht.readHumidity();

    if (isnan(temp) || isnan(hum)) {
      Serial.println("❌ 센서 오류: 값을 읽을 수 없음");
      return;
    }

    Serial.print("🌡 온도: ");
    Serial.print(temp);
    Serial.print(" °C  💧 습도: ");
    Serial.print(hum);
    Serial.println(" %");

    // BLE notify 전송
    tempChar.setValue(temp);
    tempChar.notify();

    humChar.setValue(hum);
    humChar.notify();
  }

  delay(3000); // 3초마다 전송
}

#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// ====== WiFi 정보 설정 ======
const char* ssid = "SK_WIFIGIGA6ACE_2.4G";
const char* password = "BCI77@9032";

// ====== BLE 스캔 설정 ======
BLEScan* pBLEScan;
int scanTime = 2;  // 스캔 시간 (초)
int rssi = -100;   // 기본 RSSI 초기값
float distance = 0;

// ====== 거리 추정 공식 변수 ======
const int txPower = -59; // 서버에서 1m 거리에서 측정한 RSSI
const float n = 2.0;      // 환경 계수 (실내는 보통 2.0)

// ====== 웹서버 설정 ======
WiFiServer server(80);

// ====== LED 제어 핀 ======
const int ledPin = 26;

// 콜백 클래스: 서버 장치 발견 시 처리
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == "IoT_TeamF") {
      rssi = advertisedDevice.getRSSI();
      distance = pow(10.0, ((txPower - rssi) / (10.0 * n)));
      Serial.print("RSSI: ");
      Serial.print(rssi);
      Serial.print(" dBm → 추정 거리: ");
      Serial.print(distance);
      Serial.println(" m");

      // LED 제어: 1m 이내면 깜빡
      if (distance <= 1.0) {
        digitalWrite(ledPin, !digitalRead(ledPin));
      } else {
        digitalWrite(ledPin, LOW);
      }
    }
  }
};

void setup() {
  Serial.begin(115200);

  // LED 핀 출력 설정
  pinMode(ledPin, OUTPUT);

  // WiFi 연결
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  // BLE 초기화 및 스캔 설정
  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
}

void loop() {
  // BLE 스캔 시작
  pBLEScan->start(scanTime, false);
  pBLEScan->clearResults();

  // 웹서버 요청 처리
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.length() == 0) {
            // HTTP 응답 헤더
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // HTML 콘텐츠
            client.println("<!DOCTYPE html><html><head><meta charset='utf-8'><title>BLE 거리 측정</title>");
            client.println("<meta http-equiv='refresh' content='2'></head><body><h2>BLE 거리 측정기</h2>");
            client.print("<p><strong>RSSI:</strong> ");
            client.print(rssi);
            client.println(" dBm</p>");
            client.print("<p><strong>추정 거리:</strong> ");
            client.print(distance);
            client.println(" m</p>");
            if (distance <= 1.0)
              client.println("<p style='color:red;'>⚠ 가까이 있음 (LED ON)</p>");
            else
              client.println("<p style='color:green;'>안전 거리</p>");
            client.println("</body></html>");
            break;
          } else currentLine = "";
        } else if (c != '\r') currentLine += c;
      }
    }
    delay(1);
    client.stop();
  }

  delay(1000); // 1초마다 반복
}

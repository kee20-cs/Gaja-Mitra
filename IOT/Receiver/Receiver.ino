#include <esp_now.h>
#include <WiFi.h>

const int BUZZER_PIN = 8;
const int RED_LED_PIN = 2;
const int GREEN_LED_PIN = 3;

const int BUZZ_FREQ = 2000;
const int BUZZ_DURATION_MS = 300;

typedef struct SensorPacket {
  char key;
  int accelValue;
} SensorPacket;

SensorPacket incoming;

bool buzzerActive = false;
unsigned long buzzStartTime = 0;

void onDataRecv(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(SensorPacket)) return;
  memcpy(&incoming, data, sizeof(incoming));

  Serial.print("Key: ");
  Serial.print(incoming.key ? incoming.key : '-');
  Serial.print(" | Accel: ");
  Serial.println(incoming.accelValue);

  if (incoming.key != '\0') {
    Serial.print(">> Key received (");
    Serial.print(incoming.key);
    Serial.println(") - Buzzing.");

    tone(BUZZER_PIN, BUZZ_FREQ, BUZZ_DURATION_MS);
    buzzerActive = true;
    buzzStartTime = millis();

    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);

  // Idle state: green on, red off
  digitalWrite(RED_LED_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, HIGH);

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb(onDataRecv);
  Serial.println("Receiver ready, waiting for data...");
}

void loop() {
  // Once the buzz duration has elapsed, go back to idle (green on, red off)
  if (buzzerActive && (millis() - buzzStartTime >= BUZZ_DURATION_MS)) {
    buzzerActive = false;
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(GREEN_LED_PIN, HIGH);
  }
}
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Keypad.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Keypad setup
const byte ROWS = 4, COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {4, 5, 6, 7};
byte colPins[COLS] = {15, 16, 17, 18};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// MPU6050 setup
Adafruit_MPU6050 mpu;
bool mpuConnected = false;
const int SDA_PIN = 9;
const int SCL_PIN = 10;

typedef struct SensorPacket {
  char key;
  int accelValue;
} SensorPacket;

SensorPacket outgoing;

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // keep radio active, avoids ping dropout

  Wire.begin(SDA_PIN, SCL_PIN);
  mpuConnected = mpu.begin();

  if (!mpuConnected) {
    Serial.println("MPU6050 not found - continuing without it.");
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
    Serial.println("MPU6050 ready.");
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  esp_now_add_peer(&peerInfo);

  Serial.println("Sender ready.");
}

void loop() {
  char key = keypad.getKey();

  int accelValue = -1; // sentinel: MPU not connected
  if (mpuConnected) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);
    float magnitude = sqrt(a.acceleration.x * a.acceleration.x +
                            a.acceleration.y * a.acceleration.y +
                            a.acceleration.z * a.acceleration.z);
    accelValue = (int)(magnitude * 100);
  }

  outgoing.key = key;
  outgoing.accelValue = accelValue;

  esp_now_send(broadcastAddress, (uint8_t *)&outgoing, sizeof(outgoing));

  if (key) {
    Serial.print("Key pressed: ");
    Serial.println(key);
  }

  delay(50);
}
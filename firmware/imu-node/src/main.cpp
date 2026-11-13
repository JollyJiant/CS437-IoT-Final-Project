#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ICM20X.h>
#include <Adafruit_ICM20948.h>
#include <Adafruit_Sensor.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

#define bleServerName "IMU_NODE_0"

#define SERVICE_UUID        "ee00f67e-97ca-4c37-8b5e-2137fd7dbe83"
#define CHARACTERISTIC_UUID "05ef3990-a968-4b05-81b0-550f9a8a1344"

BLEServer *pServer = NULL;
BLECharacteristic *pCharacteristic = NULL;
uint32_t value = 0;

class ServerCallbacks : public BLEServerCallbacks {
public:
  void onDisconnect(BLEServer *server) override {
    Serial.println("BLE client disconnected; restarting advertising");
    server->startAdvertising();
  }
};

Adafruit_ICM20948 icm;

void setup() {
  Serial.begin(115200);

  // Wait for the serial monitor
  unsigned long start = millis();
  while (!Serial && millis() - start < 5000) delay(10);

  //IMU set-up
  Serial.println("Initializing the IMU");
  if (!icm.begin_I2C()) {
    Serial.println("Failed to find IMU");
    while (1) delay(10);
  }

  // BLE set-up
  BLEDevice::init(bleServerName);
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();
}

void loop() {
  // sensors_event_t accel, gyro, temp, mag;
  // icm.getEvent(&accel, &gyro, &temp, &mag);

  // Serial.printf("Temp: %.1f C\n", temp.temperature);
  // Serial.printf("Accel X: %.2f  Y: %.2f  Z: %.2f m/s^2\n",
  //               accel.acceleration.x, accel.acceleration.y, accel.acceleration.z);
  // Serial.printf("Gyro  X: %.2f  Y: %.2f  Z: %.2f rad/s\n",
  //               gyro.gyro.x, gyro.gyro.y, gyro.gyro.z);
  // Serial.printf("Mag   X: %.2f  Y: %.2f  Z: %.2f uT\n\n",
  //               mag.magnetic.x, mag.magnetic.y, mag.magnetic.z);

  // delay(100);

  // notify changed value
  if (pServer->getConnectedCount() > 0) {
    pCharacteristic->setValue((uint8_t *)&value, 4);
    pCharacteristic->notify();
    Serial.println(value);
    value++;
    delay(500);
  } else {
    delay(100);
  }
}
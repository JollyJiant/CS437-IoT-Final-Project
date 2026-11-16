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
constexpr uint32_t SAMPLE_RATE_HZ = 10;
constexpr uint32_t SAMPLE_PERIOD_MS = 1000 / SAMPLE_RATE_HZ;

void setup() {
  Serial.begin(115200);

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

  // IMU set-up
  Serial.println("Initializing the IMU");
  if (!icm.begin_I2C()) {
    Serial.println("Failed to find IMU");
    while (1) delay(10);
  }
  
  // Set IMU ranges and output data rates
  icm.setAccelRange(ICM20948_ACCEL_RANGE_2_G);
  icm.setGyroRange(ICM20948_GYRO_RANGE_250_DPS);
  icm.setMagDataRate(AK09916_MAG_DATARATE_10_HZ);
  icm.setAccelRateDivisor(111); // ~10.04 Hz
  icm.setGyroRateDivisor(109);  // 10 Hz
}

void loop() {
  sensors_event_t accel, gyro, temp, mag;
  icm.getEvent(&accel, &gyro, &temp, &mag);

  Serial.printf("Accel X: %.2f  Y: %.2f  Z: %.2f m/s^2\n",
                accel.acceleration.x, accel.acceleration.y, accel.acceleration.z);
  Serial.printf("Gyro  X: %.2f  Y: %.2f  Z: %.2f rad/s\n",
                gyro.gyro.x, gyro.gyro.y, gyro.gyro.z);
  Serial.printf("Mag   X: %.2f  Y: %.2f  Z: %.2f uT\n",
                mag.magnetic.x, mag.magnetic.y, mag.magnetic.z);
  Serial.printf("\n");

  float imuData[9] {
    accel.acceleration.x,
    accel.acceleration.y,
    accel.acceleration.z,
    gyro.gyro.x,
    gyro.gyro.y,
    gyro.gyro.z,
    mag.magnetic.x,
    mag.magnetic.y,
    mag.magnetic.z,
  };

  // BLE notify
  if (pServer->getConnectedCount() > 0) {
    // send the float array's raw bytes and byte length.
    pCharacteristic->setValue(
      reinterpret_cast<uint8_t *>(imuData), sizeof(imuData)
    );
    pCharacteristic->notify();
  }
  
  delay(SAMPLE_PERIOD_MS);
}
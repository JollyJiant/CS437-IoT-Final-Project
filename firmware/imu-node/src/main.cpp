#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ICM20X.h>
#include <Adafruit_ICM20948.h>
#include <Adafruit_Sensor.h>
#include <BLEDevice.h>
#include <BLEServer.h>

#define bleServerName "IMU_NODE_0"

#define SERVICE_UUID        "ee00f67e-97ca-4c37-8b5e-2137fd7dbe83"
#define CHARACTERISTIC_UUID "05ef3990-a968-4b05-81b0-550f9a8a1344"
#define CONTROL_CHARACTERISTIC_UUID "b4ca5883-0b7f-4f90-bd7d-9f876d85c821"

BLEServer *pServer = NULL;
BLECharacteristic *pImuBatchCharacteristic = NULL;
BLECharacteristic *pControlCharacteristic = NULL;
bool clientReady = false;
bool samplingStarted = false;

class ServerCallbacks : public BLEServerCallbacks {
public:
  void onDisconnect(BLEServer *server) override {
    clientReady = false;
    Serial.println("BLE client disconnected; restarting advertising");
    server->startAdvertising();
  }
};

class ControlCallbacks : public BLECharacteristicCallbacks {
public:
  void onWrite(BLECharacteristic *characteristic) override {
    String command = characteristic->getValue();

    if (command == "READY") {
      clientReady = true;
    }
  }
};

Adafruit_ICM20948 icm;

struct ImuPacket {
  uint16_t batch_id;
  uint16_t sample_id;
  float imu[9];
};

uint16_t batch_id = 0;
uint16_t sample_id = 0;

constexpr uint32_t SAMPLE_RATE_HZ = 10;
constexpr uint32_t SAMPLE_PERIOD_MS = 1000 / SAMPLE_RATE_HZ;

constexpr uint32_t SEND_PERIOD_SECONDS = 1;
constexpr size_t PACKETS_PER_BATCH = SAMPLE_RATE_HZ * SEND_PERIOD_SECONDS;
constexpr size_t BUFFER_SIZE_BYTES = PACKETS_PER_BATCH * sizeof(ImuPacket);
ImuPacket packetBuffer[PACKETS_PER_BATCH];
size_t bufferedPackets = 0;

constexpr uint16_t BLE_LOCAL_MTU = 517;

void initializeBLE() {
  BLEDevice::init(bleServerName);
  BLEDevice::setMTU(BLE_LOCAL_MTU);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new ServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pImuBatchCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pControlCharacteristic = pService->createCharacteristic(
    CONTROL_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE
  );
  pControlCharacteristic->setCallbacks(new ControlCallbacks());
  pService->start();

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();
}

void initializeIMU() {
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

ImuPacket readImuPacket() {
  sensors_event_t accel, gyro, temp, mag;
  icm.getEvent(&accel, &gyro, &temp, &mag);

  Serial.printf("Batch: %d, Sample: %d\n", batch_id, sample_id);
  Serial.printf("Accel X: %.2f  Y: %.2f  Z: %.2f m/s^2\n",
                accel.acceleration.x, accel.acceleration.y, accel.acceleration.z);
  Serial.printf("Gyro  X: %.2f  Y: %.2f  Z: %.2f rad/s\n",
                gyro.gyro.x, gyro.gyro.y, gyro.gyro.z);
  Serial.printf("Mag   X: %.2f  Y: %.2f  Z: %.2f uT\n",
                mag.magnetic.x, mag.magnetic.y, mag.magnetic.z);
  Serial.printf("\n");

  return {
    batch_id,
    sample_id,
    {
      accel.acceleration.x,
      accel.acceleration.y,
      accel.acceleration.z,
      gyro.gyro.x,
      gyro.gyro.y,
      gyro.gyro.z,
      mag.magnetic.x,
      mag.magnetic.y,
      mag.magnetic.z,
    }
  };
}

void sendPacketBuffer() {
  pImuBatchCharacteristic->setValue(
    reinterpret_cast<uint8_t*>(packetBuffer),
    BUFFER_SIZE_BYTES
  );
  pImuBatchCharacteristic->notify();

  bufferedPackets = 0;
  ++batch_id;
}

void setup() {
  Serial.begin(115200);
  initializeBLE();
  initializeIMU();
}

void loop() {
  if (clientReady) {
    samplingStarted = true;

    if (bufferedPackets == PACKETS_PER_BATCH) {
      sendPacketBuffer();
    }

    packetBuffer[bufferedPackets++] = readImuPacket();
  }

  if (samplingStarted) {
    ++sample_id;
  }

  delay(SAMPLE_PERIOD_MS);
}
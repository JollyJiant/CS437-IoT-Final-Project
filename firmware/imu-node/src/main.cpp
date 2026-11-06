#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ICM20X.h>
#include <Adafruit_ICM20948.h>
#include <Adafruit_Sensor.h>

Adafruit_ICM20948 icm;

void setup() {
  Serial.begin(115200);

  // Wait for the serial monitor
  while (!Serial) delay(10);

  Serial.println("ICM20948 test");

  if (!icm.begin_I2C()) {
    Serial.println("Failed to find ICM20948");
    while (1) delay(10);
  }
  Serial.println("ICM20948 found");
}

void loop() {
  sensors_event_t accel, gyro, temp, mag;
  icm.getEvent(&accel, &gyro, &temp, &mag);

  Serial.printf("Temp: %.1f C\n", temp.temperature);
  Serial.printf("Accel X: %.2f  Y: %.2f  Z: %.2f m/s^2\n",
                accel.acceleration.x, accel.acceleration.y, accel.acceleration.z);
  Serial.printf("Gyro  X: %.2f  Y: %.2f  Z: %.2f rad/s\n",
                gyro.gyro.x, gyro.gyro.y, gyro.gyro.z);
  Serial.printf("Mag   X: %.2f  Y: %.2f  Z: %.2f uT\n\n",
                mag.magnetic.x, mag.magnetic.y, mag.magnetic.z);

  delay(100);
}
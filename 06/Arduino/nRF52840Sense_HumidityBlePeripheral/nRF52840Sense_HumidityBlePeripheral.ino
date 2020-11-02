// Heart Rate Monitor BLE peripheral. Copyright (c) Thomas Amberg, FHNW

// Based on https://github.com/adafruit/Adafruit_nRF52_Arduino
// /tree/master/libraries/Bluefruit52Lib/examples/Peripheral
// Copyright (c) Adafruit.com, all rights reserved.

// Licensed under the MIT license, see LICENSE or
// https://choosealicense.com/licenses/mit/

#include <bluefruit.h>

// 6b75fded-006c-4f1b-8e32-a20d9d19aa13 (GUID) =>
// 6b75xxxx-006c-4f1b-8e32-a20d9d19aa13 (Base UUID) =>
// 6b750001-006c-4f1b-8e32-a20d9d19aa13 (Humidity Service)
// 6b750002-006c-4f1b-8e32-a20d9d19aa13 (Humidity Measurement Chr.)
// 6b750003-006c-4f1b-8e32-a20d9d19aa13 (Heater State Chr.)

// The arrays below are ordered "least significant byte first":
uint8_t const humidityServiceUuid[] = { 0x13, 0xaa, 0x19, 0x9d, 0x0d, 0xa2, 0x32, 0x8e, 0x1b, 0x4f, 0x6c, 0x00, 0x01, 0x00, 0x75, 0x6b };
uint8_t const humidityMeasurementCharacteristicUuid[] = { 0x13, 0xaa, 0x19, 0x9d, 0x0d, 0xa2, 0x32, 0x8e, 0x1b, 0x4f, 0x6c, 0x00, 0x02, 0x00, 0x75, 0x6b };
uint8_t const heaterStateCharacteristicUuid[] = { 0x13, 0xaa, 0x19, 0x9d, 0x0d, 0xa2, 0x32, 0x8e, 0x1b, 0x4f, 0x6c, 0x00, 0x03, 0x00, 0x75, 0x6b };

BLEService humidityService = BLEService(humidityServiceUuid);
BLECharacteristic humidityMeasurementCharacteristic = BLECharacteristic(humidityMeasurementCharacteristicUuid);
BLECharacteristic heaterStateCharacteristic = BLECharacteristic(heaterStateCharacteristicUuid);

void connectedCallback(uint16_t connectionHandle) {
  char centralName[32] = { 0 };
  BLEConnection *connection = Bluefruit.Connection(connectionHandle);
  connection->getPeerName(centralName, sizeof(centralName));
  Serial.print(connectionHandle);
  Serial.print(", connected to ");
  Serial.print(centralName);
  Serial.println();
}

void disconnectedCallback(uint16_t connectionHandle, uint8_t reason) {
  Serial.print(connectionHandle);
  Serial.print(" disconnected, reason = ");
  Serial.println(reason); // see https://github.com/adafruit/Adafruit_nRF52_Arduino
  // /blob/master/cores/nRF5/nordic/softdevice/s140_nrf52_6.1.1_API/include/ble_hci.h
  Serial.println("Advertising ...");
}

void cccdCallback(uint16_t connectionHandle, BLECharacteristic* characteristic, uint16_t cccdValue) {
  if (characteristic->uuid == humidityMeasurementCharacteristic.uuid) {
    Serial.print("Heart Rate Measurement 'Notify', ");
    if (characteristic->notifyEnabled()) {
      Serial.println("enabled");
    } else {
      Serial.println("disabled");
    }
  }
}

void setupHumidityService() {
  humidityService.begin(); // Must be called before calling .begin() on its characteristics

  //humidityMeasurementCharacteristic.setProperties(CHR_PROPS_READ);
  humidityMeasurementCharacteristic.setProperties(CHR_PROPS_NOTIFY);
  humidityMeasurementCharacteristic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  humidityMeasurementCharacteristic.setFixedLen(1);
  humidityMeasurementCharacteristic.setCccdWriteCallback(cccdCallback);  // Optionally capture CCCD updates
  humidityMeasurementCharacteristic.begin();

  heaterStateCharacteristic.setProperties(CHR_PROPS_WRITE);
  heaterStateCharacteristic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  heaterStateCharacteristic.setFixedLen(1);
  heaterStateCharacteristic.begin();
  heaterStateCharacteristic.write8(0); // Sensor location 'Other'
}

void startAdvertising() {
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();
  Bluefruit.Advertising.addService(humidityService);
  Bluefruit.Advertising.addName();

  // See https://developer.apple.com/library/content/qa/qa1931/_index.html   
  const int fastModeInterval = 32; // * 0.625 ms = 20 ms
  const int slowModeInterval = 244; // * 0.625 ms = 152.5 ms
  const int fastModeTimeout = 30; // s
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(fastModeInterval, slowModeInterval);
  Bluefruit.Advertising.setFastTimeout(fastModeTimeout);
  // 0 = continue advertising after fast mode, until connected
  Bluefruit.Advertising.start(0);
  Serial.println("Advertising ...");
}

void setup() {
  Serial.begin(115200);
  while (!Serial) { delay(10); } // only if usb connected
  Serial.println("Setup");

  Bluefruit.begin();
  Bluefruit.setName("nRF52840");
  Bluefruit.Periph.setConnectCallback(connectedCallback);
  Bluefruit.Periph.setDisconnectCallback(disconnectedCallback);

  setupHumidityService();
  startAdvertising();
}

void loop() {
  if (Bluefruit.connected()) {
    int value = analogRead(A0);
    uint8_t humidityData[1] = { value };
    //humidityMeasurementCharacteristic.write8(0);
    if (humidityMeasurementCharacteristic.notify(humidityData, sizeof(humidityData))) {
      Serial.print("Notified, humidity = ");
      Serial.println(value);
    } else {
      Serial.println("Notify not set, or not connected");
    }
  }
  delay(1000); // ms
}
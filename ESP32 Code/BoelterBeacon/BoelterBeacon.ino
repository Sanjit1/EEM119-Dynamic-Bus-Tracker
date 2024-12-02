/*
   Based on 31337Ghost's reference code from https://github.com/nkolban/esp32-snippets/issues/385#issuecomment-362535434
   which is based on pcbreflux's Arduino ESP32 port of Neil Kolban's example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
*/

/*
   Create a BLE server that will send periodic iBeacon frames.
   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create advertising data
   3. Start advertising.
   4. wait
   5. Stop advertising.
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <BLEBeacon.h>

#define DEVICE_NAME "Boelter"
#define SERVICE_UUID "F4CB23F6-9F84-42CC-A0C7-BC265A63D622"
#define BEACON_UUID "D1FC973B-E5EB-4895-A136-C973165D1E2C"
#define BEACON_UUID_REV "2C1E5D16-73C9-36A1-9548-EBE53B97FCD1"
#define CHARACTERISTIC_UUID "82258BAA-DF72-47E8-99BC-B73D7ECD08A5"

BLEServer *pServer;
BLECharacteristic *pCharacteristic;
bool deviceConnected = false;
uint8_t value = 0;

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        deviceConnected = true;
        Serial.println("deviceConnected = true");
    };

    void onDisconnect(BLEServer *pServer)
    {
        deviceConnected = false;
        Serial.println("deviceConnected = false");

        // Restart advertising to be visible and connectable again
        BLEAdvertising *pAdvertising;
        pAdvertising = pServer->getAdvertising();
        pAdvertising->start();
        Serial.println("iBeacon advertising restarted");
    }
};

class MyCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        String rxValue = pCharacteristic->getValue();

        if (rxValue.length() > 0)
        {
            Serial.println("*********");
            Serial.print("Received Value: ");
            for (int i = 0; i < rxValue.length(); i++)
            {
                Serial.print(rxValue[i]);
            }
            Serial.println();
            Serial.println("*********");
        }
    }
};

void init_service()
{
    BLEAdvertising *pAdvertising;
    pAdvertising = pServer->getAdvertising();
    pAdvertising->stop();

    // Create the BLE Service
    BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID));

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
    pCharacteristic->setCallbacks(new MyCallbacks());
    pCharacteristic->addDescriptor(new BLE2902());

    pAdvertising->addServiceUUID(BLEUUID(SERVICE_UUID));

    // Start the service
    pService->start();

    pAdvertising->start();
}

void init_beacon()
{
    BLEAdvertising *pAdvertising;
    pAdvertising = pServer->getAdvertising();
    pAdvertising->stop();
    // iBeacon
    BLEBeacon myBeacon;
    myBeacon.setManufacturerId(0x4c00);
    myBeacon.setMajor(5);
    myBeacon.setMinor(88);
    myBeacon.setSignalPower(0xc5);
    myBeacon.setProximityUUID(BLEUUID(BEACON_UUID_REV));

    BLEAdvertisementData advertisementData;
    advertisementData.setFlags(0x1A);
    advertisementData.setManufacturerData(myBeacon.getData());
    pAdvertising->setAdvertisementData(advertisementData);

    pAdvertising->start();
}

void setup()
{
    Serial.begin(115200);
    Serial.println();
    Serial.println("Initializing...");
    Serial.flush();

    BLEDevice::init(DEVICE_NAME);
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    init_service();
    init_beacon();

    Serial.println("iBeacon + service defined and advertising!");
}

void loop()
{
    if (deviceConnected)
    {
        Serial.printf("*** NOTIFY: %d ***\n", value);
        pCharacteristic->setValue(&value, 1);
        pCharacteristic->notify();
        value++;
    }
    delay(2000);
}

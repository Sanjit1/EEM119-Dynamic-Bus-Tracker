/*
     Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleScan.cpp
     Ported to Arduino ESP32 by Evandro Copercini
     Changed to a beacon scanner to report iBeacon, EddystoneURL and EddystoneTLM beacons by beegee-tokyo
     Upgraded Eddystone part by Tomas Pilny on Feb 20, 2023
*/

#include <Arduino.h>

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <BLEEddystoneURL.h>
#include <BLEEddystoneTLM.h>
#include <BLEBeacon.h>

#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "EIII"
#define WIFI_PASSWORD "Ilovesolder"

// Insert Firebase project API Key
#define API_KEY "AIzaSyAyRBlsUI1wZ9eQT8HK2rAQuS3P_jajF4U"

// Insert RTDB URL
#define DATABASE_URL "https://dynamic-bus-tracker-16b7a-default-rtdb.firebaseio.com/"

// Define Firebase Data object
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

int scanTime = 5; // In seconds
BLEScan *pBLEScan;

String lastStop = "The Middle of Nowhere";
String currentStop = "The Middle of Nowhere";
int signalThreshold = -45;

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    void onResult(BLEAdvertisedDevice advertisedDevice)
    {
        if (advertisedDevice.haveName() && advertisedDevice.getRSSI() > signalThreshold)
        {
            if (advertisedDevice.getName() == "Boelter")
            {
                currentStop = "Boelter";
                lastStop = currentStop;
            }
            else if (advertisedDevice.getName() == "Landfair")
            {
                currentStop = "Landfair";
                lastStop = currentStop;
            }
        }
    }
};
void setup()
{
    Serial.begin(115200);
    Serial.println("Scanning...");

    BLEDevice::init("");
    pBLEScan = BLEDevice::getScan(); // create new scan
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setActiveScan(true); // active scan uses more power, but get results faster
    pBLEScan->setInterval(100);
    pBLEScan->setWindow(99); // less or equal setInterval value

    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }

    // Print local IP address
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    // Assign the api key (required)
    config.api_key = API_KEY;

    // Assign the RTDB URL (required)
    config.database_url = DATABASE_URL;

    // Sign up as an anonymous user
    if (Firebase.signUp(&config, &auth, "", ""))
    {
        Serial.println("ok");
        signupOK = true;
    }
    else
    {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    // Assign the callback function for the long running token generation task
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

void loop()
{
    // put your main code here, to run repeatedly:
    // set the current stop to middle of nowhere
    currentStop = "The Middle of Nowhere";
    BLEScanResults *foundDevices = pBLEScan->start(scanTime, false);
    pBLEScan->clearResults(); // delete results fromBLEScan buffer to release memory
    Serial.println(lastStop);
    Serial.println(currentStop);
    if (Firebase.ready() && signupOK)
    {
        // Last Stop
        if (Firebase.RTDB.setString(&fbdo, "lastStop", lastStop))
        {
            Serial.println("Set last Stop");
        }
        else
        {
            Serial.println("FAILED to set last Stop");
            Serial.println("REASON: " + fbdo.errorReason());
        }
        // if currentStop is The Middle of Nowhere, write to the stopped boolean: false else true
        if (currentStop == "The Middle of Nowhere")
        {
            if (Firebase.RTDB.setBool(&fbdo, "stopped", false))
            {
                Serial.println("Set stopped");
            }
            else
            {
                Serial.println("FAILED to set stopped");
                Serial.println("REASON: " + fbdo.errorReason());
            }
        }
        else
        {
            if (Firebase.RTDB.setBool(&fbdo, "stopped", true))
            {
                Serial.println("Set stopped");
            }
            else
            {
                Serial.println("FAILED to set stopped");
                Serial.println("REASON: " + fbdo.errorReason());
            }
        }
        Serial.println("");
        delay(1500);
    }
}
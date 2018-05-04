// Copyright (c) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Please use an Arduino IDE 1.6.8 or greater

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

#include <AzureIoTHub.h>
#include <AzureIoTProtocol_MQTT.h>
#include <AzureIoTUtility.h>

#include <ArduinoJson.h>

#include "config.h"

static bool messagePending = false;
static bool messageSending = true;

static char *connectionString;
static char *ssid;
static char *pass;

static int interval = INTERVAL;

// RPM measurements
volatile byte revolutions;
unsigned int rpm;
unsigned long timeold;

void blinkLED()
{
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
}

void initWifi()
{
    // Attempt to connect to Wifi network:
    Serial.printf("Attempting to connect to SSID: %s.\r\n", ssid);

    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED)
    {
        // Get Mac Address and show it.
        // WiFi.macAddress(mac) save the mac address into a six length array, but the endian may be different. The huzzah board should
        // start from mac[0] to mac[5], but some other kinds of board run in the oppsite direction.
        uint8_t mac[6];
        WiFi.macAddress(mac);
        Serial.printf("You device with MAC address %02x:%02x:%02x:%02x:%02x:%02x connects to %s failed! Waiting 10 seconds to retry.\r\n",
                      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ssid);
        WiFi.begin(ssid, pass);
        delay(10000);
    }
    Serial.printf("Connected to wifi %s.\r\n", ssid);
}

void initTime()
{
    time_t epochTime;
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    while (true)
    {
        epochTime = time(NULL);

        if (epochTime == 0)
        {
            Serial.println("Fetching NTP epoch time failed! Waiting 2 seconds to retry.");
            delay(2000);
        }
        else
        {
            Serial.printf("Fetched NTP epoch time is: %lu.\r\n", epochTime);
            break;
        }
    }
}

static IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle;

void setup()
{
    pinMode(LED_PIN, OUTPUT);

    initSerial();
    delay(2000);
    
    readCredentials();

    initWifi();
    initTime();
    initSensor();

    // Create the client using the connection string stored in the EPROM
    
    iotHubClientHandle = IoTHubClient_LL_CreateFromConnectionString(connectionString, MQTT_Protocol);
    
    if (iotHubClientHandle == NULL)
    {
        Serial.println("Failed on IoTHubClient_CreateFromConnectionString.");
        while (1)
            ;
    }

    // This is handy to trace what is going on in detail
    bool traceOn = false;
    IoTHubClient_LL_SetOption(iotHubClientHandle, OPTION_LOG_TRACE, &traceOn);

    IoTHubClient_LL_SetOption(iotHubClientHandle, "product_info", "RPM Sensor");
    Serial.println("Initialized!");
}

static int messageCount = 1;
unsigned int previousRpm = 0;

void loop()
{
    if (!messagePending && messageSending)
    {
        char messagePayload[MESSAGE_MAX_LEN];
        unsigned int rpm = readRPM(messageCount, messagePayload);
        Serial.println(rpm);

        sendMessage(iotHubClientHandle, messagePayload);
        messageCount++;
        blinkLED();
        delay(interval);
        previousRpm = rpm;
    }
    IoTHubClient_LL_DoWork(iotHubClientHandle);
    delay(10);
}

void initSensor()
{
    attachInterrupt(INTERRUPT_PIN, magnet_detect, RISING);
    revolutions = 0;
    rpm = 0;
    timeold = 0;
}

unsigned int readRPM(int messageId, char *payload)
{
   if (revolutions >= 2) { 
      rpm = 30*1000/(millis() - timeold)*revolutions;
      timeold = millis();
      revolutions = 0;
   } 
   else {
    rpm = 0; 
   }

    StaticJsonBuffer<MESSAGE_MAX_LEN> jsonBuffer;
    JsonObject &root = jsonBuffer.createObject();
    root["deviceId"] = DEVICE_ID;
    root["messageId"] = messageId;

    // NAN is not the valid json, change it to NULL
    if (std::isnan(rpm))
    {
        root["rpm"] = NULL;
    }
    else
    {
        root["rpm"] = rpm;
    }
    root.printTo(payload, MESSAGE_MAX_LEN);
    return rpm;
}

void magnet_detect()
{
   revolutions++;
}

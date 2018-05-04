#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiUdp.h>

static WiFiClientSecure sslClient; // for ESP8266

const char *onSuccess = "\"Successfully invoke device method\"";
const char *notFound = "\"No method found\"";
bool debugDataSwitch = true;

static void debugMessage(const char *message){
  if (debugDataSwitch){
    Serial.println(message);
  }
}

static void debugData(const char *message, const char *buffer){
  if (debugDataSwitch){
    Serial.printf(message, buffer);
  }
}

static void sendCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result, void *userContextCallback)
{
    if (IOTHUB_CLIENT_CONFIRMATION_OK == result)
    {
        debugMessage("Message sent to Azure IoT Hub");
        blinkLED();
    }
    else
    {
        debugMessage("Failed to send message to Azure IoT Hub");
    }
    messagePending = false;
}

static void sendMessage(IOTHUB_CLIENT_LL_HANDLE iotHubClientHandle, char *buffer)
{
    IOTHUB_MESSAGE_HANDLE messageHandle = IoTHubMessage_CreateFromByteArray((const unsigned char *)buffer, strlen(buffer));
    if (messageHandle == NULL)
    {
        debugMessage("Unable to create a new IoTHubMessage.");
    }
    else
    {
        debugData("Sending message: %s.\r\n", buffer);
        if (IoTHubClient_LL_SendEventAsync(iotHubClientHandle, messageHandle, sendCallback, NULL) != IOTHUB_CLIENT_OK)
        {
            debugMessage("Failed to hand over the message to IoTHubClient.");
        }
        else
        {
            messagePending = true;
            debugMessage("IoTHubClient accepted the message for delivery.");
        }

        IoTHubMessage_Destroy(messageHandle);
    }
}

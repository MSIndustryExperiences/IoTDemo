---
title: IoTDemo
---

Demo code used in various manufacturing events by \@diegotamburini and \@ercenk.

The demo intends to showcase the scenario for collecting data from a device with
internet connectivity, and the underlying platform services for processing and
visualizing the output.

The device is a simple crank attached to a wheel, and a microcontroller breakout
board (an [Adafruit Feather HUZZAH with
ESP8266](https://www.adafruit.com/product/2821)) that measures the RPM of the
wheel and sends that data to the cloud.

![](assets/crank.gif)

There are two magnets attached to the wheel, that pass in front of a [hall
effect sensor](https://en.wikipedia.org/wiki/Hall_effect_sensor). The sensor is
attached to one of the I/O pins of the breakout board, and the code simply
records the number of revolutions, then sends it to the cloud.

High Level View of the Demo
---------------------------

The device is configured with a connection to an IoT Hub, and sends messages
periodically, containing the RPM data. There is a stream analytics job that
reads the data from the IoT Hub and copies the data to a Microsoft Azure Storage
blob, as well as exposes it on Power BI.

![A close up of a logo Description generated with high confidence](assets/highlevel.png)

A close up of a logo Description generated with high confidence

We would like to create a live tile on the PowerBI dashboard that will be
displaying the incoming data, looking like the following:

![A close up of text on a white background Description generated with high confidence](assets/RPMChart.gif)

A close up of text on a white background Description generated with high
confidence

Device Code
-----------

We borrowed most of the device code from the [IoT Hub Feather HUZZAH client
application](https://github.com/Azure-Samples/iot-hub-feather-huzzah-client-app)
Azure sample, however modified it a little bit for our needs, and removed the
parts not related to the demo to demonstrate the concept.

The parted we added is for counting the times a magnet passes in front of the
hall effect sensor.

An interrupt handler is configured using the “*attachInterrupt()*” function.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    attachInterrupt(INTERRUPT_PIN, magnet_detect, RISING);
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

and every time the magnet is detected, the interrupt handler does the most
basic, and fastest thing to do, just records the revolutions.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    void magnet_detect()
    {
        revolutions++;
    }
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

We also removed the parts that receives messages from the cloud.

There are a few pieces in the sample that worth mentioning, such as securing the
device. The device uses a symmetrical key, and stores it in the EEPROM, along
with the Wi-Fi network credentials. It is a good practice for using this method,
since there is no configuration file where sensitive information appears. And of
course, production level manageability of this configuration technique is up for
debate. We recommend different techniques for provisioning large number of
devices in real life scenarios. Please take a look at [Azure IoT Hub Device
Provisioning Service](https://docs.microsoft.com/en-us/azure/iot-dps/) for
making this task easier.

The part that performs reading/writing secrets is in the file *credentials.ino*.

The basic anatomy of an Arduino project (the breakout is an Arduino compatible
device) is, the toolchain expects a function that does the setup (called
*setup()*) and runs another function, that usually has a tight loop (*loop()*).

The sample code initializes the serial port for reading inputs (Wi-Fi SSID and
password, IoT Hub connection string and prompts), reads the credentials from
EEPROM, initializes Wi-Fi, gets the current time from an NTP server, and finally
initializes the sensor.

Azure Side
----------

We have four high-level components:

-   Secure device connectivity, and ingest data, in a scalable way – Azure IoT
    Hub

-   Process streaming data – Azure Stream Analytics

-   Store – Azure Storage Blobs

-   Present the data – Microsoft Power BI

We will start with creating the IoT Hub. There are different ways for creating
required Azure resources, such as using [Azure
Portal](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-through-portal),
[Azure
PowerShell](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-using-powershell),
[Azure
CLI](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-create-using-cli),
[Azure REST
API](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-rm-rest), or via an
[Azure Resource Manager (ARM)
template](https://github.com/Azure/azure-quickstart-templates/tree/master/101-iothub-with-consumergroup-create).

Once the Azure IoT Hub resource is deployed, you will need to create a device
definition, and connect the device to the Azure IoT Hub.

The steps for creating the device definition and connecting the device is
exactly the same as they are described in the
[tutorial](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-arduino-huzzah-esp8266-get-started)
for connecting Adafruit Feather HUZZAH ESP8266 to Azure IoT Hub.

Next step will be deploying an Azure Stream Analytics job, and specifying
inputs, outputs and the query.

We will have only one input, for the data coming in from the IoT Hub. For that,
go to your resource group, and add a new “Streaming Analytics Job” resource.
Once the resource is deployed, add an “IoT Hub” streaming input.

![A screenshot of a cell phone Description generated with very high confidence](assets/ASASteps.png)

A screenshot of a cell phone Description generated with very high confidence

Next create the outputs, one for “Blob Storage” one for “Power BI”, one after
the other.

![](media/fd917aa4f45a93875b7248d2ac10cd33.png)

Now it is time to do basic processing. We are not going to write a complicated
query here, although the [query
language](https://msdn.microsoft.com/azure/stream-analytics/reference/stream-analytics-query-language-reference)
(a subset of standard T-SQL syntax) allows many sophisticated queries.

Use the following query as a template, and substitute the input and outputs
accordingly.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SELECT
    *
INTO
    [blobOut]
FROM
    [IoTHubIn]

SELECT
    *
INTO
    [powerBi]
FROM
    [IoTHubIn]    
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Once the query is defined, save the job, and start it.

Now head over to the Power BI Portal at <https://powerbi.microsoft.com> and a
new dashboard.

Add a new tile to the dashboard, and add a “Custom Streaming Data” tile from the
“Real-Time Data” section.

![A screenshot of a cell phone Description generated with very high confidence](assets/ASAOutputSteps.png)

Then select the dataset you just created, select the visualization style, as
well as the data you want to display.

Conclusion
----------

We just walked through how to bring data from a device to Power BI with little
amount of code. By just using Azure services, we also received many benefits.

-   **Security**: Azure IoT Hub uses TLS for transport layer, and also uses
    symmetric or asymmetric keys for authenticating the devices. The Azure
    Storage service is secured using an access key, and the Azure Stream
    Analytics job output uses that key. Power BI connection is secured using
    Azure Active Directory and make take advantage of multi factor
    authentication, if it is configured for your directory

-   **Scalability**: Azure IoT hub can
    [scale](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-scaling) to
    millions of messages via sharding and right tier. Azure Stream Analytics can
    scale using different streaming units, or parallelization of the queries as
    described in the online
    [documentation](https://docs.microsoft.com/en-us/azure/stream-analytics/stream-analytics-streaming-unit-consumption).

-   **Manageability**: As noted above, there are multiple ways of managing Azure
    resources, either through PowerShell, code, or portal. [Azure
    Advisor](https://docs.microsoft.com/en-us/azure/advisor/advisor-overview) is
    also a very good resource in determining proactive actions on high
    availability, security, performance and cost of your deployments.

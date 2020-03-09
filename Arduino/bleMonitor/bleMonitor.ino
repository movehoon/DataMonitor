// This sketch is a companion to the Bluetooth LE for iOS, tvOS and Android plugin for Unity3D.
// It is the hardware side of the StartingExample.

// The URL to the asset on the asset store is:
// https://assetstore.unity.com/packages/tools/network/bluetooth-le-for-ios-tvos-and-android-26661

// This sketch simply advertises as ledbtn and has a single service with 2 characteristics.
// The ledUUID characteristic is used to turn the LED on and off. Writing 0 turns it off and 1 turns it on.
// The buttonUUID characteristic can be read or subscribe to. When the button is down the characteristic
// value is 1. When the button is up the value is 0.

// This sketch was written for the Hiletgo ESP32 dev board found here:
// https://www.amazon.com/HiLetgo-ESP-WROOM-32-Development-Microcontroller-Integrated/dp/B0718T232Z/ref=sr_1_3?keywords=hiletgo&qid=1570209988&sr=8-3

// Many other ESP32 devices will work fine.

#include "BLEDevice.h"
#include "BLE2902.h"

// pin 2 on the Hiletgo
// (can be turned on/off from the iPhone app)
const uint32_t led = 2;

// pin 5 on the RGB shield is button 1
// (button press will be shown on the iPhone app)
const uint32_t button = 0;

static BLEUUID serviceUUID("A9E90000-194C-4523-A473-5FDF36AA4D20");
static BLEUUID ledUUID("A9E90001-194C-4523-A473-5FDF36AA4D20");
static BLEUUID buttonUUID("A9E90002-194C-4523-A473-5FDF36AA4D20");

bool deviceConnected = false;
bool oldDeviceConnected = false;

bool lastButtonState = false;

BLEServer* pServer = 0;
BLECharacteristic* pCharacteristicCommand = 0;
BLECharacteristic* pCharacteristicData = 0;

class BTServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer* pServer)
  {
      Serial.println("Connected...");
      deviceConnected = true;
  };
  
  void onDisconnect(BLEServer* pServer)
  {
      Serial.println("Disconnected...");
      deviceConnected = false;
  
      // don't leave the led on if they disconnect
      digitalWrite(led, LOW);
  }
};


class BTCallbacks : public BLECharacteristicCallbacks
{
  void onRead(BLECharacteristic* pCharacteristic)
  {
  }

  void onWrite(BLECharacteristic* pCharacteristic)
  {
//      uint8_t* data = pCharacteristic->getData();
//      int len = pCharacteristic->getValue().empty() ? 0 : pCharacteristic->getValue().length();
//  
//      if (len > 0)
//      {
//        Serial.println(data);
////          // if the first byte is 0x01 / on / true
////          if (data[0] == 0x01)
////              digitalWrite(led, HIGH);
////          else
////              digitalWrite(led, LOW);
//      }

      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {
        Serial.println("*********");
        Serial.print("Received Value: ");
        for (int i = 0; i < rxValue.length(); i++)
          Serial.print(rxValue[i]);

        Serial.println();
        Serial.println("*********");
      }
      
  }
};

// debounce time (in ms)
int debounce_time = 10;

// maximum debounce timeout (in ms)
int debounce_timeout = 100;

void BluetoothStartAdvertising()
{
    if (pServer != 0)
    {
        BLEAdvertising* pAdvertising = pServer->getAdvertising();
        pAdvertising->start();
    }
}

void BluetoothStopAdvertising()
{
    if (pServer != 0)
    {
        BLEAdvertising* pAdvertising = pServer->getAdvertising();
        pAdvertising->stop();
    }
}

char qnode_name[32];
void setup()
{
    Serial.begin(115200);
    Serial2.begin(115200);

    // led turned on/off from the iPhone app
    pinMode(led, OUTPUT);

    // button press will be shown on the iPhone app)
    pinMode(button, INPUT);

    uint64_t chipid = ESP.getEfuseMac();
    sprintf(qnode_name, "qnode_%04X", (uint16_t)chipid);
    Serial.println(qnode_name);

    BLEDevice::init("qnode_name");
    // BLEDevice::setCustomGattsHandler(my_gatts_event_handler);
    // BLEDevice::setCustomGattcHandler(my_gattc_event_handler);

    pServer = BLEDevice::createServer();
    BLEService* pService = pServer->createService(serviceUUID);
    pServer->setCallbacks(new BTServerCallbacks());

    pCharacteristicCommand = pService->createCharacteristic(
        buttonUUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY);

    pCharacteristicCommand->setCallbacks(new BTCallbacks());
    pCharacteristicCommand->setValue("");
    pCharacteristicCommand->addDescriptor(new BLE2902());

    pCharacteristicData = pService->createCharacteristic(
        ledUUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_WRITE |
            BLECharacteristic::PROPERTY_NOTIFY);

    pCharacteristicData->setCallbacks(new BTCallbacks());
    pCharacteristicData->setValue("");
    pCharacteristicData->addDescriptor(new BLE2902());

    pService->start();
    BluetoothStartAdvertising();
}

uint16_t count;
char buff[256];
void loop()
{
    if (pServer != 0)
    {
        // disconnecting
        if (!deviceConnected && oldDeviceConnected)
        {
            delay(500);                  // give the bluetooth stack the chance to get things ready
            pServer->startAdvertising(); // restart advertising
            Serial.println("start advertising");
            oldDeviceConnected = deviceConnected;
        }

        // connecting
        if (deviceConnected && !oldDeviceConnected)
        {
            oldDeviceConnected = deviceConnected;
        }

        uint8_t buttonState = digitalRead(button);

        if (deviceConnected && pCharacteristicCommand != 0 && buttonState != lastButtonState)
        {
            lastButtonState = buttonState;

            uint8_t packet[1];
            packet[0] = buttonState == HIGH ? 0x00 : 0x01;
            pCharacteristicCommand->setValue(packet, 1);
            pCharacteristicCommand->notify();
        }
        delay(100);
        if (deviceConnected) {
          count++;
          sprintf(buff, "{CURQA:-0.1234, COUNT:%d, HELLO THIS IS TEST NOW RUNNING}", count);
          pCharacteristicCommand->setValue((uint8_t *)buff, strlen(buff));
          pCharacteristicCommand->notify();
        }
    }
}

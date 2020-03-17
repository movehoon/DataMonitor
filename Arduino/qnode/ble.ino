#include "BLEDevice.h"
#include "BLE2902.h"

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

bool BleConnected() {
  return deviceConnected;
}

void BleSend(uint8_t *buff, uint8_t len) {
  pCharacteristicCommand->setValue(buff, len);
  pCharacteristicCommand->notify();
}

char qnode_name[32];
void setupBle()
{
    uint64_t chipid = ESP.getEfuseMac();
    sprintf(qnode_name, "QNODE_%04X", (uint16_t)chipid);
    Serial.println(qnode_name);

    BLEDevice::init(qnode_name);
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
void loopBle()
{
  delay(1);
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

//        uint8_t buttonState = digitalRead(button);
//        if (deviceConnected && pCharacteristicCommand != 0 && buttonState != lastButtonState)
//        {
//            lastButtonState = buttonState;
//
//            uint8_t packet[1];
//            packet[0] = buttonState == HIGH ? 0x00 : 0x01;
//            pCharacteristicCommand->setValue(packet, 1);
//            pCharacteristicCommand->notify();
//        }
//        if (deviceConnected) {
//          count++;
//          sprintf(buff, "{CURQA:-0.1234, COUNT:%d, HELLO THIS IS TEST NOW RUNNING}", count);
//          pCharacteristicCommand->setValue((uint8_t *)buff, strlen(buff));
//          pCharacteristicCommand->notify();
//          Serial.println(buff);
//        }
    }
}

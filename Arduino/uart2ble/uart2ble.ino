/*
    This program monitor UART data by BT

   Create a BLE server that, once we receive a connection, will send periodic notifications.
   The service advertises itself as: 6E400001-B5A3-F393-E0A9-E50E24DCCA9E
   Has a characteristic of: 6E400002-B5A3-F393-E0A9-E50E24DCCA9E - used for receiving data with "WRITE" 
   Has a characteristic of: 6E400003-B5A3-F393-E0A9-E50E24DCCA9E - used to send data with  "NOTIFY"

   The design of creating the BLE server is:
   1. Create a BLE Server
   2. Create a BLE Service
   3. Create a BLE Characteristic on the Service
   4. Create a BLE Descriptor on the characteristic
   5. Start the service.
   6. Start advertising.

   In this example rxValue is the data received (only accessible inside that function).
   And txValue is the data to be sent, in this example just a byte incremented every second. 
*/
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include "led_indicator.h"

#define BT_NAME "OAS_UART"
#define SERVICE_UUID           "6E400001-B5A3-F393-E0A9-E50E24DCCA9E" // UART service UUID
#define CHARACTERISTIC_UUID_RX "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"
#define CHARACTERISTIC_UUID_TX "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"

enum {
  LED_NONE,
  LED_RUNNING,
  LED_DISCONNECT_BLE,
};

BLEServer *pServer = NULL;
BLECharacteristic *pTxCharacteristic;
bool deviceConnected = false;
bool oldDeviceConnected = false;

#define LED_BUILTIN 2


LedIndicator ledIndicator;

// ----- Timer Setup -----//
hw_timer_t * timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

void IRAM_ATTR onTimer(){
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerMux);

  digitalWrite(LED_BUILTIN, ledIndicator.process());

  portEXIT_CRITICAL_ISR(&timerMux);
  // Give a semaphore that we can check in the loop
  xSemaphoreGiveFromISR(timerSemaphore, NULL);
  // It is safe to use digitalRead/Write here if you want to toggle an output
}
// ----- Timer Setup -----//


class BleServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
      ledIndicator.SetBlinkCount(LED_RUNNING);
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
      ledIndicator.SetBlinkCount(LED_DISCONNECT_BLE);
    }
};

class BleCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
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


void setup() {
  Serial.begin(115200);
  Serial2.begin(115200);

  pinMode(LED_BUILTIN, OUTPUT);

  // Create the BLE Device
  BLEDevice::init(BT_NAME);

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new BleServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pTxCharacteristic = pService->createCharacteristic(
                    CHARACTERISTIC_UUID_TX,
                    BLECharacteristic::PROPERTY_NOTIFY
                  );
                      
  pTxCharacteristic->addDescriptor(new BLE2902());

  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(
                       CHARACTERISTIC_UUID_RX,
                      BLECharacteristic::PROPERTY_WRITE
                    );

  pRxCharacteristic->setCallbacks(new BleCallbacks());

  // Start the service
  pService->start();

  // Start advertising
  pServer->getAdvertising()->start();
  Serial.println("Waiting a client connection to notify...");

  ledIndicator.SetBlinkCount(LED_DISCONNECT_BLE);

  // ----- Start Timer -----//
  // Create semaphore to inform us when the timer has fired
  timerSemaphore = xSemaphoreCreateBinary();
  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  timer = timerBegin(0, 80, true);
  // Attach onTimer function to our timer.
  timerAttachInterrupt(timer, &onTimer, true);
  // Set alarm to call onTimer function every 200 microsecond (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 200000, true);
  // Start an alarm
  timerAlarmEnable(timer);
  // ----- Start Timer -----//
}

void loop() {
    serialEvent();
    
    if (deviceConnected) {
//        pTxCharacteristic->setValue(&txValue, 1);
//        pTxCharacteristic->notify();
//        txValue++;
      delay(10); // bluetooth stack will go into congestion, if too many packets are sent
  }

    // disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500); // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising(); // restart advertising
        Serial.println("start advertising");
        oldDeviceConnected = deviceConnected;
    }
    // connecting
    if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
        oldDeviceConnected = deviceConnected;
    }
}

uint8_t recv_cmd2[1024];
int cmd2_index;
void serialEvent () {
  while(Serial2.available()) {
    recv_cmd2[cmd2_index] = (uint8_t)Serial2.read();
    if (recv_cmd2[cmd2_index] == 0x0A || recv_cmd2[cmd2_index] == 0x0D) {
      recv_cmd2[cmd2_index] = 0x00;

      pTxCharacteristic->setValue(recv_cmd2, cmd2_index);
      pTxCharacteristic->notify();
      Serial.printf("[U]%s\n", recv_cmd2);

      cmd2_index = 0;
      continue;
    }
    cmd2_index++;
    if (cmd2_index >= 1024)
      cmd2_index = 0;
  }
}

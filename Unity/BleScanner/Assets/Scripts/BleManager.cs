/* This is a simple example to show the steps and one possible way of
 * automatically scanning for and connecting to a device to receive
 * notification data from the device.
 *
 * It works with the esp32 sketch included at the bottom of this source file.
 */

using UnityEngine;
using UnityEngine.UI;
using System.Collections.Generic;
using System.Text;

public class BleDevice
{
    public string name;
    public string addr;
    public BleDevice(string _name, string _addr)
    {
        this.name = _name;
        this.addr = _addr;
    }
}

public class BleManager : Singleton<BleManager>
{
    public string DeviceName = "QNODE";
    public string ServiceUUID = "A9E90000-194C-4523-A473-5FDF36AA4D20";
    public string LedUUID = "A9E90001-194C-4523-A473-5FDF36AA4D20";
    public string BtnUUID = "A9E90002-194C-4523-A473-5FDF36AA4D20";

    enum States
    {
        None,
        SearchBle,
        Scan,
        ScanRSSI,
        Connect,
        Subscribe,
        Unsubscribe,
        Disconnect,
    }

    private bool _connected = false;
    private float _timeout = 0f;
    private States _state = States.None;
    private string _deviceAddress;
    private bool _foundBtnUUID = false;
    private bool _foundLedUUID = false;
    private bool _rssiOnly = false;
    private int _rssi = 0;

    private List<BleDevice> _device = new List<BleDevice>();
    public List<BleDevice> Devices
    {
        get => _device;
    }

    public List<string> _recv_message = new List<string>();
    public string Message
    {
        get
        {
            if (_recv_message.Count > 0)
            {
                string msg = _recv_message[0];
                _recv_message.RemoveAt(0);
                return msg;
            }
            else
            {
                return null;
            }
        }
    }

    public void Search()
    {
        DeviceName = "QNODE";
        _device.Clear();
        BluetoothLEHardwareInterface.Initialize(true, false, () =>
        {
            SetState(States.SearchBle, 0.1f);
        }, (error) =>
        {
            Debug.Log("Error during initialize: " + error);
        });
    }

    public void Connect(string device_name)
    {
        DeviceName = device_name;
        Connect();
    }
    public void Connect()
    {
        StartProcess();
    }

    public void Disconnect()
    {
        Debug.Log("[BleM]Disconnect");
        _Disconnect();
    }

    void _Disconnect()
    {
        if (_connected)
        {
            Debug.Log("[BleM]DisconnectPeripheral");
            BluetoothLEHardwareInterface.UnSubscribeCharacteristic(_deviceAddress, ServiceUUID, BtnUUID, null);
            BluetoothLEHardwareInterface.DisconnectPeripheral(_deviceAddress, (address) =>
            {
                Debug.Log("[BleM]DeInitialize");
                BluetoothLEHardwareInterface.DeInitialize(() =>
                {
                    _connected = false;
                    _state = States.None;
                });
            });
        }
        else
        {
            Debug.Log("[BleM]DeInitialize");
            BluetoothLEHardwareInterface.DeInitialize(() =>
            {
                _state = States.None;
            });
        }
    }

    public bool IsConnectd
    {
        get => _connected;
    }

    public void Send(string message)
    {
        if (IsConnectd)
        {
            byte[] data = Encoding.ASCII.GetBytes(message);
            BluetoothLEHardwareInterface.WriteCharacteristic(_deviceAddress, ServiceUUID, LedUUID, data, data.Length, true, (characteristicUUID) =>
            {
                BluetoothLEHardwareInterface.Log("Write Succeeded");
            });
        }
    }

    public void OnApplicationQuit()
    {
        Debug.Log("[BleM]OnApplicationQuit");
        Disconnect();
    }

    private bool AddNotExist(string name, string address)
    {
        bool _found = false;
        foreach (BleDevice device in _device)
        {
            if (device.addr == address)
            {
                _found = true;
                break;
            }
        }
        if (!_found)
        {
            _device.Add(new BleDevice(name, address));
            return false;
        }
        return true;
    }

    //private string StatusMessage
    //{
    //    set
    //    {
    //        BluetoothLEHardwareInterface.Log(value);
    //        StatusText.text = value;
    //    }
    //}

    //// NOTE: The button that triggers this is for when you are running this code in the macOS
    //// Unity Editor and have the experimental bluetooth support turned on. This is because I
    //// how found it crashes less often when you deinitialize first. You must still use caution
    //// and save your work often as it could still have issues.
    //// MacOS editor support is experimental. I am working on trying to make it work better in
    //// the editor.
    //public void OnDeinitializeButton()
    //{
    //    BluetoothLEHardwareInterface.DeInitialize(() =>
    //    {
    //        StatusMessage = "Deinitialize";
    //    });
    //}

    void Reset()
    {
        _connected = false;
        _timeout = 0f;
        _state = States.None;
        _deviceAddress = null;
        _foundBtnUUID = false;
        _foundLedUUID = false;
        _rssi = 0;
    }

    void SetState(States newState, float timeout)
    {
        _state = newState;
        _timeout = timeout;
        Debug.Log("SetState to " + newState);
    }

    void StartProcess()
    {
        Reset();
        BluetoothLEHardwareInterface.Initialize(true, false, () =>
        {
            SetState(States.Scan, 0.1f);
            Debug.Log("Initialize OK");
        }, (error) =>
        {
            Debug.Log("Error during initialize: " + error);
        });
    }

    // Use this for initialization
    void Start()
    {
        //if (DeinitializeButton != null)
        //{
        //    DeinitializeButton.SetActive(Application.isEditor);
        //}

//        StartProcess();
    }

    // Update is called once per frame
    void Update()
    {
        if (_timeout > 0f)
        {
            _timeout -= Time.deltaTime;
            if (_timeout <= 0f)
            {
                _timeout = 0f;

                switch (_state)
                {
                    case States.None:
                        { 
                        }
                        break;

                    case States.SearchBle:
                        {
                            Debug.Log("Scanning for " + DeviceName);

                            BluetoothLEHardwareInterface.ScanForPeripheralsWithServices(null, (address, name) =>
                            {
                                // if your device does not advertise the rssi and manufacturer specific data
                                // then you must use this callback because the next callback only gets called
                                // if you have manufacturer specific data

                                if (!_rssiOnly)
                                {
                                    if (name.Contains(DeviceName))
                                    {
                                        AddNotExist(name, address);
                                    }
                                }

                            }, (address, name, rssi, bytes) =>
                            {
                                // use this one if the device responses with manufacturer specific data and the rssi
                                if (name.Contains(DeviceName))
                                {
                                    AddNotExist(name, address);
                                    if (_rssiOnly)
                                    {
                                        _rssi = rssi;
                                    }
                                    else
                                    {
                                        BluetoothLEHardwareInterface.StopScan();
                                    }
                                }
                            }, _rssiOnly); // this last setting allows RFduino to send RSSI without having manufacturer data

                            if (_rssiOnly)
                                SetState(States.ScanRSSI, 0.5f);
                            break;
                        }
                    case States.Scan:
                        {
                            Debug.Log("Scanning for " + DeviceName);

                            BluetoothLEHardwareInterface.ScanForPeripheralsWithServices(null, (address, name) =>
                            {

                            // if your device does not advertise the rssi and manufacturer specific data
                            // then you must use this callback because the next callback only gets called
                            // if you have manufacturer specific data

                            if (!_rssiOnly)
                                {
                                    if (name.Contains(DeviceName))
                                    {
                                        BluetoothLEHardwareInterface.StopScan();

                                        // found a device with the name we want
                                        // this example does not deal with finding more than one
                                        _deviceAddress = address;
                                        SetState(States.Connect, 0.5f);
                                    }
                                }

                            }, (address, name, rssi, bytes) =>
                            {

                            // use this one if the device responses with manufacturer specific data and the rssi

                            if (name.Contains(DeviceName))
                                {
                                    if (_rssiOnly)
                                    {
                                        _rssi = rssi;
                                    }
                                    else
                                    {
                                        BluetoothLEHardwareInterface.StopScan();

                                        // found a device with the name we want
                                        // this example does not deal with finding more than one
                                        _deviceAddress = address;
                                        SetState(States.Connect, 0.5f);
                                    }
                                }

                            }, _rssiOnly); // this last setting allows RFduino to send RSSI without having manufacturer data

                            if (_rssiOnly)
                                SetState(States.ScanRSSI, 0.5f);
                            break;
                        }
                    case States.ScanRSSI:
                        {
                            break;
                        }

                    case States.Connect:
                        {
                            Debug.Log( "Connecting...");

                            // set these flags
                            _foundBtnUUID = false;
                            _foundLedUUID = false;

                            // note that the first parameter is the address, not the name. I have not fixed this because
                            // of backwards compatiblity.
                            // also note that I am note using the first 2 callbacks. If you are not looking for specific characteristics you can use one of
                            // the first 2, but keep in mind that the device will enumerate everything and so you will want to have a timeout
                            // large enough that it will be finished enumerating before you try to subscribe or do any other operations.
                            BluetoothLEHardwareInterface.ConnectToPeripheral(_deviceAddress, null, null, (address, serviceUUID, characteristicUUID) =>
                            {
                                if (IsEqual(serviceUUID, ServiceUUID))
                                {
                                    _foundBtnUUID = _foundBtnUUID || IsEqual(characteristicUUID, BtnUUID);
                                    _foundLedUUID = _foundLedUUID || IsEqual(characteristicUUID, LedUUID);

                                    // if we have found both characteristics that we are waiting for
                                    // set the state. make sure there is enough timeout that if the
                                    // device is still enumerating other characteristics it finishes
                                    // before we try to subscribe
                                    if (_foundBtnUUID && _foundLedUUID)
                                    {
                                        Debug.Log("Connected...");

                                        _connected = true;
                                        SetState(States.Subscribe, 2f);
                                    }
                                }
                            });
                            break;
                        }

                    case States.Subscribe:
                        {
                            Debug.Log("[BleM]Subscribe");

                            BluetoothLEHardwareInterface.SubscribeCharacteristicWithDeviceAddress(_deviceAddress, ServiceUUID, BtnUUID, (notifyAddress, notifyCharacteristic) =>
                            {
                                _state = States.None;

                                Debug.Log("Subscribing to characteristics...");

                                // read the initial state of the button
                                Debug.Log("notifyAddress:" + notifyAddress);
                                Debug.Log("notifyCharacteristic:" + notifyCharacteristic);
                                Debug.Log("_deviceAddress:" + _deviceAddress);
                                Debug.Log("ServiceUUID:" + ServiceUUID);
                                Debug.Log("BtnUUID:" + BtnUUID);
                                BluetoothLEHardwareInterface.ReadCharacteristic(_deviceAddress, ServiceUUID, BtnUUID, (characteristic, bytes) =>
                                {
                                    string msg = Encoding.Default.GetString(bytes);
                                    Debug.Log("Recv:" + msg);
                                    _recv_message.Add(msg);
                                    //if (_recv_message.Count > 1000)
                                    //{
                                    //    _recv_message.Clear();
                                    //}
                                    //ProcessButton(bytes);
                                });
                            }, (address, characteristicUUID, bytes) =>
                            {
                                if (_state != States.None)
                                {
                                // some devices do not properly send the notification state change which calls
                                // the lambda just above this one so in those cases we don't have a great way to
                                // set the state other than waiting until we actually got some data back.
                                // The esp32 sends the notification above, but if yuor device doesn't you would have
                                // to send data like pressing the button on the esp32 as the sketch for this demo
                                // would then send data to trigger this.
                                    _state = States.None;
                                }

                                //string msg = Encoding.Default.GetString(bytes);
                                //Debug.Log("Recv2:" + msg);
                                //_recv_message.Add(msg);
                                //if (_recv_message.Count > 1000)
                                //{
                                //    _recv_message.Clear();
                                //}
                                // we received some data from the device
                                //ProcessButton(bytes);
                            });
                            break;
                        }

                    case States.Unsubscribe:
                        {
                            Debug.Log("Unsubscribe for " + DeviceName);

                            BluetoothLEHardwareInterface.UnSubscribeCharacteristic(_deviceAddress, ServiceUUID, BtnUUID, null);
                            SetState(States.Disconnect, 4f);
                            break;
                        }

                    case States.Disconnect:
                        {
                            Debug.Log("Commanded disconnect.");

                            _Disconnect();

                            break;
                        }
                }
            }
        }
    }

    //string FullUUID(string uuid)
    //{
    //    string fullUUID = uuid;
    //    if (fullUUID.Length == 4)
    //        fullUUID = "0000" + uuid + "-0000-1000-8000-00805f9b34fb";

    //    return fullUUID;
    //}

    bool IsEqual(string uuid1, string uuid2)
    {
        //if (uuid1.Length == 4)
        //    uuid1 = FullUUID(uuid1);
        //if (uuid2.Length == 4)
            //uuid2 = FullUUID(uuid2);

        return (uuid1.ToUpper().Equals(uuid2.ToUpper()));
    }

    void SendByte(byte value)
    {
        byte[] data = { value };
        BluetoothLEHardwareInterface.WriteCharacteristic(_deviceAddress, ServiceUUID, LedUUID, data, data.Length, true, (characteristicUUID) =>
        {
            BluetoothLEHardwareInterface.Log("Write Succeeded");
        });
    }
}

/*

COPY FROM BELOW THIS LINE >>>
// This sketch is a companion to the Bluetooth LE for iOS, tvOS and Android plugin for Unity3D.
// It is the hardware side of the StartingExample.

// The URL to the asset on the asset store is:
// https://assetstore.unity.com/packages/tools/network/bluetooth-le-for-ios-tvos-and-android-26661

// This sketch simply advertises as ledbtn and has a single service with 2 characteristics.
// The ledUUID characteristic is used to turn the LED on and off. Writing 0 turns it off and 1 turns it on.
// The BtnUUID characteristic can be read or subscribe to. When the button is down the characteristic
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
static BLEUUID BtnUUID("A9E90002-194C-4523-A473-5FDF36AA4D20");

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
    uint8_t* data = pCharacteristic->getData();
    int len = pCharacteristic->getValue().empty() ? 0 : pCharacteristic->getValue().length();

    if (len > 0)
    {
        // if the first byte is 0x01 / on / true
        if (data[0] == 0x01)
            digitalWrite(led, HIGH);
        else
            digitalWrite(led, LOW);
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

void setup()
{
    Serial.begin(115200);

    // led turned on/off from the iPhone app
    pinMode(led, OUTPUT);

    // button press will be shown on the iPhone app)
    pinMode(button, INPUT);

    BLEDevice::init("ledbtn");
    // BLEDevice::setCustomGattsHandler(my_gatts_event_handler);
    // BLEDevice::setCustomGattcHandler(my_gattc_event_handler);

    pServer = BLEDevice::createServer();
    BLEService* pService = pServer->createService(serviceUUID);
    pServer->setCallbacks(new BTServerCallbacks());

    pCharacteristicCommand = pService->createCharacteristic(
        BtnUUID,
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
    }
}

<<< COPY TO ABOVE THIS LINE
*/

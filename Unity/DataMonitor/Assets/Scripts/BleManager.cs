using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class BleDevice
{
    public string name;
    public string addr;
}

public class BleManager : Singleton<BleManager> {

    public string DeviceName = "OAS_UART";
    public string ServiceUUID = "6e400001-b5a3-f393-e0a9-e50e24dcca9e";
    public string SubscribeCharacteristic = "6e400003-b5a3-f393-e0a9-e50e24dcca9e";
    public string WriteCharacteristic = "6e400002-b5a3-f393-e0a9-e50e24dcca9e";

    enum States
    {
        None,
        Scan,
        ScanRSSI,
        Connect,
        Subscribe,
        Unsubscribe,
        Disconnect,
    }

    List<BleDevice> _bleDevices = new List<BleDevice>();

    private bool _connected = false;
    private float _timeout = 0f;
    private States _state = States.None;
    private string _deviceAddress;
    private bool _foundSubscribeID = false;
    private bool _foundWriteID = false;
    private byte[] _dataBytes = null;
    private bool _rssiOnly = false;
    private int _rssi = 0;

    void Reset()
    {
        _connected = false;
        _timeout = 0f;
        _state = States.None;
        _deviceAddress = null;
        _foundSubscribeID = false;
        _foundWriteID = false;
        _dataBytes = null;
        _rssi = 0;
        _bleDevices.Clear();
    }

    void SetState(States newState, float timeout)
    {
        _state = newState;
        _timeout = timeout;
    }

    public string GetDeviceAddress()
    {
        return _deviceAddress;
    }

    public List<BleDevice> GetFoundDevice()
    {
        return _bleDevices;
    }

    public void Scan()
    {
        Reset();
        BluetoothLEHardwareInterface.Initialize(true, false, () => {

            SetState(States.Scan, 0.1f);

        }, (error) => {

            BluetoothLEHardwareInterface.Log("Error during initialize: " + error);
        });
    }

    public bool IsConnected()
    {
        return _connected;
    }

    public int AvailableData()
    {
        if (IsConnected())
        {
            if (_dataBytes != null)
            {
                return _dataBytes.Length;
            }
        }
        return 0;
    }

    public byte[] GetData()
    {
        byte[] tmp = _dataBytes;
        _dataBytes = null;
        return tmp;
    }

    public void Connect(string address)
    {
        if (!_connected)
        {
            _deviceAddress = address;
            SetState(States.Connect, 0.5f);
        }
        else
        {
            SetState(States.Unsubscribe, 0.5f);
        }
    }

    public void Send(string message)
    {
        byte[] data = System.Text.Encoding.UTF8.GetBytes(message);
        BluetoothLEHardwareInterface.WriteCharacteristic(_deviceAddress, ServiceUUID, WriteCharacteristic, data, data.Length, true, (characteristicUUID) => {
            BluetoothLEHardwareInterface.Log("Write Succeeded");
        });
    }

    public string GetMessage()
    {
        return "";
    }

    // Use this for initialization
    void Start()
    {
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
                        break;

                    case States.Scan:
                        BluetoothLEHardwareInterface.ScanForPeripheralsWithServices(null, (address, name) => {

                            // if your device does not advertise the rssi and manufacturer specific data
                            // then you must use this callback because the next callback only gets called
                            // if you have manufacturer specific data

                            if (!_rssiOnly)
                            {
                                if (name.Contains(DeviceName))
                                {
                                    BluetoothLEHardwareInterface.StopScan();

                                    BleDevice device = new BleDevice();
                                    device.name = name;
                                    device.addr = address;
                                    _bleDevices.Add(device);

                                    // found a device with the name we want
                                    // this example does not deal with finding more than one
                                    _deviceAddress = address;
                                }
                            }

                        }, (address, name, rssi, bytes) => {

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

                    case States.ScanRSSI:
                        break;

                    case States.Connect:
                        // set these flags
                        _foundSubscribeID = false;
                        _foundWriteID = false;

                        // note that the first parameter is the address, not the name. I have not fixed this because
                        // of backwards compatiblity.
                        // also note that I am note using the first 2 callbacks. If you are not looking for specific characteristics you can use one of
                        // the first 2, but keep in mind that the device will enumerate everything and so you will want to have a timeout
                        // large enough that it will be finished enumerating before you try to subscribe or do any other operations.
                        BluetoothLEHardwareInterface.ConnectToPeripheral(_deviceAddress, null, null, (address, serviceUUID, characteristicUUID) => {

                            if (string.Equals(serviceUUID, ServiceUUID, System.StringComparison.OrdinalIgnoreCase))
                            {
                                _foundSubscribeID = _foundSubscribeID || (string.Equals(characteristicUUID, SubscribeCharacteristic, System.StringComparison.OrdinalIgnoreCase));
                                _foundWriteID = _foundWriteID || (string.Equals(characteristicUUID, WriteCharacteristic, System.StringComparison.OrdinalIgnoreCase));

                                // if we have found both characteristics that we are waiting for
                                // set the state. make sure there is enough timeout that if the
                                // device is still enumerating other characteristics it finishes
                                // before we try to subscribe
                                if (_foundSubscribeID && _foundWriteID)
                                {
                                    _connected = true;
                                    SetState(States.Subscribe, 2f);
                                }
                            }
                        });
                        break;

                    case States.Subscribe:
                        BluetoothLEHardwareInterface.SubscribeCharacteristicWithDeviceAddress(_deviceAddress, ServiceUUID, SubscribeCharacteristic, null, (address, characteristicUUID, bytes) => {

                            // we don't have a great way to set the state other than waiting until we actually got
                            // some data back. For this demo with the rfduino that means pressing the button
                            // on the rfduino at least once before the GUI will update.
                            _state = States.None;

                            // we received some data from the device
                            _dataBytes = bytes;
                        });
                        break;

                    case States.Unsubscribe:
                        BluetoothLEHardwareInterface.UnSubscribeCharacteristic(_deviceAddress, ServiceUUID, SubscribeCharacteristic, null);
                        SetState(States.Disconnect, 4f);
                        break;

                    case States.Disconnect:
                        if (_connected)
                        {
                            BluetoothLEHardwareInterface.DisconnectPeripheral(_deviceAddress, (address) => {
                                BluetoothLEHardwareInterface.DeInitialize(() => {

                                    _connected = false;
                                    _state = States.None;
                                });
                            });
                        }
                        else
                        {
                            BluetoothLEHardwareInterface.DeInitialize(() => {

                                _state = States.None;
                            });
                        }
                        break;
                }
            }
        }
    }

}

using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class Program_Qnode : MonoBehaviour
{

    public Dropdown dropDown_DeviceList;
    public Button button_Ble;
    public InputField inputField_Message;

    public void ScanBle()
    {
        dropDown_DeviceList.ClearOptions();
        BleManager.Instance.Search();
    }

    public void Connect()
    {
        int index = dropDown_DeviceList.value;
        string device_name = dropDown_DeviceList.options[index].text;
        Debug.Log("Connect to " + device_name);
        BleManager.Instance.Connect(device_name);
    }

    public void Send()
    {
        BleManager.Instance.Send("hello");
    }

    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        ColorBlock colorBlock = button_Ble.colors;
        if (BleManager.Instance.IsConnectd)
        {
            colorBlock.normalColor = Color.blue;
        }
        else
        {
            colorBlock.normalColor = Color.gray;
        }
        button_Ble.colors = colorBlock;

        List<BleDevice> bleDevice = BleManager.Instance.Devices;
        if (bleDevice.Count > 0)
        {
            foreach (BleDevice device in bleDevice)
            {
                bool _found = false;
                int nDropdown = dropDown_DeviceList.options.Count;
                foreach (Dropdown.OptionData od in dropDown_DeviceList.options)
                {
                    if (device.name == od.text)
                    {
                        _found = true;
                    }
                }
                if (!_found)
                {
                    Dropdown.OptionData od = new Dropdown.OptionData();
                    od.text = device.name;
                    dropDown_DeviceList.options.Add(od);
                    dropDown_DeviceList.RefreshShownValue();
                }
            }
        }

        string message = BleManager.Instance.Message;
        if (message != null)
        {
            inputField_Message.text += message;
        }
    }
}

using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class Program_Qnode : MonoBehaviour
{

    public Dropdown dropDown_DeviceList;
    public Button button_Ble;
    public Text text_Message;
    public Text text_Decode;
    public GameObject panelBle;
    public InputField inputfield_AP;
    public InputField inputfield_PW;

    float message_time_counter = 0;
    float message_period = 0.5f;
    bool req_message_f = false;

    const string REQ_COMMAND = "@01:EX;VX;MST;CURQA;CURQD;CURDD;DIN;DOUT\n";

    Dictionary<string, string> slot = new Dictionary<string, string>();

    public void ScanBle()
    {
        if (!BleManager.Instance.IsConnectd)
        {
            dropDown_DeviceList.ClearOptions();
            BleManager.Instance.Search();
            panelBle.SetActive(true);
        }
        else
        {
            BleManager.Instance.Disconnect();
        }
    }

    int count;
    public void AddMessage()
    {
        string message = "{\"CURQA\":" + UnityEngine.Random.Range(-1.0f, 1.0f) + ", COUNT:" + count + ", HELLO THIS IS TEST NOW RUNNING}\r\n";
        count = count + 1;
        BleManager.Instance.AddMessageTest(message);
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

    public void Decode(string message)
    {
        Debug.Log("message origin: " + message);
        string[] removeCharacters = new string[] { "\"", "{", "}" };
        foreach (string removeChar in removeCharacters)
        {
            message = message.Replace(removeChar, string.Empty);
        }
        //message = message.Trim(new char[] { ' ', '\"', '{', '}' });
        Debug.Log("message trim  : " + message);

        string[] separator_level1 = new string[] { ",", ";" };
        string[] separator_level2 = new string[] { ":", "=" };
        string[] vars = message.Split(separator_level1, 100, System.StringSplitOptions.RemoveEmptyEntries);
        foreach (string v in vars)
        {
            Debug.Log(v);
            string[] pairs = v.Trim().Split(separator_level2, 2, System.StringSplitOptions.RemoveEmptyEntries);
            if (pairs.Length == 2)
            {
                slot[pairs[0].Trim()] = pairs[1].Trim();
            }
        }
        foreach (KeyValuePair<string, string> kvp in slot)
        {
            Debug.Log("Key=" + kvp.Key + ", Value=" + kvp.Value);
        }
    }

    string stack_string;
    public string StackUntilLine(string msg)
    {
        stack_string += msg;
        int newLine = stack_string.IndexOf(Environment.NewLine);
        Debug.Log("newLine: " + newLine);
        if (newLine > 0)
        {
            string tmpString = stack_string;
            stack_string = "";
            return tmpString;
        }
        return "";
    }

    public void ClearMessage()
    {
      text_Message.text = "";
      text_Decode.text = "";
    }

    public void ReadSetting()
    {
        StartCoroutine(Setting(false));
    }

    public void WriteSetting()
    {
        StartCoroutine(Setting(true));
    }

    private IEnumerator Setting(bool write=false)
    {
        string command = "";
        command = "ap" + ((write) ? "=" + inputfield_AP.text : "");
        BleManager.Instance.Send(command + "");
        yield return new WaitForSeconds(0.5f);
        //BleManager.Instance.AddMessageTest("ap=oas\n");

        command = "pw" + ((write) ? "=" + inputfield_PW.text : "");
        BleManager.Instance.Send(command + "");
        yield return new WaitForSeconds(0.5f);
        //BleManager.Instance.AddMessageTest("pw=oas12345\r\n");

        BleManager.Instance.Send("ip=121.137.95.17");
        yield return new WaitForSeconds(0.5f);
        //BleManager.Instance.AddMessageTest("ip=111.222.333.444\r\n");

        if (write)
        {
//            BleManager.Instance.Send("re");
        }
    }

    void SendReqCommand()
    {
        message_time_counter = 0;
        req_message_f = false;
        BleManager.Instance.Send(REQ_COMMAND);
    }

    // Start is called before the first frame update
    void Start()
    {
        string message = "{\"CURQA\":-0.1234, COUNT:100, HELLO THIS IS TEST NOW RUNNING}\r\n";
        Decode(message);
    }

    // Update is called once per frame
    void Update()
    {
        // BLE ICON
        if (BleManager.Instance.IsConnectd)
        {
            button_Ble.GetComponentInChildren<Image>().color = Color.blue;
        }
        else
        {
            button_Ble.GetComponentInChildren<Image>().color = Color.black;
        }

        // Show BLE list
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

        message_time_counter += Time.deltaTime;
        if (message_time_counter > message_period)
        {
            message_time_counter = 0;
            req_message_f = true;
            //Debug.Log("Timer Tick");
        }

        // Process Messages
        string message = BleManager.Instance.Message;
        if (message != null)
        {
            text_Message.text += message;
            string message_line = StackUntilLine(message);
            if (message_line.Length > 0)
            {
                Decode(message_line);
                SendReqCommand();
                text_Decode.text = "";
                foreach (KeyValuePair<string, string> kvp in slot)
                {
                    text_Decode.text += kvp.Key + "=" + kvp.Value + Environment.NewLine;
                    //Debug.Log("Key=" + kvp.Key + ", Value=" + kvp.Value);
                    if (kvp.Key.ToUpper() == "AP")
                        inputfield_AP.text = kvp.Value;
                    else if (kvp.Key.ToUpper() == "PW")
                        inputfield_PW.text = kvp.Value;
                }
            }
        }
        else
        {
            if (BleManager.Instance.IsConnectd)
            {
                if (req_message_f)
                {
                    SendReqCommand();
                }
            }
        }
    }
}

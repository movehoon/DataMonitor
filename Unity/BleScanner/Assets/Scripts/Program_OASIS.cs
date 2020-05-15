using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class Program_OASIS : MonoBehaviour
{

    public Dropdown dropDown_DeviceList;
    public Button button_Ble;
    public Text text_Message;
    public Text text_Decode;
    public GameObject panelBle;
    public InputField inputfield_AP;
    public InputField inputfield_PW;

    //Dictionary<string, string> slot = new Dictionary<string, string>();

    List<Slot> slots = new List<Slot>();

    public void ScanBle()
    {
        if (!BleManager.Instance.IsConnectd)
        {
            dropDown_DeviceList.ClearOptions();
            BleManager.Instance.Search("OAS");
            panelBle.SetActive(true);
        }
        else
        {
            BleManager.Instance.Disconnect();
        }
    }

    public void ClearInformation()
    {
        slots.Clear();
        text_Message.text = "";
        text_Decode.text = "";
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
                bool _found = false;
                foreach (Slot s in slots)
                {
                    if (s.Key == pairs[0].Trim())
                    {
                        _found = true;
                        s.Value = pairs[1].Trim();
                        s.count++;
                        TimeSpan timeSpan = System.DateTime.Now - s.time;
                        s.duration = (int)timeSpan.TotalMilliseconds;
                        s.time = System.DateTime.Now;
                        break;
                    }
                }
                if (!_found)
                {
                    Slot s = new Slot();
                    s.Key = pairs[0].Trim();
                    s.Value = pairs[1].Trim();
                    s.count = 1;
                    s.duration = 0;
                    s.time = System.DateTime.Now;
                    slots.Add(s);
                }
            }

        }
    }

    string stack_string;
    public string StackUntilLine(string msg)
    {
        stack_string += msg;
        int newLine = stack_string.IndexOf(Environment.NewLine);
        if (newLine > 0)
        {
            Debug.Log("newLine: " + newLine);
            string tmpString = stack_string;
            stack_string = "";
            return tmpString;
        }
        return "";
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

    // Start is called before the first frame update
    void Start()
    {
        //string message = "{\"CURQA\":-0.1234, COUNT:100, HELLO THIS IS TEST NOW RUNNING}\r\n";
        //Decode(message);
    }

    // Update is called once per frame
    void Update()
    {
        if (BleManager.Instance.IsConnectd)
        {
            button_Ble.GetComponentInChildren<Image>().color = Color.blue;
        }
        else
        {
            button_Ble.GetComponentInChildren<Image>().color = Color.black;
        }

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
            text_Message.text += message;
            string message_line = StackUntilLine(message);
            if (message_line.Length > 0)
            {
                Decode(message_line);
                text_Decode.text = "";
                foreach (Slot s in slots)
                {
                    text_Decode.text += s.Key + "=" + s.Value + "   (" + s.duration.ToString() + "ms, " + s.count.ToString() + ")" + Environment.NewLine;
                }
            }
        }
    }
}

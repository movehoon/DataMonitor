using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class Program : MonoBehaviour
{
    public Text StatusText;

    private int count;
    public void Send()
    {
        count++;
        BleManager.Instance.Send("Bluetooth is a wireless technology standard used for exchanging data between fixed and mobile devices over short distances using short-wavelength UHF radio waves in the industrial, scientific and medical radio bands, from 2.400 to 2.485 GHz, and building personal area networks (PANs). It was originally conceived as a wireless alternative to RS-232 data cables." + count);
    }
    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
//        Debug.Log("Count is " + BleManager.Instance.Devices.Count);
        foreach (BleDevice device in BleManager.Instance.Devices)
        {
            Debug.Log("Name: " + device.name + ", Addr: " + device.addr);
        }

        if (BleManager.Instance.IsConnectd)
        {
            string msg = BleManager.Instance.Message;
            if (msg != null)
            {
                StatusText.text += msg + "\r\n";
            }
        }
    }

}

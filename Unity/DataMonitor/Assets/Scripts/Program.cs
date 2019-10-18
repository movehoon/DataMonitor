using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;

public class Program : MonoBehaviour {

    public Button btnConnect;
    public InputField inputDevice;
    public InputField inputLog;

    public void ScanDevice()
    {
        BleManager.Instance.Scan();
    }

    public void ConnectDevice()
    {
        BleManager.Instance.Connect(inputDevice.text);
    }

    public void ApplySetting()
    {
        StartCoroutine(Setting());
    }

    private IEnumerator Setting()
    {
        //BleManager.Instance.Send("ap=" + inputAp.text);
        //yield return new WaitForSeconds(0.1f);
        //BleManager.Instance.Send("pw=" + inputPw.text);
        //yield return new WaitForSeconds(0.1f);
        //BleManager.Instance.Send("mo=0");
        //yield return new WaitForSeconds(0.1f);
        //BleManager.Instance.Send("si=" + dropdownGeo.options[dropdownGeo.value].text);
        yield return new WaitForSeconds(0.1f);
        //BleManager.Instance.Send("re");
    }

    // Use this for initialization
    void Start () {
		
	}
	
	// Update is called once per frame
	void Update () {
        inputDevice.text = BleManager.Instance.GetDeviceAddress();

        if (BleManager.Instance.IsConnected())
        {
            btnConnect.GetComponentInChildren<Text>().text = "Disconnect";
  //          btnApply.interactable = true;
            if (BleManager.Instance.AvailableData() > 0)
            {
                inputLog.text += Environment.NewLine + System.Text.Encoding.UTF8.GetString(BleManager.Instance.GetData());
            }
        }
        else
        {
            btnConnect.GetComponentInChildren<Text>().text = "Connect";
//            btnApply.interactable = false;
        }
    }
}

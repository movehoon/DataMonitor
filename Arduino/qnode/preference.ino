#include <Preferences.h>

#define PREFS_NAME "saved_data"
#define PREFS_MODE "MD"
#define PREFS_AP "AP"
#define PREFS_AP_1 "AP_1"
#define PREFS_PW "PW"
#define PREFS_PW_1 "PW_1"
#define PREFS_IP "IP"
#define PREFS_IP_1 "IP_1"
#define PREFS_ID  "ID"
#define PREFS_ID_1 "ID_1"

#define PREFS_DEFAULT_MODE "BT"
#define PREFS_DEFAULT_AP ""
#define PREFS_DEFAULT_PW ""
#define PREFS_DEFAULT_IP "121.137.95.17"
#define PREFS_DEFAULT_ID ""

Preferences prefs;

String md;
String ap;
String pw;
String ip;
String id;


void LoadPreference() {
  prefs.begin(PREFS_NAME);
//  md = prefs.getString(PREFS_MODE, PREFS_DEFAULT_md);
//  ap = prefs.getString(PREFS_AP, PREFS_DEFAULT_AP);
//  pw = prefs.getString(PREFS_PW, PREFS_DEFAULT_PW);
//  ip = prefs.getString(PREFS_IP, PREFS_DEFAULT_IP);
}

const char *GetPreferenceMode() {
  md = prefs.getString(PREFS_MODE, PREFS_DEFAULT_MODE);
  return md.c_str();
}

const char *GetPreferenceAP() {
  ap = prefs.getString(PREFS_AP, PREFS_DEFAULT_AP);
  if (ap.length() >= 8) {
    ap += prefs.getString(PREFS_AP_1, "");
  }
  return ap.c_str();
}

const char *GetPreferencePW() {
  pw = prefs.getString(PREFS_PW, PREFS_DEFAULT_PW);
  if (pw.length() >= 8) {
    pw += prefs.getString(PREFS_PW_1, "");
  }
  return pw.c_str();
}

const char *GetPreferenceIP() {
  ip = prefs.getString(PREFS_IP, PREFS_DEFAULT_IP);
  if (ip.length() >= 8) {
    ip += prefs.getString(PREFS_IP_1, "");
  }
  return ip.c_str();
}

const char *GetPreferenceID() {
  id = prefs.getString(PREFS_ID, PREFS_DEFAULT_ID);
  if (id.length() >= 8) {
    id += prefs.getString(PREFS_ID_1, "");
  }
  return id.c_str();
}

//String GetPreference(String key) {
//  if (key == PREFS_AP) {
//    ap = prefs.getString(PREFS_AP, PREFS_DEFAULT_AP).c_str();
//    return ap.c_str();
//  }
//  else if (key == PREFS_PW) {
//    pw = prefs.getString(PREFS_PW, PREFS_DEFAULT_PW).c_str();
//    return String(pw.c_str());
//  }
//  else if (key == PREFS_IP) {
//    ip = prefs.getString(PREFS_IP, PREFS_DEFAULT_IP).c_str();
//    printf("ip:%s\n", ip);
//    return String(ip.c_str());
//  }
//  return "";
//}

void SavePreferenceMode(String value) {
  prefs.putString(PREFS_MODE, value);
}
void SavePreferenceAP(String value) {
  prefs.putString(PREFS_AP, value.substring(0, 8));
  if (value.length() > 8) {
    prefs.putString(PREFS_AP_1, value.substring(8, 16));
  }
  else {
    prefs.putString(PREFS_AP_1, "");
  }
}
void SavePreferencePW(String value) {
  prefs.putString(PREFS_PW, value.substring(0, 8));
  if (value.length() > 8) {
    prefs.putString(PREFS_PW_1, value.substring(8, 16));
  }
  else {
    prefs.putString(PREFS_PW_1, "");
  }
}
void SavePreferenceIP(String value) {
  prefs.putString(PREFS_IP, value.substring(0, 8));
  if (value.length() > 8) {
    prefs.putString(PREFS_IP_1, value.substring(8, 16));
  }
  else {
    prefs.putString(PREFS_IP_1, "");
  }
}
void SavePreferenceID(String value) {
  prefs.putString(PREFS_ID, value.substring(0, 8));
  if (value.length() > 8) {
    prefs.putString(PREFS_ID_1, value.substring(8, 16));
  }
  else {
    prefs.putString(PREFS_ID_1, "");
  }
}

//void SavePreference(String key, String value) {
//  if (key == PREFS_AP)
//    prefs.putString(PREFS_AP, value);
//  else if (key == PREFS_PW)
//    prefs.putString(PREFS_PW, value);
//  else if (key == PREFS_IP)
//    prefs.putString(PREFS_IP, value);
//}

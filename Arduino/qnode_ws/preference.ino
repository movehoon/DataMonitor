#include <Preferences.h>

#define PREFS_NAME "saved_data"
#define PREFS_ID  "ID"

#define PREFS_DEFAULT_ID "02"

Preferences prefs;

String id;

void LoadPreference() {
  prefs.begin(PREFS_NAME);
}

const char *GetPreferenceID() {
  id = prefs.getString(PREFS_ID,PREFS_DEFAULT_ID);
  return id.c_str();
}

void SavePreferenceID(String value) {
  prefs.putString(PREFS_ID, value);
}

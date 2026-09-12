#include "storage.h"
#include "config.h"
#include <Preferences.h>

static Preferences prefs;

bool storageLoad(AppSettings& s) {
  if (!prefs.begin(NVS_NAMESPACE, true)) {
    s = AppSettings{};
    return false;
  }
  s.wifiSsid = prefs.getString("ssid", "");
  s.wifiPass = prefs.getString("pass", "");
  s.xrplAddress = prefs.getString("addr", "");
  s.token = prefs.getString("token", "XRP");
  s.currency = prefs.getString("fiat", "GBP");
  s.issuer = prefs.getString("issuer", "");
  s.configured = prefs.getBool("ok", false);
  prefs.end();
  s.token.toUpperCase();
  s.currency.toUpperCase();
  return true;
}

bool storageSave(const AppSettings& s) {
  if (!prefs.begin(NVS_NAMESPACE, false)) return false;
  prefs.putString("ssid", s.wifiSsid);
  prefs.putString("pass", s.wifiPass);
  prefs.putString("addr", s.xrplAddress);
  prefs.putString("token", s.token);
  prefs.putString("fiat", s.currency);
  prefs.putString("issuer", s.issuer);
  prefs.putBool("ok", settingsReady(s));
  prefs.end();
  return true;
}

void storageResetAll() {
  if (!prefs.begin(NVS_NAMESPACE, false)) return;
  prefs.clear();
  prefs.end();
}

bool settingsReady(const AppSettings& s) {
  return s.wifiSsid.length() > 0 &&
         s.wifiPass.length() > 0 &&
         s.xrplAddress.length() >= 25 &&
         s.token.length() > 0 &&
         s.currency.length() == 3;
}

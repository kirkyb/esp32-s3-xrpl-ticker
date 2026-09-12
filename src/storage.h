#pragma once
#include <Arduino.h>

struct AppSettings {
  String wifiSsid;
  String wifiPass;
  String xrplAddress;
  String token;       // XRP, CSC, XAH, or other ticker
  String currency;    // GBP, USD, EUR
  String issuer;      // IOU issuer. Empty for native XRP / XAH
  bool configured;
};

bool storageLoad(AppSettings& s);
bool storageSave(const AppSettings& s);
void storageResetAll();
bool settingsReady(const AppSettings& s);

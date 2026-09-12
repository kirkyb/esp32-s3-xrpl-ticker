#include "ble_setup.h"
#include "config.h"
#include "storage.h"
#include "tokens.h"
#include <NimBLEDevice.h>

static AppSettings* gSettings = nullptr;
static bool rebootReq = false;
static bool resetReq = false;
static bool changed = false;
static NimBLECharacteristic* rxChar = nullptr;
static NimBLECharacteristic* txChar = nullptr;
static String rxBuf;

static const char* NUS_SVC = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E";
static const char* NUS_RX  = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E";
static const char* NUS_TX  = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E";

static void bleSend(const String& s) {
  if (!txChar) return;
  String line = s + "\n";
  const int chunk = 180;
  for (int i = 0; i < (int)line.length(); i += chunk) {
    String part = line.substring(i, i + chunk);
    txChar->setValue((uint8_t*)part.c_str(), part.length());
    txChar->notify();
    delay(20);
  }
}

static String dumpSettings(const AppSettings& s) {
  String out;
  out += "SSID=" + s.wifiSsid + "\n";
  out += "PASS=" + String(s.wifiPass.length() ? "****" : "(empty)") + "\n";
  out += "ADDR=" + s.xrplAddress + "\n";
  out += "TOKEN=" + s.token + "\n";
  out += "FIAT=" + s.currency + "\n";
  out += "ISSUER=" + s.issuer + "\n";
  out += "READY=" + String(settingsReady(s) ? "YES" : "NO") + "\n";
  return out;
}

static const char* HELP =
  "XRPL Ticker commands\n"
  "HELP\n"
  "GET\n"
  "SET WIFI <ssid>|<password>\n"
  "SET ADDR <r-address>\n"
  "SET TOKEN <XRP|CSC|XAH|...>\n"
  "SET FIAT <GBP|USD|EUR>\n"
  "SET ISSUER <issuer or NONE>\n"
  "SAVE\n"
  "RESET ALL\n"
  "REBOOT\n";

static void handleLine(String line) {
  line.trim();
  if (line.length() == 0) return;
  String up = line;
  up.toUpperCase();

  if (up == "HELP" || up == "?") {
    bleSend(HELP);
    return;
  }
  if (up == "GET" || up == "STATUS") {
    bleSend(dumpSettings(*gSettings));
    return;
  }
  if (up == "SAVE") {
    gSettings->configured = settingsReady(*gSettings);
    storageSave(*gSettings);
    changed = true;
    bleSend(gSettings->configured ? "SAVED READY=YES" : "SAVED READY=NO (need WIFI ADDR TOKEN FIAT)");
    return;
  }
  if (up == "RESET ALL" || up == "RESETALL" || up == "FACTORY") {
    storageResetAll();
    *gSettings = AppSettings{};
    gSettings->token = "XRP";
    gSettings->currency = "GBP";
    resetReq = true;
    changed = true;
    bleSend("RESET ALL. Reboot to start setup again.");
    return;
  }
  if (up == "REBOOT") {
    bleSend("Rebooting...");
    rebootReq = true;
    return;
  }

  if (up.startsWith("SET WIFI ")) {
    String rest = line.substring(9);
    rest.trim();
    int bar = rest.indexOf('|');
    if (bar < 0) {
      bleSend("ERR use SET WIFI ssid|password");
      return;
    }
    gSettings->wifiSsid = rest.substring(0, bar);
    gSettings->wifiPass = rest.substring(bar + 1);
    gSettings->wifiSsid.trim();
    gSettings->wifiPass.trim();
    storageSave(*gSettings);
    changed = true;
    bleSend("WIFI set to " + gSettings->wifiSsid);
    return;
  }
  if (up.startsWith("SET ADDR ")) {
    String a = line.substring(9);
    a.trim();
    if (a.length() < 25 || a[0] != 'r') {
      bleSend("ERR address should start with r");
      return;
    }
    gSettings->xrplAddress = a;
    storageSave(*gSettings);
    changed = true;
    bleSend("ADDR set");
    return;
  }
  if (up.startsWith("SET TOKEN ")) {
    String t = line.substring(10);
    t.trim();
    t.toUpperCase();
    gSettings->token = t;
    const KnownToken* k = findKnownToken(t);
    if (k && gSettings->issuer.length() == 0) gSettings->issuer = k->issuer;
    storageSave(*gSettings);
    changed = true;
    String msg = "TOKEN=" + t;
    if (k) {
      msg += k->xahau ? " (Xahau)" : " (XRPL)";
      if (strlen(k->issuer)) msg += String(" issuer ") + k->issuer;
    } else {
      msg += " (unknown, set ISSUER if this is an IOU)";
    }
    bleSend(msg);
    return;
  }
  if (up.startsWith("SET FIAT ") || up.startsWith("SET CURRENCY ")) {
    int sp = line.indexOf(' ', 4);
    String c = line.substring(sp + 1);
    c.trim();
    c.toUpperCase();
    if (c.length() != 3) {
      bleSend("ERR use GBP USD EUR etc");
      return;
    }
    gSettings->currency = c;
    storageSave(*gSettings);
    changed = true;
    bleSend("FIAT=" + c);
    return;
  }
  if (up.startsWith("SET ISSUER ")) {
    String i = line.substring(11);
    i.trim();
    if (i.equalsIgnoreCase("NONE") || i.equalsIgnoreCase("CLEAR")) i = "";
    gSettings->issuer = i;
    storageSave(*gSettings);
    changed = true;
    bleSend(i.length() ? ("ISSUER=" + i) : "ISSUER cleared");
    return;
  }

  bleSend("ERR unknown command. HELP");
}

class RxCallbacks : public NimBLECharacteristicCallbacks {
  void onWrite(NimBLECharacteristic* c) override {
    std::string v = c->getValue();
    for (char ch : v) {
      if (ch == '\n' || ch == '\r') {
        if (rxBuf.length()) {
          handleLine(rxBuf);
          rxBuf = "";
        }
      } else {
        rxBuf += ch;
        if (rxBuf.length() > 240) rxBuf = "";
      }
    }
  }
};

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer*) override {
    Serial.println("BLE connected");
    delay(150);
    bleSend("XRPL Ticker ready. Type HELP");
    bleSend(dumpSettings(*gSettings));
  }
  void onDisconnect(NimBLEServer*) override {
    Serial.println("BLE disconnected");
    NimBLEDevice::startAdvertising();
  }
};

void bleBegin(AppSettings& settings) {
  gSettings = &settings;
  NimBLEDevice::init(BLE_NAME);
  NimBLEDevice::setMTU(185);
  NimBLEServer* server = NimBLEDevice::createServer();
  server->setCallbacks(new ServerCallbacks());
  NimBLEService* svc = server->createService(NUS_SVC);
  txChar = svc->createCharacteristic(NUS_TX, NIMBLE_PROPERTY::NOTIFY);
  rxChar = svc->createCharacteristic(NUS_RX, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);
  rxChar->setCallbacks(new RxCallbacks());
  svc->start();
  NimBLEAdvertising* adv = NimBLEDevice::getAdvertising();
  adv->addServiceUUID(NUS_SVC);
  adv->setName(BLE_NAME);
  adv->start();
  Serial.println("BLE advertising as " BLE_NAME);
}

void bleTick() {}

bool bleTakeRebootRequest() {
  if (!rebootReq) return false;
  rebootReq = false;
  return true;
}

bool bleTakeResetRequest() {
  if (!resetReq) return false;
  resetReq = false;
  return true;
}

bool bleSettingsChanged() {
  if (!changed) return false;
  changed = false;
  return true;
}

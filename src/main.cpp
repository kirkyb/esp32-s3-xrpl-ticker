#include <Arduino.h>
#include <WiFi.h>
#include "config.h"
#include "storage.h"
#include "display.h"
#include "ble_setup.h"
#include "market.h"

enum class Mode { Welcome, Setup, Connecting, Run, Error };

static AppSettings settings;
static MarketData market;
static Mode mode = Mode::Welcome;
static uint32_t modeAt = 0;
static uint32_t lastFetch = 0;
static uint32_t lastWifiTry = 0;
static int page = 0;
static uint32_t pageAt = 0;
static bool wifiEver = false;

static void showPage() {
  if (!market.ok) {
    displayStatus(market.error.length() ? market.error : "WAIT");
    return;
  }
  String fiat = fiatSymbol(settings.currency);
  String msg;
  switch (page % 3) {
    case 0:
      msg = settings.token + " " + formatSig3(market.priceFiat) + " " + fiat;
      break;
    case 1:
      msg = "QTY " + formatQty(market.quantity) + " " + settings.token;
      break;
    default:
      msg = "VAL " + formatSig3(market.valueFiat) + " " + fiat;
      break;
  }
  displaySetScroll(msg);
}

static void connectWifi() {
  if (settings.wifiSsid.isEmpty()) return;
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("xrpl-ticker");
  WiFi.begin(settings.wifiSsid.c_str(), settings.wifiPass.c_str());
  lastWifiTry = millis();
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println("\nESP32-S3 XRPL LED Ticker");

  storageLoad(settings);
  if (settings.token.isEmpty()) settings.token = "XRP";
  if (settings.currency.isEmpty()) settings.currency = "GBP";

  bool displays = displayBegin();
  if (!displays) Serial.println("One or more HT16K33 backpacks missing. Check wiring and addresses.");

  displayWelcome();
  bleBegin(settings);
  mode = Mode::Welcome;
  modeAt = millis();
}

void loop() {
  displayTick();
  bleTick();

  if (bleTakeRebootRequest()) {
    delay(300);
    ESP.restart();
  }
  if (bleTakeResetRequest()) {
    displayStatus("RESET");
    delay(800);
    ESP.restart();
  }
  if (bleSettingsChanged()) {
    storageLoad(settings);
    if (settingsReady(settings)) {
      mode = Mode::Connecting;
      modeAt = millis();
      displayStatus("WIFI");
      connectWifi();
    } else {
      mode = Mode::Setup;
      displayStatus("SETUP BT");
    }
  }

  uint32_t now = millis();

  switch (mode) {
    case Mode::Welcome:
      if (now - modeAt >= WELCOME_MS) {
        if (settingsReady(settings)) {
          mode = Mode::Connecting;
          displayStatus("WIFI");
          connectWifi();
        } else {
          mode = Mode::Setup;
          displayStatus("SETUP BT");
        }
        modeAt = now;
      }
      break;

    case Mode::Setup:
      if (now - modeAt > 8000 && !displayIsScrolling()) {
        displayStatus("SETUP BT");
        modeAt = now;
      }
      break;

    case Mode::Connecting:
      if (WiFi.status() == WL_CONNECTED) {
        wifiEver = true;
        displayStatus("ONLINE");
        mode = Mode::Run;
        modeAt = now;
        lastFetch = 0;
        page = 0;
        pageAt = now;
      } else if (now - lastWifiTry > WIFI_RETRY_MS) {
        displayStatus("WIFI ERR");
        connectWifi();
      }
      break;

    case Mode::Run:
      if (WiFi.status() != WL_CONNECTED) {
        mode = Mode::Connecting;
        displayStatus("WIFI");
        connectWifi();
        break;
      }
      if (lastFetch == 0 || now - lastFetch >= PRICE_REFRESH_MS) {
        lastFetch = now;
        MarketData fresh;
        if (marketFetch(settings, fresh)) {
          market = fresh;
          Serial.printf("price=%s qty=%s val=%s\n",
                        formatSig3(market.priceFiat).c_str(),
                        formatQty(market.quantity).c_str(),
                        formatSig3(market.valueFiat).c_str());
        } else {
          market = fresh;
          Serial.println(market.error);
        }
        showPage();
        pageAt = now;
      }
      if (now - pageAt >= PAGE_MS && !displayIsScrolling()) {
        page = (page + 1) % 3;
        pageAt = now;
        showPage();
      }
      if (displayIsScrolling() && now - pageAt >= PAGE_MS * 2) {
        page = (page + 1) % 3;
        pageAt = now;
        showPage();
      }
      break;

    case Mode::Error:
      if (now - modeAt > 5000) {
        mode = settingsReady(settings) ? Mode::Connecting : Mode::Setup;
        modeAt = now;
      }
      break;
  }
}

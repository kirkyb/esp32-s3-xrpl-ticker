#pragma once
#include <Arduino.h>
#include "storage.h"

struct MarketData {
  bool ok = false;
  String error;
  double priceFiat = 0;     // token price in chosen fiat
  double quantity = 0;      // wallet holding of chosen token
  double valueFiat = 0;     // quantity * price
  String token;
  String fiat;
};

String formatSig3(double v);
String formatQty(double v);
String fiatSymbol(const String& code);

bool marketFetch(const AppSettings& s, MarketData& out);

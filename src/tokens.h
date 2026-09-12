#pragma once
#include <Arduino.h>

struct KnownToken {
  const char* ticker;
  const char* currency;   // XRPL currency code (3 char or 40 hex)
  const char* issuer;     // empty for native
  const char* geckoId;    // CoinGecko id if listed
  bool xahau;             // true = query Xahau instead of XRPL
};

// Built-in map. Unknown tickers still work if SET ISSUER is provided
// and a DEX book against XRP exists.
static const KnownToken KNOWN_TOKENS[] = {
  {"XRP", "XRP", "", "ripple", false},
  {"CSC", "CSC", "rCSCManTZ8ME9EoLrSHHYKW8PPwWMgkwr", "casinocoin", false},
  {"XAH", "XAH", "", "xahau", true},
  {"SOLO", "534F4C4F00000000000000000000000000000000", "rsoLo2qkE8BkA3eZksoVdfcpPjPGoyEakN", nullptr, false},
  {"RLUSD", "524C555344000000000000000000000000000000", "rMxCKbEDwqr76QuheSUMdEGf4B9xJ8m5De", nullptr, false},
};

inline const KnownToken* findKnownToken(const String& ticker) {
  String t = ticker;
  t.toUpperCase();
  for (const auto& k : KNOWN_TOKENS) {
    if (t == k.ticker) return &k;
  }
  return nullptr;
}

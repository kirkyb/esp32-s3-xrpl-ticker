#include "market.h"
#include "tokens.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>

static const char* XRPL_RPC = "https://s1.ripple.com:51234/";
static const char* XAHAU_RPC = "https://xahau.network/";
static const char* GECKO = "https://api.coingecko.com/api/v3/simple/price";
static const char* COINBASE = "https://api.coinbase.com/v2/prices/";

static String httpGet(const String& url) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(12000);
  http.setUserAgent("XRPL-Ticker-ESP32/1.0");
  if (!http.begin(client, url)) return "";
  int code = http.GET();
  String body;
  if (code == 200) body = http.getString();
  else Serial.printf("GET %s -> %d\n", url.c_str(), code);
  http.end();
  return body;
}

static String httpPostJson(const String& url, const String& payload) {
  WiFiClientSecure client;
  client.setInsecure();
  HTTPClient http;
  http.setTimeout(12000);
  http.setUserAgent("XRPL-Ticker-ESP32/1.0");
  if (!http.begin(client, url)) return "";
  http.addHeader("Content-Type", "application/json");
  int code = http.POST(payload);
  String body;
  if (code == 200) body = http.getString();
  else Serial.printf("POST %s -> %d\n", url.c_str(), code);
  http.end();
  return body;
}

String fiatSymbol(const String& code) {
  if (code == "GBP") return "GBP";
  if (code == "USD") return "USD";
  if (code == "EUR") return "EUR";
  return code;
}

String formatSig3(double v) {
  if (!isfinite(v)) return "---";
  double a = fabs(v);
  char buf[24];
  if (a >= 100) snprintf(buf, sizeof(buf), "%.0f", v);
  else if (a >= 10) snprintf(buf, sizeof(buf), "%.1f", v);
  else if (a >= 1) snprintf(buf, sizeof(buf), "%.2f", v);
  else if (a >= 0.1) snprintf(buf, sizeof(buf), "%.3f", v);
  else if (a >= 0.01) snprintf(buf, sizeof(buf), "%.4f", v);
  else if (a >= 0.001) snprintf(buf, sizeof(buf), "%.5f", v);
  else if (a == 0) return "0";
  else snprintf(buf, sizeof(buf), "%.2e", v);
  return String(buf);
}

String formatQty(double v) {
  if (!isfinite(v)) return "---";
  char buf[28];
  double a = fabs(v);
  if (a >= 1000000) snprintf(buf, sizeof(buf), "%.2fM", v / 1000000.0);
  else if (a >= 10000) snprintf(buf, sizeof(buf), "%.1fK", v / 1000.0);
  else if (a >= 100) snprintf(buf, sizeof(buf), "%.1f", v);
  else if (a >= 1) snprintf(buf, sizeof(buf), "%.2f", v);
  else snprintf(buf, sizeof(buf), "%.4f", v);
  return String(buf);
}

static bool geckoPrice(const char* id, const String& fiat, double& out) {
  if (!id) return false;
  String vs = fiat;
  vs.toLowerCase();
  String url = String(GECKO) + "?ids=" + id + "&vs_currencies=" + vs;
  String body = httpGet(url);
  if (body.isEmpty()) return false;
  JsonDocument doc;
  if (deserializeJson(doc, body)) return false;
  if (doc[id][vs].isNull()) return false;
  out = doc[id][vs].as<double>();
  return out > 0;
}

static bool coinbaseXrpFiat(const String& fiat, double& out) {
  String url = String(COINBASE) + "XRP-" + fiat + "/spot";
  String body = httpGet(url);
  if (body.isEmpty()) return false;
  JsonDocument doc;
  if (deserializeJson(doc, body)) return false;
  const char* amt = doc["data"]["amount"];
  if (!amt) return false;
  out = atof(amt);
  return out > 0;
}

static bool rpcAccountXrp(const String& rpc, const String& account, double& xrpOut) {
  String payload = "{\"method\":\"account_info\",\"params\":[{\"account\":\"" +
                   account + "\",\"ledger_index\":\"validated\"}]}";
  String body = httpPostJson(rpc, payload);
  if (body.isEmpty()) return false;
  JsonDocument doc;
  if (deserializeJson(doc, body)) return false;
  const char* bal = doc["result"]["account_data"]["Balance"];
  if (!bal) {
    const char* err = doc["result"]["error_message"] | doc["error_message"];
    Serial.printf("account_info: %s\n", err ? err : "no balance");
    return false;
  }
  xrpOut = atof(bal) / 1000000.0;
  return true;
}

static bool rpcAccountIou(const String& rpc, const String& account,
                          const String& currency, const String& issuer,
                          double& qtyOut) {
  String payload = "{\"method\":\"account_lines\",\"params\":[{\"account\":\"" +
                   account + "\",\"ledger_index\":\"validated\"}]}";
  String body = httpPostJson(rpc, payload);
  if (body.isEmpty()) return false;
  JsonDocument doc;
  if (deserializeJson(doc, body)) return false;
  JsonArray lines = doc["result"]["lines"].as<JsonArray>();
  if (lines.isNull()) return false;
  for (JsonObject line : lines) {
    String cur = line["currency"].as<String>();
    String peer = line["account"].as<String>();
    if (cur == currency && (issuer.isEmpty() || peer == issuer)) {
      qtyOut = atof(line["balance"] | "0");
      if (qtyOut < 0) qtyOut = 0;
      return true;
    }
  }
  qtyOut = 0;
  return true;
}

static bool dexPriceInXrp(const String& currency, const String& issuer, double& xrpPerToken) {
  if (issuer.isEmpty()) return false;
  String payload =
      "{\"method\":\"book_offers\",\"params\":[{"
      "\"taker_gets\":{\"currency\":\"XRP\"},"
      "\"taker_pays\":{\"currency\":\"" + currency + "\",\"issuer\":\"" + issuer + "\"},"
      "\"limit\":8}]}";
  String body = httpPostJson(XRPL_RPC, payload);
  if (body.isEmpty()) return false;
  JsonDocument doc;
  if (deserializeJson(doc, body)) return false;
  JsonArray offers = doc["result"]["offers"].as<JsonArray>();
  if (offers.isNull() || offers.size() == 0) return false;
  double best = 0;
  for (JsonObject o : offers) {
    double gets = 0, pays = 0;
    if (o["TakerGets"].is<const char*>()) gets = atof(o["TakerGets"]) / 1000000.0;
    else if (o["TakerGets"]["value"].is<const char*>()) gets = atof(o["TakerGets"]["value"]);
    if (o["TakerPays"]["value"].is<const char*>()) pays = atof(o["TakerPays"]["value"]);
    else if (o["TakerPays"].is<const char*>()) pays = atof(o["TakerPays"]) / 1000000.0;
    if (pays > 0 && gets > 0) {
      best = gets / pays;
      break;
    }
  }
  if (best <= 0) return false;
  xrpPerToken = best;
  return true;
}

bool marketFetch(const AppSettings& s, MarketData& out) {
  out = MarketData{};
  out.token = s.token;
  out.fiat = s.currency;

  if (WiFi.status() != WL_CONNECTED) {
    out.error = "NO WIFI";
    return false;
  }

  const KnownToken* known = findKnownToken(s.token);
  String currency = s.token;
  String issuer = s.issuer;
  const char* gecko = nullptr;
  bool xahau = false;
  if (known) {
    currency = known->currency;
    if (issuer.isEmpty()) issuer = known->issuer;
    gecko = known->geckoId;
    xahau = known->xahau;
  }

  double price = 0;
  bool gotPrice = false;
  if (gecko) gotPrice = geckoPrice(gecko, s.currency, price);
  if (!gotPrice && s.token == "XRP") gotPrice = coinbaseXrpFiat(s.currency, price);

  if (!gotPrice && s.token != "XRP" && s.token != "XAH" && issuer.length() > 0) {
    double xrpFiat = 0;
    if (geckoPrice("ripple", s.currency, xrpFiat) || coinbaseXrpFiat(s.currency, xrpFiat)) {
      double xrpPer = 0;
      if (dexPriceInXrp(currency, issuer, xrpPer)) {
        price = xrpPer * xrpFiat;
        gotPrice = price > 0;
      }
    }
  }

  if (!gotPrice) {
    out.error = "PRICE ERR";
    return false;
  }
  out.priceFiat = price;

  const char* rpc = xahau ? XAHAU_RPC : XRPL_RPC;
  bool gotQty = false;
  if (s.token == "XRP" || s.token == "XAH") {
    gotQty = rpcAccountXrp(rpc, s.xrplAddress, out.quantity);
  } else {
    gotQty = rpcAccountIou(rpc, s.xrplAddress, currency, issuer, out.quantity);
  }
  if (!gotQty) {
    out.error = "ADDR ERR";
    out.quantity = 0;
    out.valueFiat = 0;
    out.ok = true;
    return true;
  }

  out.valueFiat = out.quantity * out.priceFiat;
  out.ok = true;
  return true;
}

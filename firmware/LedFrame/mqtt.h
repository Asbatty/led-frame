#pragma once
// ---- MQTT link to Home Assistant ----
// One retained "device discovery" message creates every entity in Home Assistant.
// Home Assistant pushes weather and Basil data to ledframe/weather and ledframe/basil.
#include <WiFi.h>
#include <PubSubClient.h>

WiFiClient mqttNet;
PubSubClient mqtt(mqttNet);
String devId;
IPAddress brokerIP;
uint32_t lastMqttTry = 0, rediscoverAt = 0, lastDiag = 0;

String topic(const char* t) { return String("ledframe/") + t; }

bool resolveBroker() {
  String h = MQTT_HOST;
  if (h.endsWith(".local")) {
    IPAddress ip = MDNS.queryHost(h.substring(0, h.length() - 6), 2000);
    if (ip == IPAddress()) {
      if (brokerIP != IPAddress()) return true;  // keep the last address that worked
      Serial.println("MQTT: couldn't find " + h);
      return false;
    }
    brokerIP = ip;
    return true;
  }
  return WiFi.hostByName(h.c_str(), brokerIP) == 1;
}

void publishDiscovery() {
  JsonDocument d;
  JsonObject dev = d["dev"].to<JsonObject>();
  dev["ids"].add(devId);
  dev["name"] = "LED Frame";
  dev["mf"] = "DIY";
  dev["mdl"] = "64x32 RGB matrix + ESP32";
  dev["sw"] = FW_VERSION;
  dev["cu"] = "http://" + WiFi.localIP().toString();
  d["o"]["name"] = "led-frame";
  d["o"]["sw"] = FW_VERSION;
  d["avty_t"] = topic("status");

  JsonObject c = d["cmps"].to<JsonObject>();
  auto add = [&](const char* key, const char* platform, const char* name) {
    JsonObject e = c[key].to<JsonObject>();
    e["p"] = platform;
    e["uniq_id"] = devId + "_" + key;
    if (name) e["name"] = name; else e["name"] = nullptr;
    return e;
  };

  JsonObject e = add("light", "light", nullptr);
  e["schema"] = "json";
  e["cmd_t"] = topic("light/set");
  e["stat_t"] = topic("light/state");
  e["brightness"] = true;
  e["icon"] = "mdi:led-strip-variant";

  e = add("mode", "select", "Mode");
  e["cmd_t"] = topic("mode/set");
  e["stat_t"] = topic("mode/state");
  for (int i = 0; i < MODE_COUNT; i++) e["options"].add(MODE_NAMES[i]);
  e["icon"] = "mdi:view-carousel";

  e = add("message", "text", "Message");
  e["cmd_t"] = topic("message/set");
  e["stat_t"] = topic("message/state");
  e["max"] = 100;
  e["icon"] = "mdi:message-text";

  e = add("color", "select", "Message color");
  e["cmd_t"] = topic("color/set");
  e["stat_t"] = topic("color/state");
  for (int i = 0; i < COLOR_COUNT; i++) e["options"].add(COLORS[i].name);
  e["icon"] = "mdi:palette";

  e = add("cd_name", "text", "Countdown name");
  e["cmd_t"] = topic("cd_name/set");
  e["stat_t"] = topic("cd_name/state");
  e["max"] = 15;
  e["icon"] = "mdi:calendar-star";

  e = add("cd_date", "text", "Countdown date");
  e["cmd_t"] = topic("cd_date/set");
  e["stat_t"] = topic("cd_date/state");
  e["max"] = 10;
  e["pattern"] = "^(\\d{4}-\\d{2}-\\d{2})?$";
  e["icon"] = "mdi:calendar";

  e = add("alert", "notify", "Alert");
  e["cmd_t"] = topic("notify");

  e = add("restart", "button", "Restart");
  e["cmd_t"] = topic("restart");
  e["device_class"] = "restart";
  e["entity_category"] = "config";

  e = add("rssi", "sensor", "Wi-Fi signal");
  e["stat_t"] = topic("diag");
  e["value_template"] = "{{ value_json.rssi }}";
  e["unit_of_measurement"] = "dBm";
  e["device_class"] = "signal_strength";
  e["state_class"] = "measurement";
  e["entity_category"] = "diagnostic";

  e = add("ip", "sensor", "IP address");
  e["stat_t"] = topic("diag");
  e["value_template"] = "{{ value_json.ip }}";
  e["entity_category"] = "diagnostic";
  e["icon"] = "mdi:ip-network";

  String out;
  serializeJson(d, out);
  bool ok = mqtt.publish(("homeassistant/device/" + devId + "/config").c_str(), (const uint8_t*)out.c_str(), out.length(), true);
  Serial.printf("MQTT: discovery %s (%u bytes)\n", ok ? "sent" : "FAILED", out.length());
}

void publishState() {
  if (!mqtt.connected()) return;
  JsonDocument l;
  l["state"] = S.on ? "ON" : "OFF";
  l["brightness"] = S.brightness;
  String ls;
  serializeJson(l, ls);
  mqtt.publish(topic("light/state").c_str(), ls.c_str(), true);
  mqtt.publish(topic("mode/state").c_str(), MODE_NAMES[S.mode], true);
  mqtt.publish(topic("color/state").c_str(), COLORS[S.color].name, true);
  mqtt.publish(topic("message/state").c_str(), S.message.c_str(), true);
  mqtt.publish(topic("cd_name/state").c_str(), S.cdName.c_str(), true);
  mqtt.publish(topic("cd_date/state").c_str(), S.cdDate.c_str(), true);
}

void publishDiag() {
  JsonDocument d;
  d["rssi"] = WiFi.RSSI();
  d["ip"] = WiFi.localIP().toString();
  d["uptime"] = millis() / 1000;
  String s;
  serializeJson(d, s);
  mqtt.publish(topic("diag").c_str(), s.c_str(), true);
}

float numOr(JsonVariantConst v) { return v.is<float>() ? v.as<float>() : NAN; }

void onMqtt(char* t, byte* payload, unsigned int len) {
  String tp(t), p;
  p.reserve(len);
  for (unsigned i = 0; i < len; i++) p += (char)payload[i];

  if (tp == "homeassistant/status") {
    if (p == "online") rediscoverAt = millis() + random(1000, 4000);
    return;
  }
  if (tp == topic("notify")) { alertFromPayload(p); return; }
  if (tp == topic("restart")) { delay(200); ESP.restart(); }
  if (tp == topic("weather")) {
    JsonDocument d;
    if (deserializeJson(d, p)) return;
    L.cond = d["cond"] | "";
    L.temp = numOr(d["temp"]);
    L.hi = numOr(d["hi"]);
    L.lo = numOr(d["lo"]);
    L.rainSoon = d["rain_soon"] | false;
    L.rainIn = d["rain_in"] | -1;
    L.haveWeather = L.cond.length() > 0;
    return;
  }
  if (tp == topic("basil")) {
    p.trim();
    L.basilDays = (p.length() && (isDigit(p[0]) || p[0] == '.')) ? (int)floorf(p.toFloat()) : -1;
    return;
  }

  JsonDocument patch;
  if (tp == topic("light/set")) {
    JsonDocument d;
    if (deserializeJson(d, p)) return;
    if (d["state"].is<const char*>()) patch["on"] = String(d["state"].as<const char*>()) == "ON";
    if (d["brightness"].is<int>()) patch["brightness"] = d["brightness"].as<int>();
  } else if (tp == topic("mode/set")) patch["mode"] = p;
  else if (tp == topic("color/set")) patch["color"] = p;
  else if (tp == topic("message/set")) patch["message"] = p;
  else if (tp == topic("cd_name/set")) patch["cd_name"] = p;
  else if (tp == topic("cd_date/set")) patch["cd_date"] = p;
  else return;
  applyPatch(patch.as<JsonObjectConst>());
}

void mqttSetup() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char id[20];
  snprintf(id, sizeof id, "ledframe_%02x%02x%02x", mac[3], mac[4], mac[5]);
  devId = id;
  mqtt.setBufferSize(4096);
  mqtt.setKeepAlive(30);
  mqtt.setSocketTimeout(5);
  mqtt.setCallback(onMqtt);
}

void mqttLoop() {
  if (WiFi.status() != WL_CONNECTED) { L.mqttUp = false; return; }
  if (!mqtt.connected()) {
    L.mqttUp = false;
    if (lastMqttTry && millis() - lastMqttTry < 10000) return;
    lastMqttTry = millis();
    if (!resolveBroker()) return;
    mqtt.setServer(brokerIP, MQTT_PORT);
    if (!mqtt.connect(devId.c_str(), MQTT_USER, MQTT_PASS, topic("status").c_str(), 0, true, "offline")) {
      Serial.printf("MQTT: connect to %s failed (state %d)\n", brokerIP.toString().c_str(), mqtt.state());
      return;
    }
    Serial.println("MQTT: connected to " + brokerIP.toString());
    L.mqttUp = true;
    mqtt.subscribe("ledframe/+/set");
    mqtt.subscribe(topic("notify").c_str());
    mqtt.subscribe(topic("restart").c_str());
    mqtt.subscribe(topic("weather").c_str());
    mqtt.subscribe(topic("basil").c_str());
    mqtt.subscribe("homeassistant/status");
    publishDiscovery();
    mqtt.publish(topic("status").c_str(), "online", true);
    publishState();
    publishDiag();
    lastDiag = millis();
    return;
  }
  mqtt.loop();
  if (rediscoverAt && (int32_t)(millis() - rediscoverAt) >= 0) {
    rediscoverAt = 0;
    publishDiscovery();
    mqtt.publish(topic("status").c_str(), "online", true);
    publishState();
  }
  if (millis() - lastDiag > 60000) { lastDiag = millis(); publishDiag(); }
}

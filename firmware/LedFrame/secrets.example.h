#pragma once
// Copy to secrets.h and fill in. Wi-Fi is NOT here: the frame asks for it
// through its own setup hotspot ("LED-Frame-Setup") the first time.
#define MQTT_HOST     "your-broker.local" // machine running the MQTT broker
#define MQTT_PORT     1883
#define MQTT_USER     "ledframe"
#define MQTT_PASS     "change-me"         // password of the broker user
#define OTA_PASSWORD  "change-me"         // used for wireless firmware updates

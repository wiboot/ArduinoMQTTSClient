/****** Secure ESP8266 MQTT Client *****/
#define MAJOR_VER "00"
#define MINOR_VER "03"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <time.h>
#include "AsyncWait.h"
#include "globals.h"
#include "SetupWifi.h"



//TODO: implement secure credentials as a runtime config file
//      rather than a header file.
#include "secure_credentials.h"

SetupWifi setupWifi(
    STASSID, STAPSK,
    CA_CERT_PROG, CLIENT_CERT_PROG, CLIENT_KEY_PROG
);


const char* mqtt_server = MQTT_SERVER;
//IPAddress broker(192,168,1,1); // IP address of your MQTT broker


const char *ID = "sec_mqtt_client_" MAJOR_VER "_" MINOR_VER;  // Name of our device, must be unique
const String TOPIC("mytopic/messaging");

static PubSubClient pubsubClient(setupWifi.getWiFiClient());


// Handle incomming messages from the broker
void callback(char* topic, byte* payload, unsigned int length) {
    String topicStr;
    String payloadStr;

    for (int i = 0; topic[i]; i++) {
        topicStr += topic[i];
    }

    for (unsigned int i = 0; i < length; i++) {
        payloadStr += (char)payload[i];
    }

    DEBUG_LOGLN("");
    DEBUG_LOG("Message arrived - [");
    DEBUG_LOG(topicStr);
    DEBUG_LOG("] ");
    DEBUG_LOGLN(payloadStr);

    if (topicStr == TOPIC) {
        Serial.printf("Suscribed Topic Message : "+payloadStr);
    }
}

// Reconnect to the MQTT client.
void reconnectToMQTT(MilliSec currentMilliSec) {
  if (pubsubClient.connected()) {
    // We are connected so nothing further needs to be done.
    return;
  }

  static AsyncWait waitToRetry;
  if (waitToRetry.isWaiting(currentMilliSec)) {
    return;
  }

  DEBUG_LOG("Attempting MQTT connection...");
  // Attempt to connect
  if (pubsubClient.connect(ID)) {
    DEBUG_LOGLN("connected");

    pubsubClient.subscribe(TOPIC.c_str());
    DEBUG_LOG("Subcribed to: ");
    DEBUG_LOGLN(TOPIC);
    
  } else {
    DEBUG_LOGLN(" try again in 5 seconds.");
    waitToRetry.startWaiting(currentMilliSec, 5000);
  }
}


void setup() {
  #ifdef DEBUG
  Serial.begin(115200); // Start serial communication at 115200 baud
  #endif

  setupWifi.setupWifi();
  //pubsubClient.setServer(broker, 1883);
  pubsubClient.setServer(mqtt_server, 8883);
  pubsubClient.setCallback(callback); // Initialize the callback routine
}


#ifdef DEBUG
void startupTest(MilliSec currentMilliSec) {
  static AsyncWait startupTestWait;
  static uint8_t startupTestValue;
  static bool firstTime = true;
  static const unsigned turnOnSeconds = 1; // 1 second.
  static const unsigned nextIterationDuration = 1250; // 1,250 milliseconds.

  bool changed = false;

  if (firstTime) {
    firstTime = false;
    startupTestValue = 0;
    pubsubClient.publish("/pubtopic/rec","first");
  }

  if (startupTestValue >= 0 && startupTestValue <= 7) {
    if (!startupTestWait.isWaiting(currentMilliSec)) {
      ++startupTestValue;
      pubsubClient.publish("/pubtopic/rec","test");
    }
  }
}
#endif // DEBUG


void loop() {
    setupWifi.loopWifi();
    if (!setupWifi.isReadyForProcessing()) {
        // The WiFi is not ready yet so
        // don't do any further processing.
        return;
    }

    if (!pubsubClient.connected()) {
        // Reconnect if connection is lost.
        MilliSec currentMilliSec = millis();
        reconnectToMQTT(currentMilliSec);
    }
    pubsubClient.loop();

    { // App code.
        MilliSec currentMilliSec = millis();

        #ifdef DEBUG
        startupTest(currentMilliSec);
        #endif
    }
}

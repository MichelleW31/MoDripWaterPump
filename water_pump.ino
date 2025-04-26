// LIBRARIES
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>

#include "secrets.h"
#include "utilities.h"

// MQTT SERVER
const char* mqtt_server ="902e1d0dba3944fa88c5f6caac765b57.s1.eu.hivemq.cloud";
const char* localModId = "67e1f54045d20e4703ad5c4a";
int globalMoisture;
bool pumpRunning = false;
int desiredMoistureThreshold = 0;

const char* ca_cert = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

WiFiClientSecure espSecureClient;
PubSubClient client(espSecureClient);

const int RELAY_PIN = 5;

//////////////////////////////////
///////////// SETUP //////////////
//////////////////////////////////

void setup() {
  Serial.begin(9600);

  // START WITH SENSOR OFF
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  
  // START WIFI 
  WiFi.begin(ssid, password);

  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print("Not Connected");
  }

  Serial.println("");
  Serial.print("Connected to WiFi network");

  // SET UP TO MQTT SERVER
  espSecureClient.setCACert(ca_cert);
  client.setServer(mqtt_server, 8883);
  client.setCallback(callback);

  connectToMessageBroker();
}

//////////////////////////////////
///////// CONNECT TO MQTT ////////
//////////////////////////////////

void connectToMessageBroker() {
  char clientId[50];
  sprintf(clientId, "WaterPump-%s", localModId);

  // Keep running this until connected to mqtt broker
  while(!client.connected()) {
    if (client.connect(clientId, mqqt_username, mqqt_password)) {
      Serial.println("");
      Serial.println("Broker connected");

      client.subscribe("mod/drip/+");
      client.subscribe("mod/readings/+");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");

      delay(5000);
    }
  }

  client.subscribe("mod/drip/+");
  client.subscribe("mod/readings/+");
}

//////////////////////////////////
//////////// CALLBACK ////////////
//////////////////////////////////

void callback(std::string topic, byte* payload, unsigned int length) {
  String incomingMessage;

  for (int i = 0; i < length; i++) {
    incomingMessage += (char)payload[i];
  }

  StaticJsonDocument<256> doc;
  DeserializationError error = deserializeJson(doc, incomingMessage);

  // Check topic and then determine what data to pull
  if(topic.find("drip") != std::string::npos) {
    const char* incomingModId = doc["modId"];
    bool drip = doc["drip"];
    int dripDuration = doc["dripDuration"];

    // Check if mod matches
    if (strcmp(incomingModId, localModId) == 0) {
      Serial.println("modId matches this device. Executing drip...");

      if(drip) {
        triggerWaterPump(dripDuration);
      }
    }
  } else if (topic.find("readings") != std::string::npos) {
    const char* incomingModId = doc["modId"];
    int incomingMoisture = doc["moisture"];

    if (strcmp(incomingModId, localModId) == 0) {
      Serial.println("modId matches this device. Reading moisture...");

      globalMoisture = getMoisturePercentage(incomingMoisture);
    }
  }
}

void triggerWaterPump(int dripDuration) {
    desiredMoistureThreshold = dripDuration;
    pumpRunning = true;

    Serial.println("pump on");
    digitalWrite(RELAY_PIN, HIGH);
}

void loop() {
  client.loop();

  if (pumpRunning && globalMoisture >= desiredMoistureThreshold) {
    pumpRunning = false;
    Serial.println("pump OFF");
    digitalWrite(RELAY_PIN, LOW);
  }
}
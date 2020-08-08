#include <PubSubClient.h>
#include <ESP8266WiFi.h>
const char* ssid = "spgcontrols";
const char* password = "spg579!@";
long startTime = 0;
char* topic = "S1000";
char* server = "mqtt.eclipse.org";
String clientName = "PaulZ";
WiFiClient wifiClient;
void callback(char* topic, byte* payload, unsigned int length);
PubSubClient client(server, 1883, callback, wifiClient);
bool led = false;

void callback(char* topic, byte* payload, unsigned int length) {
 
}


String macToStr(const uint8_t* mac)
{
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off by making the voltage LOW
  startTime = millis();
  Serial.begin(115200);
  delay(10);
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Generate client name based on MAC address and last 8 bits of microsecond counter

  uint8_t mac[6];
  WiFi.macAddress(mac);

  Serial.print("Connecting to ");
  Serial.print(server);
  Serial.print(" as ");
  Serial.println(clientName);
  client.setCallback(callback);

  for (int i = 0; ; i++) {
    // here you have to type your key from your Broker
    if (client.connect("", "", "")) {
      Serial.println("Connected to MQTT broker");
      Serial.print("Topic is: ");
      Serial.println(topic);
      client.subscribe(topic);
      client.setCallback(callback);
      if (client.publish(topic, "Board Connected")) {
        Serial.println("Publish ok");
        break;
      }
      else {
        Serial.println("Publish failed");
      }
    }
    else if (i > 5) {
      Serial.println("MQTT connect failed");
      Serial.println("Will reset and try again...");
      abort();
    }
    delay(500);
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    if (client.connect("", "", "")) {
      Serial.println("Connected to MQTT broker");
      Serial.print("Topic is: ");
      Serial.println(topic);
      client.subscribe(topic);
      client.setCallback(callback);
      if (client.publish(topic, "Board Connected")) {
        Serial.println("Publish ok");
      }
      else {
        Serial.println("Publish failed");
      }
    }
  }
}


void loop() {
  if (client.connected()) {
    client.loop();
    if (Serial.available() > 0) {
      digitalWrite(LED_BUILTIN, LOW);   // turn the LED on (HIGH is the voltage level)
      String newLine = Serial.readStringUntil('\n');   // Until CR (Carriage Return)
      if (client.publish(topic, newLine.c_str()) == false) {
        Serial.println("Failed to publish" + newLine);
    }
    //Serial.println(newLine);
    digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off by making the voltage LOW
    }
  }
  else
    reconnect();
}

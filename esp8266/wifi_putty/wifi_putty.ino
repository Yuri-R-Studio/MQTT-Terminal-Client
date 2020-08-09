#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <ESP8266mDNS.h>

#define ssidDefault "spgcontrols"
#define passwordDefault "spg579!@"
#define topicDefault "S1000"
#define serverDefault "mqtt.eclipse.org"
#define BUTTON_PIN 0

/* Set these to your desired softAP credentials. They are not configurable at runtime */
#ifndef APSSID
#define APSSID "SPG Logger"
#define APPSK  ""
#endif

const char *softAP_ssid = APSSID;
const char *softAP_password = APPSK;

char BoardName[100] = topicDefault;
char Ssid[100] = ssidDefault;
char WifiPassword[100] = passwordDefault;
char MqttHost[100] = serverDefault;
char MqttPort[10] = "";
char UserName[100] = "";
char MqttPassword[100] = "";
/* hostname for mDNS. Should work at least on windows. Try http://esp8266.local */
const char *myHostname = "SPG Controls";

// DNS server
const byte DNS_PORT = 53;
DNSServer dnsServer;

/* Soft AP network parameters */
//IPAddress apIP(192, 168, 4, 1);
IPAddress apIP(8, 8, 8, 8);
IPAddress netMsk(255, 255, 255, 0);

/** Should I connect to WLAN asap? */
boolean connect;

// Web server
ESP8266WebServer server(80);

/** Current WLAN status */
unsigned int status = WL_IDLE_STATUS;

static char *StaticPage = "<html>\
  <head>\
    <title>Loogger Configuration</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; Color: #000088; }\
    </style>\
  </head>\
  <body>\
  <h2>Logger Configuration</h2>\
  <br>\
    <form method=\"post\" action=\"/settings/\">\
 <h4>Board Settings</h4>\
    Name:<input type=\"text\" name=\"name\" value=\"%s\"><br><br><br>\
  <h4>Wifi Settings</h4>\
      SSID: <input type=\"text\" name=\"ssid\" value=\"%s\"><br>\
    Password: <input type=\"text\" name=\"password\" value=\"%s\"><br><br><br>\
  <h4>MQTT</h4>\
    Host:<input type=\"text\" name=\"host\" value=\"%s\"><br>\
    Port:<input type=\"text\" name=\"port\" value=\"%s\"><br>\
    UserName:<input type=\"text\" name=\"user\" value=\"%s\"><br>\
    Password:<input type=\"text\" name=\"mpassword\" value=\"%s\"><br><br>\
    <br><br>\
      <input type=\"submit\" value=\"Save\">\
    </form>\
  </body>\
</html>";


WiFiClient wifiClient;
void callback(char* topic, byte* payload, unsigned int length);
PubSubClient client(MqttHost, 1883, callback, wifiClient);
bool led = false;

void callback(char* topic, byte* payload, unsigned int length) {

}

/** Handle root or redirect to captive portal */
void handleRoot() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");

  char Page[1024];

  snprintf(Page, 1024, StaticPage, BoardName, Ssid, WifiPassword, MqttHost, MqttPort, UserName, MqttPassword);
  server.send(200, "text/html", Page);
}

/** Is this an IP? */
boolean isIp(String str) {
  for (size_t i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname) + ".local")) {
    Serial.println("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send(302, "text/plain", "");   // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

void handleNotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");

  for (uint8_t i = 0; i < server.args(); i++) {
    message += String(F(" ")) + server.argName(i) + F(": ") + server.arg(i) + F("\n");
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", message);
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

void readFile(char * path, char * variable, size_t len)
{
  File file = LittleFS.open(path, "r");
  if (file)
  {
    file.read((unsigned char*)variable, len);
    file.close();
  }
}

void writeFile(char * path, char * variable)
{
  size_t len = strlen(variable);
  Serial.print("Saving file: ");
  Serial.write(variable, len);
  variable[len + 1] = '\0';
  File file = LittleFS.open(path, "w");
  if (file)
  {
    file.write((unsigned char*)variable, len+1);
    file.close();
  }
  else
    Serial.println("Failed to write the file");

}

void initVariables()
{
  readFile("/BoardName", BoardName, 1000);
  readFile("/Ssid", Ssid, 1000);
  readFile("/WifiPassword", WifiPassword, 1000);
  readFile("/MqttHost", MqttHost, 1000);
  readFile("/MqttPort", MqttPort, 1000);
  readFile("/UserName", UserName, 10);
  readFile("/MqttPassword", MqttPassword, 1000);
}

void handleSave() {
  if (server.method() != HTTP_POST) {
    server.send(405, "text/plain", "Method Not Allowed");
  } else {
    String message = F("\nArguments: ");
    message += server.args();
    message += F("\n");

    for (uint8_t i = 0; i < server.args(); i++) {
      message += String(F(" ")) + server.argName(i) + F(": ") + server.arg(i) + F("\n");
    }
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.arg("name").toCharArray(BoardName, 1000);
    server.arg("ssid").toCharArray(Ssid, 1000);
    server.arg("password").toCharArray(WifiPassword, 1000);
    server.arg("host").toCharArray(MqttHost, 1000);
    server.arg("port").toCharArray(MqttPort, 10);
    server.arg("user").toCharArray(UserName, 1000);
    server.arg("mpassword").toCharArray(MqttPassword, 1000);


    writeFile("/BoardName", BoardName);
    writeFile("/Ssid", Ssid);
    writeFile("/WifiPassword", WifiPassword);
    writeFile("/MqttHost", MqttHost);
    writeFile("/MqttPort", MqttPort);
    writeFile("/UserName", UserName);
    writeFile("/MqttPassword", MqttPassword);
    server.send(200, "text/plain", "Settings Saved" + message);
  }
}

void startWebServer()
{
  /* You can remove the password parameter if you want the AP to be open. */
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(softAP_ssid, softAP_password);
  delay(500); // Without delay I've seen the IP address blank
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  /* Setup the DNS server redirecting all the domains to the apIP */
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", apIP);

  /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
  server.on("/", handleRoot);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/settings/", handleSave);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.onNotFound(handleNotFound);
  server.begin(); // Web server start
  Serial.println("HTTP server started");
  WiFi.disconnect();
  while (true)
  {
    MDNS.update();
    // Do work:
    //DNS
    dnsServer.processNextRequest();
    //HTTP
    server.handleClient();
  }
}

void setup() {

  if (!LittleFS.begin()) {
    LittleFS.format();
    if (!LittleFS.begin())
    {
      Serial.println("LittleFS mount failed");
      return;
    }
  }

  initVariables();
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off by making the voltage LOW
  Serial.begin(115200);
  delay(10);
  Serial.print("Connecting to ");
  Serial.write(Ssid, strlen(Ssid));
  WiFi.begin(Ssid, WifiPassword);
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    counter ++;
    if (counter > 10)
    {
      startWebServer();
    }
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // Generate client name based on MAC address and last 8 bits of microsecond counter

  uint8_t mac[6];
  WiFi.macAddress(mac);

  Serial.print("Connecting to ");
  Serial.write(MqttHost, strlen(MqttHost));
  Serial.print(" as ");
  Serial.print(BoardName);
  client.setCallback(callback);

  for (int i = 0; ; i++) {
    // here you have to type your key from your Broker
    if (client.connect("", "", "")) {
      Serial.println("Connected to MQTT broker");
      Serial.print("Topic is: ");
      Serial.write((const char*)MqttHost, strlen(MqttHost));
      client.subscribe((const char*)MqttHost);
      client.setCallback(callback);
      if (client.publish((const char*)BoardName, "Board Connected")) {
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
      Serial.write((const char*)MqttHost, strlen(MqttHost));
      client.subscribe((const char*)MqttHost);
      client.setCallback(callback);
      if (client.publish((const char*)BoardName, "Board Connected")) {
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
      if (client.publish((const char*)BoardName, newLine.c_str()) == false) {
        Serial.println("Failed to publish" + newLine);
      }
      //Serial.println(newLine);
      digitalWrite(LED_BUILTIN, HIGH);    // turn the LED off by making the voltage LOW
    }
  }
  else
    reconnect();

  if (digitalRead(BUTTON_PIN) == false)
  {
    Serial.println("Button Pressed");
    delay(1000);
  }
}

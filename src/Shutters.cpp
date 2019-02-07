#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
// #include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1
#include <ArduinoJson.h>
#include <FS.h>
#include "CaptivePortal.h"

const char* host = "dweet.io";
const int httpsPort = 443;
typedef int (*GeneralFunction) (const String arg1);
JsonObject* config_json;
// JsonObject* actuators;

// Use web browser to view and copy
// SHA1 fingerprint of the certificate
const char* fingerprint = "27 6F AA EF 5D 8E CE F8 8E 6E 1E 48 04 A1 58 E2 65 E8 C9 34";
bool setup_mode = false;
// ESP8266WebServer old_server(80);
String escape(String unescaped){
  unescaped.remove(0, 1);
  unescaped.remove(unescaped.length()-2, 1);
  String result = "";
  for (int i = 0; i < unescaped.length(); i++) {
    char c = unescaped.charAt(i);
    int code = (int)c;
    if (code != 92){
      result += c;
    }
  }
  return result;
}

uint8_t pinToEspPin(String pinStr){
  if(pinStr=="0"){
    return D0;
  }
  else if (pinStr=="1")
  {
    return D1;
  }
  else if (pinStr=="2")
  {
    return D2;
  }
  else if (pinStr=="3")
  {
    return D3;
  }
  else if (pinStr=="4")
  {
    return D4;
  }
  else if (pinStr=="5")
  {
    return D5;
  }
  else if (pinStr=="6")
  {
    return D6;
  }
  else if (pinStr=="7")
  {
    return D7;
  }
  else if (pinStr=="8")
  {
    return D8;
  }
  else{
    return D0;
  }
}

int pinToGpioPin(String pinStr){
  return pinStr.toInt();
}

uint8_t* getPins(uint8_t EspPins[], String thingName, bool actuator){
    int i = 0;
    JsonArray& actuators = (*config_json)["actuators"];
    for (auto& actuator : actuators){
        if(actuator["name"] == thingName){
          JsonArray& pins = actuator["pins"];
          for (String pin : pins){
            Serial.println(pin);
            EspPins[i] = pinToEspPin(pin);
          }
        }
    }
}


int callback_relay(String line){

  StaticJsonBuffer<400> jsonBuffer;
  

  Serial.println("calledback with :");
  Serial.println(line);
  // line = escape(line);
  JsonObject& root = jsonBuffer.parseObject(line);
  const String thisjson = root["with"][0]["content"]["status"];
  const String currentThing = root["with"][0]["thing"];
  uint8_t pins[10];
  getPins(pins, currentThing, true);
  Serial.println("result:");
  Serial.println(thisjson);
  Serial.println(thisjson=="up");
  // int* pins = getPins(currentThing, true);
  Serial.println("pins[0]");
  Serial.println(pins[0]);


  if(thisjson=="up"){
    digitalWrite(pins[0], HIGH);
  }
  else{
    digitalWrite(pins[0] , LOW);
  }
  return 0;
}

void httpPingGET(String domain, String route, GeneralFunction callback){
  HTTPClient http;
  String full_address = "http://"+domain+route;
  Serial.println("http://"+domain+route);
  http.begin(full_address);
  http.addHeader("Content-Type", "application/json");
  String last_response = "";
  String response;
  while(1){
    delay(2000);
    //GET HTTP
    int httpCode = http.GET();
    Serial.println(httpCode);
    if(httpCode>0){
      response = http.getString();
      Serial.println(response);
    }
    // if response differs, callback
    if(last_response != response){
      last_response = response;
      callback(response);
    }

  }
}

String httpGET(String domain, String url, boolean chunck, GeneralFunction callback){
  //setup client
    // Use WiFiClientSecure class to create TLS connection
  WiFiClientSecure client;
  String line = "";
  String chunck_id = "";
  boolean first = true;
  Serial.print("connecting to ");
  Serial.println(host);
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return "connection failed";
  }

  // if (client.verify(fingerprint, host)) {
  //   Serial.println("certificate matches");
  // } else {
  //   Serial.println("certificate doesn't match");
  // }


  //connect
  Serial.print("requesting URL: ");
  Serial.println(url);
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: " + host + "\r\n" +
               "User-Agent: BuildFailureDetectorESP8266\r\n" +
               "Connection: close\r\n\r\n");

  Serial.println("request sent");

  while (client.connected()) {
    Serial.println("listening");
    if(chunck && first){
      first = false;
      Serial.println("first");
      String protocol = "";
      while(client.connected() && protocol == ""){
        Serial.println("firstwhile");
        protocol = client.readStringUntil('\n');
      }
      String access_header = client.readStringUntil('\n');
      String content_type = client.readStringUntil('\n');
      String encoding = client.readStringUntil('\n');
      String date = client.readStringUntil('\n');
      String connection = client.readStringUntil('\n');
      client.readStringUntil('\n');
      Serial.println("protocol");
      Serial.println(protocol);
      Serial.println("access_header");
      Serial.println(access_header);
      Serial.println("content_type");
      Serial.println(content_type);
      Serial.println("encoding");
      Serial.println(encoding);
      Serial.println("date");
      Serial.println(date);
      Serial.println("connection");
      Serial.println(connection);

    }
    while(client.connected() && chunck_id == ""){
      chunck_id = client.readStringUntil('\n');
    }
    line = client.readStringUntil('\n');
    Serial.println("chunck_id");
    Serial.println(chunck_id);
    Serial.println("line");
    Serial.println(line);
    if (line != "" && line != "\n" && line != "\r"){
      callback (line);
    }
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  return line;
}

void apply_config(){
  File config = SPIFFS.open("/configuration.json", "r");
  DynamicJsonBuffer *cmdJsonBuffer;
  cmdJsonBuffer = new DynamicJsonBuffer;
  config_json = &cmdJsonBuffer->parseObject(config.readString());
  // String ssid = (*conf_json)["ssid"];
}


void setup() {
  Serial.begin(115200);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);
  pinMode(D0, INPUT);
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(D4, INPUT);
  SPIFFS.begin();
  //Letting time for SPIFFS to startup
  delay(1000);
  boolean has_config = SPIFFS.exists("/configuration.json");
  if(has_config){
    File config = SPIFFS.open("/configuration.json", "r");
    String conf_sv = "";
    while (config.available()){
      conf_sv += char(config.read());
    }
    Serial.println(conf_sv);
    SPIFFS.remove("/configuration.json");
    Serial.println("Reset now to flush configuration...");
    digitalWrite(LED_BUILTIN, LOW);
    delay(3000);
    digitalWrite(LED_BUILTIN, HIGH);
    File configw = SPIFFS.open("/configuration.json", "w");
    configw.print(conf_sv);

    Serial.println("Configuration rescued:");
    File configafs = SPIFFS.open("/configuration.json", "r");
    Serial.println(configafs.readString());
    delay(1000);
  }
  if(!has_config || !connect_from_file("/configuration.json", 10, 5)){
    setup_portal();
    setup_mode = true;
  }
  else{
    apply_config();
  }

  // Serial.println("WiFi connected");
  // Serial.println("IP address: ");
  // Serial.println(WiFi.localIP());
}



void perform_actuator(String platform, JsonObject& actuator){
  GeneralFunction callback;
  String driver = actuator["driver"];
  String name = actuator["name"];
  if(driver=="relay"){
    callback = callback_relay;
  }
  String route = "/get/latest/dweet/for/"+name ;
  httpPingGET(platform, route, callback);
}

void loop() {
  {
    if(setup_mode){
      // old_server.handleClient();
    }
    else{
      String platform = (*config_json)["platform"];
      JsonArray& actuators = (*config_json)["actuators"];
      Serial.println(platform);
      
      for (auto& actuator : actuators){
        perform_actuator(platform, actuator);
      }

    }
  }
  delay(2000);

}

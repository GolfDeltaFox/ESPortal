#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

#include "ESPTemplateProcessor.h"

String scanNetworks() {
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  byte numSsid = WiFi.scanNetworks();
  String options = "";
  String ssid;
  // print the list of networks seen:
  Serial.print("SSID List:");
  Serial.println(numSsid);
  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet<numSsid; thisNet++) {
    ssid = WiFi.SSID(thisNet);
    options += "<option value='"+ssid+"'>"+ssid+"</option>";
  }
  return options;
}

String indexProcessor(const String& key) {
  if (key == "SSIDS_LIST") return scanNetworks();
  else if (key == "DEVICE_ID") return create_single_id();
  return "oops";
}

boolean connect(String ssid, String password, int timeout, int retry){
  WiFi.mode(WIFI_STA);
  char ssid_char[ssid.length()+1];
  ssid.toCharArray(ssid_char, ssid.length()+1);
  char password_char[password.length()+1];
  password.toCharArray(password_char, password.length()+1);
  Serial.print('"');
  Serial.print(ssid_char);
  Serial.println('"');
  Serial.print('"');
  Serial.print(password_char);
  Serial.println('"');
  bool is_connected = false;
  for(int i=0; i < retry+1 ; i++){
      if(!is_connected){
        WiFi.begin(ssid_char, password_char);
        int count = 0;
        int count_max = timeout*2;
        while (!is_connected && count < count_max) {
          is_connected = ( WiFi.status() == WL_CONNECTED);
          delay(500);
          Serial.println(WiFi.status());
          count += 1;
        }
        if(!is_connected){
          delay(5000);
          WiFi.persistent(false);
        }
      }
  }

  return is_connected;
}

void configurationHandler(){
  Serial.println("Config received");
  String config_json = server.arg("plain");
  Serial.println(config_json);
  StaticJsonBuffer<400> jsonBuffer;
  JsonObject& data = jsonBuffer.parseObject(config_json);
  String ssid = data["ssid"];
  String password = data["pwd"];
  boolean wifi_success = connect(ssid, password, 30, 2);
  Serial.println(wifi_success);
  if(wifi_success){
    server.send(200, "text/plain", "Device connected.");
    delay(2000);
    Serial.println("Saving credentials");
    SPIFFS.remove("/configuration.json");
    File f = SPIFFS.open("/configuration.json", "w");
    data.printTo(f);
    delay(1000);
    f.close();
    server.stop();
    server.close();
    WiFi.softAPdisconnect();
    ESP.reset();
    ESP.restart();
  }

}

void indexHandler(){
  if (ESPTemplateProcessor(server).send(String("/index.html"), indexProcessor)) {

    Serial.println("SUCCESS");

  } else {
    Serial.println("FAIL");
    server.send(404, "text/plain", "page not found.");
  }
}

void serve_file(){
  String filePath = "/css/style.css";
  File file = SPIFFS.open(filePath, "r");
  String filestr = file.readString();
  server.send(200, "text/css",filestr);
}

boolean connect_from_file(String filePath, int timeout, int retry){
  if(SPIFFS.exists(filePath)){
    File file = SPIFFS.open(filePath, "r");
    Serial.println("connection from file :");
    DynamicJsonBuffer *cmdJsonBuffer;
    cmdJsonBuffer = new DynamicJsonBuffer;
    config_json = &cmdJsonBuffer->parseObject(file.readString());
    String ssid = (*config_json)["ssid"].as<String>();
    String password = (*config_json)["password"].as<String>();
    ssid.replace("\r","");
    password.replace("\r","");
    boolean wifi_success = connect(ssid, password, timeout, retry);
    Serial.println(wifi_success);
    return wifi_success;
  }
  else{
    return false;
  }
}


String create_single_id(){
  String id = "ESP";
  for(int i=3; i<19; i++){
    byte randomValue = random(0, 37);
    char letter = randomValue + 'a';
    if(randomValue > 26){
        letter = (randomValue - 26) + '0';
    }
    id += letter;
  }
  return id;
}

void setup_portal(void){
  SPIFFS.begin();

  delay(1000);

  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point!");

  WiFi.softAP("ESP_setup");

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", indexHandler);
  server.on("/configuration", configurationHandler);


  // server.on("./css/style.css", serve_file);
  // server.on(".css/style.css", serve_file);
  // server.on("/css/style.css", serve_file);
  // server.on("css/style.css", serve_file);
  server.serveStatic("/css/",  SPIFFS, "/css/", "max-age=31536000");
  server.begin();
  Serial.println("HTTP server started");
}

// #include <ESP8266WiFi.h>
// #include <WiFiClient.h>
// #include <ESP8266WebServer.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

AsyncWebServer server(80);
String ssids = "";
String body_data = "";

String GETSsids() {
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  byte numSsid = WiFi.scanNetworks();
  String result = "[";
  String ssid;
  // print the list of networks seen:
  Serial.print("SSID List:");
  Serial.println(numSsid);
  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet<numSsid; thisNet++) {
    ssid = WiFi.SSID(thisNet);
    result += "\""+ssid+"\",";
  }
  int result_len = result.length();
  result[result_len-1] = ']';
  Serial.println(result);
  return result;
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
  WiFi.begin(ssid_char, password_char);
  bool is_connected = true;
  // int count = 0;
  // while ( !is_connected and count<retry*1000){
  //   is_connected = WiFi.status() == WL_CONNECTED;
  //   Serial.println(is_connected);
  //   ++count;
  // }
  return is_connected;
}

//
// bool POSTConfigurationHandler(AsyncWebServerRequest *request, uint8_t *data) {
//
// }


// void serve_index(){
//   Serial.println("serve_index");
//   File f = SPIFFS.open("/index.html", "r");
//   Serial.println("loaded");
//   String filestr = f.readString();
//   Serial.println("read");
//   server.send(200, "text/html",filestr);
//   Serial.println("sent");
// }
//
// void serve_css(){
//   Serial.println("serve_css");
//   File f = SPIFFS.open("/static/css/main.04f11c10.css", "r");
//   Serial.println("loaded");
//   String filestr = f.readString();
//   Serial.println("read");
//   server.send(200, "text/css",filestr);
//   Serial.println("sent");
//
// }
//
// void serve_js(){
//   Serial.println("serve_js");
//   File f = SPIFFS.open("/static/css/main.4bee1f99.js", "r");
//   Serial.println("loaded");
//   String filestr = f.readString();
//   Serial.println("read");
//   server.send(200, "text/javascript",filestr);
//   Serial.println("sent");
//
// }

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

boolean POSTConfigurationHandler(){
    Serial.println("Config received");
    Serial.println(body_data);
    StaticJsonBuffer<500> jsonBuffer;
    JsonObject& config_json = jsonBuffer.parseObject(body_data);
    // if (!config_json.success()) request->send(200, "text/plain", "bad json.");
    String ssid = config_json["ssid"];
    String password = config_json["pwd"];
    boolean wifi_success = connect(ssid, password, 30, 2);
    Serial.println(wifi_success);
    if(wifi_success){
      // return "Device connected.";
      Serial.println("Saving credentials");
      SPIFFS.remove("/configuration.json");
      File f = SPIFFS.open("/configuration.json", "w");
      config_json.printTo(f);
      f.close();
      WiFi.softAPdisconnect();
      ESP.reset();
      ESP.restart();
      return 0;
    }
    else{
      return 1;
    }
}

void setup_portal(void){
  SPIFFS.begin();

  delay(1000);

  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point!");

  WiFi.softAP("ESP_setup");
  ssids = GETSsids();
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  // server.on("/ssids", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send(200, "text/plain", GETCachedSsids());
  // });

  server.on("/ssids", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", ssids);
  });



  server.on("/configuration", HTTP_POST,
    [](AsyncWebServerRequest *request)
    {
      Serial.println("1");
    },
    [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final)
    {
      Serial.println("2");
    },
    [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
    {
      Serial.println("3");
      Serial.println((String) len);
      Serial.println((String) index);
      Serial.println((String) total);
      if(index == 0){
        body_data =  (char*)data;
      }
      else{
        body_data += (char*)data;
      }
      if(index+len == total){
        if(POSTConfigurationHandler()){
          request->send(200, "text/plain", "Bad.");
        }
        else{
          request->send(200, "text/plain", "Good.");
        }
      }
    });

  // server.on("/configuration", HTTP_POST, [](AsyncWebServerRequest *request){
  //     Serial.println("Config received");
  //     Serial.println("[REQUEST]");
  //     Serial.println(request->hasParam("body", true));
  //     if(request->hasParam("body", true))
  //     {
  //       AsyncWebParameter* p = request->getParam("body", true);
  //       String json = p->value();
  //       Serial.println(json);
  //     }

      // StaticJsonBuffer<500> jsonBuffer;
      // JsonObject& config_json = jsonBuffer.parseObject((const char*)data);
      // if (!config_json.success()) request->send(200, "text/plain", "bad json.");
      // String ssid = config_json["ssid"];
      // String password = config_json["pwd"];
      // boolean wifi_success = connect(ssid, password, 30, 2);
      // Serial.println(wifi_success);
      // if(wifi_success){
      //   // return "Device connected.";
      //   delay(2000);
      //   Serial.println("Saving credentials");
      //   SPIFFS.remove("/configuration.json");
      //   File f = SPIFFS.open("/configuration.json", "w");
      //   config_json.printTo(f);
      //   delay(1000);
      //   f.close();
      //   WiFi.softAPdisconnect();
      //   ESP.reset();
      //   ESP.restart();
      //   request->send(200, "text/plain", "Good.");
      // }
      // else{
      //   request->send(200, "text/plain", "Bad.");
      //
      // }
  // });

  // server.on("/configuration", HTTP_POST, [](AsyncWebServerRequest *request){
  //   Serial.println("Config received");
  //   String config_json = request->getParam("body", true)->value();
  //   Serial.println(config_json);
    // StaticJsonBuffer<400> jsonBuffer;
    // JsonObject& data = jsonBuffer.parseObject(config_json);
    // String ssid = data["ssid"];
    // String password = data["pwd"];
    // boolean wifi_success = connect(ssid, password, 30, 2);
    // Serial.println(wifi_success);
    // if(wifi_success){
    //   request->send(200, "text/plain", "Device connected.");
    //
    //   delay(2000);
    //   Serial.println("Saving credentials");
    //   SPIFFS.remove("/configuration.json");
    //   File f = SPIFFS.open("/configuration.json", "w");
    //   data.printTo(f);
    //   delay(1000);
    //   f.close();
    //   WiFi.softAPdisconnect();
    //   ESP.reset();
    //   ESP.restart();
    // }
    // else{
    //   request->send(200, "text/plain", "Wrong wifi credentials.");
    // }

  // });

  // server.on("/ssids", GETCachedSsids);
  // server.on("/configuration", POSTConfigurationHandler);
  // server.on("/static/js/main.4bee1f99.js", serve_js);
  // server.on("/static/css/main.04f11c10.css", serve_css);
  // server.on("/", serve_index);

  // server.on("./css/style.css", serve_file);
  // server.on(".css/style.css", serve_file);
  // server.on("/css/style.css", serve_file);
  // server.on("css/style.css", serve_file);
  // server.serveStatic("/static",  SPIFFS, "/static" ,"max-age=86400");
  // server.serveStatic("/",  SPIFFS, "/index.html", "max-age=31536000");
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html");
  server.serveStatic("/static/js/main.4bee1f99.js", SPIFFS, "/static/js/main.4bee1f99.js");
  server.serveStatic("/static/css/main.04f11c10.css", SPIFFS, "/static/css/main.04f11c10.css");
  // server.serveStatic("/asset-manifest.json",  SPIFFS, "/asset-manifest.json" ,"max-age=86400");
  // server.serveStatic("/manifest.json",  SPIFFS, "/manifest.json" ,"max-age=86400");
  // server.serveStatic("/service-worker.js",  SPIFFS, "/service-worker.js", "max-age=86400");
  server.begin();
  Serial.println("HTTP server started");
}

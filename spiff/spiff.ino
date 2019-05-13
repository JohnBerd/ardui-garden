#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include "FS.h"

ESP8266WebServer server(80);
String names[3];
String dates[6];
int hygroActivated[3];

int hygroRequired;
int humidityRequired;
int humidityActivated;

float temperature;
float humidity;
float hygro[3];

String infoAtm;

char ssid[] = "wifi_sophie";
char pass[] = "oceane999";

int status = WL_IDLE_STATUS;

bool loadConfig() {
    File configFile = SPIFFS.open("/config.json", "r");
    if (!configFile) {
        Serial.println("Failed to open config file");
        return false;
    }
    size_t size = configFile.size();
    if (size > 1024) {
        Serial.println("Config file size is too large");
        return false;
    }
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);

    const size_t bufferSize = JSON_ARRAY_SIZE(3) + 4*JSON_OBJECT_SIZE(3) + 130;
    DynamicJsonBuffer jsonBuffer(bufferSize);
    JsonObject& json = jsonBuffer.parseObject(buf.get());
    if (!json.success()) {
        Serial.println("Failed to parse config file");
        return false;
    }
    String tmp_hygro;
    humidityActivated = json["humactive"];
    humidityRequired = json["hum"];
    hygroRequired = json["hygro"];
    JsonArray& plants = json["plants"];
    for (int i = 0; i < 3; i++) {
        names[i] = strdup(plants[i]["name"]);
        tmp_hygro = strdup(plants[i]["hygroactive"]);
        hygroActivated[i] = tmp_hygro.toInt();
    }
    sendInfoToArduino();
    return true;
}

void reloadConfig() {
    String jsonString;

    const size_t bufferSize = JSON_ARRAY_SIZE(3) + 4*JSON_OBJECT_SIZE(3);
    DynamicJsonBuffer jsonBuffer(bufferSize);

    JsonObject& root = jsonBuffer.createObject();
    root["hum"] = humidityRequired;
    root["humactive"] = humidityActivated;
    root["hygro"] = hygroRequired;

    JsonArray& plants = root.createNestedArray("plants");

    JsonObject& plants_0 = plants.createNestedObject();
    plants_0["name"] = names[0];
    plants_0["cro"] = dates[0];
    plants_0["flo"] = dates[1];
    plants_0["hygroactive"] = hygroActivated[0];

    JsonObject& plants_1 = plants.createNestedObject();
    plants_1["name"] = names[1];
    plants_1["cro"] = dates[2];
    plants_1["flo"] = dates[3];
    plants_1["hygroactive"] = hygroActivated[1];

    JsonObject& plants_2 = plants.createNestedObject();
    plants_2["name"] = names[2];
    plants_2["cro"] = dates[4];
    plants_2["flo"] = dates[5];
    plants_2["hygroactive"] = hygroActivated[2];

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing");
        return ;
    }
    root.printTo(configFile);
    root.printTo(jsonString);
    sendInfoToArduino();
    server.send(200, "text/json; charset=utf-8", jsonString);
}

void handleSetNames(){
    for (int i = 0; i < 3; i++) {
        server.arg(i).trim();
        if (server.arg(i).length() != 0) {
            names[i] = server.arg(i);
        }
    }
    reloadConfig();
}

void handleSetDates(){
    for (int i = 0; i < 6; i++) {
        server.arg(i).trim();
        if (server.arg(i).length() != 0) {
            dates[i] = server.arg(i);
        }
    }
    reloadConfig();
}

void handleActivateHygro() {
    hygroActivated[server.arg(0).toInt()] = server.arg(1).toInt();
    reloadConfig();
}

void handleSetHygro() {
    humidityRequired = server.arg(0).toInt();
    reloadConfig();
}

void handleActivateHumidity() {
    humidityActivated = server.arg(0).toInt();
    reloadConfig();
}

void handleSetHumidity() {
    humidityRequired = server.arg(0).toInt();
    reloadConfig();
}

void handleGetConfig(){
    reloadConfig();
}

void handleRoot(){
    server.sendHeader("Location", "/index.html", true);
    server.send(302, "text/html; charset=utf-8", "");
}

void handleWebRequests(){
    if(loadFromSpiffs(server.uri())) return;
    String message = "File Not Detected\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET)?"GET":"POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i=0; i<server.args(); i++){
        message += " NAME:"+server.argName(i) + "\n VALUE:" + server.arg(i) + "\n";
    }
    server.send(404, "text/plain", message);
}

void handleGetInfo(){
  server.send(200, "text/json; charset=utf-8", infoAtm);
}

void setup() {
    Serial.begin(115200);
    Serial.print("Attempting to connect to SSID: ");
    Serial.println("coco");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    printWifiStatus();
    Serial.println();
    if (!SPIFFS.begin()) {
        Serial.println("erreur SPIFFS");
        while (true);
    }
    server.on("/", handleRoot);
    server.on("/getconfig", handleGetConfig);
    server.on("/setnames", handleSetNames);
    server.on("/setdates", handleSetDates);
    server.on("/activatehumidity", handleActivateHumidity);
    server.on("/sethumidity", handleSetHumidity);
    server.on("/activatehygro", handleActivateHygro);
    server.on("/sethygro", handleSetHygro);
    server.on("/getinfo", handleGetInfo);
    server.onNotFound(handleWebRequests);
    server.begin();
    loadConfig();
}


void loop() {
    server.handleClient();
    checkInfoFromArduino();
}

void checkInfoFromArduino()
{
  bool StringReady;
  String json;
  
   while (Serial.available()){
   json=Serial.readString();
   StringReady = true;
  }
  infoAtm = json;
  if (StringReady){
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(json);
    if(!root.success()) {
      //Serial.println("parseObject() failed");
      return ;
    }
    temperature = root["t"];
    humidity = root["h"];
    hygro[0] = root["hy1"];
    hygro[1] = root["hy2"];
    hygro[2] = root["hy3"];
  }
}

void sendInfoToArduino()
{
  DynamicJsonBuffer jbuffer;
  JsonObject& root = jbuffer.createObject();
  root["humr"] = humidityRequired;
  root["huma"] = humidityActivated;
  root["hyr"] = hygroRequired;
  root["hy1"] = hygroActivated[0];
  root["hy2"] = hygroActivated[1];
  root["hy3"] = hygroActivated[2];
  root.printTo(Serial);
  Serial.println();
}

void printWifiStatus() {
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());

    // print your WiFi shield's IP address:
    IPAddress ip = WiFi.localIP();
    Serial.print("IP Address: ");
    Serial.println(ip);

    // print the received signal strength:
    long rssi = WiFi.RSSI();
    Serial.print("signal strength (RSSI):");
    Serial.print(rssi);
    Serial.println(" dBm");
}

bool loadFromSpiffs(String path){
    String dataType = "text/plain";
    if(path.endsWith("/")) path += "index.htm";

    if(path.endsWith(".src")) path = path.substring(0, path.lastIndexOf("."));
    else if(path.endsWith(".html")) dataType = "text/html";
    else if(path.endsWith(".htm")) dataType = "text/html";
    else if(path.endsWith(".css")) dataType = "text/css";
    else if(path.endsWith(".js")) dataType = "application/javascript";
    else if(path.endsWith(".png")) dataType = "image/png";
    else if(path.endsWith(".gif")) dataType = "image/gif";
    else if(path.endsWith(".jpg")) dataType = "image/jpeg";
    else if(path.endsWith(".ico")) dataType = "image/x-icon";
    else if(path.endsWith(".xml")) dataType = "text/xml";
    else if(path.endsWith(".pdf")) dataType = "application/pdf";
    else if(path.endsWith(".zip")) dataType = "application/zip";
    File dataFile = SPIFFS.open(path.c_str(), "r");
    if (server.hasArg("download")) dataType = "application/octet-stream";
    if (server.streamFile(dataFile, dataType) != dataFile.size()) {
    }

    dataFile.close();
    return true;
}

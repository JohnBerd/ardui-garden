#include <ArduinoJson.h>
#include "DHT.h"

#define HYGROPIN1 A0
#define HYGROPIN2 A1
#define HYGROPIN3 A2

#define WATERPUMP1 2
#define WATERPUMP2 3
#define WATERPUMP3 4
#define HUMIDIFICATOR 5
#define HALOPIN 6
#define DHTPIN 7

#define DHTTYPE DHT22

#define MAX_HYGRO 800

DHT dht(DHTPIN, DHTTYPE);

int hygroActivated[3];
int hygroRequired;
float hygro[3];

int humidityRequired;
int humidityActivated;

float temperature;
float humidity;

int waterEmpty;

void setup() {
  Serial.begin(115200);
  pinMode(HUMIDIFICATOR, OUTPUT);
  pinMode(WATERPUMP1, OUTPUT);
  pinMode(WATERPUMP2, OUTPUT);
  pinMode(WATERPUMP3, OUTPUT);
  pinMode(HALOPIN, INPUT);

  waterEmpty = 0;

  dht.begin();
  delay(5000);
}

void loop() {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  //waterEmpty = digitalRead(HALOPIN);
  
  if (hygroActivated[0])
    hygro[0] = map(1023 - analogRead(HYGROPIN1), 0, MAX_HYGRO, 0, 100);
  if (hygroActivated[1])
    hygro[1] = map(1023 - analogRead(HYGROPIN2), 0, MAX_HYGRO, 0, 100);
  if (hygroActivated[2])
    hygro[2] = map(1023 - analogRead(HYGROPIN3), 0, MAX_HYGRO, 0, 100);
  if (hygroActivated[0] && hygro[0] < hygroRequired && !waterEmpty)
    digitalWrite(WATERPUMP1, HIGH);
  if (hygroActivated[1] && hygro[1] < hygroRequired && !waterEmpty)
    digitalWrite(WATERPUMP2, HIGH);
  if (hygroActivated[1] && hygro[1] < hygroRequired && !waterEmpty)
    digitalWrite(WATERPUMP2, HIGH);
  if (hygroActivated[0] && hygro[0] > 100)
    digitalWrite(WATERPUMP1, LOW);
  if (hygroActivated[0] && hygro[1] > 100)
    digitalWrite(WATERPUMP2, LOW);
  if (hygroActivated[0] && hygro[2] > 100)
    digitalWrite(WATERPUMP3, LOW);

  if (humidityActivated && humidity < humidityRequired && !waterEmpty)
    digitalWrite(HUMIDIFICATOR, HIGH);
  else
    digitalWrite(HUMIDIFICATOR, LOW);
    
  sendInfoToESP();
  checkInfoFromESP();
  delay(2000);
}

void sendInfoToESP()
{
  DynamicJsonBuffer jbuffer;
  JsonObject& root = jbuffer.createObject();
  root["t"] = temperature;
  root["h"] = humidity;
  root["w"] = waterEmpty;
  root["hy1"] = hygro[0];
  root["hy2"] = hygro[1];
  root["hy3"] = hygro[2];

  root.printTo(Serial);
  Serial.println();
}

void checkInfoFromESP()
{
  bool StringReady;
  String jsonString ="";

  if (!Serial.available())
    return ;
  while (Serial.available()){
   jsonString =Serial.readString();
   StringReady = true;
  }
  if (StringReady){
    Serial.println(jsonString);
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.parseObject(jsonString);
    if (!root.success()) {
        Serial.println("Failed to parse config file");
        return ;
    }
    Serial.println("PARSED");
    humidityRequired = root["humr"];
    humidityActivated = root["huma"];
    hygroRequired = root["hyr"];
    hygroActivated[0] = root["hy1"];
    hygroActivated[1] = root["hy2"];
    hygroActivated[2] = root["hy3"];
  }
}

//libraries
#include <ArduinoIoTCloud.h>
#include <WiFiNINA.h>
#include <AM2302-Sensor.h>
#include <DFRobot_DF1201S.h>

//defines
AM2302::AM2302_Sensor am2302{0U};
DFRobot_DF1201S DFPlayer;
WiFiConnectionHandler WiFiConnection(SSID, PASS);

//macros
#define MAX_WATER_LEVEL 44
#define DF1201S_SERIAL Serial1

//variables and buffers
const char SSID[] = "307B"; //Orange-yHhQGT-2G
const char PASS[] = "Frigider007"; //xEbd9uP7G3K5Z59Gfk
float temperature;
float humidity;
int musicControl;
const int SoilSensorPin = A1;
const int PumpPin = 1;
int dryThreshold = 0;
int wetThreshold = 1023;
int soilMoistureLevel;  
bool pumpControl = 0;
bool stateMoisture = 0;
unsigned long pumpStartTime = 0;
unsigned long totalPumpTime = 0;

// This function controls the music player based on user requests
void setMusicChanges() {
    Serial.print("Music Control Command: ");
    Serial.println(musicControl);
        
    if (musicControl == 0) {
        Serial.println("Pause Music");
        DFPlayer.pause();
    } else if (musicControl == 1) {
        Serial.println("Play Music");
        DFPlayer.start();
    } else if (musicControl == 2) {
        Serial.println("Next Track");
        DFPlayer.next();
    } else if (musicControl >= 3 && musicControl <= 30) {
        Serial.print("Set Volume to:");
        Serial.println(musicControl);
        DFPlayer.setVol(musicControl);
    }
}

// This function sets the wet threshold based on dry threshold for levels of the moisture sensor
void setMoistureLevels() {
  switch(dryThreshold){
    case 30:
      wetThreshold = 40;
      break;
    case 35:
      wetThreshold = 50;
      break;
    case 40:
      wetThreshold = 55;
      break;
    case 45:
      wetThreshold = 60;
      break;
    case 50:
      wetThreshold = 65;
      break;
    case 55:
      wetThreshold = 70;
      break;
    default:
      break;
  }

  // Serial.print("Modificare umiditate sol: ");
  // Serial.print("Umiditate Minima:"); Serial.println(dryThreshold);
  // Serial.print("Umiditate Maxima:"); Serial.println(wetThreshold);
}

//This function get the moisture humidity level
void getMoistureLevel(){
  soilMoistureLevel = analogRead(SoilSensorPin)/10;
  // Serial.print("Moisture: "); Serial.println(soilMoistureLevel);
  // delay(2000);
}

//This function is been executed once per device's runnig time
void setup() {
  Serial.begin(115200);
  DF1201S_SERIAL.begin(115200);

  pinMode(PumpPin, OUTPUT);

  if (!am2302.begin()) {
      Serial.println("Sensor error!");
      while (true);
  }
  
  if (!DFPlayer.begin(DF1201S_SERIAL)) {
      Serial.println("DF1201S not detected!");
      while (true);
  }
  Serial.println("DF1201S Ready!");
  DFPlayer.setVol(20);

  Serial.println("Connect to Wifi");
  WiFi.begin(SSID, PASS);
  while (WiFi.status() != WL_CONNECTED) {
      // delay(500);
      Serial.print(".");
  }
  Serial.println(" Connected!");
 
  ArduinoCloud.begin(WiFiConnection);
  ArduinoCloud.addProperty(musicControl, WRITE, ON_CHANGE, setMusicChanges);
  ArduinoCloud.addProperty(temperature, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(humidity, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(soilMoistureLevel, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(dryThreshold, WRITE, ON_CHANGE, setMoistureLevels);
}

void loop() {
    ArduinoCloud.update();

    if (am2302.read() == 0) {
      temperature = am2302.get_Temperature();
      humidity = am2302.get_Humidity();
      // Serial.print("Temp: "); Serial.println(temperature);
      // Serial.print("Humidity: "); Serial.println(humidity);
    }

    getMoistureLevel();

    if (soilMoistureLevel < dryThreshold)
    {
      digitalWrite(PumpPin, HIGH);
      if(stateMoisture == 0)
      {
        stateMoisture = 1;
        pumpStartTime = millis();
      }
      
      if((millis() - (pumpStartTime + totalPumpTime)/1000) > MAX_WATER_LEVEL)
      {
        digitalWrite(PumpPin, LOW);
        Serial.println("Recipient GOL in timpul udarii! Necesar reumplere imediata!");
      }
    }
    else if (soilMoistureLevel >= wetThreshold && stateMoisture == 1)
    {
      digitalWrite(PumpPin, LOW);
      stateMoisture = 0;
      if (pumpStartTime > 0) 
      {
        totalPumpTime += (millis() - pumpStartTime);
        pumpStartTime = 0;
      }
      if (totalPumpTime/1000 >= MAX_WATER_LEVEL) 
      {
        Serial.println(totalPumpTime);
        totalPumpTime = 0;
        Serial.println("Recipient GOL!");
      }
    }
}

//libraries
#include <ArduinoIoTCloud.h>
#include <WiFiNINA.h>
#include <AM2302-Sensor.h>
#include <DFRobot_DF1201S.h>

//defines
AM2302::AM2302_Sensor am2302{0U};
DFRobot_DF1201S DFPlayer;

//macros
#define MAX_WATER_LEVEL 44
#define DF1201S_SERIAL Serial1

//variables and buffers
const char SSID[] = "Orange-yHhQGT-2G"; //Orange-yHhQGT-2G //307B
const char PASS[] = "xEbd9uP7G3K5Z59Gfk"; //xEbd9uP7G3K5Z59Gfk //Frigider007
WiFiConnectionHandler WiFiConnection(SSID, PASS);
float temperature;
float humidity;
int musicControl;
const int SoilSensorPin = A1;
const int PumpPin = 1;
int dryThreshold = 0;
int soilMoistureLevel;  
int rawValue;
int userSet = 0;
bool pumpControl = 0;
unsigned long pumpStartTime = 0;
unsigned long totalPumpTime = 0;
unsigned long pumpCheckTime = 0;
unsigned long lastPumpCheckTime = 0;
String messageText;
String userName;

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

//This function get the moisture humidity level
void getMoistureLevel(){
  rawValue = analogRead(SoilSensorPin);
  soilMoistureLevel = map(rawValue, 800, 200, 0, 100);  // 0 = ud, 100 = uscat
  Serial.print("Moisture: "); Serial.println(soilMoistureLevel);
  delay(1000);
}

void getRoomState(){
  if (am2302.read() == 0) { 
    temperature = am2302.get_Temperature();
    humidity = am2302.get_Humidity();
    Serial.print("Temp: "); Serial.println(temperature);
    Serial.print("Humidity: "); Serial.println(humidity);
  }
}

//This function return the current run-time in hours
int getCurrentHours() {
  return millis() / 3600000UL;
}

void checkSoilMoisture(){
  //Check for lack of water
  if (soilMoistureLevel < dryThreshold)
  {
    messageText = "Am fost dezhidratata. M-am alimentat.";
    pumpStartTime = millis()/1000;
    digitalWrite(PumpPin, HIGH);
    delay(3000);
    digitalWrite(PumpPin, LOW);
    totalPumpTime += ((millis()/1000) - pumpStartTime);
    //Check for water level of recipient
    if(totalPumpTime/1000 > MAX_WATER_LEVEL)
    {
      Serial.println("Nu mai am apa! Te rog umple tot rezervorul pentru apa!");
      messageText = "ATENTIE: Nu mai am apa! Te rog umple rezervorul de apa!";
      totalPumpTime = 0;
    }
  }
  else
  {
    messageText = "Am verficat nivelul umiditatii din solul meu si este OK.";
  }
}

void onChatMessageChange() {
  if(userSet < 2){
    if(userSet == 0){
      messageText = "Buna, eu sunt Planta ta inteligenta! Cum te numesti?";
    }
    int lastSpace = messageText.lastIndexOf(' ');
    if (lastSpace != -1 && lastSpace < messageText.length() - 1) {
      userName = messageText.substring(lastSpace + 1);
    } else {
      userName = messageText;
    }
    while (userName.length() > 0 && !isAlpha(userName.charAt(userName.length() - 1))) {
      userName.remove(userName.length() - 1);
    }
    if(userSet == 1){
      messageText = "Incantata de cunostiinta, " + String(userName) + "! Astept intrebarile tale despre incaperea in care ma aflu sau despre solul meu.";
    }
    userSet++;
    return;
  }
  String msg = messageText;
  msg.toLowerCase();
  if (msg.indexOf("verific") != -1){
    checkSoilMoisture();
  }
  else if (msg.indexOf("sol") != -1 || msg.indexOf("pamant") != -1) {
    messageText = "Solul este " + String(soilMoistureLevel) + "% ud, " + String(userName) + ".";
  }
  else if (msg.indexOf("camer") != -1 || msg.indexOf("incaper") != -1) {
    messageText = "Temperatura este de " + String(temperature, 1) + " grade Celsius, iar aerul este " + String(humidity, 1) + "% umed, " + String(userName) + ".";
  }
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
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
 
  ArduinoCloud.begin(WiFiConnection);
  ArduinoCloud.addProperty(musicControl, WRITE, ON_CHANGE, setMusicChanges);
  ArduinoCloud.addProperty(temperature, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(humidity, READ, ON_CHANGE, NULL);
  ArduinoCloud.addProperty(messageText, READWRITE, ON_CHANGE, onChatMessageChange);
  ArduinoCloud.addProperty(dryThreshold, WRITE, ON_CHANGE, NULL);
}

void loop() {
  ArduinoCloud.update();

  //Get room state
  getRoomState();

  //Get soil humidity
  getMoistureLevel();

  //Get activity hours of runtime
  pumpCheckTime = getCurrentHours();

  //Check if new hour begins
  if (pumpCheckTime > lastPumpCheckTime) {
    lastPumpCheckTime = pumpCheckTime;
    checkSoilMoisture();
  }
}

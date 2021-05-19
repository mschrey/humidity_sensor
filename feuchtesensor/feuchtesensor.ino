/*
 ESP12E--Client-Aussentemperatur

 Temperatur wird über angeschlossenen Sensor DS18B20 ausgelesen und per Url an einen Server uebertragen.
 Als Server (also Empfaenger) kann ebenfalls ein NodeMcu-Board verwendet werden.
 Ein Beispiel-Empfaenger empfehlen wir das Script "NodeMCU-Server-TFT-Temperatur" auf unser
 Projektseite. Dieses gibt die empfangene Temperatur als auch lokale Temperatur auf einem
 QVGA-Farbdisplay aus.
 Die Temperatur wird nur alle 15 Minuten übertragen um Energie zu sparen und Batterie
 Betrieb zu ermöglichen. Zwischendurch legt sich das Board schlafen.
 
 Temperatursensor DS18B20 an Pin D2 
 
 Bezugsquelle Temperatursensor: Reichelt / Conrad / Amazon - http://amzn.to/2i3WlRX 
 Bezugsquelle NodeMCU Board: http://amzn.to/2iRkZGi

 Notwendige Lib:
 https://github.com/milesburton/Arduino-Temperature-Control-Library
 
 Programm erprobt ab Arduino IDE 1.6.13
 Projektbeschreibung und weitere Beispiele unter https://www.mikrocontroller-elektronik.de/

 additional input for DHT-22 sensor from https://lastminuteengineers.com/esp8266-dht11-dht22-web-server-tutorial/
*/

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <base64.h>
#include "DHT.h"

#define DHTPIN D6
#define DHTTYPE DHT22

#define ID0 D1
#define ID1 D2
#define ID2 D5
#define ID3 D7
#define ADC A0

DHT dht(DHTPIN, DHTTYPE);

const char* ssid = "YOUR_WIFI_NAME";  //Hier SSID eures WLAN Netzes eintragen
const char* password = "YOUR_WIFI_PASSWORD"; //Hier euer Passwort des WLAN Netzes eintragen
const char* host = "IP_OF_WIFI_ACCESSPOINT"; //RaspberryPi - Server der die temperatur empfangen soll (Feste freie IP angeben)
const int sleepzeit_Min=10;          //Wieviel Minuten soll das Modul nach der Übermittlung der Daten schlafen
const int sleepzeitError_Min=1;      //Wieviel Minuten soll das Modul nach Verbindungsproblem schlafen

int battVoltage = 0;
int adcvalue = 0;

#define LED D4 //Interne Led auf dem NodeMCU Board LED_BUILTIN

void setup() {
  int timeout;
 
  //DS18B20.begin(); 
  Serial.begin(115200);
  delay(10);
  dht.begin();

  pinMode(ID0, INPUT);
  pinMode(ID1, INPUT);
  pinMode(ID2, INPUT);
  pinMode(ID3, INPUT);

  EEPROM.begin(512);
  double adc_corr_val = get_adc_corr_val();  
  adcvalue = analogRead(ADC);
  battVoltage = adc_corr_val * adcvalue;  //scaled to resistor divider 
    
  Serial.println();
  Serial.println();
  Serial.print("Verbinde mich mit Netz: ");
  Serial.println(ssid);
 
  WiFi.begin(ssid, password);

  timeout=40;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    timeout--;
    if (timeout<0){
      Serial.println("WLAN Verbindung kann nicht aufgebaut werden!");
      WiFi.disconnect(); 
      Serial.print("Schlafe jetzt ...");
      ESP.deepSleep(sleepzeitError_Min*60000000); //Angabe in Minuten
    }
   }

  Serial.println("");
  Serial.println("WiFi Verbindung aufgebaut"); 
  Serial.print("Eigene IP des ESP-Modul: ");
  Serial.println(WiFi.localIP());
  
}


int getID() {
  return digitalRead(ID0) + 2*digitalRead(ID1) + 4*digitalRead(ID2) + 8*digitalRead(ID3);
}


float getTemperatur() {
  float temp;
  int timeout = 30;

  do {
    temp = dht.readTemperature();
    timeout--;
    if (timeout<0) temp=99.9; //Wenn Sensor defekt
    delay(2000);    
  } while (isnan(temp));

  return temp;
}


float getRelHumidity() {
  float humidity;
  int timeout = 30;

  do {
    humidity = dht.readHumidity();
    timeout--;
    if (timeout<0) humidity=99.9; //Wenn Sensor defekt
    delay(2000);    
  } while (isnan(humidity)) ;
 
  return humidity;

}


double get_adc_corr_val() {
  int checkval;
  double adc_corr_val = 0;
  char corrvalStr[10];
  EEPROM.get(0, checkval);
  delay(100);
  if(checkval == 0xBADEAFFE) {
    EEPROM.get(4, adc_corr_val);
    delay(100);
    sprintf(corrvalStr, "%5.3f", adc_corr_val);
    Serial.print("ADC correction value is ");
    Serial.println(corrvalStr);
  } else {
    Serial.println("adc coefficient NOT found");
  }  
  return adc_corr_val;
}


//In unserem Beispiel wird die loop Schleife eigentlich nur einmal durchlaufen
void loop() {
  int idnr;
  char temperaturStr[6];
  char humidityStr[6];
  char battVoltageStr[10];
  char idStr[3];
  float humidity = getRelHumidity();  
  float temperatur = getTemperatur();  
  dtostrf(temperatur, 2, 1, temperaturStr);
  dtostrf(humidity, 4, 1, humidityStr);
  itoa(battVoltage, battVoltageStr, 10);
  idnr = getID();
  itoa(idnr, idStr, 10);
  
  Serial.print("ID: ");
  Serial.print(idStr);
  Serial.print(", Temperature: "); 
  Serial.print(temperaturStr); 
  Serial.print("°C, RelHumidity: ");
  Serial.print(humidityStr);
  Serial.print("%  BattVoltage: ");
  Serial.print(battVoltageStr);
  Serial.print("mV (");
  Serial.print(adcvalue);
  Serial.println(")");

  int versuche=1; 
  WiFiClient client;
  const int httpPort = 8034;
  int erg;
  do
  {
    Serial.print("Verbindungsaufbau zu Server ");
    Serial.println(host);

    erg =client.connect(host, httpPort);
    if (!erg) {
      versuche++; 
      Serial.println("Client Verbindungsaufbau nicht moeglich!!!");
      if (versuche>3) {
        Serial.println("Klappt nicht, ich versuche es spaeter noch mal!");
        client.stop();
        WiFi.disconnect(); 
        Serial.print("Schlafe jetzt ...");
        ESP.deepSleep(sleepzeitError_Min*60000000); //Angabe in Minuten
      }
    }
    delay(2000);
  } while (erg!=1);
 
  String command = "system=remoteHumiditySensor";
  command += ";idnr=";
  command += idnr;
  command += ";temperature=";
  command += temperaturStr;
  command += ";humidity=";
  command += humidityStr;
  command += ";voltage=";
  command += battVoltageStr;
  
  Serial.print("Folgendes Kommando wird übertragen: ");
  Serial.println(command);
  client.print(command);
  
  unsigned long timeout = millis();
  while (client.available() == 0) {
    if (millis() - timeout > 5000) {
      Serial.println(">>> Client Timeout !");
      Serial.println("Uebergabe klappt nicht, ich versuche es spaeter noch mal!");
      client.stop();
      WiFi.disconnect(); 
      Serial.print("Schlafe jetzt ...");
      ESP.deepSleep(sleepzeitError_Min*60000000); 
    }
  }

  Serial.print("Rueckgabe vom Server: ");
  while(client.available()){
    String line = client.readStringUntil('\r');
    Serial.print(line);
  }

  client.stop();
  WiFi.disconnect(); 
  Serial.println("\r\nVerbindung beendet");
 
  Serial.print("Schlafe jetzt ...");
  if(idnr >= 8) {
    ESP.deepSleep(10*1000000); 
  } else {
    ESP.deepSleep(sleepzeit_Min*60000000);
  }
  
}

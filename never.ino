//Alle Bibliotheken, welche ich für meine Hardware benötige
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include "Adafruit_BME680.h"
#include <DHT.h>
//zusätzliche Bibliotheken
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//Pin Setup des Wemos D1 Mini
#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13
#define D8 15

//wird nicht gebraucht,
//typdef meiner Node
typedef struct t_reading {
  float bme_temp;
  float bme_press;
  float bme_humid;
  float bmp_temp;
  float bmp_press;
  float dht_temp;
  float dht_humid;
  int time_min;
  int time_hour;
  struct t_reading * nxt;
} readings;

//Erstellen der Sensorobjekte mithilfe der Bibliotheken
Adafruit_BMP280 bmp;
Adafruit_BME680 bme;
DHT dht(D3, DHT22);

//Werte für den Webserver
const char * ssid = "Nokia";//Name/SSID des Routers //MartinRouterKing
const char * password = "12345678";//Passwort des Routers

//Serverobjekt auf Port 80 initialisieren
WiFiServer server(80);

//Debug Variablen
unsigned int loop_counter;
unsigned int time_counter;

//Deklarierung der globalen Variablen:

//In C lassen sich Strings nicht direkt bearbeiten, also verwende ich einen CharArray, welchen ich bei belieben manipuliere
char tmpString[80];
//Ein "Mülleimer" für Werte, wird für den Bme benötigt, da ich alle Werte auslesen muss, mich allerdings nicht alle interessieren
float trash;
//Die Werte, welche mich interessieren, speichere ich zuerst in eigenen Variablen
float temp_bme;
float humid_bme;
float press_bme;
//In diesem Array speichere ich alle Werte, die 7 sind dabei alle Messungen, davon 10, für die zeitliche Versetzung
float values[10][7];
//In diesem Array speichere ich den Zeitpunkt der Messung
int time_values[10][5];
//Gibt die Zeitdifferenz zwischen den Messungen in Millisekunden an
unsigned int delay_time;
//Serieller Input an den Controller, wird in dieser Variabel gespeichert
String cmd;
//Das Setupt wird zum Start des Programms einmalig ausgeführt
void setup() {
  delay(1000);
  Serial.begin(9600);
  buildSensorConnection();
  buildRouterConnection();
  buildTimeConnection();
  delay_time = 60000;
  loop_counter = 0;

  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP = ");
  Serial.println(WiFi.localIP());
  server.begin();
  
  for (int i = 0; i < 11; i++) {
    write_timings();
    write_readings();
    shift_readings();
    Serial.println(i);
    delay(delay_time);//1 Minute zwischen Messungen
  }
  Serial.println("done");
}
//Nach dem Setup, wird auf ewig die loop-Methode ausgeführt
void loop() {
  WiFiClient client = server.available();//time_counter = 0;
  if(!client){
    return;
  }
   while(!client.available()){
      delay(50);
   }
   
   realSite(client);
   //placeholderSite(client);
  
  /**
  while (client.connected()) {
    if (client.available()) {
      //placeholderSite(client);
      realSite(client);
    }
  }**/
  /**
    while(!client.available() || time_counter < 10000){//halbe Minute auf Client warten, dann neu messen/laden
    delay(1);
    time_counter++;
    }**/
  delay(500);


  if (Serial.available()) {
    cmd = Serial.readStringUntil('\n');
    if (cmd.equals("end")) {
      Serial.print("Bye");
      Serial.end();
    } else if (cmd.equals("read")) {
      write_readings();
      print_readings_serial();
    } else {
      Serial.println("Unknown command");
    }
  }
  loop_counter++;
}
//Nachdem ein Client verfügbar ist, wird seine Anfrage gelesen(aber ignoriert) und ihm wird die Seite gesendet
void realSite(WiFiClient c) {
  String request = c.readStringUntil('\r');

  c.flush();
  c.println("HTTP/1.1 200 OK");
  c.println("Content-type:text/html");
  c.println("Connection: close");
  c.println();
  htmljsCode(c);
  c.println();
}
//sprintf ist wie Java String.format(), ich schreibe in einen CharArray den String der Methode und tausche die % Werte durch die weiteren Parameter der Methode aus
void printbmetemp(WiFiClient c) {
  for (int i = 0; i < 10; i++) {
    sprintf(tmpString, "     { x: new Date(2019, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][0]);
    c.print(tmpString);
  }
}
void printbmepress(WiFiClient c) {
  for (int i = 0; i < 10; i++) {
    sprintf(tmpString, "     { x: new Date(2019, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][1]);
    c.print(tmpString);
  }
}
void printbmehumid(WiFiClient c) {
  for (int i = 0; i < 10; i++) {
    sprintf(tmpString, "     { x: new Date(2019, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][2]);
    c.print(tmpString);
  }
}
void printbmptemp(WiFiClient c) {
  for (int i = 0; i < 10; i++) {
    sprintf(tmpString, "     { x: new Date(2019, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][3]);
    c.print(tmpString);
  }
}
void printbmppress(WiFiClient c) {
  for (int i = 0; i < 10; i++) {
    sprintf(tmpString, "     { x: new Date(2019, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][4]);
    c.print(tmpString);
  }
}
void printdhttemp(WiFiClient c) {
  for (int i = 0; i < 10; i++) {
    sprintf(tmpString, "     { x: new Date(2019, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][5]);
    c.print(tmpString);
  }
}
void printdhthumid(WiFiClient c) {
  for (int i = 0; i < 10; i++) {
    sprintf(tmpString, "     { x: new Date(2019, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][6]);
    c.print(tmpString);
  }
}
//eine Testseite, welche zu Debugzwecken noch verwendet werden kann
void placeholderSite(WiFiClient c) {
  String request = c.readStringUntil('\r');
  c.flush();
  c.println("HTTP/1.1 200 OK");
  c.println("Content-type:text/html");
  c.println("Connection: close");
  c.println();
  c.println("<!DOCTYPE html><html>");
  //client.println("<head><meta http-equiv = \"Refresh\" content=\"10; url=https://www.youtube.com/watch?v=YiBTN5GE15w.com\" />");
  c.println("<head></head>");
  c.println("<body><h1>Wird noch gemessen</h1></body></html>");
  c.println();
}
//Verschiebt die Messungen in meinem Array eine Reihe weiter
void shift_readings() {
  for (int i = 8; i >= 0; i--) {
    for (int j = 0; j < 7; j++) {
      values[i + 1][j] = values[i][j];
    }
  }
}
//Verschiebt die Zeitpunkte in meinem Array eine Reihe weiter
void write_timings() {
  time_t t_now = time(nullptr);
  struct tm * t_obj = localtime(&t_now);
  for (int i = 8; i >= 0; i--) {
    for (int j = 0; j < 5; j++) {
      time_values[i + 1][j] = time_values[i][j];
    }
  }
  time_values[0][0] = t_obj->tm_mon;
  time_values[0][1] = t_obj->tm_mday;
  time_values[0][2] = (t_obj->tm_hour) - 2;
  time_values[0][3] = t_obj->tm_min;
  time_values[0][4] = t_obj->tm_sec;
}
//Schreibt die Messungen in die erste Reihe des Arrays
void write_readings() {
  temp_bme = bme.temperature;//der bme Sensor gibt keine bzw. ungültige Werte, wenn man von ohm nicht alles abfrägt
  press_bme = bme.pressure;  //deshalb schreibe ich die von mir nicht gebrauchten Werte in den "Mülleimer"
  humid_bme = bme.humidity;
  trash = bme.gas_resistance;
  trash = bme.readAltitude(1034.0);
  values[0][3] = bmp.readTemperature();
  values[0][4] = bmp.readPressure();
  values[0][0] = bme.temperature;//temp_bme;
  values[0][1] = (bme.pressure / 1000); //press_bme;
  values[0][2] = bme.humidity;//humid_bme;
  values[0][5] = dht.readTemperature();
  values[0][6] = dht.readHumidity();
}

//Debug Methode, welche durch einen einen seriellen Befehl die Werte der Sensoren ausgibt
void print_readings_serial() {
  sprintf(tmpString, "DHT:\n\t Temp: %f\n\tHumid: %f\n", values[0][5], values[0][6]);
  Serial.print(tmpString);
  sprintf(tmpString, "BMP:\n\t Temp: %f\n\tPress: %f\n", values[0][3], values[0][4]);
  Serial.print(tmpString);
  sprintf(tmpString, "BME:\n\t Temp: %f\n\tPress: %f\n\t Humid: %f", values[0][0], values[0][1], values[0][2]);
  Serial.print(tmpString);
}

//Setupfunktion, hier werden die Übertragungen zu den Sensoren gestartet
void buildSensorConnection() {
  dht.begin(9600);
  while (!bme.begin(118)) {
    delay(50);
    Serial.print("-");
  }
  while (!bmp.begin(119)) {
    delay(50);
    Serial.print("_");
  }
  //Setupparameter aus der Bibliothek
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
  //Setupparameter aus der Bibliothek
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);
}

//Stellt eine Verbindung zum Router her
void buildRouterConnection() {
  Serial.print("Connect to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
}
//Versucht eine Verbindung zu einem Server aufzubauen, welcher mir die die Zeit zurück gibt
void buildTimeConnection() {
  configTime((1) * 3600, 7200 , "pool.ntp.org", "time.nist.gov");
  while (!time(nullptr)) {
    Serial.println("Its not time yet");
    delay(100);
  }
}
//meine Hauptseite, die Datapoints werden durch seperate Methoden gesendet
void htmljsCode(WiFiClient c) {
  c.println(""
            "<!DOCTYPE HTML>\n"
            "<meta charset=\"UTF-8\">\n"
            "<html>\n"
            "<head>\n"
            "<script>\n"
            "window.onload = function () {\n"
            "  var pressureChart = new CanvasJS.Chart(\"press\", {\n"
            " title:{\n"
            "   text: \"Druck\"\n"
            " },\n"
            " \n"
            " axisX:{      \n"
            "            valueFormatString: \"HH-mm-ss\" ,   \n"
            "            lableAngle: -50\n"
            "        },\n"
            " axisY:[{\n"
            "   title: \"BMP\",\n"
            "   lineColor: \"#00FF7F\",//SpringGreen\n"
            "   tickColor: \"#00FF7F\",\n"
            "   labelFontColor: \"#00FF7F\",\n"
            "   suffix: \"kPa\",\n"
            "   minimum: 70 \n"
            " },\n"
            " ],\n"
            " axisY2: {\n"
            "   title: \"BME\",\n"
            "   lineColor: \"#00CD00\",//green3 \n"
            "   tickColor: \"#00CD00\",\n"
            "   labelFontColor: \"#00CD00\",\n"
            "   suffix: \"kPa\",\n"
            "   minimum: 70 \n"
            " },\n"
            " toolTip: {\n"
            "   shared: true\n"
            " },\n"
            " data: [\n"
            " {\n"
            "   type: \"line\",\n"
            "   name: \"BMP\",\n"
            "   color: \"#00FF7F\",\n"
            "   axisYIndex: 0,\n"
            "   showInLegend: true,\n"
            "   dataPoints: [\n");
  printbmppress(c);
  c.println(""
            "   ]\n"
            " },\n"
            " {\n"
            "   type: \"line\",\n"
            "   name: \"BME\",\n"
            "   color: \"#00CD00\",\n"
            "   axisYType: \"secondary\",\n"
            "   showInLegend: true,\n"
            "   dataPoints: [\n");
  printbmepress(c);
  c.println(""
            "   ]\n"
            " }]\n"
            "});\n"
            " var temperatureChart = new CanvasJS.Chart(\"temp\", {\n"
            " title:{\n"
            "   text: \"Temperatur\"\n"
            " },\n"
            " axisX:{      \n"
            "            valueFormatString: \"HH-mm-ss\" ,   \n"
            "            lableAngle: -50\n"
            "        },\n"
            " axisY:[{\n"
            "   title: \"BME\",\n"
            "   lineColor: \"#CDAF95\",//PeachPuff3\n"
            "   tickColor: \"#CDAF95\",\n"
            "   labelFontColor: \"#CDAF95\",\n"
            "   suffix: \"°C\",\n"
            "   minimum: 15\n"
            " },\n"
            " {\n"
            "   title: \"BMP\",\n"
            "   lineColor: \"#CD7054\",//salmon3\n"
            "   tickColor: \"#CD7054\",\n"
            "   labelFontColor: \"#CD7054\",\n"
            "   suffix: \"°C\",\n"
            "   minimum: 15\n"
            " }],\n"
            " axisY2: {\n"
            "   title: \"DHT\",\n"
            "   lineColor: \"#8B3E2F\",//corall4\n"
            "   tickColor: \"#8B3E2F\",\n"
            "   labelFontColor: \"#8B3E2F\",\n"
            "   suffix: \"°C\",\n"
            "   minimum: 15\n"
            " },\n"
            " toolTip: {\n"
            "   shared: true\n"
            " },\n"
            " legend: {\n"
            "   cursor: \"pointer\",\n"
            "   itemclick: toggleDataSeries\n"
            " },\n"
            " data: [{\n"
            "   type: \"line\",\n"
            "   name: \"BMP\",\n"
            "   color: \"#CD7054\",\n"
            "   showInLegend: true,\n"
            "   axisYIndex: 1,\n"
            "   dataPoints: [\n");
  printbmptemp(c);
  c.println(""
            "   ]\n"
            " },\n"
            " {\n"
            "   type: \"line\",\n"
            "   name: \"BME\",\n"
            "   color: \"#CDAF95\",\n"
            "   axisYIndex: 0,\n"
            "   showInLegend: true,\n"
            "   dataPoints: [\n");
  printbmetemp(c);
  c.println(""
            "   ]\n"
            " },\n"
            " {\n"
            "   type: \"line\",\n"
            "   name: \"DHT\",\n"
            "   color: \"#8B3E2F\",\n"
            "   axisYType: \"secondary\",\n"
            "   showInLegend: true,\n"
            "   dataPoints: [\n");
  printdhttemp(c);
  c.println(""
            "   ]\n"
            " }]\n"
            "});\n"
            "\n"
            "var humidityChart = new CanvasJS.Chart(\"humid\", {\n"
            " title:{\n"
            "   text: \"Feuchtigkeit\"\n"
            " },\n"
            " axisX:{      \n"
            "            valueFormatString: \"HH-mm-ss\" ,   \n"
            "            lableAngle: -50\n"
            "        },\n"
            " axisY:[{\n"
            "   title: \"BME\",\n"
            "   lineColor: \"#00BFFF\",//DeepSkyBlue\n"
            "   tickColor: \"#00BFFF\",\n"
            "   labelFontColor: \"#00BFFF\",\n"
            "   suffix: \"%\",\n"
            "   minimum: 20\n"
            " },\n"
            " ],\n"
            " axisY2: {\n"
            "   title: \"DHT\",\n"
            "   lineColor: \"#1C86EE\",//DodgerBlue2\n"
            "   tickColor: \"#1C86EE\",\n"
            "   labelFontColor: \"#1C86EE\",\n"
            "   suffix: \"%\",\n"
            "   minimum: 20\n"
            " },\n"
            " toolTip: {\n"
            "   shared: true\n"
            " },\n"
            " data: [\n"
            " {\n"
            "   type: \"line\",\n"
            "   name: \"BME\",\n"
            "   color: \"#00BFFF\",\n"
            "   axisYIndex: 0,\n"
            "   showInLegend: true,\n"
            "   dataPoints: [\n");
  printbmehumid(c);
  c.println(""
            "   ]\n"
            " },\n"
            " {\n"
            "   type: \"line\",\n"
            "   name: \"DHT\",\n"
            "   color: \"#1C86EE\",\n"
            "   axisYType: \"secondary\",\n"
            "   showInLegend: true,\n"
            "   dataPoints: [\n");
  printdhthumid(c);
  c.println(""
            "   ]\n"
            " }]\n"
            "});\n"
            "temperatureChart.render();\n"
            "humidityChart.render();\n"
            "pressureChart.render();\n"
            "\n"
            "function toggleDataSeries(e) {\n"
            " if (typeof (e.dataSeries.visible) === \"undefined\" || e.dataSeries.visible) {\n"
            "   e.dataSeries.visible = false;\n"
            " } else {\n"
            "   e.dataSeries.visible = true;\n"
            " }\n"
            " e.temperatureChart.render();\n"
            " e.humidityChart.render();\n"
            " e.pressureChart.render();\n"
            "}\n"
            "\n"
            "}\n"
            "</script>\n"
            "</head>\n"
            "<body>\n"
            "<div id=\"temp\" style=\"height: 300px; width: 100%;\"></div>\n"
            "<div id=\"press\" style=\"height: 300px; width: 100%;\"></div>\n"
            "<div id=\"humid\" style=\"height: 300px; width: 100%;\"></div>\n"
            "<script src=\"https://canvasjs.com/assets/script/canvasjs.min.js\"></script>\n"
            "</body>\n"
            "</html>\n");
}

//debug Methods

//fügt eine neue Struktur an meine Kette
void push(readings ** head, float bmetemp, float  bmepress, float bmehumid, float bmptemp, float bmppress, float dhttemp, float dhthumid, int tminut, int thour) {
  readings * nxt_reading;
  //nxt_reading =(struct t_reading *)malloc(sizeof(readings)); NICHT GUT

  nxt_reading->bme_temp = bmetemp;
  nxt_reading->bme_press = bmepress;
  nxt_reading->bme_humid = bmehumid;
  nxt_reading->bmp_temp = bmptemp;
  nxt_reading->bmp_press = bmppress;
  nxt_reading->dht_temp = dhttemp;
  nxt_reading->dht_humid = dhthumid;
  nxt_reading->time_min = tminut;
  nxt_reading->time_hour = thour;
  nxt_reading->nxt = *head;
  *head = nxt_reading;
}
//gibt seriell die Werte aus der Kette zurück
void printlisttemp(readings * head) {
  readings * current = head;
  while (current != NULL) {
    Serial.println(current->bme_temp);
    current = current->nxt;
  }
}
//gibt die Werte des Arrays auf dem seriellen Monitor aus
void printr() {
  for (int i = 0; i < 10; i++) {
    sprintf(tmpString, "i:%i  bmetemp:%f, bmptemp:%f, dhttemp:%f, dhthumid:%f", i, values[i][0], values[i][3], values[i][5], values[i][2]);
    Serial.println(tmpString);
  }
}

//!DISCLAIMER
//I'm both hosting the Webserver and reading/storing the Sensor-Data
//on a WEMOS D1 Mini, which ideally should be seperated

#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BMP280.h>
#include "Adafruit_BME680.h"
#include <DHT.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

//Pin Setup of the Wemos D1 Mini
#define D1 5
#define D2 4
#define D3 0
#define D4 2
#define D5 14
#define D6 12
#define D7 13
#define D8 15

Adafruit_BMP280 bmp;
Adafruit_BME680 bme;
DHT dht(D3, DHT22);

//SSID and PASSWORD for your local Network
const char * ssid = "mySSID";
const char * password = "myPassword";

WiFiServer server(80);

char tmpString[80];
float trash;
float temp_bme;
float humid_bme;
float press_bme;

unsigned int len;
float values[32][7];
int time_values[32][5];
unsigned int delay_time;
String cmd;
unsigned int loop_counter;
unsigned int minutestash;

void setup() {
  
  delay(1000);
  Serial.begin(9600);
  buildSensorConnection();
  buildRouterConnection();
  buildTimeConnection();
  delay_time = 60000;
  loop_counter = 0;
  len = 32;

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
    delay(delay_time);
  }
  Serial.println("done");
}

void loop() {
  //every Minute the Site is updated 
  checkMinute();
  WiFiClient client = server.available();
  if(!client){
    return;
  } 
  while(!client.available() && (minutestash == getMinute())){
      delay(1);
   }
   if(client.available()) realSite(client);

  if (Serial.available()) {
    cmd = Serial.readStringUntil('\n');
    if (cmd.equals("end")) {
      Serial.print("Bye");
      Serial.end();
    } else if (cmd.equals("read")) {
      write_timings();
      write_readings();
      shift_readings();
    } else {
      Serial.println("Unknown command");
    }
  }
}

//Setup the Connections to the Sensors
void buildSensorConnection() {
  //DTH22
  dht.begin(9600);
  while (!bme.begin(118)) {
    delay(50);
    Serial.print("-");
  }
  //BMP280
  while (!bmp.begin(119)) {
    delay(50);
    Serial.print("_");
  }
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL,
                  Adafruit_BMP280::SAMPLING_X2,
                  Adafruit_BMP280::SAMPLING_X16,
                  Adafruit_BMP280::FILTER_X16,
                  Adafruit_BMP280::STANDBY_MS_500);
  
  //BME680
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150);
}

//Connect to your local Network
void buildRouterConnection() {
  Serial.print("Connect to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    //should only be a high value if the expected latency is as well
    delay(200);
    Serial.print(".");
  }
}

void buildTimeConnection() {
  //needs to be adjusted for your local time(this is for Germany Summertime)
  configTime((1) * 3600, 7200 , "pool.ntp.org", "time.nist.gov");
  while (!time(nullptr)) {
    Serial.println("Its not time yet");
    delay(100);
  }
}

int getMinute(){
  time_t t_now = time(nullptr);
  struct tm * t_obj = localtime(&t_now);
  return t_obj->tm_min;
}
//check if a Minute has passed
void checkMinute(){
  if(minutestash == NULL || minutestash != getMinute()){
    Serial.println("New readings");
    write_timings();
    write_readings();
    shift_readings();
    minutestash = getMinute();
  }
}

void write_timings() {
  time_t t_now = time(nullptr);
  struct tm * t_obj = localtime(&t_now);
  for (int i = len-2; i >= 0; i--) {
    for (int j = 0; j < 5; j++) {
      time_values[i + 1][j] = time_values[i][j];
    }
  }
  time_values[0][0] = t_obj->tm_mon;
  time_values[0][1] = t_obj->tm_mday;
  time_values[0][2] = (t_obj->tm_hour) - 1;
  time_values[0][3] = t_obj->tm_min;
  time_values[0][4] = t_obj->tm_sec;
}

void write_readings() {
  //the bme680 return false values on the first reading
  //this is not how such an error should be handled, but it works for me
  temp_bme = bme.temperature;
  press_bme = bme.pressure;  
  humid_bme = bme.humidity;
  trash = bme.gas_resistance;
  trash = bme.readAltitude(1034.0);
  
  //writing the values in the array
  values[0][3] = bmp.readTemperature();
  values[0][4] = bmp.readPressure();
  values[0][0] = bme.temperature;
  values[0][1] = (bme.pressure / 1000); 
  values[0][2] = bme.humidity;
  values[0][5] = dht.readTemperature();
  values[0][6] = dht.readHumidity();
}

void shift_readings() {
  for (int i = len-2; i >= 0; i--) {
    for (int j = 0; j < 7; j++) {
      values[i + 1][j] = values[i][j];
    }
  }
}

void realSite(WiFiClient c) {
  String request = c.readStringUntil('\r');
  c.flush();
  c.println("HTTP/1.1 200 OK");
  c.println("Content-type:text/html");
  c.println("Connection: close");
  c.println("");
  htmljsCode(c);
  c.println();
}

//this should not be done like that
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
            "   suffix: \"Pa\",\n"
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
            "<p> Test </p>\n"
            "<div id=\"temp\" style=\"height: 300px; width: 100%;\"></div>\n"
            "<div id=\"press\" style=\"height: 300px; width: 100%;\"></div>\n"
            "<div id=\"humid\" style=\"height: 300px; width: 100%;\"></div>\n"
            "<script src=\"https://canvasjs.com/assets/script/canvasjs.min.js\"></script>\n"
            "</body>\n"
            "</html>");
}
void printbmetemp(WiFiClient c) {
  for (int i = 0; i < len; i++) {
    sprintf(tmpString, "     { x: new Date(2020, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][0]);
    c.print(tmpString);
  }
}
void printbmepress(WiFiClient c) {
  for (int i = 0; i < len; i++) {
    sprintf(tmpString, "     { x: new Date(2020, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][1]);
    c.print(tmpString);
  }
}
void printbmehumid(WiFiClient c) {
  for (int i = 0; i < len; i++) {
    sprintf(tmpString, "     { x: new Date(2020, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][2]);
    c.print(tmpString);
  }
}
void printbmptemp(WiFiClient c) {
  for (int i = 0; i < len; i++) {
    sprintf(tmpString, "     { x: new Date(2020, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][3]);
    c.print(tmpString);
  }
}
void printbmppress(WiFiClient c) {
  for (int i = 0; i < len; i++) {
    sprintf(tmpString, "     { x: new Date(2020, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][4]);
    c.print(tmpString);
  }
}
void printdhttemp(WiFiClient c) {
  for (int i = 0; i < len; i++) {
    sprintf(tmpString, "     { x: new Date(2020, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][5]);
    c.print(tmpString);
  }
}
void printdhthumid(WiFiClient c) {
  for (int i = 0; i < len; i++) {
    sprintf(tmpString, "     { x: new Date(2020, %i, %i, %i, %i, %i, 0), y: %f }, \n", time_values[i][0],
            time_values[i][1],
            time_values[i][2],
            time_values[i][3],
            time_values[i][4],
            values[i][6]);
    c.print(tmpString);
  }
}

//DEBUG
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

void print_readings_serial() {
  sprintf(tmpString, "DHT:\n\t Temp: %f\n\tHumid: %f\n", values[0][5], values[0][6]);
  Serial.print(tmpString);
  sprintf(tmpString, "BMP:\n\t Temp: %f\n\tPress: %f\n", values[0][3], values[0][4]);
  Serial.print(tmpString);
  sprintf(tmpString, "BME:\n\t Temp: %f\n\tPress: %f\n\t Humid: %f", values[0][0], values[0][1], values[0][2]);
  Serial.print(tmpString);
}

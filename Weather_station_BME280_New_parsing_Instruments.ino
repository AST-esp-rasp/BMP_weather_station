/*********
  Build by Andreas Stolpe based on 
  Rui Santos
  Complete project details at https://randomnerdtutorials.com/esp32-web-server-with-bme280-mini-weather-station/

// This sketch also draws BMP images pulled from SPIFFS onto the TFT. It is an
// an example from this library: https://github.com/Bodmer/TFT_eSPI

// Images in SPIFFS must be put in the root folder (top level) to be found
// Use the SPIFFS library example to verify SPIFFS works!

// The example image used to test this sketch can be found in the sketch
// Data folder, press Ctrl+K to see this folder. Use the IDE "Tools" menu
// option to upload the sketches data folder to the SPIFFS

// This sketch has been tested on the ESP32
*********/

// Load Wi-Fi library
#include <WiFi.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <Adafruit_Sensor.h>
#include <WiFiManager.h>
#include "DTime.h"
#include <HTTPClient.h>
//Andreas Stolpe
//
#include "Free_Fonts.h" // Include the header file attached to this sketch
#include <ArduinoJson.h>
#include <JPEGDecoder.h>
#include "SPIFFS.h"
#include "analog_meter.h"

//time stuff
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
//time stuff
const int ledPin = 5;

// Call up the TFT library
#include <TFT_eSPI.h> // Hardware-specific library for ESP8266

// Invoke TFT library
//TFT_eSPI tft = TFT_eSPI();

WiFiClient wifiClient; // wifi client object
String con1= "http://192.168.2.139/solar_api/v1/GetPowerFlowRealtimeData.fcgi";
String con2= "http://api.thingspeak.com/channels/216948/fields/1/last.json?";
String con3= "http://api.openweathermap.org/data/2.5/weather?id=3249083&lang=en&units=metric&APPID=a6f335a6da5861174b55efae32d16dee";
//http://192.168.2.139/solar_api/v1/GetInverterRealtimeData.cgi?Scope=Device&DeviceId=1&DataCollection=CommonInverterData


String time_to_TFT;
String T_OUT;

int n = 0;
int c1 =0;
String OPWeather_temp, OPWeather_pres, OPWeather_humi, OPWeather_bmp;

String sol_production, sol_autonomy, sol_day_production, sol_battery_status, sol_consumption; 
int Body_Data_Inverters_1_SOC;
int Body_Data_Site_P_PV;
int Body_Data_Inverters_1_P;

float pressure;
//#define SEALEVELPRESSURE_HPA (1013.25)
#define I2C_SDA 32 //A4
#define I2C_SCL 33 //A5

Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Button start position, Button sizes and spacing
//#define KEY_X 60 // Centre of key
#define KEY_Y 420
#define KEY_W 100 // Width and height
#define KEY_H 50
#define KEY_SPACING_X 20 // X and Y gap
#define KEY_SPACING_Y 20
#define KEY_TEXTSIZE 0.5   // Font size multiplier

// Create 2 keys for the keypad
char keyLabel[2][5] = {"<<",">>"};
uint16_t keyColor[3] = {TFT_BLUE, TFT_BLUE, TFT_BLUE};
TFT_eSPI_Button key[3];

int getWifiQuality() {
  int dbm = WiFi.RSSI();
  if(dbm <= -100) {
      return 0;
  } else if(dbm >= -50) {
      return 100;
  } else {
      return 2 * (dbm + 100);
  }
}
void wifi_quality() {
  
    int quality = getWifiQuality();
  for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8 * (i + 1); j++) {
      if (quality > i * 25 || j == 0) {
        tft.drawPixel(120 + 2 * i, 61 - j,TFT_BLUE);
        
      }
    }
  }
}

//Time function
void printLocalTime(){
  struct tm timeinfo;
  
  int xpos = tft.width() / 2; // Half the screen width
  int ypos = 55;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  char timeStringBuff[50]; //50 chars should be enough
  strftime(timeStringBuff, sizeof(timeStringBuff), "%H:%M", &timeinfo);
  //print like "const char*"
//  Serial.println(timeStringBuff);
  
//  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
//  Serial.println(&timeinfo, "%H:%M");
  if (time_to_TFT != timeStringBuff){
  tft.fillRect(0, 0, tft.width(), 48, TFT_WHITE);
  time_to_TFT = timeStringBuff;  
  }
  
  tft.drawString(timeStringBuff, xpos, ypos - tft.fontHeight(GFXFF), GFXFF);  
}

//Time function
//wifimanager setupfunktion
void WM_SETUP(){

    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;
    
    //reset settings - wipe credentials for testing
    //wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    bool res;
    // res = wm.autoConnect(); // auto generated AP name from chipid
    // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
    res = wm.autoConnect("AutoConnectAP","password"); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }
}
String GetDatafromHTTP(String connection){
  String payload;
  if ((WiFi.status() == WL_CONNECTED)) { //Check the current connection status
   
    HTTPClient http;
 
    http.begin(connection); //Specify the URL
    int httpCode = http.GET();                                        //Make the request
 
    if (httpCode > 0) { //Check for the returning code
 
      payload = http.getString();
      
//        Serial.println(httpCode);
//        Serial.println(payload);
      }
 
    else {
      Serial.println("Error on HTTP request");
    }
 
    http.end(); //Free the resources
  }
return payload;
}


void Get_data_from_thingspeak(){
 String p = GetDatafromHTTP(con2);
 const size_t capacity = JSON_OBJECT_SIZE(3) + 116;
DynamicJsonBuffer jsonBuffer(capacity);
JsonObject& root = jsonBuffer.parseObject(p);
const char* created_at = root["created_at"]; // "2019-02-23T17:46:44Z"
long entry_id = root["entry_id"]; // 89181
const char* field1 = root["field1"]; // "-4.68"
Serial.println("Temperature: "+String(field1) + "°C");
Serial.println("Date/Time"+String(created_at));
T_OUT = String(field1) + "C";
}


void Get_data_from_solar(){
  String p = GetDatafromHTTP(con1);

const size_t capacity = JSON_OBJECT_SIZE(0) + 2*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(2) + 3*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(7) + JSON_OBJECT_SIZE(12) + 1035;
DynamicJsonBuffer jsonBuffer(capacity);

  
JsonObject& root = jsonBuffer.parseObject(p);
JsonObject& Body_Data = root["Body"]["Data"];

JsonObject& Body_Data_Inverters_1 = Body_Data["Inverters"]["1"];
const char* Body_Data_Inverters_1_Battery_Mode = Body_Data_Inverters_1["Battery_Mode"]; // "normal"
int Body_Data_Inverters_1_E_Day = Body_Data_Inverters_1["E_Day"]; // 15352
Body_Data_Inverters_1_P = Body_Data_Inverters_1["P"]; // 672
Body_Data_Inverters_1_SOC = Body_Data_Inverters_1["SOC"]; // 64

JsonObject& Body_Data_Site = Body_Data["Site"];
bool Body_Data_Site_BatteryStandby = Body_Data_Site["BatteryStandby"]; // false
Body_Data_Site_P_PV = Body_Data_Site["P_PV"]; // 0
int Body_Data_Site_rel_Autonomy = Body_Data_Site["rel_Autonomy"]; // 100
Serial.println("Production: "+String(Body_Data_Site_P_PV) + " W");
sol_production= "Pro: "+String(Body_Data_Site_P_PV) + " W";
Serial.println("Autonomy: "+String(Body_Data_Site_rel_Autonomy) + " %");
sol_autonomy= "Aut: "+String(Body_Data_Site_rel_Autonomy) + " %";
Serial.println("Day Production: "+String(Body_Data_Inverters_1_E_Day) + " Wh");
sol_day_production = "DPr: "+String(Body_Data_Inverters_1_E_Day) + " Wh";
Serial.println("Battery: "+String(Body_Data_Inverters_1_SOC) + " %");
sol_battery_status="Bat: "+String(Body_Data_Inverters_1_SOC) + " %" ;
Serial.println("Battery Mode: "+String(Body_Data_Inverters_1_Battery_Mode));
Serial.println("Consumtion: "+String(Body_Data_Inverters_1_P) + " W");
sol_consumption="Cons: "+String(Body_Data_Inverters_1_P) + " W"; 
}

void Get_data_from_Openweather(){
const size_t capacity = JSON_ARRAY_SIZE(1) + JSON_OBJECT_SIZE(1) + 2*JSON_OBJECT_SIZE(2) + JSON_OBJECT_SIZE(4) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(6) + JSON_OBJECT_SIZE(12) + 949;
DynamicJsonBuffer jsonBuffer(capacity);
String json = GetDatafromHTTP(con3);
JsonObject& root = jsonBuffer.parseObject(json);

float coord_lon = root["coord"]["lon"]; // 14.18
float coord_lat = root["coord"]["lat"]; // 52.64

JsonObject& weather_0 = root["weather"][0];
int weather_0_id = weather_0["id"]; // 800
const char* weather_0_main = weather_0["main"]; // "Clear"
const char* weather_0_description = weather_0["description"]; // "clear sky"
const char* weather_0_icon = weather_0["icon"]; // "01n"
const char* base = root["base"]; // "stations"
OPWeather_bmp = "/"+ String(weather_0_icon) + ".bmp";
Serial.println(OPWeather_bmp);

Serial.println("Icon description: "+String(weather_0_description) );

JsonObject& main = root["main"];
float main_temp = main["temp"]; // 7.51
int main_pressure = main["pressure"]; // 1023
int main_humidity = main["humidity"]; // 75
int main_temp_min = main["temp_min"]; // 7
int main_temp_max = main["temp_max"]; // 8

OPWeather_temp=String(main_temp)+ "C";
OPWeather_pres=String(main_pressure)+ "hPa";
OPWeather_humi=String(main_humidity)+ "%";

Serial.println("Temperature OPWM: "+String(main_temp)+" °C" );
Serial.println("Pressure OPWM: "+String(main_pressure)+" hPa" );
Serial.println("Humidity: "+String(main_humidity)+" %" );
int visibility = root["visibility"]; // 10000

float wind_speed = root["wind"]["speed"]; // 3.6
int wind_deg = root["wind"]["deg"]; // 250

int clouds_all = root["clouds"]["all"]; // 0

long dt = root["dt"]; // 1550688600

JsonObject& sys = root["sys"];
int sys_type = sys["type"]; // 1
int sys_id = sys["id"]; // 1262
float sys_message = sys["message"]; // 0.0033
const char* sys_country = sys["country"]; // "DE"

long sys_sunrise = sys["sunrise"]; // 1550642921
long sys_sunset =  sys["sunset"]; // 1550679964
DTime c(sys_sunrise);
//Serial.println("Sunrise:"+String(hour(sys_sunrise))+":"+String(minute(sys_sunrise)));        
//Serial.println("Sunset:"+String(hour(sys_sunset))+":"+String(minute(sys_sunset)));
  Serial.print(c.year);
  Serial.print("-");
  Serial.print(c.month);
  Serial.print("-");
  Serial.print(c.day);
  Serial.print("  at  ");
  Serial.print(c.hour);
  Serial.print(":");
  Serial.print(c.minute);

long id = root["id"]; // 3249083
const char* name = root["name"]; // "Landkreis Märkisch-Oderland"
int cod = root["cod"]; // 200
}

//void drawKeypad()
//{
//  // Draw the keys
//  for (uint8_t row = 0; row < 1; row++) {
//    for (uint8_t col = 0; col < 2; col++) {
//      uint8_t b = col + row * 3;
//      
//      key[b].initButton(&tft, (tft.width() / 2 - KEY_W / 2) + col * (KEY_W + KEY_SPACING_X),
//                        KEY_Y + row * (KEY_H + KEY_SPACING_Y), // x, y, w, h, outline, fill, text
//                        KEY_W, KEY_H, TFT_WHITE, keyColor[b], TFT_WHITE,
//                        keyLabel[b], KEY_TEXTSIZE);
//      key[b].drawButton();
//    }
//  }
//}



//====================================================================================
//                                    Setup
//====================================================================================
void setup() {
  Serial.begin(115200);
  pinMode (ledPin, OUTPUT);
  
  WM_SETUP();
  WiFi.setHostname("Weatherstation");
  bool status;
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialisation failed!");
    while (1) yield(); // Stay here twiddling thumbs waiting
  }
  Serial.println("\r\nSPIFFS initialised.");

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  //status = bme.begin();
  Wire.begin(I2C_SDA,I2C_SCL);  
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }


  updateTime = millis(); // Next update time

 //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  
server.begin();
  Serial.println("server started");
//init TFT
  uint16_t calData[5] = { 351, 3462, 308, 3610, 0 };
  tft.begin();
  tft.setRotation(2);  // 0 & 2 Portrait. 1 & 3 landscape
  tft.fillScreen(TFT_WHITE);
  
  tft.setFreeFont(FSB24);
  tft.setTouch(calData);

  printLocalTime();
  Serial.println("tft started");
  drawKeypad();
  wifi_quality(); 
}


//====================================================================================
//                                    Loop
//====================================================================================
void loop(){

//c1=c1+1;
//n=n+1; 
uint16_t t_x = 0, t_y = 0; // To store the touch coordinates
int BTN_Press;
  int x = tft.width() / 2 - 64;
  int y = 200;
  int xpos ;
  int ypos ;
  tft.setTextDatum(TC_DATUM); // Centre text on x,y position
  tft.setTextColor(TFT_BLUE,TFT_WHITE);   
  xpos = tft.width() / 2; // Half the screen width
  ypos = 50;
boolean pressed = tft.getTouch(&t_x, &t_y);
for (uint8_t b = 0; b < 2; b++) 
    {
    if (pressed && key[b].contains(t_x, t_y)) 
      {
       BTN_Press = b ;  // tell the button it is pressed
      } 
    }

if (pressed){
  if (BTN_Press==0){
  digitalWrite (ledPin, LOW);
  n=n-1;
}
if (BTN_Press==1){
  digitalWrite (ledPin, HIGH);
  n=n+1;
}  
 delay(10);

 


    
//n=n+1;
if (n==4){
    n=0;  
}
if (n==-1){
    n=3;  
}
tft.fillScreen(TFT_WHITE);


if (n==0){
  Get_data_from_Openweather();
  tft.drawString(OPWeather_temp, xpos, ypos, GFXFF);
  tft.drawString(OPWeather_pres, xpos, ypos + tft.fontHeight(GFXFF), GFXFF);
  tft.drawString(OPWeather_humi, xpos, ypos + 2*tft.fontHeight(GFXFF), GFXFF);
  drawBmp(OPWeather_bmp.c_str(), x, y);
  drawKeypad(); 
}
if (n==1){
 
  String T_Indoor = String(bme.readTemperature()-3.5)+ "C";
  String P_Indoor = String(int(bme.readPressure() / 100.0F))+"hPa";
  String H_Indoor = String(bme.readHumidity())+ "%";

  tft.drawString(T_Indoor, xpos, ypos, GFXFF);
  tft.drawString(P_Indoor, xpos, ypos + tft.fontHeight(GFXFF), GFXFF);
  tft.drawString(H_Indoor, xpos, ypos + 2*tft.fontHeight(GFXFF), GFXFF);
  drawBmp("/House.bmp", x, y);
  drawSdJpeg("/test.jpg", x, y);
  drawKeypad(); 
}
// screen 1
if (n==2){
  Get_data_from_thingspeak();
  tft.drawString(T_OUT, xpos, ypos, GFXFF);  // Draw the text string in the selected GFX free font
  drawBmp("/tent.bmp", x, y); 
  drawKeypad(); 
// n = -1;  
}
if (n==3){
  Get_data_from_solar();
//  tft.drawString(sol_production, xpos, ypos, GFXFF);
//  tft.drawString(sol_autonomy, xpos, ypos + tft.fontHeight(GFXFF), GFXFF);
//  tft.drawString(sol_day_production, xpos, ypos + 2*tft.fontHeight(GFXFF), GFXFF);
//  tft.drawString(sol_battery_status, xpos, ypos + 3*tft.fontHeight(GFXFF), GFXFF);
//  tft.drawString(sol_consumption, xpos, ypos + 4*tft.fontHeight(GFXFF), GFXFF);
  analogMeter(0,0,100,"%","Battery"); // Draw analogue meter
  analogMeter(0,160,6,"kW","Prod.");
  analogMeter(0,320,6,"kW","Consum.");
  value1[0] = Body_Data_Site_P_PV/60;
  value1[1] = Body_Data_Inverters_1_SOC;
  value1[2] = Body_Data_Inverters_1_P/60;
  plotNeedle1(0, 0, 160,"Prod.",0);// It takes between 2 and 12ms to replot the needle with zero delay
  plotNeedle1(0, 0, 0,"Battery",1);
  plotNeedle1(0, 0, 320,"Consum.",2);
  updateTime = millis(); // Next update time
//  drawKeypad();  
}
Serial.println("Pressed");

}//End if pressed


//Serial.println(n);
  printLocalTime();

  
  WiFiClient wifiClient = server.available();   // Listen for incoming clients

  if (wifiClient) {                             // If a new client connects,
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (wifiClient.connected()) {            // loop while the client's connected
      if (wifiClient.available()) {             // if there's bytes to read from the client,
        char c = wifiClient.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            wifiClient.println("HTTP/1.1 200 OK");
            wifiClient.println("Content-type:text/html");
            wifiClient.println("Connection: close");
            wifiClient.println();
            
            // Display the HTML web page
            wifiClient.println("<!DOCTYPE html><html>");
            wifiClient.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            wifiClient.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the table 
            wifiClient.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial;}");
            wifiClient.println("table { border-collapse: collapse; width:35%; margin-left:auto; margin-right:auto; }");
            wifiClient.println("th { padding: 12px; background-color: #0043af; color: white; }");
            wifiClient.println("tr { border: 1px solid #ddd; padding: 12px; }");
            wifiClient.println("tr:hover { background-color: #bcbcbc; }");
            wifiClient.println("td { border: none; padding: 12px; }");
            wifiClient.println(".sensor { color:white; font-weight: bold; background-color: #bcbcbc; padding: 1px; }");
            
            // Web Page Heading
            wifiClient.println("</style></head><body><h1>ESP32 Weather Station</h1>");
            wifiClient.println("<table><tr><th>MEASUREMENT</th><th>VALUE</th></tr>");
            wifiClient.println("<tr><td>Temp. Celsius</td><td><span class=\"sensor\">");
            wifiClient.println(bme.readTemperature()-3.5);
            wifiClient.println(" *C</span></td></tr>");  
            wifiClient.println("<tr><td>Temp. Fahrenheit</td><td><span class=\"sensor\">");
            wifiClient.println(1.8 * bme.readTemperature() + 32);
            wifiClient.println(" *F</span></td></tr>");       
            wifiClient.println("<tr><td>Pressure</td><td><span class=\"sensor\">");
            wifiClient.println(bme.readPressure() / 100.0F);
            wifiClient.println(" hPa</span></td></tr>");
            wifiClient.println("<tr><td>Approx. Altitude</td><td><span class=\"sensor\">");
            wifiClient.println(bme.readAltitude(pressure));
            wifiClient.println(" m</span></td></tr>"); 
            wifiClient.println("<tr><td>Humidity</td><td><span class=\"sensor\">");
            wifiClient.println(bme.readHumidity());
            wifiClient.println(" %</span></td></tr>");
            wifiClient.println("<tr><td>Outside Temperature Thinkspeak</td><td><span class=\"sensor\">");
            wifiClient.println(T_OUT);
            wifiClient.println(" %</span></td></tr>");
            wifiClient.println("<tr><td>Openweathermap data</td><td><span class=\"sensor\">");
            wifiClient.println(OPWeather_temp);
            wifiClient.println(OPWeather_pres);
            wifiClient.println(OPWeather_humi);
            wifiClient.println("</body></html>");
            
            // The HTTP response ends with another blank line
            wifiClient.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
//   
    }
    // Clear the header variable
    header = "";
    wifiClient.stop();
    
  }
  delay(100);
  
}

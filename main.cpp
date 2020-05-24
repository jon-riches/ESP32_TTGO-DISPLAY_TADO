/*
Written by Jonathan Riches, inspired by many. Apologies for anyone's code portions I used and haven't credited!
This code isn't great, but it does work...
References: -
https://circuits4you.com 
https://arduinojson.org/v6/api/json/deserializejson/
https://shkspr.mobi/blog/2019/02/tado-api-guide-updated-for-2019/
http://blog.scphillips.com/posts/2017/01/the-tado-api-v2/
The Espressif / Arduino documentation / examples
https://github.com/esp8266/Arduino/tree/master/libraries 
*/

#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h> // https://arduinojson.org/

// REQUIRES the following Arduino libraries:
// - DHT Sensor Library: https://github.com/adafruit/DHT-sensor-library
// - Adafruit Unified Sensor Lib: https://github.com/adafruit/Adafruit_Sensor
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <TFT_eSPI.h>
#include <SPI.h>
//#include <Wire.h>
//#include <Button2.h>

#ifndef TFT_DISPOFF
#define TFT_DISPOFF 0x28
#endif

#ifndef TFT_SLPIN
#define TFT_SLPIN 0x10
#endif

#define TFT_MOSI 19
#define TFT_SCLK 18
#define TFT_CS 5
#define TFT_DC 16
#define TFT_RST 23

#define TFT_BL 4 // Display backlight control pin
#define ADC_EN 14
#define ADC_PIN 34
#define BUTTON_1 35
#define BUTTON_2 0

#define DHTPIN 22     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22 // Sensor Type DHT 22 (AM2302)

TFT_eSPI tft = TFT_eSPI(135, 240); // Invoke custom library
//Button2 btn1(BUTTON_1);
//Button2 btn2(BUTTON_2);

DHT_Unified dht(DHTPIN, DHTTYPE);

//##################################################################################################################
/* Set these to your desired credentials. */

// WiFi
const char *ssid = "xxxxxx"; // "your-ssid";
const char *password = "xxxxxx"; // "your_password";

// Tado
const char *tlogin = "xxxxx"; // "your_tado_login";
const char *tpassword = "xxxxx";        // "your_tado_password";
String thome = "xxxxx";                        // "your_home_id";
String tzone = "x";                            // "your_tado_zone"
String tdevice = "xxxxxx";               // "your_tado_device";
String room = "Kitchen";                 // "your_room_name"; // Display purposes only
float DHTadjust = -0.4;                        // Error correction for adjustment of DHT 22 - this value is added to sensor reading
const float mindiff = -0.5;                    // Difference below which offset will be adjusted
const float maxdiff = 1.0;                     // Difference above which offset will be adjusted

// End of credential / user setup block
//##################################################################################################################

float actTemp = 20;        // Actual temperature of the room from DHT22 Sensor
float txoffset = 0;        //  Existing offset value returned by Tado
float toffset = 0;         // Offset value to send to Tado
float tsensor = 0;         // True Tado sensor reading before offset
char toffsetS[] = "T0000"; // Converted value to actually submit to Tado
bool poffset = 1;          // Whether to push offset

//Web/Server address to read/write from

const char *host = "auth.tado.com";
const char *host2 = "my.tado.com";
const int httpsPort = 443;

// Set root CA
const char *root_ca =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIEDzCCAvegAwIBAgIBADANBgkqhkiG9w0BAQUFADBoMQswCQYDVQQGEwJVUzEl\n"
    "MCMGA1UEChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMp\n"
    "U3RhcmZpZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMDQw\n"
    "NjI5MTczOTE2WhcNMzQwNjI5MTczOTE2WjBoMQswCQYDVQQGEwJVUzElMCMGA1UE\n"
    "ChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMpU3RhcmZp\n"
    "ZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwggEgMA0GCSqGSIb3\n"
    "DQEBAQUAA4IBDQAwggEIAoIBAQC3Msj+6XGmBIWtDBFk385N78gDGIc/oav7PKaf\n"
    "8MOh2tTYbitTkPskpD6E8J7oX+zlJ0T1KKY/e97gKvDIr1MvnsoFAZMej2YcOadN\n"
    "+lq2cwQlZut3f+dZxkqZJRRU6ybH838Z1TBwj6+wRir/resp7defqgSHo9T5iaU0\n"
    "X9tDkYI22WY8sbi5gv2cOj4QyDvvBmVmepsZGD3/cVE8MC5fvj13c7JdBmzDI1aa\n"
    "K4UmkhynArPkPw2vCHmCuDY96pzTNbO8acr1zJ3o/WSNF4Azbl5KXZnJHoe0nRrA\n"
    "1W4TNSNe35tfPe/W93bC6j67eA0cQmdrBNj41tpvi/JEoAGrAgEDo4HFMIHCMB0G\n"
    "A1UdDgQWBBS/X7fRzt0fhvRbVazc1xDCDqmI5zCBkgYDVR0jBIGKMIGHgBS/X7fR\n"
    "zt0fhvRbVazc1xDCDqmI56FspGowaDELMAkGA1UEBhMCVVMxJTAjBgNVBAoTHFN0\n"
    "YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4xMjAwBgNVBAsTKVN0YXJmaWVsZCBD\n"
    "bGFzcyAyIENlcnRpZmljYXRpb24gQXV0aG9yaXR5ggEAMAwGA1UdEwQFMAMBAf8w\n"
    "DQYJKoZIhvcNAQEFBQADggEBAAWdP4id0ckaVaGsafPzWdqbAYcaT1epoXkJKtv3\n"
    "L7IezMdeatiDh6GX70k1PncGQVhiv45YuApnP+yz3SFmH8lU+nLMPUxA2IGvd56D\n"
    "eruix/U0F47ZEUD0/CwqTRV/p2JdLiXTAAsgGh1o+Re49L2L7ShZ3U0WixeDyLJl\n"
    "xy16paq8U4Zt3VekyvggQQto8PT7dL5WXXp59fkdheMtlb71cZBDzI0fmgAKhynp\n"
    "VSJYACPq4xJDKVtHCN2MQWplBqjlIapBtJUhlbl90TSrE9atvNziPTnNvT51cKEY\n"
    "WQPJIrSPnNVeKtelttQKbfi3QBFGmh95DmK/D5fs4C8fF5Q=\n"
    "-----END CERTIFICATE-----\n";

//=======================================================================
//                    Power on setup
//=======================================================================

void setup()
{
  dht.begin();
  // Print temperature sensor details.
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  delay(1000);
  Serial.begin(115200);
  WiFi.mode(WIFI_OFF); //Prevents reconnection issue (taking too long to connect)
  delay(1000);
  WiFi.mode(WIFI_STA); //Only Station No AP, This line hides the viewing of ESP as wifi hotspot

  WiFi.begin(ssid, password); //Connect to your WiFi router
  Serial.println("");

  Serial.print("Connecting");
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  //If connection successful show IP address in serial monitor
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); //IP address assigned to your ESP

  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE);
  tft.setCursor(0, 0);
  tft.setTextDatum(MC_DATUM);
  tft.setTextSize(2);

  if (TFT_BL > 0)
  {                                         // TFT_BL has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
    pinMode(TFT_BL, OUTPUT);                // Set backlight pin to output mode
    digitalWrite(TFT_BL, TFT_BACKLIGHT_ON); // Turn backlight on. TFT_BACKLIGHT_ON has been set in the TFT_eSPI library in the User Setup file TTGO_T_Display.h
  }
}

//=======================================================================
//                    Main Program Loop
//=======================================================================
void loop()
{
  String Ttoken;                //Declare string to hold Tado authentication token
  WiFiClientSecure httpsClient; //Declare object of class WiFiClient

  Serial.println("Start of loop ##################################################################### \r\n");
  Serial.print("Zone ID:");
  Serial.println(tzone);
  Serial.print("Device ID:");
  Serial.println(tdevice);
  //#######################################################################################################################################
  // Read local device temperature, set actTemp

  sensors_event_t event;

  dht.temperature().getEvent(&event);
  if (isnan(event.temperature))
  {
    Serial.println(F("Error reading temperature!"));
  }
  else
  {
    // Serial.print(F("Temperature: "));
    // Serial.print(event.temperature);
    // Serial.println(F("°C"));
  }
  actTemp = (event.temperature + DHTadjust);

  //  Serial.print(F("actTemp: "));
  // Serial.print(actTemp);
  // Serial.println(F("°C"));

  // //##################################################################################################################################
  // //Obtain Authentication Token from Tado

  Serial.println("POST Authentication, receive Token");
  Serial.println(host);

  httpsClient.setCACert(root_ca);
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);

  Serial.print("HTTPS Connecting... ");
  int r = 0; //retry counter
  while ((!httpsClient.connect(host, httpsPort)) && (r < 30))
  {
    delay(100);
    Serial.print(".");
    r++;
  }
  if (r == 30)
  {
    Serial.println("Connection failed");
  }
  else
  {
    Serial.println("Connected to web");
  }

  String getData, Link;

  //POST Data
  // Link = "/post";
  Link = "/oauth/token";
  Serial.print("requesting URL: ");
  Serial.println(host);

  String httpBody = (String("client_id=tado-web-app&grant_type=password&scope=home.user&username=") +
                     tlogin + "&password=" + tpassword +
                     "&client_secret=wZaRN7rpjn3FoNyF5IFuxg9uMzYJcvOoQ8QWiIqS3hfk6gLhVlG57j5YNoZL2Rtc");

  int httpBodyLength = httpBody.length();
  //   Serial.print("httpBodyLength: ");
  //   Serial.println(httpBodyLength);

  String httpText = (String("POST ") + Link + " HTTP/1.1\r\n" +
                     "Host: " + host + "\r\n" +
                     "Content-Type: application/x-www-form-urlencoded" + "\r\n" +
                     "Accept: */*" + "\r\n" +
                     "Cache-Control: no-cache" + "\r\n" +
                     "Host: auth.tado.com" + "\r\n" +
                     "Content-Length: " + httpBodyLength + "\r\n" +
                     "Connection: keep-alive" + "\r\n" +
                     +"\r\n" +
                     httpBody + "Connection: close\r\n\r\n");

  httpsClient.print(httpText);

  Serial.println("HTTP request is: ");
  Serial.println(httpText);

  Serial.print("request sent... ");

  while (httpsClient.connected())
  {
    String line = httpsClient.readStringUntil('\n');
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }

  String linez;
  String lineT;

  linez = httpsClient.readStringUntil('\n'); //Read Line by Line
  lineT = httpsClient.readStringUntil('\n'); //Read Line by Line

  Ttoken = lineT.substring(17, 916);

  Serial.println("==========");

  // Serial.println("Token Value:");
  // Serial.println(Ttoken);
  httpsClient.stop();

  //########################################################################################################################################
  // GET thermostat temperature from Tado
  Serial.println("GET thermostat temperature from Tado");
  Serial.println(host2);

  httpsClient.setCACert(root_ca);
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);

  Serial.print("HTTPS Connecting... ");
  int rr = 0; //retry counter
  while ((!httpsClient.connect(host2, httpsPort)) && (rr < 30))
  {
    delay(100);
    Serial.print(".");
    rr++;
  }
  if (rr == 30)
  {
    Serial.println("Connection failed");
  }
  else
  {
    Serial.println("Connected to web");
  }

  Serial.print("requesting URL: ");
  Serial.println(host2);

  String httpText2 = (String("GET /api/v2/homes/" + thome + "/zones/" + tzone + "/state HTTP/1.1") + "\r\n" +
                      "Host: my.tado.com" + "\r\n" +
                      "Authorization: Bearer " + Ttoken + "\r\n" +
                      "Accept: */*" + "\r\n" +
                      "Cache-Control: no-cache" + "\r\n" +
                      "Host: my.tado.com" + "\r\n" +
                      "Connection: close\r\n\r\n");

  httpsClient.print(httpText2);

  Serial.println("HTTP request is: ");
  Serial.println(httpText2);

  Serial.println("request sent");

  while (httpsClient.connected())
  {
    String line = httpsClient.readStringUntil('\n');
    // Serial.println(line); //Print response
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }

  Serial.println("reply was:");
  Serial.println("==========");

  String linez2;
  String lineT2;

  linez2 = httpsClient.readStringUntil('\n'); //Read Line by Line
  lineT2 = httpsClient.readStringUntil('\n'); //Read Line by Line

  Serial.println("\r\n lineT2");
  Serial.println(lineT2);
  // Serial.println("\r\n");

  httpsClient.stop();
  //###################################################################################################################
  // JSON split to get Tado Temp
  // See https://arduinojson.org/v6/assistant/     and    https://arduinojson.org/v6/api/json/deserializejson/

  Serial.println("JSON split to get Tado Temp");

  const size_t capacity = 4000; //3*JSON_OBJECT_SIZE(1) + 5*JSON_OBJECT_SIZE(2) + 4*JSON_OBJECT_SIZE(3) + JSON_OBJECT_SIZE(5) + JSON_OBJECT_SIZE(13) + 710;
  DynamicJsonDocument doc(capacity);

    // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, lineT2);

  // Test if parsing succeeds.
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }

  // Fetch values

  JsonObject sensorDataPoints_insideTemperature = doc["sensorDataPoints"]["insideTemperature"];
  float sensorDataPoints_insideTemperature_celsius = sensorDataPoints_insideTemperature["celsius"]; // 21.48

  //###################################################################################################################
  // GET existing offset Value

  Serial.println(host2);

  httpsClient.setCACert(root_ca);
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);

  Serial.print("HTTPS Connecting...");
  int rrrr = 0; //retry counter
  while ((!httpsClient.connect(host2, httpsPort)) && (rrrr < 30))
  {
    delay(100);
    Serial.print(".");
    rrrr++;
  }
  if (rrrr == 30)
  {
    Serial.println("Connection failed");
  }
  else
  {
    Serial.println("Connected to web");
  }

  Serial.print("requesting URL: ");
  Serial.println(host2);

  String httpText4 = (String("GET /api/v2/devices/" + tdevice + "/temperatureOffset HTTP/1.1") + "\r\n" +
                      "Host: my.tado.com" + "\r\n" +
                      "Authorization: Bearer " + Ttoken + "\r\n" +
                      "Accept: */*" + "\r\n" +
                      "Cache-Control: no-cache" + "\r\n" +
                      "Host: my.tado.com" + "\r\n" +
                      "Connection: close\r\n\r\n");

  httpsClient.print(httpText4);

  Serial.println("HTTP request is: ");
  Serial.println(httpText4);

  Serial.print("request sent... ");

  while (httpsClient.connected())
  {
    String line = httpsClient.readStringUntil('\n');
    // Serial.println(line); //Print response
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }

  Serial.println("reply was:");
  Serial.println("==========");

  String linez4;
  String lineT4;

  linez4 = httpsClient.readStringUntil('\n'); //Read Line by Line
  lineT4 = httpsClient.readStringUntil('\n'); //Read Line by Line

  Serial.println("\r\n lineT4");
  Serial.println(lineT4);
  // Serial.println("\r\n");
  httpsClient.stop();

  //################################################################################################################
  // JSON split to get Tado Offset

  const size_t capacity2 = JSON_OBJECT_SIZE(2) + 30;
  DynamicJsonDocument doc2(capacity2);

  //const char* json = "{\"celsius\":-21.8,\"fahrenheit\":-35.24}";
  DeserializationError error2 = deserializeJson(doc2, lineT4);

  // Test if parsing succeeds.
  if (error2)
  {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error2.c_str());
    return;
  }

  txoffset = doc2["celsius"]; // -21.8

  //###################################################################################################################
  // Calculate offset value required
  Serial.println("Calculate offset value required");
  Serial.print("Actual Temp: ");
  Serial.println(actTemp);
  Serial.print("Current Offset: ");
  Serial.println(txoffset);
  Serial.print("Tado reading inc. current offset: ");
  Serial.println(sensorDataPoints_insideTemperature_celsius);
  // Serial.println("\r\n");
  tsensor = (sensorDataPoints_insideTemperature_celsius - txoffset);
  Serial.print("Tado true sensor reading: ");
  Serial.println(tsensor);
  toffset = actTemp - tsensor;
  Serial.print("Offset calculated: ");
  Serial.println(toffset);
  sprintf(toffsetS, "%.2f", toffset);
  Serial.print("Offset to submit: ");
  Serial.println(toffsetS);
  if ((toffset - txoffset) > maxdiff || (toffset - txoffset) < mindiff)
  {
    Serial.println("Offset out of range - update");
    poffset = 1;
  }
  else
  {
    poffset = 0;
  }
  Serial.print("poffset: ");
  Serial.println(poffset);


  //###################################################################################################################
  // Display Temperatures & offset
  String temperature;
  String radiator;
  String offset;
  temperature = "Temperature: " + String(actTemp) + "C";
  radiator = "Radiator   : " + String(tsensor) + "C";
  if (poffset == 1)
  {
    offset = "Offset     : " + String(toffsetS) + "C";
  }
  else
  {
    offset = "Offset     : " + String(txoffset) + "C";
  }

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM); //Was MC
  tft.setTextSize(2);
  // tft.drawString(room,         tft.width() / 2, (tft.height() / 2) - 40 );
  // tft.drawString(temperature,  tft.width() / 2, (tft.height() / 2) - 20 );
  // tft.drawString(radiator,     tft.width() / 2, (tft.height() / 2)      );
  // tft.drawString(offset,       tft.width() / 2, (tft.height() / 2) + 20 );
  tft.drawString(room, 10, 20);
  tft.drawString(temperature, 10, 60);
  tft.drawString(radiator, 10, 80);
  tft.drawString(offset, 10, 100);

  //###################################################################################################################
  // PUT Offset to Tado
    if (poffset == 1)
  {

  Serial.print("PUT Offset to Tado: ");
  Serial.println(host2);
  httpsClient.setCACert(root_ca);
  httpsClient.setTimeout(15000); // 15 Seconds
  delay(1000);

  Serial.print("HTTPS Connecting... ");
  int rrr = 0; //retry counter
  while ((!httpsClient.connect(host2, httpsPort)) && (rrr < 30))
  {
    delay(100);
    Serial.print(".");
    rrr++;
  }
  if (rrr == 30)
  {
    Serial.println("Connection failed");
  }
  else
  {
    Serial.println("Connected to web");
  }

  Serial.print("requesting URL: ");
  Serial.println(host2);

  String httpBody3 = (String("{\r\n") +
                      "            celsius: " + toffsetS + "\r\n" +
                      "}");
  int httpBodyLength3 = httpBody3.length();

  String httpText3 = (String("PUT /api/v2/devices/" + tdevice + "/temperatureOffset HTTP/1.1") + "\r\n" +
                      "Host: my.tado.com" + "\r\n" +
                      "Authorization: Bearer " + Ttoken + "\r\n" +
                      "Accept: */*" + "\r\n" +
                      "Cache-Control: no-cache" + "\r\n" +
                      "Host: my.tado.com" + "\r\n" +
                      "Content-Length: " + httpBodyLength3 + "\r\n" +
                      "Connection: keep-alive\r\n\r\n" +
                      httpBody3);

  httpsClient.print(httpText3);

  Serial.println("HTTP request is: ");
  Serial.println(httpText3);

  Serial.print("request sent... ");

  while (httpsClient.connected())
  {
    String line = httpsClient.readStringUntil('\n');
    // Serial.println(line); //Print response
    if (line == "\r")
    {
      Serial.println("headers received");
      break;
    }
  }

  Serial.println("reply was:");
  Serial.println("==========");

  String linez3;
  String lineT3;

  linez3 = httpsClient.readStringUntil('\n'); //Read Line by Line
  lineT3 = httpsClient.readStringUntil('\n'); //Read Line by Line

  Serial.print("\r\n lineT3: ");
  Serial.println(lineT3);
  // Serial.println("\r\n");

  httpsClient.stop();
  }
  else 
  {
   Serial.println("Offset put skipped"); 
  }
  //###################################################################################################################
  // Serial.println();
  Serial.println("==========");
  Serial.println("closing connection");
  Serial.print("End of loop ##################################################################### \r\n");

  delay(300000); //POST Data at every 5 minute
}
//=======================================================================

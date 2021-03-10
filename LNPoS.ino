

//////////////LOAD LIBRARIES////////////////

#include <M5Stack.h>
#include "FS.h"
#include <WiFiManager.h> 
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>
#include "SPIFFS.h"

/////////////////LOAD SPLASH///////////////////

#include "image.c"

/////////////////SOME DEFINES///////////////////

#define LED_PIN 25
  
/////////////////SOME VARIABLES///////////////////

char lnbits_server[40] = "lnbits.com";
char invoice_key[500] = "";
char lnbits_description[100] = "";
char lnbits_amount[500] = "1000";
char high_pin[5] = "16";
char time_pin[20] = "3000";
char static_ip[16] = "10.0.1.56";
char static_gw[16] = "10.0.1.1";
char static_sn[16] = "255.255.255.0";
String payReq = "";
String dataId = "";
bool paid = false;
bool shouldSaveConfig = false;
bool down = false;
const char* spiffcontent = "";
String spiffing; 

String choice;
String payhash;
String on_sub_currency = on_currency.substring(3);
String key_val;
String cntr = "0";
String inputs;
int keysdec;
int keyssdec;
float temp;  
String fiat;
float satoshis;
String nosats;
float conversion;
  
/////////////////////SETUP////////////////////////

void setup() {
  Serial.begin(115200);
  M5.begin();
  M5.Lcd.drawBitmap(0, 0, 320, 240, (uint8_t *)logo_map);
  delay(3000);
  lnbits_screen();
  portal();
}

///////////////////MAIN LOOP//////////////////////

void loop() {
  page_input();
  cntr = "1";
  while (cntr == "1"){
    M5.update();
    get_keypad(); 
    if (M5.BtnC.wasReleased()) {
      page_processing();
      getinvoice(nosats);
      qrdisplay_screen();
      checkpaid();
      key_val = "";
      inputs = "";
    }
    else if (M5.BtnB.wasReleased()) {
      page_processing();
      nosats = "0";
      getinvoice(nosats);
      qrdisplay_screen();
      checkpaid();
      key_val = "";
      inputs = "";
    }
    else if (M5.BtnA.wasReleased()) {
      M5.Lcd.fillScreen(BLACK);
      M5.Lcd.setCursor(0, 0);
      M5.Lcd.setTextColor(TFT_WHITE);
      page_input();
      key_val = "";
      inputs = "";  
      nosats = "";
    }
    inputs += key_val;
    temp = inputs.toInt();
    temp = temp / 100;
    fiat = temp;
    satoshis = temp/conversion;
    int intsats = (int) round(satoshis*100000000.0);
    nosats = String(intsats);
    M5.Lcd.setTextSize(3);
    M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
    M5.Lcd.setCursor(70, 88);
    M5.Lcd.println(fiat);
    M5.Lcd.setTextColor(TFT_GREEN, TFT_BLACK);
    M5.Lcd.setCursor(87, 136);
    M5.Lcd.println(nosats);
    delay(100);
    key_val = "";
  }
}


//////////////////M5 STACK///////////////////


void input_screen()
{
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setCursor(0, 40);
  M5.Lcd.println("Amount then C");
  M5.Lcd.println("");
  M5.Lcd.println(on_currency.substring(3) + ": ");
  M5.Lcd.println("");
  M5.Lcd.println("SATS: ");
  M5.Lcd.println("");
  M5.Lcd.println("");
  M5.Lcd.setTextSize(2);
  M5.Lcd.setCursor(50, 200);
  M5.Lcd.println("TO RESET PRESS A");
}

void processing_screen()
{ 
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(40, 80);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("PROCESSING");
 
}

void get_keypad(){

   if(digitalRead(KEYBOARD_INT) == LOW) {
    Wire.requestFrom(KEYBOARD_I2C_ADDR, 1);  // request 1 byte from keyboard
    while (Wire.available()) { 
       uint8_t key = Wire.read();                  // receive a byte as character
       key_val = key;

       if(key != 0) {
        if(key >= 0x20 && key < 0x7F) { // ASCII String
          if (isdigit((char)key)){
          key_val = ((char)key);
          }
          else {
          key_val = "";
        } 
        }
      }
    }
  }
}

void processing_screen()
{ 
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(40, 80);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("PROCESSING");
}

void lnbits_screen()
{ 
  M5.Lcd.fillScreen(WHITE);
  M5.Lcd.setCursor(10, 90);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_BLACK);
  M5.Lcd.println("INITIALIZING");
}

void portal_screen()
{ 
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(30, 80);
  M5.Lcd.setTextSize(3);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("PORTAL LAUNCHED");
}

void complete_screen()
{ 
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(60, 80);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("COMPLETE");
}

void error_screen()
{ 
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setCursor(70, 80);
  M5.Lcd.setTextSize(4);
  M5.Lcd.setTextColor(TFT_WHITE);
  M5.Lcd.println("ERROR");
}

void qrdisplay_screen()
{  
  M5.Lcd.fillScreen(BLACK); 
  M5.Lcd.qrcode(payReq,45,0,240,14);
  delay(100);
}

//////////////////OPENNODE CALL///////////////////

void on_rates(){
  WiFiClientSecure client;
  if (!client.connect("api.opennode.co", httpsPort)) {
    return;
  }

  String url = "/v1/rates";
  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
               "Host: api.opennode.co\r\n" +
               "User-Agent: ESP32\r\n" +
               "Connection: close\r\n\r\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
  }
  String line = client.readStringUntil('\n');
    const size_t capacity = 169*JSON_OBJECT_SIZE(1) + JSON_OBJECT_SIZE(168) + 3800;
    DynamicJsonDocument doc(capacity);
    deserializeJson(doc, line);
    conversion = doc["data"][on_currency][on_currency.substring(3)]; 

}

//////////////////LNBITS CALLS///////////////////

void getinvoice(String nosats) {
  WiFiClientSecure client;
  const char* lnbitsserver = lnbits_server;
  const char* invoicekey = invoice_key;
  const char* lnbitsdescription = lnbits_description;

  if (!client.connect(lnbitsserver, 443)){
    down = true;
    return;   
  }

  String topost = "{\"out\": false,\"amount\" : " + nosats + ", \"memo\" :\""+ String(lnbitsdescription) + String(random(1,1000)) + "\"}";
  String url = "/api/v1/payments";
  client.print(String("POST ") + url +" HTTP/1.1\r\n" +
                "Host: " + lnbitsserver + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "X-Api-Key: "+ invoicekey +" \r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n" +
                "Content-Length: " + topost.length() + "\r\n" +
                "\r\n" + 
                topost + "\n");
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
    if (line == "\r") {
      break;
    }
  }
  
  String line = client.readString();

  StaticJsonDocument<1000> doc;
  DeserializationError error = deserializeJson(doc, line);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  const char* payment_hash = doc["checking_id"];
  const char* payment_request = doc["payment_request"];
  payReq = payment_request;
  dataId = payment_hash;
}


void checkinvoice(){

  WiFiClientSecure client;
  const char* lnbitsserver = lnbits_server;
  const char* invoicekey = invoice_key;

  Serial.println(lnbits_server);
  Serial.println(invoice_key);
  if (!client.connect(lnbitsserver, 443)){
    down = true;
    return;   
  }

  String url = "/api/v1/payments/";
  client.print(String("GET ") + url + dataId +" HTTP/1.1\r\n" +
                "Host: " + lnbitsserver + "\r\n" +
                "User-Agent: ESP32\r\n" +
                "X-Api-Key:"+ invoicekey +"\r\n" +
                "Content-Type: application/json\r\n" +
                "Connection: close\r\n\r\n");
   while (client.connected()) {
   String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
    if (line == "\r") {
      break;
    }
  }
  String line = client.readString();
  Serial.println(line);
  StaticJsonDocument<200> doc;
  DeserializationError error = deserializeJson(doc, line);
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
    return;
  }
  bool charPaid = doc["paid"];
  paid = charPaid;
}

void portal(){

  WiFiManager wm;
  Serial.println("mounting FS...");
  while(!SPIFFS.begin(true)){
    Serial.println("failed to mount FS");
    delay(200);
   }

//CHECK IF RESET IS TRIGGERED/WIPE DATA
  for (int i = 0; i <= 100; i++) {
    if (M5.BtnA.wasPressed()){
    portal_screen();
    delay(1000);
    File file = SPIFFS.open("/config.txt", FILE_WRITE);
    file.print("placeholder");
    wm.resetSettings();
    i = 100;
    }
    delay(50);
    M5.update();
  }

//MOUNT FS AND READ CONFIG.JSON
  File file = SPIFFS.open("/config.txt");
  
  spiffing = file.readStringUntil('\n');
  spiffcontent = spiffing.c_str();
  DynamicJsonDocument json(1024);
  deserializeJson(json, spiffcontent);
  if(String(spiffcontent) != "placeholder"){
    strcpy(lnbits_server, json["lnbits_server"]);
    strcpy(lnbits_description, json["lnbits_description"]);
    strcpy(invoice_key, json["invoice_key"]);
  }

//ADD PARAMS TO WIFIMANAGER
  wm.setSaveConfigCallback(saveConfigCallback);
  
  WiFiManagerParameter custom_lnbits_server("server", "LNbits server", lnbits_server, 40);
  WiFiManagerParameter custom_lnbits_description("description", "Memo", lnbits_description, 200);
  WiFiManagerParameter custom_invoice_key("invoice", "LNbits invoice key", invoice_key, 500);
  wm.addParameter(&custom_lnbits_server);
  wm.addParameter(&custom_lnbits_description);
  wm.addParameter(&custom_invoice_key);

//IF RESET WAS TRIGGERED, RUN PORTAL AND WRITE FILES
  if (!wm.autoConnect("⚡LNPoS⚡", "password1")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }
  Serial.println("connected :)");
  strcpy(lnbits_server, custom_lnbits_server.getValue());
  strcpy(lnbits_description, custom_lnbits_description.getValue());
  strcpy(invoice_key, custom_invoice_key.getValue());
  if (shouldSaveConfig) {
    Serial.println("saving config");
    DynamicJsonDocument json(1024);
    json["lnbits_server"] = lnbits_server;
    json["lnbits_description"]   = lnbits_description;
    json["invoice_key"]   = invoice_key;

    File configFile = SPIFFS.open("/config.txt", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
      }
      serializeJsonPretty(json, Serial);
      serializeJson(json, configFile);
      configFile.close();
      shouldSaveConfig = false;
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());
  Serial.println(WiFi.gatewayIP());
  Serial.println(WiFi.subnetMask());
}

void saveConfigCallback () {
  processing_screen();
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

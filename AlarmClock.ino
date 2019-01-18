#include <Wire.h> //I2C library
#include <RtcDS3231.h> //RTC library
#include <ESP8266WiFi.h>
#include <WiFiClient.h> 
#include <ESP8266WebServer.h>
#include <FS.h>

RtcDS3231<TwoWire> rtcObject(Wire);
RtcDateTime currentTime;
static const int PD5   = 14;
char strTime[20]; //declare a string as an array of chars
bool gun[1440];
int acilisSaat;
int acilisDakika;
int kapanisSaat;
int kapanisDakika;
bool alarmAktifMi;


const char *ssid = "Ssid!";
const char *password = "Pass!";
 
ESP8266WebServer server(80);  // Start Server on Port 80

void handleRoot();              // function prototypes for HTTP handlers
void handleAlarmChangePage();
void handleAlarmChange();
void handleNotFound();
void alarmHesaplayici(int acilisSaat,int acilisDakika, int kapanisSaat, int kapanisDakika); 
void startSPIFFS();
bool handleFileRead(String path);
String getContentType(String filename);
void dataWrite();
void handleLedOn();
void handleLedOff();
void handleAlarmOn();
void handleAlarmOff();

void setup() {
  Serial.begin(115200); //Starts serial connection
  rtcObject.Begin();    //Starts I2C
  pinMode(PD5,OUTPUT);
  digitalWrite(PD5,HIGH);
  
  WiFi.softAP(ssid, password);
  
  server.on("/", handleRoot);
  server.on("/alarmChangePage", handleAlarmChangePage);
  server.on("/alarmChange", handleAlarmChange);
  server.on("/ledOn", handleLedOn);
  server.on("/ledOff", handleLedOff);
  server.on("/alarmOn", handleAlarmOn);
  server.on("/alarmOff", handleAlarmOff);
  server.onNotFound(handleNotFound);        // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"
  server.begin();
  Serial.println("HTTP server started");

  startSPIFFS();               // Start the SPIFFS and list all contents
  
  dataWrite();
  //RtcDateTime currentTime = RtcDateTime(18,11,19,20,31,0); //define date and time object
  //rtcObject.SetDateTime(currentTime);                      //configure the RTC with object
}

void loop() {
  server.handleClient();
  
  if (alarmAktifMi)
  {
    alarmHesaplayici(acilisSaat,acilisDakika,kapanisSaat,kapanisDakika);
  }
  delay(1000); //1 seconds delay
}

void handleRoot() {
  currentTime = rtcObject.GetDateTime();
  sprintf(strTime, "%d:%d", currentTime.Hour(),currentTime.Minute());
  delay(10);
  String alarm;
  if(alarmAktifMi)alarm="Aktif";
  else alarm="Pasif";
  server.send(200, "text/html", "<!DOCTYPE html><html><head><title>Smart House</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><link rel=\"icon\" href=\"data:,\"><style>html{font-family: Helvetica; display:inline-block; margin: 0px auto; text-align: center;}h1{color: #0F3376; padding: 2vh;}h2{color: #ad0a0a; padding: 2vh;}p{font-size: 1.5rem;}.button{display: inline-block; background-color: #e7bd3b; border: none;border-radius: 4px; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}.button2{background-color: #ad0a0a;}</style></head><body><h1>ESP Web Server</h1><p>LED</p><p><a href=\"/ledOn\"><button class=\"button\">ON</button></a> <a href=\"/ledOff\"><button class=\"button button2\">OFF</button></a></p><h2>Alarm Bilgileri</h2><p>"+alarm+"</p><p>Acilis:"+String(acilisSaat)+":"+String(acilisDakika)+ "</p><p>Kapanis:"+String(kapanisSaat)+":"+String(kapanisDakika)+ "</p><p>Guncel Saat:" + strTime + "</p><p><a href=\"/alarmOn\"><button class=\"button\">Aktif Yap</button></a> <a href=\"/alarmOff\"><button class=\"button button2\">Pasif Yap</button></a></p><p><a href=\"/alarmChangePage\"><button class=\"button button2\">Saat Degistir</button></a></p></body></html>");
}
void handleAlarmChangePage(){
  server.send(200, "text/html","<!DOCTYPE html><html><head><title>Smart House</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><link rel=\"icon\" href=\"data:,\"><style>html{font-family: Helvetica; display:inline-block; margin: 0px auto; text-align: center;}h1{color: #0F3376; padding: 2vh;}h2{color: #ad0a0a; padding: 2vh;}p{font-size: 1.5rem;}.button{display: inline-block; background-color: #e7bd3b; border: none;border-radius: 4px; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}.button2{background-color: #ad0a0a;}.satir{margin: 2px;}input{min-height: 25px}</style></head><body><h1>ESP Web Server</h1><h2>Alarm Saatlerini Giriniz</h2><form action=\"/alarmChange\" method=\"POST\"><div class=\"satir\"><input type=\"number\" name=\"aSaat\" placeholder=\"Acilis Saat\"></div><div class=\"satir\"><input type=\"number\" name=\"aDakika\" placeholder=\"Acilis Dakika\"></div><div class=\"satir\"><input type=\"number\" name=\"kSaat\" placeholder=\"Kapanis Saat\"></div><div class=\"satir\"><input type=\"number\" name=\"kDakika\" placeholder=\"Kapanis Dakika\"></div><input type=\"submit\" value=\"Kaydet\" class=\"button button2\"></form></body></html>");
}
void handleAlarmChange(){
  if (! server.hasArg("aSaat")||! server.hasArg("aDakika")||! server.hasArg("kSaat")||! server.hasArg("kDakika")|| server.arg("aSaat")==NULL|| server.arg("aDakika")==NULL|| server.arg("kSaat")==NULL|| server.arg("kDakika")==NULL)
  {
    server.send(400, "text/html", "<!DOCTYPE html><html><head><title>Smart House</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><link rel=\"icon\" href=\"data:,\"><style>html{font-family: Helvetica; display:inline-block; margin: 0px auto; text-align: center;}h1{color: #0F3376; padding: 2vh;}h2{color: #ad0a0a; padding: 2vh;}p{font-size: 1.5rem;}.button{display: inline-block; background-color: #e7bd3b; border: none;border-radius: 4px; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}.button2{background-color: #ad0a0a;}.satir{margin: 2px;}input{min-height: 25px}</style></head><body><h1>ESP Web Server</h1><h2>Eksik Yada Hatali Giris Yaptiniz.</h2><p><a href=\"/\"><button class=\"button button2\">Ana Sayfa</button></a></p></body></html>");
    return;
  }
  acilisSaat = server.arg("aSaat").toInt();
  acilisDakika = server.arg("aDakika").toInt();
  kapanisSaat = server.arg("kSaat").toInt();
  kapanisDakika = server.arg("kDakika").toInt();
  File f = SPIFFS.open("/data.txt", "w");
  if (!f) {
      Serial.println("file open failed");
  }
  Serial.println("====== Writing to SPIFFS file =========");
  f.println(alarmAktifMi);
  f.println(acilisSaat);
  f.println(acilisDakika);
  f.println(kapanisSaat);
  f.println(kapanisDakika);
  f.close();
  server.send(200, "text/html", "<!DOCTYPE html><html><head><title>Smart House</title><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><link rel=\"icon\" href=\"data:,\"><style>html{font-family: Helvetica; display:inline-block; margin: 0px auto; text-align: center;}h1{color: #0F3376; padding: 2vh;}h2{color: #ad0a0a; padding: 2vh;}p{font-size: 1.5rem;}.button{display: inline-block; background-color: #e7bd3b; border: none;border-radius: 4px; color: white; padding: 16px 40px; text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}.button2{background-color: #ad0a0a;}.satir{margin: 2px;}input{min-height: 25px}</style></head><body><h1>ESP Web Server</h1><h2>Alarm Ayarlandi.</h2><p><a href=\"/\"><button class=\"button button2\">Ana Sayfa</button></a></p></body></html>");
}
void handleLedOn(){
  alarmAktifMi=false;
  digitalWrite(PD5,LOW);
  handleRoot();

}
void handleLedOff(){
  alarmAktifMi=false;
  digitalWrite(PD5,HIGH);
  handleRoot();
}
void handleAlarmOn(){
  alarmAktifMi=true;
  File f = SPIFFS.open("/data.txt", "w");
  if (!f) {
      Serial.println("file open failed");
  }
  Serial.println("====== Writing to SPIFFS file =========");
  f.println(alarmAktifMi);
  f.println(acilisSaat);
  f.println(acilisDakika);
  f.println(kapanisSaat);
  f.println(kapanisDakika);
  f.close();
  handleRoot();
}
void handleAlarmOff(){
  alarmAktifMi=false;
  File f = SPIFFS.open("/data.txt", "w");
  if (!f) {
      Serial.println("file open failed");
  }
  Serial.println("====== Writing to SPIFFS file =========");
  f.println(alarmAktifMi);
  f.println(acilisSaat);
  f.println(acilisDakika);
  f.println(kapanisSaat);
  f.println(kapanisDakika);
  f.close();
  handleRoot();
}
void handleNotFound(){
  if (!handleFileRead(server.uri()))
  server.send(404, "text/plain", "404: Not found"); // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
}

void alarmHesaplayici(int acilisSaat,int acilisDakika, int kapanisSaat, int kapanisDakika){
  currentTime = rtcObject.GetDateTime();
  delay(10);
  int acilis=(acilisSaat+1)*60+acilisDakika;
  int kapanis=(kapanisSaat+1)*60+kapanisDakika;
  int suanki=(currentTime.Hour()+1)*60+currentTime.Minute();

  if (acilis<=kapanis){
    if (suanki>=acilis&&suanki<=kapanis) digitalWrite(PD5,LOW);
    else digitalWrite(PD5,HIGH);
  }
  else{
    if (suanki>=acilis&&suanki<=(24*59)) digitalWrite(PD5,LOW);
    else if(suanki<=acilis&&suanki<=kapanis) digitalWrite(PD5,LOW);
    else digitalWrite(PD5,HIGH);
  }  
}

void startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}
String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".txt")) return "text/css";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";         // If a folder is requested, send the index file
  String contentType = getContentType(path);            // Get the MIME type
  if (SPIFFS.exists(path)) {                            // If the file exists
    File file = SPIFFS.open(path, "r");                 // Open it
    size_t sent = server.streamFile(file, contentType); // And send it to the client
    file.close();                                       // Then close the file again
    return true;
  }
  Serial.println("\tFile Not Found");
  return false;                                         // If the file doesn't exist, return false
}

/*__________________________________________________________Write File__________________________________________________________*/
void dataWrite(){
  File f = SPIFFS.open("/data.txt", "r");
  if (!f) {
      Serial.println("file open failed");
  }  Serial.println("====== Reading from SPIFFS file =======");
  
  String s=f.readStringUntil('\n');
  Serial.println(s);
  alarmAktifMi = s.toInt();
  s=f.readStringUntil('\n');
  Serial.println(s);
  acilisSaat = s.toInt();
  s=f.readStringUntil('\n');
  Serial.println(s);
  acilisDakika = s.toInt();
  s=f.readStringUntil('\n');
  Serial.println(s);
  kapanisSaat = s.toInt();
  s=f.readStringUntil('\n');
  Serial.println(s);
  kapanisDakika = s.toInt();
  f.close();
}

/*__________________________________________________________HELPER_FUNCTIONS__________________________________________________________*/

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}


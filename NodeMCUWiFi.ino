#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <string>
#include <stdbool.h>

#define baudRate 115200

#define SSID_SIZE 40
#define PASSWORD_SIZE 40

bool WiFiNotSet = true;

#define pillContainersCount 8

//container max digit count
#define maxDigitCount 2
#define maxPillName 15


#define hourDigits 2
#define minuteDigits 2
#define dayDigits 2
#define monthDigits 2
#define yearDigits 4

#define sep_character ' '

#define debug true

const size_t JSON_SIZE = JSON_OBJECT_SIZE(30);

const int RX_pin = 13;
const int TX_pin = 15;

int param = 0;

StaticJsonDocument<JSON_SIZE> doc;

JsonObject object;

String json;

bool updatedFields = false;


char ssid[SSID_SIZE] ="SweetPotatoJam";      //  your network SSID (name)

char password[PASSWORD_SIZE] ="jEVezYP92*BRPiyC8zxhceAF";   // your network password

const char* rest_host = "http://apd-webapp-backend.herokuapp.com/api/analytics";

 char temp_original_date[20];
 char temp_taken_date[20];

//"ATT-HOMEBASE-5077";
// "04148468";


struct alarm{ //hour, minute, pillQuantities

    unsigned char hour;
    unsigned char minute;
    int pillQuantities[pillContainersCount];
    char pillNames[pillContainersCount][maxPillName]; //remove pillNames from alarm

};

struct refill{ //pillNames and pillQuantities

    int pillQuantities[pillContainersCount];
    char pillNames[pillContainersCount][maxPillName]; //TODO remove pillNames from Alarm struct

};



struct analytic{ //original_date, taken_date, completed, pill_names, pill_quanitites
    unsigned char hour; //original hour for alarm
    unsigned char minute; // original minute for alarm
    int pillQuantities[pillContainersCount];
    char pillNames[pillContainersCount][maxPillName];
    bool taken; // true if user was reminded successfully
    char day;   // current day
    char month; //current month
    int year;   // current year
    char dow;   // current day of week
    char TakenH; //hour when analytic was generated
    char TakenM; //minute when analytic was generated
};

struct alarm schedule[pillContainersCount];

struct analytic analytics;

struct refill fill;

ESP8266WebServer server(80);


//pillNames declaration

void handleRoot(){
  server.send(200, "text/plain", "hello from esp8266");
}

void handlePOST(){

  //check plain
  if(server.arg("plain")){
      deserializeJson(doc, server.arg("plain"));
      object = doc.as<JsonObject>();
      updatedFields = true;
  }
    server.send(201, "text/plain", "the fields have been updated");
}

void handleNotFound(){

  if (server.method() == HTTP_OPTIONS)
    {
        server.sendHeader("Access-Control-Allow-Origin", "*");
        server.sendHeader("Access-Control-Max-Age", "10000");
        server.sendHeader("Access-Control-Allow-Methods", "HEAD,PUT,POST,GET,OPTIONS");
        server.sendHeader("Access-Control-Allow-Headers", "*");
        server.send(204);
    }

  
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  
  if(server.method() == HTTP_GET){
    message += "GET";  
  }
  else{
    message += "POST";
  }

  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for(int i = 0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }

  server.send(404, "text/plain", message);
}

void print_args(){
      Serial.print("Hour");
      Serial.print(":");
      delay(10);
      Serial.print(object["hour"].as<char*>());
      delay(10);
      Serial.print(" ");

      delay(10);
      Serial.print("Minute");
      Serial.print(":");
      delay(10);
      Serial.print(object["minute"].as<char*>());
      delay(10);
      Serial.print(" ");  

      Serial.print("pillNames:[");
      for(int i=0; i<pillContainersCount-1; i++){
        delay(10);
        Serial.print(object["pillNames"][i].as<char*>());
        Serial.print(",");
      }
      if(pillContainersCount >= 2)
        Serial.print(object["pillNames"][pillContainersCount-1].as<char*>()); //runs if pillContainersCount is at least 2
      Serial.print("] ");

      Serial.print("pillQuantities:[");
      
      for(int i=0; i<pillContainersCount-1; i++){
        delay(10);
        Serial.print(object["pillQuantities"][i].as<int>());
        Serial.print(",");
      }
      if(pillContainersCount >= 2)
      Serial.print(object["pillQuantities"][pillContainersCount-1].as<int>()); //runs if pillContainersCount is at least 2
      Serial.print("]\n");

      updatedFields = false;
      
}

bool WiFiCredentialsReady(){
  if(!ssid[0]) return false;
  if(!password[0]) return false;
  return true;
}

void WiFi_setup(){
  if(debug) Serial.print("connecting to Wifi \n");
  
  WiFi.begin(ssid, password);
  
  while( WiFi.status() != WL_CONNECTED){ // while not Wireless Lan Connected
    delay(500);
    Serial.print(".");
  }

  if(debug){
    Serial.println();
    Serial.println("Connected to WiFi successfully");
  }

  Serial.print("NODEMCU IP Address : ");

  Serial.print(WiFi.localIP());

  if(debug)
    Serial.println("\nsetting up MDNS responder");
  
  while(!MDNS.begin("apdwifimodule")){

  Serial.print(".");
  delay(1000);
    
  }
  if(debug)
    Serial.println("\nMDNS has started");

  server.begin();

  if(debug)
    Serial.println("\nHTTP Server has started");
  
  MDNS.addService("http", "tcp", 80);
}

int readnInt(int digitsToRead){
  int result = 0;
  
    for(int i = digitsToRead; i>=0; i--){
      
      if(Serial.peek() == sep_character) //
         i = -1;
      
      else
        result += (Serial.read() - '0') * pow(10, i-1);
      
    }

  return result;
}

char* readnChar(char stringLength, char* list){

    if(debug) Serial.printf("readnChar function was entered\n");

    
  
      for(int i=0; i<stringLength; i++){

//        if(debug) Serial.printf("readnChar function - character %c, i=%d\n", Serial.peek(), i);

        if(Serial.peek() == sep_character || Serial.peek() == '\0'){
         Serial.read();
         list[i] = '\0';
         i = stringLength + 1;
        }
        else
          list[i] = Serial.read();
      }
  
      if(debug){
        Serial.printf("readnChar function: list = %s\n", list);
        
        for(int i = 0; i<stringLength; i++){
          Serial.printf("char at pos %d: %c\n", i, list[i]);
        }
        
        Serial.printf("readnChar function was finished\n");
      }

      return list;
}



int* readnIntList(char digitCount, int* list, int listLength){

  if(debug) Serial.printf("readnIntList function was entered\n");
  

  for(int i=0; i<listLength; i++) 
      list[i] = 0; //setting the list to 0
  
  for(int i = 0; i<listLength; i++){
    for(int j = digitCount; j>=0; j--){
        if(Serial.peek() == ',' || Serial.peek() == ']'){
           Serial.read();
           j = -1;
          }
          else{
            list[i] += (Serial.read() - '0') * pow(10, j-1);
          }
    }
    if(debug) Serial.printf("readnIntList function: list[%d] = %d\n", i, list[i]);
    
  }

  if(debug) Serial.printf("readnIntList function was finished\n");
  
  
  return list;
          
}

void RX(){
  
  if(debug) Serial.print("\nENTERED RX\n");
  
  if(Serial.find("param:")){
    
    param = Serial.read() - '0';
    
  if(debug) Serial.printf("\nparam = %d\n", param);
  }
  
  if(param == 1){ //Analytics
  //  Receive the parameters for pills, pillNames, day, year, month for the analytics through the UART port to the ESP8266
  //  If params are not null
  //    Send params using an HTTP_POST() to the remote server
  
  if(debug) Serial.print("\nEntered param = 1\n");
    
  if(Serial.find("Hour:")){
    analytics.hour = readnInt(hourDigits);
    if(debug) Serial.printf("Analytics Hour = %d\n", analytics.hour);
  }
  
  if(Serial.find("Minute:")){
    analytics.minute = readnInt(minuteDigits);
    if(debug) Serial.printf("Analytics Minute = %d\n", analytics.minute);
  }
  
  if(Serial.find("pillNames:[")){
    if(debug) Serial.print("pillNames are\n");
    
  
    for(int i=0; i<pillContainersCount; i++){
      for(int j=0; j<maxPillName; j++){

        if(Serial.peek() == ','){
         Serial.read();
         analytics.pillNames[i][j] = '\0';
         j = maxPillName + 1;
        }
        
        else if(Serial.peek() == ']'){
          
          i = pillContainersCount-1;
          j = maxPillName;
        }
        else{
          analytics.pillNames[i][j] = Serial.read();
        }
      }
  
      if(debug) Serial.printf("analytics.pillNames[%d] = %s\n", i, analytics.pillNames[i]);
      
    }

    if(debug) Serial.print("END PILL NAMES \n");
  }

  if(Serial.find("pillQuantities:[")){
    if(debug) Serial.print("pillQuantities are \n");
    readnIntList(maxDigitCount, analytics.pillQuantities, pillContainersCount);
    if(debug) Serial.println("END PILLQUANTITY");
  }



  // Day
    if(Serial.find("Day:")){
      analytics.day = readnInt(dayDigits); // set analytics day
      if(debug) Serial.printf("Analytics day is %d\n", analytics.day);
    }

    //Month
    if(Serial.find("Month:")){
      analytics.month = readnInt(monthDigits); // set analytics month
      if(debug)
            Serial.printf("Analytics month is %d\n", analytics.month);
    }
    
    //Year
    if(Serial.find("Year:")){
      analytics.year = readnInt(yearDigits); // set analytics year
      if(debug)
            Serial.printf("Analytics year is %d\n", analytics.year);
    }

    //Day of Week
    if(Serial.find("DOW:")){
      analytics.dow = readnInt(1); // set analytics dow
      if(debug)
            Serial.printf("Analytics Day of the Week is %d\n", analytics.dow);
    }

    //Taken Hour
    if(Serial.find("TakenH:")){
      analytics.TakenH = readnInt(hourDigits); // set analytics taken hr
      if(debug)
            Serial.printf("Analytics Taken Hour is %d\n", analytics.TakenH);
    }


    //Taken Minute
    if(Serial.find("TakenM:")){
      analytics.TakenM = readnInt(minuteDigits); // set analytics taken min
      if(debug)
            Serial.printf("Analytics Taken Hour is %d\n", analytics.TakenM);
    }

    //Taken boolean
    if(Serial.find("Taken:")){
      analytics.taken = readnInt(1); // set analytics taken
      if(debug)
            Serial.printf("Analytics Taken is %d\n", analytics.taken);
    }

    Serial.find(sep_character);
    
    Analytics_to_JSON(); //adds the analytics generated to a json and posts the result to the Database
  }
  
  if(param == 2){
  //  Network Configuration
  //  Receive parameters for ssid and password for the network through the UART port
  //  If ssid is not NULL and password is not NULL
  //  Set global variables ssid and password to the passed params
  //  Test connection
  
    Serial.find("ssid:"); //char* readnChar(char stringLength, char* list)
    readnChar(SSID_SIZE, ssid); // read SSID from UART
    
    Serial.find("password:");
    readnChar(PASSWORD_SIZE, password); // read password from UART
  }
  
  if(param == 3){
  //  reloading pills
  //  send parameters for pillName for each pill to add and pillOrder for the pill configuration and refilling through the UART port

    Serial.find("pillNames:[");
    
      for(int i=0; i<pillContainersCount; i++){
        for(int j=0; j<maxPillName; j++){
  
          if(Serial.peek() == ','){
           Serial.read();
           fill.pillNames[i][j] = '\0';
           j = maxPillName + 1;
          }
          
          else if(Serial.peek() == ']'){
            
            i = pillContainersCount-1;
            j = maxPillName;
          }
          else{
            fill.pillNames[i][j] = Serial.read();
          }
        }
  
      if(debug) Serial.printf("fill.pillNames[%d] = %s\n", i, fill.pillNames[i]);
      }



    Serial.find("]");
    
    Serial.find("pillQuantities:[");
    readnIntList(maxDigitCount, fill.pillQuantities, pillContainersCount); // read quantities from UART
    Serial.find("]");
  }
  if(param == 4){
  //  Receives the time by using an HTTP_GET() request
  //  sends parameters for day, year, month, hour, minute, second from the network through the UART port

  // use WiFi client to pull time data from network

  //requires TX to MSP to complete
  }

  param = 0;
    if(debug)
    Serial.print("\nFINISHED RX\n");
}

void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(baudRate);
  if(debug){
    Serial.println();
  }

  WiFi.mode(WIFI_STA); //WIFI_STA - Devices that connect to WiFi network are called stations (STA)


  Serial.flush();
  
  
  if(WiFiCredentialsReady() && WiFiNotSet){
    if(debug) Serial.print("WiFi ssid and password are set and not set up\n");
    WiFi_setup();
    if(debug) Serial.print("WiFi ssid and password are set and is set up\n");
    WiFiNotSet = false;
  }

  if(debug)
    Serial.println();

  //setup Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/submit_data", HTTP_POST, handlePOST);

  server.onNotFound(handleNotFound);
  
  //Serial.println("HTTP server started");

//  attachInterrupt(digitalPinToInterrupt(RX_pin), RX, CHANGE);
  Serial.print("Finished Basic Setup");
}

void post_DB(String json){

  if(debug)
    Serial.println("json is " + json + "\n");


  while( WiFi.status() != WL_CONNECTED){ // while not Wireless Lan Connected
    if(debug)
      Serial.println("WiFi is not connected");
        
    delay(1000);
  }

  if(WiFi.status() == WL_CONNECTED){
    
    WiFiClient client;

    HTTPClient http;

    if(debug) Serial.println("NodeMCU is connected to the internet\nStarting http request to rest_host");

    
    if (http.begin(client, rest_host)) {  // HTTP
      if(debug) Serial.println("[HTTP] GET...");

      http.addHeader("Content-Type", "application/json");
      
      int httpCode = http.POST(json);

    
    if (httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        if(debug) Serial.printf("[HTTP] GET... code: %d", httpCode);

          // file found at server
  //        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
  //          
  //        }
      } 
      else {
          if(debug)
            Serial.printf("[HTTP] GET... failed, error: %s", http.errorToString(httpCode).c_str());
      }
  
        http.end();
      }
    }
    
    else {
      if(debug)
        Serial.printf("[HTTP} Unable to connect");
    }
}

void Analytics_to_JSON(){

//            'original_date': self.original_date,
//            'taken_date': self.taken_date,
//            'completed': self.completed,
//            'pill_names': self.pill_names,
//            'pill_quantities': self.pill_quantities

//   char* temp_original_date;
//   char* temp_taken_date;


// allocate the memory for the document

  DynamicJsonDocument Analyticsdoc(1024);
  

  JsonObject jobject = Analyticsdoc.to<JsonObject>();

  if(debug) Serial.printf("original_date is %02d-%02d-%02d %02d:%02d:00\n", analytics.year, analytics.month, analytics.day, analytics.hour, analytics.minute);
  
  sprintf(temp_original_date, "%02d-%02d-%02d %02d:%02d:00", analytics.year, analytics.month, analytics.day, analytics.hour, analytics.minute); //verify if errors
    jobject["original_date"] = temp_original_date;

  if(debug) Serial.printf("taken_date is %02d-%02d-%02d %02d:%02d:00\n", analytics.year, analytics.month, analytics.day, analytics.hour, analytics.minute);
  
  sprintf(temp_taken_date, "%02d-%02d-%02d %02d:%02d:00", analytics.year, analytics.month, analytics.day, analytics.TakenH, analytics.TakenM); //verify if errors
  jobject["taken_date"] = temp_taken_date;
  
  jobject["completed"] = analytics.taken;
  
  JsonArray pillNameArr = Analyticsdoc.createNestedArray("pill_names");
  
  for(int i = 0; i<pillContainersCount; i++){
   // for(int j = 0; j<maxPillName; j++){
      pillNameArr.add(analytics.pillNames[i]);
   // }
  }

  JsonArray pillQuantitiesArr = Analyticsdoc.createNestedArray("pill_quantities");
  
  for(int i = 0; i<pillContainersCount; i++){
      pillQuantitiesArr.add(analytics.pillQuantities[i]);
  }
  serializeJson(Analyticsdoc, json);

//pillNameArr.clear();
//pillQuantitiesArr.clear();

//jobject.clear();
//
//Analyticsdoc.garbageCollect();
//Analyticsdoc.clear();


//Analyticsdoc.~BasicJsonDocument(); //destructor for JSONDocument

post_DB(json);

json = "";
}

void loop() {
  // put your main code here, to run repeatedly:

//  WiFiClient client = server.available();

  if(WiFiCredentialsReady() && WiFiNotSet){
    if(debug) Serial.print("\nWiFi ssid and password are set and not set up\n");
    WiFi_setup();
    if(debug) Serial.print("\nWiFi ssid and password are set and is set up\n");
    WiFiNotSet = false;
  }
  if(!WiFiNotSet){
    MDNS.update();
  }

  server.handleClient();
  
  if(Serial.available()){
    RX();
  }
  
  if(updatedFields){
    print_args();
  }
  
  delay(500);

}

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <math.h>
#include <stdbool.h>

#define baudRate 115200

#define pillContainersCount 8

#define maxDigitCount 2

#define maxPillQuantity 15

#define debug true

const size_t JSON_SIZE = JSON_OBJECT_SIZE(30);

const int RX_pin = 13;
const int TX_pin = 15;

int param = 0;

StaticJsonDocument<JSON_SIZE> doc;

//void ICACHE_RAM_ATTR RX_ISR();

JsonObject object;

String json;

bool updatedFields = false;


char ssid[] = "SweetPotatoJam";      //  your network SSID (name)
char password[] = "jEVezYP92*BRPiyC8zxhceAF";   // your network password

const char* rest_host = "https://apd-webapp-backend.herokuapp.com/api/analytics";

WiFiClient client;

// char ssid[] = "ATT-HOMEBASE-5077";
// char password[] = "04148468";


struct alarm{ //hour, minute, pillQuantities

    unsigned char hour;
    unsigned char minute;
    int pillQuantities[pillContainersCount];
    char pillNames[pillContainersCount][maxPillQuantity]; //TODO remove pillNames from Alarm struct

};

struct analytic{ //original_date, taken_date, completed, pill_names, pill_quanitites

    unsigned char hour;
    unsigned char minute;
    int pillQuantities[pillContainersCount];
    char pillNames[pillContainersCount][maxPillQuantity];
    bool taken;

};

struct alarm schedule[pillContainersCount];

struct analytic analytics;

ESP8266WebServer server(80);


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

void RX_ISR(){
  if(debug)
    Serial.print("\nENTERED RX_ISR\n");
  
  //  RX ISR
  if(Serial.find("param:")){
    
    param = Serial.read() - '0';
    
    if(debug)
      Serial.printf("\nparam = %d", param);
  }
  if(param == 1){
  //  Receive the parameters for pills, pillNames, day, year, month for the analytics through the UART port to the ESP8266
  //  If params are not null
  //    Send params using an HTTP_POST() to the remote server
  
  if(debug)
    Serial.print("\nEntered param = 1\n");
    
  if(Serial.find("Hour:")){
    analytics.hour = (Serial.read() - '0') * 10;
    analytics.hour += (Serial.read() - '0');

    if(debug)
      Serial.printf("Analytics Hour = %d\n", analytics.hour);
  }
  
  if(Serial.find("Minute:")){
    analytics.minute = (Serial.read() - '0') * 10;
    analytics.minute += (Serial.read() - '0');

    if(debug)
      Serial.printf("Analytics Minute = %d\n", analytics.minute);
  }
  
  if(Serial.find("pillNames:[")){

    if(debug)
      Serial.print("pillNames are\n");
    
    for(int i=0; i<pillContainersCount; i++){
      for(int j=0; j<maxPillQuantity; j++){

//        if(debug)
//          Serial.printf("i=%d, j=%d | char in serial is %c\n", i, j, Serial.peek());
  
        if(Serial.peek() == ','){
         Serial.read();
         j = maxPillQuantity + 1;
        }
        
        else if(Serial.peek() == ']'){
          
          i = pillContainersCount-1;
          j = maxPillQuantity;
        }
        else{
          analytics.pillNames[i][j] = Serial.read();
        }
      }
  
      if(debug){
        Serial.printf("pillName %d: %s \n", i, analytics.pillNames[i]);
      }
    }

    if(debug){
        Serial.print("END PILL NAMES \n");
    }
    
  }

  if(Serial.find("pillQuantities:[")){

        if(debug){
          Serial.print("pillQuantities are \n");
        }
    
    int digit_count = 0;

    for(int i=0; i<pillContainersCount; i++) 
      analytics.pillQuantities[i] = 0; // reset analytics to 0

    
    for(int i=0; i<pillContainersCount; i++){
      for(int j = maxDigitCount; j>=0; j--){
        if(Serial.peek() == ',' || Serial.peek() == ']'){
          
           Serial.read();
           j = -1;
          }
          else{
            analytics.pillQuantities[i] += (Serial.read() - '0') * pow(10, j-1);
          }
      }
      if(debug)
            Serial.printf("pillQuantity for container %d: %d\n", i, analytics.pillQuantities[i]);
    }

    if(debug)
      Serial.println("END PILLQUANTITY");

    Serial.find("]");
  }
  
  
  
  
  

  }
  if(param == 2){
  //  Network Configuration
  //  Receive parameters for ssid and password for the network through the UART port
  //  If ssid is not NULL and password is not NULL
  //  Set global variables ssid and password to the passed params
  //  Test connection
    Serial.find("ssid:");
    Serial.read(); // read SSID from UART
    
    Serial.find("password:");
    Serial.read(); // read password from UART
  }
  if(param == 3){
  //  reloading pills
  //  send parameters for pillName for each pill to add and pillOrder for the pill configuration and refilling through the UART port

    Serial.find("pillName:");
    Serial.read(); // read SSID from UART
    
    Serial.find("password:");
    Serial.read(); // read password from UART
  }
  if(param == 4){
  //  Receives the time by using an HTTP_GET() request
  //  sends parameters for day, year, month, hour, minute, second from the network through the UART port

  // use WiFi client to pull time data from network
  }
}

void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(baudRate);
  if(debug){
    Serial.println();
    Serial.print("WiFi connecting to ");
  }

  //Serial.println(ssid);
  WiFi.mode(WIFI_STA); //WIFI_STA - Devices that connect to WiFi network are called stations (STA)
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
    Serial.println();
    
  if(MDNS.begin("esp8266")){
    
    if(debug)
      Serial.println("MDNS has started");
      
  }

  //setup Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/submit_data", HTTP_POST, handlePOST);

  server.onNotFound(handleNotFound);
  
  server.begin();
  //Serial.println("HTTP server started");

//  attachInterrupt(digitalPinToInterrupt(RX_pin), RX_ISR, CHANGE);
  
}

void loop() {
  // put your main code here, to run repeatedly:
  
  server.handleClient();
  
  if(Serial.available()){
    RX_ISR();
  }
  
  if(updatedFields){
    print_args();
  }
  
  delay(500);

}

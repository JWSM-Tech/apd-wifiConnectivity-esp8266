#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

#define baudRate 115200

#define pillContainersCount 8

const size_t JSON_SIZE = JSON_OBJECT_SIZE(30);

StaticJsonDocument<JSON_SIZE> doc;

JsonObject object;

String json;

bool updatedFields = false;


char ssid[] = "SweetPotatoJam";      //  your network SSID (name)
char password[] = "jEVezYP92*BRPiyC8zxhceAF";   // your network password


//char ssid[] = "ATT-HOMEBASE-5077";
//char password[] = "04148468";

//String pillNames[pillContainersCount];
//
//String qty[pillContainersCount];
//
//String Hour = "0";
//String Minute = "0";


struct alarm{

    unsigned char hour;
    unsigned char minute;
    int quantities[8];
    char pill_names[8][20];

};

struct alarm schedule[8];

ESP8266WebServer server(80);


void handleRoot(){
  server.send(200, "text/plain", "hello from esp8266");
}

void handlePOST(){

  //check plain

  if(server.arg("plain")){
//      Serial.println(server.arg("plain"));
      String json;
      for( int i = 0; i < server.arg("plain").length(); i++){
        json += server.arg("plain")[i];
      }
//      Serial.println(json);
      deserializeJson(doc, json);
      //deserializeJson(doc, json);
      object = doc.as<JsonObject>();
  }
    updatedFields = true;
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
      Serial.print(object["pillNames"][7].as<char*>());
      Serial.print("] ");

      Serial.print("pillQuantities:[");
      
      for(int i=0; i<pillContainersCount-1; i++){
        delay(10);
        Serial.print(object["pillQuantities"][i].as<int>());
        Serial.print(",");
      }
      
      Serial.print(object["pillQuantities"][7].as<int>());
      Serial.print("]\n");

      updatedFields = false;
      
}



void setup() {
  // put your setup code here, to run once:
  
  Serial.begin(baudRate);
  
  Serial.println();

  Serial.print("WiFi connecting to ");

  Serial.println(ssid);
  WiFi.mode(WIFI_STA); //WIFI_STA - Devices that connect to WiFi network are called stations (STA)
  WiFi.begin(ssid, password);
  
  while( WiFi.status() != WL_CONNECTED){ // while not Wireless Lan Connected
    delay(500);
    Serial.print(".");
  }
      
  Serial.println();
  Serial.println("Connected to WiFi successfully");

  Serial.print("NODEMCU IP Address : ");

  Serial.println(WiFi.localIP());

  if(MDNS.begin("esp8266")){
    Serial.println("MDNS responder started");
  }

  //setup Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/submit_data", HTTP_POST, handlePOST);

  server.onNotFound(handleNotFound);
  
  server.begin();
  Serial.println("HTTP server started");
  
}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();

  if(updatedFields){
    print_args();
  }
  
  delay(500);

}

/*********
Door Control Sonoff SV

Luke Jones - https://github.com/ozjonesy/Door

Credit - Rui Santos Complete project details at http://randomnerdtutorials.com
*********/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

MDNSResponder mdns;

// Replace with your network credentials
const char* ssid = "network";
const char* password = "password";
const char* boardName = "door";
const char* OTAPassword = "password";

ESP8266WebServer server(80);

int gpio13Led = 13;
int gpio12Relay = 12;
int gpio14Input = 14;
int gpio5Input = 5;
int gpio4Input = 4;
int gpio0Input = 0;
int gpio14State = HIGH;
int gpio5State = HIGH;
int gpio4State = HIGH;
WiFiClient client;

// List of other Sonoff's FQDN
const char* Front = "FrontLights.example.com";
const char* Garage = "GarageLights.example.com";

void unlock() {
  digitalWrite(gpio12Relay, HIGH);
  delay(2000);
  digitalWrite(gpio12Relay, LOW);
}

void unlockWeb() { //Calls turnOn and sends the web page for on status
  String page = "<!DOCTYPE html><html><head><style> .button {display: inline-block; height: 500px;width: 490px; background-image: url('https://pbs.twimg.com/profile_images/378800000335916520/dfa012790a899ca4ab974bbeb13801fe.png'); cursor: pointer; background-color: #fff; border: none; } .button:active { height: 490px;width: 480px;background-size: 480px 490px;}</style></head><body><div style=\"text-align:center;\"><p><a href=\"\"><br><br><br><button class=\"button\"></button></a></p></div></body>";
  server.send(200, "text/html", page);
  unlock();
}

void HTTPGet(const char* site, char* action){
  client.connect(site, 80);
    client.print(String("GET /") + action + " HTTP/1.1\r\n" +
             "Host: " + site + "\r\n" +
             "Connection: close\r\n" +
             "\r\n"
            );
    client.flush();
    client.stop();
}

void flashLed(int numFlash) { //flashes the LED numFlash times and returns it to previous state
  for (int count = 0; count < numFlash; count++) {
    digitalWrite(gpio13Led, HIGH); //LED OFF
    delay(200);
    digitalWrite(gpio13Led, LOW); //LED ON
    delay(200);
  }
}

void checkSwitch() {
    //Check GPIO14 for external switch
  int currentState = digitalRead(gpio14Input); //read external switch state
  if (gpio14State != currentState) { // compare this switch state to the last read state, if not the same then do stuff
    if (currentState == HIGH){ // state is HIGH when not grounded ie. external switch off
      HTTPGet(Front,"off"); //do something if switch is turned off
      gpio14State = currentState;
    }
    else {
      HTTPGet(Front,"on");//do something if switch is turned on
      gpio14State = currentState;
      int count = 0; //counter for number of loops
      int flips = 1; //number of times the switch has been flipped
      bool keepLooping = true;
      int loopState = LOW;
      while (keepLooping) { //Reset loop
        int loopCurrentState = digitalRead(gpio14Input);
        if (flips >= 6){
          flashLed(2);
          HTTPGet(Front,"off");
          ESP.restart();
        }
        else if (count > 200) {
           keepLooping = false;
           digitalWrite(gpio13Led, HIGH); //LED OFF
           delay(200);
           digitalWrite(gpio13Led, LOW); //LED ON
        }
        else if (loopState != loopCurrentState) {
          flips ++;
          count = 0;
          loopState = loopCurrentState;
        }
        count++;
        delay(5);
      }
    }
  }

  //Check GPIO5 for external switch
  currentState = digitalRead(gpio5Input); //read external switch state
  if (gpio5State != currentState) { // compare this switch state to the last read state, if not the same then do stuff
    if (currentState == HIGH){ // state is HIGH when not grounded ie. external switch off
      HTTPGet(Garage,"off");//do something if switch is turned off
      gpio5State = currentState;
    }
    else {
      HTTPGet(Garage,"on");//do something if switch is turned on
      gpio5State = currentState;
    }
  }

  //Check GPIO4 for external switch
  currentState = digitalRead(gpio4Input); //read external switch state
  if (gpio4State != currentState) { // compare this switch state to the last read state, if not the same then do stuff
    if (currentState == HIGH){ // state is HIGH when not grounded ie. external switch off
      HTTPGet(Garage,"off");//do something if switch is turned off
      gpio4State = currentState;
    }
    else {
      HTTPGet(Garage,"on");//do something if switch is turned on
      gpio4State = currentState;
    }
  }
  
}

void checkButton() {
    // Check GPIO0 for internal switch
  int counter = 1;
  if (digitalRead(gpio0Input) == LOW) {
    unlock();

    while (digitalRead(gpio0Input) == LOW) {
      delay(100);
      counter ++;
    }
  }
  if (counter >= 50) { 
    flashLed(2);
    ESP.restart();
  }
}

void setup(void){
  // preparing GPIOs
  pinMode(gpio13Led, OUTPUT);
  digitalWrite(gpio13Led, HIGH);
  
  pinMode(gpio12Relay, OUTPUT);
  digitalWrite(gpio12Relay, LOW);
  pinMode(gpio14Input, INPUT_PULLUP);
  pinMode(gpio5Input, INPUT_PULLUP);
  pinMode(gpio4Input, INPUT_PULLUP);
  pinMode(gpio0Input, INPUT_PULLUP);
 
  Serial.begin(115200);
  Serial.println("start setup"); 
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }
  WiFi.hostname(boardName);
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(gpio13Led, LOW);
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  //OTA
  ArduinoOTA.setHostname(boardName);
  ArduinoOTA.setPassword(OTAPassword);
  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  
  if (mdns.begin(boardName, WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }
  
  Serial.println("Ready");

  server.on("/", unlockWeb);
  
  server.begin();
  Serial.println("HTTP server started");
}
 
void loop(void){
   ArduinoOTA.handle();
  server.handleClient(); //do web server stuff

  checkSwitch();

  checkButton();
} 


//DoorBell announcement to MQTT
//Andy Susanto
//
//Requires PubSubClient found here: https://github.com/knolleary/pubsubclient
//
//ESP8266 Simple MQTT DoorBell Announcer


#include <PubSubClient.h>
#include <ESP8266WiFi.h>


//EDIT THESE LINES TO MATCH YOUR SETUP
#define MQTT_SERVER "192.168.10.100"

char ssid[]= "SSID1";
char password[]= "MYSSIDpassword";
const char* ssid1 = "SSID";
const char* password1 = "MYSSIDpassword";
const char* ssid2 = "SSID1";
const char* password2 = "MYSSIDpassword";
const int DiagPin = LED_BUILTIN;

// GPIO-0 defined as LEDPin is not currently used, reserved for future use
const int LEDPin = 0;
const int DoorBellPin = 2;
int LastDoorBellState = HIGH;
int CurrentDoorBellState = HIGH;
int TimerFlag=false;

// TimerFlag2 is used for a timer to prevent repeated MQTT msgs being sent out due to 
// doorbell switch being repeatedly pressed in quick succession.
int TimerFlag2=false;
char Uptime[]="{\"Uptime\":\"00000000\"}";  

const char* CPUstateTopic = "stat/DoorBell/LWT";
const char* DoorBellInfoTopic = "stat/DoorBellInfo";
const char* DoorBellInfoReqTopic = "cmnd/DoorBellInfo";
const char* DoorBellRestartTopic = "cmnd/DoorBellCPUrestart";
const char* DoorBellResetTopic = "cmnd/DoorBellCPUreset";
const char* DoorBellPressedTopic = "tele/FrontDoorBell";

void callback(char* topic, byte* payload, unsigned int length);
boolean debounce1(boolean last);
String macToStr(const uint8_t* mac);

WiFiClient wifiClient;
PubSubClient client(MQTT_SERVER, 1883, callback, wifiClient);

void timer0_ISR (void){
  // Do some work
  TimerFlag=true;
  TimerFlag2=true;
  // Set-up the next interrupt cycle
  timer0_write(ESP.getCycleCount() + 80000000); //80Mhz CPU -> 10*80M = 1 seconds
}

void setup() {
  delay(1000);
  //initialize the light as an output and set to LOW (off)
  pinMode(DiagPin, OUTPUT);
  pinMode(LEDPin, OUTPUT);
  pinMode(DoorBellPin, INPUT);
  digitalWrite(LEDPin, LOW);

  //start the serial line for debugging
  Serial.begin(115200);
  delay(500);

  noInterrupts();
  timer0_isr_init();
  timer0_attachInterrupt(timer0_ISR);
  timer0_write(ESP.getCycleCount() + 80000000); //80Mhz CPU -> 1*80M = 1 second
  interrupts();

//start wifi subsystem
  strcpy(ssid,ssid1);
  strcpy(password,password1);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
//attempt to connect to the WIFI network and then connect to the MQTT server
  reconnect();
//wait a bit before starting the main loop
  delay(2000);
}



void loop(){
  static int TimerCount=0;
  static int TimerCount2=4;
//reconnect if connection is lost
  if (TimerFlag) {
    TimerCount++;
//only fires after 60 timercount (60s)
    if (TimerCount>=60) {   
      if (client.connected() && WiFi.status() == WL_CONNECTED) {
        client.publish(CPUstateTopic,"Online",true);
        sprintf (Uptime, "{\"Uptime\":\"%d\"}", millis()/1000);
        client.publish(DoorBellInfoTopic,Uptime);
        Serial.print(" esp1 uptime: ");
        Serial.println(Uptime);
      }
      TimerCount=0;
    }
  }

  if (TimerFlag2) {TimerCount2++;}
  if (TimerCount2>10) {TimerCount2=4;}
  CurrentDoorBellState = debounce1(LastDoorBellState);
  if (LastDoorBellState != CurrentDoorBellState) {
    if (CurrentDoorBellState==LOW) {
      if (TimerCount2>=4) {   
        client.publish(DoorBellPressedTopic,"ON");
        TimerCount2=0;
      }
    }
    LastDoorBellState = CurrentDoorBellState;
  }

  TimerFlag=false;
  TimerFlag2=false;

  //reconnect if connection is lost
  if (!client.connected() && WiFi.status() == 3) {reconnect();}

  //maintain MQTT connection
  client.loop();

  //MUST delay to allow ESP8266 WIFI functions to run
  delay(10); 
}


void callback(char* topic, byte* payload, unsigned int length) {

  //convert topic to string to make it easier to work with
  String topicStr = topic; 
  char msg[30];

  //Print out some debugging info
  Serial.println("Callback update.");
  Serial.print("Topic: ");
  Serial.println(topicStr);
  Serial.println(payload[0]);

  if((topicStr == DoorBellInfoReqTopic) && (payload[0] == 49)){ 
    client.publish(CPUstateTopic,"Online",true);
    sprintf (Uptime, "{\"Uptime\":\"%d\"}", millis()/1000);
    client.publish(DoorBellInfoTopic,Uptime);
    Serial.print(" Doorbell ESP1 uptime: ");
    Serial.println(Uptime);
  }
  if((topicStr==DoorBellRestartTopic) && (payload[0] == 49)){ ESP.restart();}
  if((topicStr==DoorBellResetTopic) && (payload[0] == 49)){ ESP.reset();}
}


void reconnect() {
  int attempt = 0;
  int DiagState = 0;
  //attempt to connect to the wifi if connection is lost
  if(WiFi.status() != WL_CONNECTED){
    //debug printing
    Serial.print("Connecting to ");
    Serial.println(ssid);

    //loop while we wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      digitalWrite(DiagPin, DiagState);
      attempt++;
      Serial.println(attempt);
      if(attempt%30==0){
        if(ssid==ssid1){
           strcpy(ssid,ssid2);
           strcpy(password,password2);
        } else {
           strcpy(ssid,ssid1);
           strcpy(password,password1);
        }
        WiFi.disconnect();
        WiFi.begin(ssid, password);
        Serial.print("Switching SSID to ");
        Serial.println(ssid);
      }
      if(attempt>120){ESP.restart();}
      Serial.print("WiFi.status()=");
      Serial.print(WiFi.status());
      Serial.print(" Attempt=");
      if(DiagState>60){ 
        DiagState=0; 
        Serial.println();
      }
      DiagState+=1;
    }
    //print out some more debug once connected
    Serial.println("");
    Serial.println("WiFi connected");  
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());
  }
  //make sure we are connected to WIFI before attemping to reconnect to MQTT
  if(WiFi.status() == WL_CONNECTED){
  // Loop until we're reconnected to the MQTT server
    attempt=0;
    while (!client.connected()) {
      Serial.print("Attempting MQTT connection...");

      // Generate client name based on MAC address and last 8 bits of microsecond counter
      String clientName;
      clientName += "DoorBellESP1-";
      uint8_t mac[6];
      WiFi.macAddress(mac);
      clientName += macToStr(mac);
      attempt++;
      if(attempt>20){ESP.reset();}
      //if connected, subscribe to the topic(s) we want to be notified about
      if (client.connect((char*) clientName.c_str(),CPUstateTopic,2,true,"Offline")) {
        Serial.println("\tMQTT Connected");
        digitalWrite(DiagPin, LOW);
        client.publish(CPUstateTopic,"Online",true);
        client.subscribe(DoorBellRestartTopic);
        client.subscribe(DoorBellResetTopic);
        client.subscribe(DoorBellInfoReqTopic);

  // otherwise print failed for debugging
      } else {
 //       Serial.println("\tFailed."); 
        abort();
      }
    }
  }
}

//generate unique name from MAC addr
String macToStr(const uint8_t* mac){

  String result;

  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);

    if (i < 5){
      result += ':';
    }
  }
  return result;
}

boolean debounce1(boolean last)
{
  boolean current = digitalRead(DoorBellPin);
  if (last != current)
  {
    delay(5);
    current = digitalRead(DoorBellPin);
  }
  return current;
}


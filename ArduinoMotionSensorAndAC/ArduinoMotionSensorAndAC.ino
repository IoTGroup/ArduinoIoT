#include <Thread.h>
#include <ThreadController.h>
#include <ArduinoJson.h>
#include <IRremote.h>

#define IR_PIN 9
#define LED_PIN 4
#define ON "on"
#define OFF "off"
#define AC_MAX 31
#define AC_MIN 16

#define MAX 512
#define TIME 3000
#define BEACON "OK"
#define UUID "97845dr2-mega-431d-adb2-eb6b9e346018"
#define CIB_SENSORS "context.ambient.sensors" 
#define AC_ACTUATOR "control.ambient.ac.yang"
//motion sensor
#define MOTION_SENSOR "control.ambient.motion.sensor"

#define ERROR_MESSAGE -1
#define BEACON_MESSAGE 0
#define GET_REQUEST_MESSAGE 1
#define GET_RESPONSE_MESSAGE 2
#define SET_REQUEST_MESSAGE 3
#define SET_RESPONSE_MESSAGE 4

#define TOKEN1 "MessageType"
#define TOKEN2 "UUID"
#define TOKEN3 "Timeout"
#define TOKEN4 "CIB"
#define TOKEN5 "Precision"
#define TOKEN6 "Data"
#define TOKEN7 "Timestamp"
#define TOKEN8 "APPID"

#define MOTIONSENSOR 11
#define TEMGENTE 12


IRsend irsend;
Thread threadBeacon;

typedef struct {
  int messageType; 
  const char* uuid; 
  int timeout;
  const char* cib;
  double precision;
  const char* data;
  unsigned long timestamp;  
  const char* appid;
} Message;

void on_yang_ac();
void off_yang_ac();
int yang_ac(int value);

String setAC(String command);

void blinkingLED(); // pisca o led ao transmitir pelo infravermelho
void sendBeacon(); 
String getListSensointrs();
String encoding(int messageType, String uuid, int timeout, String cib, double precision, String data, unsigned long timestamp, String appid);
Message decoding(String json);
void interpreter(String json);
int getMotionData();

int statusMotionSensor;

void setup() {
  Serial.begin(115200); 
  Serial1.begin(57600);
  
  Serial.setTimeout(50);
  Serial1.setTimeout(50);
  
  threadBeacon.setInterval(TIME);
  threadBeacon.onRun(sendBeacon); 
  
  pinMode(IR_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  
  digitalWrite(LED_PIN, LOW);
  
  pinMode(TEMGENTE, OUTPUT);
  pinMode(MOTIONSENSOR, INPUT);
}

void loop() {    
  
  if (threadBeacon.shouldRun()) {
      threadBeacon.run();    
  } 
 
  if (Serial1.available() > 0) {
    // serial 1 utilizada pelo bluetooth
    String str = Serial1.readString();    
    Serial.println(str);
    interpreter(str);
  }
  
  //getMotionData(); 
}

// recebe do Loccam
void interpreter(String json) {
  String response = "";
  Message msg = decoding(json);
  
  switch(msg.messageType){ 
  case GET_REQUEST_MESSAGE:
    if (String(msg.cib) == CIB_SENSORS) { // sensores
      response = encoding(GET_RESPONSE_MESSAGE, UUID, TIME, msg.cib, 1.0, getListSensors(), millis(), msg.appid); 
    }
    else if (String(msg.cib) == MOTION_SENSOR){
      // MOTION SENSOR
      response = encoding(GET_RESPONSE_MESSAGE, UUID, TIME, msg.cib, 1.0,String(getMotionData()) , millis(), msg.appid); 
    }
    else {
      response = encoding(ERROR_MESSAGE, UUID, -1, "", -1.0, "Failure interpreter!", millis(), "");
    }
    break;
    
  case SET_REQUEST_MESSAGE: 
    if (String(msg.cib) == AC_ACTUATOR) { // atuadores
      String command = String(msg.data);
      String data = setAC(command); 
      response = encoding(SET_RESPONSE_MESSAGE, UUID, TIME, msg.cib, 1.0, data, millis(), msg.appid);
    } else {
      response = encoding(ERROR_MESSAGE, UUID, -1, "", -1.0, "Failure interpreter!", millis(), "");
    }
    break;
    
  default:
    response = encoding(ERROR_MESSAGE, UUID, -1, "", -1.0, "Failure interpreter!", millis(), "");
    break;
  }
  
  Serial.println(response);
  Serial1.println(response);
}

String encoding(int messageType, String uuid, int timeout, String cib, double precision, String data, unsigned long timestamp, String appid) {
  StaticJsonBuffer<MAX> jsonBufferObject;

  JsonObject& object = jsonBufferObject.createObject();
  object[TOKEN1] = messageType; 
  object[TOKEN2] = uuid; 
  object[TOKEN3] = timeout;
  object[TOKEN4] = cib;
  object[TOKEN5] = precision;
  object[TOKEN6] = data; 
  object[TOKEN7] = timestamp; 
  object[TOKEN8] = appid;
  
  char buffer[MAX];
  memset(buffer, 0, sizeof(buffer));
  object.printTo(buffer, sizeof(buffer));
  return String(buffer);
}

Message decoding(String json) {
  Message msg;
  char buffer[MAX];
  
  StaticJsonBuffer<MAX> jsonBufferObject;
  memset(buffer, 0, sizeof(buffer));
  json.toCharArray(buffer, sizeof(buffer));
  JsonObject& object = jsonBufferObject.parseObject(buffer);

  if (!object.success()) {
    Serial.println("parseObject() failed!");
    msg.messageType = ERROR_MESSAGE;
    msg.uuid = "";
    msg.timeout = -1;
    msg.cib = "";
    msg.precision = -1.0;
    msg.data = "Parse object failed!";
    msg.timestamp = 0;
    msg.appid = "";
    return msg;
  }

  msg.messageType = object[TOKEN1];         
  msg.uuid = object[TOKEN2];                
  msg.timeout = object[TOKEN3];             
  msg.cib = object[TOKEN4];     
  msg.precision = object[TOKEN5];
  msg.data = object[TOKEN6];  
  msg.timestamp = object[TOKEN7];   
  msg.appid = object[TOKEN8];
  return msg;
}

String getListSensors() {
  String str = "";
  str += AC_ACTUATOR;
  str += ",";
  str += MOTION_SENSOR;
  return str;
}

void sendBeacon() {;
  String beacon = encoding(BEACON_MESSAGE, UUID, TIME, "", 1.0, BEACON, millis(), "");
  Serial.println(beacon);
  Serial1.println(beacon);
} 

int yang_ac(int temp) {
  if ((temp < AC_MIN) || (temp > AC_MAX)) {
    return -1;
  } else {
    if (temp == 16) {
      unsigned int rawTemp16[228] = {3450,1650,500,1350,500,1250,550,400,500,450,500,450,500,1250,550,400,550,400,500,1350,500,1300,500,400,550,1250,500,450,500,400,550,1250,550,1250,550,450,500,1300,500,1300,500,400,550,400,500,1300,500,450,500,400,550,1300,550,400,500,450,500,400,550,400,500,450,500,400,550,400,550,400,550,400,550,400,550,400,500,450,500,400,550,400,550,400,500,500,500,400,550,1250,500,450,500,400,550,1250,550,400,550,400,500,1350,500,1300,500,400,550,400,500,450,500,400,550,400,500,450,500,1350,500,1250,550,1250,550,1250,550,400,500,400,550,400,550,400,500,1350,500,1300,500,400,550,400,550,400,500,450,500,400,550,400,500,500,500,400,550,400,500,450,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,450,500,400,550,450,500,450,500,400,550,400,550,400,500,450,500,400,550,400,500,500,500,400,550,400,500,450,500,450,500,400,550,400,500,450,500,450,550,1250,550,1250,500,1300,500,450,500,400,550,1250,550,400,500,}; //263B8D3C
      irsend.sendRaw(rawTemp16, 228, 38); 
      return 1;
    } else if (temp == 17) { 
      unsigned int rawTemp17[228] = {3500,1650,550,1250,600,1200,550,400,550,400,550,350,600,1200,550,400,550,400,550,1250,600,1200,600,350,550,1250,550,400,550,350,600,1200,550,1250,550,450,550,1250,550,1200,600,350,550,400,550,1250,550,350,600,350,600,1250,550,400,550,350,600,350,550,400,550,400,550,350,600,350,550,450,550,350,600,350,550,400,550,400,550,350,600,350,550,400,550,400,600,350,550,1250,550,400,550,350,600,1200,600,350,550,400,550,1250,600,1200,600,350,550,400,550,350,600,350,600,350,550,400,550,400,600,1200,550,1250,550,1250,550,400,550,350,600,350,550,400,550,1300,550,1200,600,350,550,400,550,400,550,350,600,350,550,400,550,400,600,350,550,400,550,400,550,350,600,350,550,400,550,350,600,400,550,400,550,400,550,350,600,350,550,400,550,350,600,350,550,450,550,350,600,350,600,350,550,400,550,350,600,350,550,400,550,400,600,350,550,400,550,400,550,350,600,350,550,400,550,350,600,1250,600,350,550,1250,550,1250,550,350,600,350,600,1200,550,400,550,};  //58DF8FB9
      irsend.sendRaw(rawTemp17, 228, 38); 
      return 1;
    } else if (temp == 18) {
      unsigned int rawTemp18[228] = {3400,1650,550,1300,550,1250,550,400,500,450,500,400,550,1250,550,400,500,450,500,1350,500,1250,550,400,500,1300,500,450,500,400,550,1250,550,1250,500,500,500,1300,500,1300,500,400,550,400,500,1300,500,450,500,400,550,1300,500,450,500,450,500,400,550,400,500,450,500,400,550,400,500,500,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,450,550,400,500,1300,500,450,500,450,500,1250,550,400,500,450,500,1350,500,1250,550,400,500,450,500,450,500,400,550,400,500,450,500,1350,500,400,550,1250,500,1300,500,450,500,450,500,400,550,400,500,1350,500,1300,500,400,550,400,500,450,500,450,500,400,500,450,500,500,500,400,550,400,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,450,500,400,550,400,500,450,500,450,500,400,550,450,500,450,500,400,550,400,500,450,500,450,500,400,550,400,500,500,500,400,550,400,500,450,500,400,550,400,500,450,500,450,500,450,550,400,500,1300,500,1300,500,400,550,400,550,1250,550,400,550,}; //B98B2E1
      irsend.sendRaw(rawTemp18, 228, 38); 
      return 1;
    } else if (temp == 19) {
      unsigned int rawTemp19[228] = {3450,1700,500,1300,500,1300,500,400,550,400,550,400,500,1250,550,450,500,400,550,1300,500,1300,500,400,550,1300,500,400,550,400,500,1300,500,1300,500,450,550,1250,550,1250,550,400,500,450,500,1300,500,400,550,400,500,1350,500,400,550,400,500,450,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,450,500,400,550,450,500,450,500,1300,500,400,550,400,500,1300,550,400,500,400,550,1300,550,1250,500,450,500,450,500,400,550,400,500,450,500,400,550,450,500,450,500,1300,500,1300,500,400,550,400,500,450,500,400,550,1300,550,1250,550,400,500,450,500,400,550,400,500,450,500,400,550,450,500,450,500,400,550,400,500,450,500,450,500,400,550,400,500,500,500,400,550,400,500,450,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,450,500,400,550,450,500,450,500,450,500,400,550,400,500,450,500,400,550,400,500,1350,550,1250,500,450,500,1250,550,400,500,450,500,1300,500,400,550,}; //72492A3D
      irsend.sendRaw(rawTemp19, 228, 38); 
      return 1;
    } else if (temp == 20) {  
      unsigned int rawTemp20[228] = {3500,1650,500,1300,500,1300,550,400,550,400,500,400,550,1250,500,450,550,400,500,1300,550,1250,500,450,550,1250,500,450,500,400,550,1250,500,1300,550,450,500,1250,550,1250,550,400,550,400,550,1250,500,400,550,400,550,1300,550,400,500,400,550,400,550,400,550,400,500,400,550,400,550,450,500,400,550,400,550,400,500,400,550,400,550,400,550,400,500,450,550,400,550,1250,550,400,500,400,550,1250,550,400,550,400,500,1300,550,1250,550,400,550,400,500,400,550,400,550,400,550,400,500,1300,550,1250,550,400,550,1250,550,400,500,400,550,400,550,400,500,1300,550,1250,550,400,550,400,550,400,500,400,550,400,550,400,500,450,550,400,550,400,550,400,500,400,550,400,550,400,500,400,550,450,550,400,500,400,550,400,550,400,550,400,500,400,550,400,550,450,500,400,550,400,550,400,550,400,500,400,550,400,550,400,500,450,550,400,550,400,550,400,500,400,550,400,550,400,500,400,550,450,550,1250,550,400,500,1300,500,400,550,400,550,1250,550,400,500,}; //69E09267
      irsend.sendRaw(rawTemp20, 228, 38); 
      return 1;
    } else if (temp == 21) {  
      unsigned int rawTemp21[228] = {3450,1650,500,1300,550,1250,550,400,500,450,500,400,550,1250,550,400,500,450,500,1350,500,1250,550,400,500,1300,500,450,500,450,500,1250,550,1250,550,450,500,1300,500,1300,500,400,550,400,500,1300,500,450,500,400,550,1300,500,450,500,450,500,400,550,400,500,450,500,400,550,400,500,500,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,500,500,400,550,1250,500,450,500,450,500,1250,550,400,500,450,500,1350,500,1300,500,400,550,400,500,450,500,400,550,400,500,450,500,450,550,1250,550,400,500,1300,500,450,500,400,550,400,500,450,500,1350,500,1250,550,400,500,450,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,450,500,400,550,400,500,450,500,400,550,400,500,500,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,450,500,450,500,1300,550,400,500,450,500,1300,500,400,550,400,500,1300,500,450,500,}; //65631E03
      irsend.sendRaw(rawTemp21, 228, 38); 
      return 1;
    } else if (temp == 22) {  
      unsigned int rawTemp22[228] = {3450,1650,550,1300,500,1300,500,400,550,400,500,450,500,1300,500,400,550,400,550,1300,500,1300,500,450,500,1250,550,400,500,450,500,1300,500,1300,500,450,550,1250,500,1300,500,450,500,400,550,1250,550,400,500,450,500,1350,500,400,550,400,500,450,500,400,550,400,500,450,500,450,500,450,550,400,500,450,500,400,550,400,500,450,500,450,500,400,550,450,500,450,500,1250,550,400,550,400,500,1300,500,450,500,400,550,1300,500,1300,500,450,500,400,550,400,500,450,500,450,500,400,550,1300,550,400,500,400,550,1250,550,400,500,450,500,450,500,400,550,1300,500,1300,500,450,500,400,550,400,500,450,500,400,550,400,500,500,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,450,550,400,550,400,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,450,500,400,550,400,500,450,500,450,500,400,550,450,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,500,500,400,550,400,500,1300,500,450,500,400,550,1250,500,450,500,}; //DE63DA41
      irsend.sendRaw(rawTemp22, 228, 38); 
      return 1;
    } else if (temp == 23) {  
      unsigned int rawTemp23[228] = {3450,1700,500,1300,500,1300,500,450,500,400,550,400,500,1300,500,450,500,400,550,1300,500,1300,500,450,500,1300,500,400,550,400,500,1300,500,1300,500,450,550,1250,550,1250,500,450,500,450,500,1250,550,400,550,400,500,1350,500,400,550,400,500,450,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,450,500,400,550,450,500,450,500,1300,500,400,550,400,500,1300,500,450,500,400,550,1300,500,1300,500,450,500,400,550,400,550,400,500,450,500,400,550,450,500,450,500,400,550,1250,550,400,500,450,500,400,550,400,550,1300,500,1300,500,450,500,400,550,400,500,450,500,400,550,400,500,500,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,450,500,400,550,400,500,450,500,400,550,400,550,450,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,1350,500,1300,500,1250,550,400,500,450,500,450,500,1250,550,400,500,}; //8C890A43
      irsend.sendRaw(rawTemp23, 228, 38); 
      return 1;
    } else if (temp == 24) {  
      unsigned int rawTemp24[228] = {3450,1700,500,1300,500,1300,500,450,500,400,550,400,500,1300,500,450,500,400,550,1300,550,1250,500,450,500,1300,500,400,550,400,500,1300,500,1300,500,500,500,1250,550,1250,550,400,500,450,500,1300,500,400,550,400,500,1350,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,1300,500,400,550,400,500,1300,500,450,500,450,500,1300,550,1250,500,450,500,450,500,400,550,400,500,450,500,400,550,1300,550,1250,500,1300,500,450,500,400,550,400,500,450,500,450,500,1300,550,1250,550,400,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,450,500,400,550,400,500,450,500,450,500,400,550,450,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,500,500,400,550,400,500,450,500,400,550,400,500,450,500,450,500,450,550,400,500,450,500,400,550,400,500,450,500,450,500,400,550,450,500,1300,500,1300,500,400,550,400,500,450,500,1300,500,400,550,}; //CF24D42F
      irsend.sendRaw(rawTemp24, 228, 38); 
      return 1;
    } else if (temp == 25) {  
      unsigned int rawTemp25[228] = {3450,1650,500,1350,500,1300,500,400,550,400,500,450,500,1300,500,400,550,400,500,1350,500,1300,500,400,550,1250,550,400,500,450,500,1300,500,1250,550,450,500,1300,500,1300,500,450,500,400,550,1250,500,450,500,450,500,1300,550,400,500,450,500,450,500,400,550,400,500,450,500,400,550,450,500,450,500,400,550,400,500,450,500,450,500,400,550,400,500,500,500,400,550,1250,550,400,500,450,500,1300,500,400,550,400,500,1350,500,1300,500,400,550,400,500,450,500,450,500,400,550,400,500,500,500,1300,500,1250,550,400,500,450,500,450,500,400,550,400,500,1350,500,1300,500,400,550,400,500,450,500,450,500,400,550,400,500,500,500,400,550,400,500,450,500,400,550,400,500,450,500,450,500,450,550,400,500,450,500,400,550,400,500,450,500,450,500,400,550,450,500,450,500,400,550,400,500,450,500,450,500,400,550,400,500,500,500,400,550,400,500,450,500,400,550,400,550,400,500,450,500,1300,550,400,500,1300,500,450,500,400,550,400,500,1300,500,450,500,}; //88976ED6
      irsend.sendRaw(rawTemp25, 228, 38); 
      return 1;
    } else if (temp == 26) {  
      unsigned int rawTemp26[228] = {3450,1650,550,1300,500,1300,500,450,500,400,550,400,500,1300,500,450,500,400,550,1300,500,1300,500,450,500,1300,500,400,550,400,500,1300,500,1300,500,450,550,1250,550,1250,500,450,500,450,500,1300,500,400,550,400,500,1350,500,400,550,400,500,450,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,450,500,400,550,450,500,450,500,1300,500,400,550,400,500,1300,500,450,500,400,550,1300,500,1300,500,450,500,450,500,400,550,400,500,450,500,400,550,1300,500,450,500,1300,500,400,550,400,500,450,500,450,500,400,550,1300,500,1300,500,450,500,400,550,400,500,450,500,450,500,400,550,450,500,450,500,400,550,400,500,450,500,450,500,400,550,400,500,500,500,400,550,400,500,450,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,400,550,400,500,450,500,450,500,400,550,450,500,450,500,400,550,400,500,450,500,450,500,400,550,400,500,500,500,400,550,1250,550,400,500,450,500,400,550,1250,550,400,500,}; //8A52189
      irsend.sendRaw(rawTemp26, 228, 38); 
      return 1;
    } else if (temp == 27) {  
      unsigned int rawTemp27[228] = {3450,1650,500,1300,550,1250,550,400,500,450,500,400,550,1250,550,400,500,450,500,1350,500,1250,550,400,500,1300,500,450,500,400,550,1250,550,1250,500,500,500,1300,500,1300,500,400,550,400,500,1300,500,450,500,400,550,1300,500,450,500,450,500,400,550,400,500,450,500,400,550,400,500,500,500,400,550,400,500,450,500,450,500,400,550,400,500,450,500,450,550,400,500,1300,500,450,500,400,550,1250,550,400,500,450,500,1350,500,1250,550,400,500,450,500,450,500,400,550,400,500,450,500,450,550,400,500,1300,500,450,500,400,550,400,500,450,500,450,500,1300,550,1250,500,450,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,450,500,400,550,400,500,450,500,400,550,400,550,450,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,450,550,400,550,400,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,450,500,400,550,400,500,450,500,450,500,400,550,1300,500,1300,500,450,500,400,550,400,500,450,500,1300,500,400,550,}; //312949D3
      irsend.sendRaw(rawTemp27, 228, 38); 
      return 1;
    } else if (temp == 28) {  
      unsigned int rawTemp28[228] = {3450,1650,500,1350,500,1300,500,400,550,400,500,450,500,1300,500,400,550,400,500,1350,500,1300,500,400,550,1250,550,400,500,450,500,1300,500,1250,550,450,500,1300,500,1300,500,400,550,400,550,1250,500,450,500,450,500,1300,550,400,500,450,500,400,550,400,500,450,500,450,500,400,550,450,500,450,500,400,550,400,500,450,500,450,500,400,550,400,500,500,500,400,550,1250,550,400,500,450,500,1300,500,400,550,400,500,1350,500,1300,500,400,550,400,500,450,500,450,500,400,550,400,500,1350,500,1300,500,400,550,400,500,450,500,450,500,400,550,400,500,1350,500,1300,500,400,550,400,500,450,500,400,550,400,500,450,500,500,500,400,550,400,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,450,500,400,550,400,500,450,500,450,500,400,550,450,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,500,500,400,550,400,500,450,500,400,550,400,500,450,500,450,500,450,500,1300,500,450,500,450,500,400,550,400,500,1300,500,450,500,}; //A509ABAC
      irsend.sendRaw(rawTemp28, 228, 38); 
      return 1;
    } else if (temp == 29) {  
      unsigned int rawTemp29[228] = {3450,1700,500,1300,550,1250,550,400,500,400,550,400,500,1300,550,400,500,400,550,1300,550,1250,550,400,500,1300,500,400,550,400,500,1300,550,1250,550,450,500,1250,550,1250,550,400,500,450,500,1300,500,400,550,400,500,1350,500,400,550,400,550,400,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,1300,500,400,550,400,500,1300,550,400,500,400,550,1300,550,1250,550,400,500,450,500,400,550,400,500,450,500,400,550,450,500,1300,500,450,500,400,550,400,500,450,500,450,500,400,550,1300,550,1250,550,400,500,400,550,400,500,450,500,450,500,400,550,450,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,450,550,400,550,400,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,450,500,400,550,400,500,450,500,450,500,400,550,450,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,1350,500,400,550,400,500,450,500,450,500,400,550,1250,550,400,500,}; //8593CCAB
      irsend.sendRaw(rawTemp29, 228, 38); 
      return 1;
    } else if (temp == 30) {  
      unsigned int rawTemp30[228] = {3450,1700,500,1300,500,1300,500,450,500,400,550,400,500,1300,500,450,500,400,550,1300,550,1250,500,450,500,1300,500,400,550,400,500,1300,500,1300,500,450,550,1250,550,1250,550,400,500,450,500,1300,500,400,550,400,500,1350,500,400,550,400,500,450,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,450,500,400,550,450,500,450,500,1300,500,400,550,400,500,1300,500,450,500,400,550,1300,500,1300,500,450,500,450,500,400,550,400,500,450,500,400,550,1300,500,450,500,450,500,400,550,400,500,450,500,400,550,400,550,1300,500,1300,500,450,500,400,550,400,500,450,500,400,550,400,500,500,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,450,500,400,550,400,500,450,500,400,550,400,500,500,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,1300,500,450,500,}; //1EFCFFB1
      irsend.sendRaw(rawTemp30, 228, 38); 
      return 1;
    } else if (temp == 31) {  
      unsigned int rawTemp31[228] = {3500,1650,500,1300,550,1250,500,450,500,450,500,400,550,1250,500,450,500,450,500,1300,550,1250,500,450,500,1300,500,450,500,400,550,1250,500,1300,500,500,500,1250,550,1250,550,400,500,450,500,1300,500,400,550,400,500,1350,500,450,500,400,550,400,500,450,500,400,550,400,500,450,500,500,500,400,550,400,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,1300,500,450,500,400,550,1250,500,450,500,450,500,1300,550,1250,500,450,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,450,500,400,550,400,500,450,500,400,550,400,500,1350,500,1300,500,450,500,400,550,400,500,450,500,400,550,400,500,500,500,400,550,400,550,400,500,450,500,400,550,400,500,450,500,450,550,400,500,450,500,450,500,400,550,400,500,450,500,450,500,450,500,450,500,450,500,400,550,400,500,450,500,400,550,400,500,500,500,400,550,400,500,450,500,450,500,400,550,400,500,450,500,1350,500,1250,550,1250,550,1250,500,1300,500,1300,500,450,500,400,550,}; //D02B526B
      irsend.sendRaw(rawTemp31, 228, 38); 
      return 1;
    }
  }
  return -1;
}

String setAC(String command) {
  int result = 0;
  String response = command;
  
  if (command == OFF) {
    unsigned int rawTurnOff[228] = {3700,1400,550,1300,600,1150,650,300,550,350,650,300,600,1250,550,350,550,400,600,1250,600,1200,600,300,600,1200,600,300,600,400,600,1200,600,1200,550,400,600,1250,550,1200,600,350,550,350,600,1200,600,350,550,400,600,1200,600,350,600,300,650,350,550,350,600,350,600,350,600,300,600,400,600,350,600,300,600,350,600,350,600,350,600,300,600,350,600,350,650,300,600,350,600,350,600,350,600,1200,550,350,600,350,600,1250,600,300,600,350,600,350,550,400,550,350,600,350,600,350,600,1250,550,350,550,400,600,1200,600,350,550,350,600,350,600,350,600,1250,550,1250,550,350,550,400,600,350,550,350,600,350,600,350,600,350,600,350,600,350,600,300,600,350,600,350,600,350,600,300,600,400,600,350,600,300,600,350,600,350,600,350,550,350,600,350,600,400,600,300,600,350,600,350,600,300,600,350,600,350,600,350,600,350,600,350,600,350,600,300,600,350,600,350,600,350,600,300,600,400,600,1200,600,350,550,350,600,350,550,400,600,1200,600,300,600,};
    irsend.sendRaw(rawTurnOff, 228, 38);
    result = 1;
  }
  else if (command == ON) {
    unsigned int rawTurnOn[228] = {3700,1450,500,1300,600,1150,650,350,550,350,650,300,550,1200,600,400,500,400,650,1200,650,1150,600,350,550,1250,550,350,550,400,600,1200,600,1200,600,400,550,1200,600,1200,650,300,550,400,600,1200,550,350,550,400,600,1250,600,350,500,400,600,350,550,400,600,300,600,350,600,350,600,350,650,300,600,350,600,350,550,350,600,350,600,350,600,350,550,400,600,350,550,1250,550,350,550,400,600,1200,600,350,500,400,650,1200,600,350,500,450,600,300,550,400,600,350,550,400,550,350,550,1300,550,400,500,400,650,1150,600,350,500,450,600,350,500,400,600,1250,600,1200,600,350,500,400,650,300,500,450,600,300,550,400,600,400,550,400,550,350,550,400,600,350,550,350,600,350,550,400,550,400,600,350,550,400,600,350,550,350,600,350,550,400,550,400,550,400,600,350,550,400,550,350,600,350,550,400,550,350,600,350,550,450,600,350,550,350,600,350,550,400,550,350,600,350,550,400,550,400,600,1200,600,1200,600,350,500,450,550,350,550,1250,600,350,500,};
    irsend.sendRaw(rawTurnOn, 228, 38);
    result = 1;
  } 
  else { 
    String temp = command.substring(5, command.length());
    int value = temp.toInt();
    result = yang_ac(value);  
  }
  
  if (result != -1) {
    blinkingLED();
  } else {
    response = "Invalid command. Error : " + command;
  }
  
  return response;
}

void blinkingLED() {
  digitalWrite(LED_PIN, HIGH);
  delay(100);
  digitalWrite(LED_PIN, LOW);
  delay(50);
}

int getMotionData(){
  // to get motion sensor
  int statusMotionSensor = digitalRead(MOTIONSENSOR); //Le o valor do sensor PIR
  //digitalWrite(TEMGENTE, HIGH);
 if (statusMotionSensor == LOW)  //Sem movimento, mantem led azul ligado
 {
    digitalWrite(TEMGENTE, LOW);
    Serial.println("no one");
 }
 else  //Caso seja detectado um movimento, aciona o led vermelho
 {
    digitalWrite(TEMGENTE, HIGH);
    Serial.println("moviment detected");
 }
 
 return statusMotionSensor;
} 

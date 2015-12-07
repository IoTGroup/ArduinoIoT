#include <Thread.h>
#include <ThreadController.h>
#include <ArduinoJson.h>
#include <IRremote.h>


#define MAX 512
#define TIME 3000
#define BEACON "OK"
#define UUID "97845dr2-mega-431d-adb2-eb6b9e346018"
#define CIB_SENSORS "context.ambient.sensors" 
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


void sendBeacon(); 
String getListSensors();
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
  
  pinMode(TEMGENTE, OUTPUT);
  pinMode(MOTIONSENSOR, INPUT);
}

void loop() {    
  
  if (threadBeacon.shouldRun()) {
      threadBeacon.run();    
  } 
 
  if (Serial1.available() > 0) {
    String str = Serial1.readString();    
    Serial.println(str);
    interpreter(str);
  }
  
  getMotionData(); 
}

void interpreter(String json) {
  String response = "";
  Message msg = decoding(json);
  
  switch(msg.messageType){ 
  case GET_REQUEST_MESSAGE:
    if (String(msg.cib) == CIB_SENSORS) { // recebe mensagem pelo bluetooth
      response = encoding(GET_RESPONSE_MESSAGE, UUID, TIME, msg.cib, 1.0, getListSensors(), millis(), msg.appid); 
    }
    else {
      response = encoding(ERROR_MESSAGE, UUID, -1, "", -1.0, "Failure interpreter!", millis(), "");
    }
    break;
    
  case SET_REQUEST_MESSAGE: 
    if (String(msg.cib) == AC_ACTUATOR) {
      String command = String(msg.data);
      String data = setAC(command); // envia comando pelo IR
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
  return str;
}

void sendBeacon() {;
  String beacon = encoding(BEACON_MESSAGE, UUID, TIME, "", 1.0, BEACON, millis(), "");
  Serial.println(beacon);
  Serial1.println(beacon);
} 




void getMotionData(){
  // to get motion sensor
  statusMotionSensor = digitalRead(MOTIONSENSOR); //Le o valor do sensor PIR
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

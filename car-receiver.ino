
#include <SPI.h>
#include <DW1000.h>
#include <ArduinoJson.h>

// Motor
#define Left_Motor 25
#define Right_Motor 26
#define forward 32
#define reverse 33
#define shoot 27

// JoyStick Ranges

#define VERT_FORWARD_MAX 4000
#define VERT_FORWARD_MIN  2100

#define VERT_REVERSE_MAX  0
#define VERT_REVERSE_MIN  1600

#define HORI_LEFT_MAX  4000
#define HORI_LEFT_MIN  2200

#define HORI_RIGHT_MAX  0
#define HORI_RIGHT_MIN  1600


// connection pins
const uint8_t PIN_SCK = 18;
const uint8_t PIN_MOSI = 23;
const uint8_t PIN_MISO = 19;
const uint8_t PIN_SS = 2;
const uint8_t PIN_RST = 15;
const uint8_t PIN_IRQ = 17;

int verti;
int hori;
int state;
int id;


// DAC
long left_dac;
long right_dac;
// DEBUG packet sent status and count
volatile boolean received = false;
volatile boolean error = false;
volatile int16_t numReceived = 0; // todo check int type
String message;
int _val = 0;

void setup() {
  // motor
  

  pinMode(forward, OUTPUT);
  pinMode(reverse, OUTPUT);
  pinMode(shoot, OUTPUT);
  digitalWrite(shoot,LOW);
  // DEBUG monitoring
  Serial.begin(9600);
  Serial.println(F("### DW1000-arduino-receiver-test ###"));
  // initialize the driver
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_SS);
  Serial.println(F("DW1000 initialized ..."));
  // general configuration
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(6);
  DW1000.setNetworkId(10);
  DW1000.enableMode(DW1000.MODE_LONGDATA_RANGE_LOWPOWER);
  DW1000.setChannel(DW1000.CHANNEL_4);
  DW1000.commitConfiguration();
  Serial.println(F("Committed configuration ..."));
  // DEBUG chip info and registers pretty printed
  char msg[128];
  DW1000.getPrintableDeviceIdentifier(msg);
  Serial.print("Device ID: "); Serial.println(msg);
  DW1000.getPrintableExtendedUniqueIdentifier(msg);
  Serial.print("Unique ID: "); Serial.println(msg);
  DW1000.getPrintableNetworkIdAndShortAddress(msg);
  Serial.print("Network ID & Device Address: "); Serial.println(msg);
  DW1000.getPrintableDeviceMode(msg);
  Serial.print("Device mode: "); Serial.println(msg);
  // attach callback for (successfully) received messages
  DW1000.attachReceivedHandler(handleReceived);
  DW1000.attachReceiveFailedHandler(handleError);
  DW1000.attachErrorHandler(handleError);
  // start reception
  receiver();
}

void handleReceived() {
  // status change on reception success
  received = true;
}

void handleError() {
  error = true;
}

void receiver() {
  DW1000.newReceive();
  DW1000.setDefaults();
  // so we don't need to restart the receiver manually
  DW1000.receivePermanently(true);
  DW1000.startReceive();
}

void loop() {
  // enter on confirmation of ISR status change (successfully received)
  StaticJsonBuffer<200> jsonBuffer;
  if (received) {
    numReceived++;
    // get data as string
    DW1000.getData(message);
    JsonObject& root = jsonBuffer.parseObject(message);
    if (!root.success()) {
      Serial.println("parseObject() failed");
      return;
    }
    id = root["id"];
    verti = root["x"];
    hori = root["y"];
    state = root["b"];
    if (id == 99) {
          if (state == 0) {
          digitalWrite(shoot,HIGH);  
          }
          else if ( state == 1) {
            digitalWrite(shoot,LOW);
          }
          if (verti > VERT_FORWARD_MIN) {
            foward();
          } else if (hori > HORI_LEFT_MIN) {
            turnLeft();
          } else if (hori < HORI_RIGHT_MIN) {
           turnRight();
          } else if (verti < VERT_REVERSE_MIN) {
            backward();
          } else {
           Stop();
          }

          Serial.print("VERT: ");
          Serial.print(verti);
          Serial.print(", HORI: ");
          Serial.print(hori);
          Serial.print(", STATE: "); 
          Serial.print(state);
          Serial.print(", LEFT_DAC: ");
          Serial.print(left_dac);
          Serial.print(", RIGHT_DAC: ");
          Serial.print(right_dac);
          Serial.println();
      
          
          dacWrite(Left_Motor,left_dac);
          dacWrite(Right_Motor,right_dac);
          received = false;
        }
        if (error) {
          Serial.println("Error receiving a message");
          error = false;
          DW1000.getData(message);
          Serial.print("Error data is ... "); Serial.println(message);
        }
        delay(100);
    } else {
      delay(100);
    }
}

long map_limit(int x,int x_low, int x_high, int y_low, int y_high) {
    long dac_val = map(x,x_low,x_high,y_low,y_high);
    if (dac_val > 255){
      dac_val = 255;  
    }
    if (dac_val < 0) {
      dac_val = 0;
    }
    return dac_val;
}

void foward() {
    Serial.println("Forward: ");
    left_dac = map_limit(verti,VERT_FORWARD_MIN,VERT_FORWARD_MAX,0,255);
    right_dac = map_limit(verti,2000,4000,0,255);
    digitalWrite(forward, HIGH);
    digitalWrite(reverse,LOW);
    delay(10);
}

void backward() {
    Serial.println("Backward");
    left_dac = map_limit(verti,VERT_REVERSE_MIN,VERT_REVERSE_MAX,0,255);
    right_dac = map_limit(verti,VERT_REVERSE_MIN,VERT_REVERSE_MAX,0,255);
    digitalWrite(forward, LOW);
    digitalWrite(reverse,HIGH);
    delay(10);
}

void turnLeft() {
    Serial.println("Left");
    left_dac = 0;
    right_dac = map_limit(hori,HORI_LEFT_MIN,HORI_LEFT_MAX,0,255);
    digitalWrite(forward, HIGH);
    digitalWrite(reverse,LOW);
    delay(10);
}

void turnRight() {
    Serial.println("Right");
    left_dac = map_limit(hori,HORI_RIGHT_MIN,HORI_RIGHT_MAX,0,255);
    right_dac = 0;
    digitalWrite(forward, HIGH);
    digitalWrite(reverse,LOW);
    delay(10);
}

void Stop() {
    Serial.println("Stop");
    left_dac = 0;
    right_dac = 0;
    digitalWrite(forward, HIGH);
    digitalWrite(reverse,LOW);
    delay(10);
}

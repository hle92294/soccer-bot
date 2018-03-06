#include <SPI.h>
#include <DW1000.h>
#include <ArduinoJson.h>


#define ANALOG_PIN_0  34 //X 
#define ANALOG_PIN_1  35 //Y
#define BUTTON_PIN    13 //button

// connection pins
const uint8_t PIN_SCK = 18;
const uint8_t PIN_MOSI = 23;
const uint8_t PIN_MISO = 19;
const uint8_t PIN_SS = 2;
const uint8_t PIN_RST = 15;
const uint8_t PIN_IRQ = 17;


// const uint8_t PIN_RST = 9; // reset pin
// const uint8_t PIN_IRQ = 2; // irq pin
// const uint8_t PIN_SS = SS; // spi select pin

// DEBUG packet sent status and count
boolean sent = false;
volatile boolean sentAck = false;
volatile unsigned long delaySent = 0;
int16_t sentNum = 0; // todo check int type
String jsonStr;
DW1000Time sentTime;

//Joystick
int joyX = ANALOG_PIN_1;
int joyY = ANALOG_PIN_0;
int joyButton = BUTTON_PIN;

int xPos = 0;
int yPos = 0;
int buttonState = 0;

void setup() {
  // DEBUG monitoring
  Serial.begin(9600);

  pinMode(joyX, INPUT);
  pinMode(joyY, INPUT);
  pinMode(joyButton, INPUT_PULLUP);

  Serial.println(F("### DW1000-arduino-sender-test ###"));
  // initialize the driver
  DW1000.begin(PIN_IRQ, PIN_RST);
  DW1000.select(PIN_SS);
  Serial.println(F("DW1000 initialized ..."));
  // general configuration
  DW1000.newConfiguration();
  DW1000.setDefaults();
  DW1000.setDeviceAddress(5);
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
  // attach callback for (successfully) sent messages
  DW1000.attachSentHandler(handleSent);
  // start a transmission
  transmitter();
}

void handleSent() {
  // status change on sent success
  sentAck = true;
}

void transmitter() {
  // transmit some data
  Serial.print("Transmitting packet ... #"); Serial.println(sentNum);
  DW1000.newTransmit();
  DW1000.setDefaults();
  
  //String msg = "Hello DW1000, it's #"; msg += sentNum;
  DW1000.setData(jsonStr);
  // delay sending the message for the given amount
  DW1000Time deltaTime = DW1000Time(10, DW1000Time::MILLISECONDS);
  DW1000.setDelay(deltaTime);
  DW1000.startTransmit();
  delaySent = millis();
  jsonStr = "";
}

void loop() {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root  = jsonBuffer.createObject();
  if (!sentAck) {
    return;
  }
  xPos = analogRead(joyX);
  yPos = analogRead(joyY);
  buttonState = digitalRead(joyButton);
  root["id"] = 99;
  root["x"] = xPos;
  root["y"] = yPos;
  root["b"] = buttonState;
  
  root.printTo(jsonStr);
  Serial.println(jsonStr);
  sentAck = false;
  // update and print some information about the sent message
  Serial.print("ARDUINO delay sent [ms] ... "); Serial.println(millis() - delaySent);
  DW1000Time newSentTime;
  DW1000.getTransmitTimestamp(newSentTime);
//  Serial.print("Processed packet ... #"); Serial.println(sentNum);
//  Serial.print("Sent timestamp ... "); Serial.println(newSentTime.getAsMicroSeconds());
//   note: delta is just for simple demo as not correct on system time counter wrap-around
//  Serial.print("DW1000 delta send time [ms] ... "); Serial.println((newSentTime.getAsMicroSeconds() - sentTime.getAsMicroSeconds()) * 1.0e-3);
  sentTime = newSentTime;
  sentNum++;
  // again, transmit some data
  transmitter();
  delay(100);
}

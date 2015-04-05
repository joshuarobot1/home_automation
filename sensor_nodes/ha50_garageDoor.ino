// **********************************************************************************************************
// This program is written to be installed on a moteino built by lowpowerlabs
// 2014-07-14 (C) felix@lowpowerlab.com, http://www.LowPowerLab.com
// **********************************************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this code/library but please abide with the CCSA license:
// http://creativecommons.org/licenses/by-sa/4.0/
// **********************************************************************************

#include <RFM69.h>         //get it here: http://github.com/lowpowerlab/rfm69
#include <SPIFlash.h>      //get it here: http://github.com/lowpowerlab/spiflash
#include <SPI.h>           //comes with Arduino IDE (www.arduino.cc)

//*****************************************************************************************************************************
// ADJUST THE SETTINGS BELOW DEPENDING ON YOUR HARDWARE/SITUATION!
//*****************************************************************************************************************************
#define GATEWAYID   1
#define NODEID      50
#define NETWORKID   101
//#define FREQUENCY     RF69_433MHZ
//#define FREQUENCY     RF69_868MHZ
#define FREQUENCY       RF69_915MHZ //Match this with the version of your Moteino! (others: RF69_433MHZ, RF69_868MHZ)
#define ENCRYPTKEY      "1111111111111111" //has to be same 16 characters/bytes on all nodes, not more not less!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!

#define HALLSENSOR1          4  //open
#define HALLSENSOR0          5  //closed

#define CAR1TRIG             10
#define CAR1ECHO             11
#define CAR2TRIG             8
#define CAR2ECHO             9

#define RELAYPIN1             6
#define RELAYPIN2             7
#define RELAY_PULSE_MS      250  //just enough that the opener will pick it up

#define DOOR_MOVEMENT_TIME 14000 // this has to be at least as long as the max between [door opening time, door closing time]
                                 // my door opens and closes in about 12s
#define STATUS_CHANGE_MIN  5000  // this has to be at least as long as the delay 
                                 // between a opener button press and door movement start
                                 // most garage doors will start moving immediately (within half a second)
//*****************************************************************************************************************************                                 
// added by JWR to handle different sending/receiving structure
typedef struct {		
  int                   nodeID; 
  int			sensorID;
  unsigned long         var1_usl; 
  float                 var2_float; 
  float			var3_float;	
} Payload;
Payload theData;

volatile struct 
{
  int                   nodeID;
  int			sensorID;		
  unsigned long         var1_usl;
  float                 var2_float;
  float			var3_float;
  int                   var4_int;
} 
SensorNode;
                                 
                                 
//*****************************************************************************************************************************
//#define HALLSENSOR_OPENSIDE   0
//#define HALLSENSOR_CLOSEDSIDE 1

#define STATUS_CLOSED        0
#define STATUS_CLOSING       1
#define STATUS_OPENING       2
#define STATUS_OPEN          3
#define STATUS_UNKNOWN       4

#define LED                  9   //pin connected to onboard LED
#define LED_PULSE_PERIOD  5000   //5s seems good value for pulsing/blinking (not too fast/slow)
#define SERIAL_BAUD     115200
#define SERIAL_EN                //comment out if you don't want any serial output

#ifdef SERIAL_EN
  #define DEBUG(input)   {Serial.print(input); delay(1);}
  #define DEBUGln(input) {Serial.println(input); delay(1);}
#else
  #define DEBUG(input);
  #define DEBUGln(input);
#endif

void setStatus(byte newSTATUS, boolean reportStatus=true);
byte STATUS;
unsigned long lastStatusTimestamp=0;
unsigned long ledPulseTimestamp=0;
byte lastRequesterNodeID=GATEWAYID;
int ledPulseValue=0;
boolean ledPulseDirection=false; //false=down, true=up
RFM69 radio;


int openWarning = 0;
unsigned long openStartTime = 0;
unsigned long openCurrentTime = 0;
unsigned long openWarningDelay = 150000;
unsigned long closedTimer = 0;
unsigned long closedTimerDelay = 20000;
int closedStep = 0;

long duration1 = 0;
long duration2 = 0;
int car1Here = 100;
int car2Here = 100;
int car1This = 0;
int car1Last = 0;
int car2This = 0;
int car2Last = 0;
int car1Flag = 0;
int car2Flag = 0;
#define CAR_HERE          1
#define CAR_GONE          0

long cm1 = 0;
long cm2 = 0;

unsigned long doorPulseCount = 0;
char input;

/////////////////////////////////////////////////////////////////////////////
// flash(SPI_CS, MANUFACTURER_ID)
// SPI_CS          - CS pin attached to SPI flash chip (8 in case of Moteino)
// MANUFACTURER_ID - OPTIONAL, 0xEF30 for windbond 4mbit flash (Moteino OEM)
/////////////////////////////////////////////////////////////////////////////
SPIFlash flash(8, 0xEF30);

void setup(void)
{
  Serial.begin(SERIAL_BAUD);
  pinMode(HALLSENSOR1, INPUT);
  pinMode(HALLSENSOR0, INPUT);
  pinMode(CAR1TRIG, OUTPUT);
  pinMode(CAR1ECHO, INPUT);
  pinMode(CAR2TRIG, OUTPUT);
  pinMode(CAR2ECHO, INPUT);
  pinMode(RELAYPIN1, OUTPUT);
  pinMode(RELAYPIN2, OUTPUT);
  pinMode(LED, OUTPUT);
  
    radio.initialize(FREQUENCY,NODEID,NETWORKID);
  #ifdef IS_RFM69HW
    radio.setHighPower(); //uncomment only for RFM69HW!
  #endif
  radio.encrypt(ENCRYPTKEY);

  char buff[50];
  sprintf(buff, "GarageMote : %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  DEBUGln(buff);

  if (digitalRead(HALLSENSOR0)==true)
    setStatus(STATUS_CLOSED);
  else {
    setStatus(STATUS_OPEN);
    openStartTime = millis();
  }
  
  // INIT ULTRASONIC
  digitalWrite(CAR1TRIG, LOW); 
  digitalWrite(CAR2TRIG, LOW); 
}

void loop()
{
  if (Serial.available())
    input = Serial.read();
    
  if (input=='r')
  {
    DEBUGln("Relay test...");
    pulseRelay();
    input = 0;
  }
  

  // ***************************** detect cars *****************************************
 /* 
  // car 1
  digitalWrite(CAR1TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(CAR1TRIG, HIGH);
  delayMicroseconds(5);
  digitalWrite(CAR1TRIG, LOW); 
  
  duration1 = pulseIn(CAR1ECHO, HIGH);
  
  delay(50);
  
  cm1 = microsecondsToCentimeters(duration1);
  Serial.print("car 1");
  Serial.println(cm1);
  
  // figure out if car status has changed
  car1Last = car1This;
  
  if (cm1 > car1Here) {
    car1This = CAR_GONE;
  } else {
    car1This = CAR_HERE;
  }
  
  if (car1This != car1Last) {
    car1Flag = 1;
  } else {
    car1Flag = 0;
  }
   
  // car 2
  digitalWrite(CAR2TRIG, LOW);
  delayMicroseconds(2);
  digitalWrite(CAR2TRIG, HIGH);
  delayMicroseconds(5);
  digitalWrite(CAR2TRIG, LOW); 
  
  duration2 = pulseIn(CAR2ECHO, HIGH);
  
  delay(50);
  
  cm2 = microsecondsToCentimeters(duration2);
  Serial.print("car 2");
  Serial.println(cm2);
  
  // figure out if car status has changed
  car2Last = car2This;
  
  if (cm2 > car2Here) {
    car2This = CAR_GONE;
  } else {
    car2This = CAR_HERE;
  }
  
  if (car2This != car2Last) {
    car2Flag = 1;
  } else {
    car2Flag = 0;
  }
 
  // send data to server if status has changed 
  if (car1Flag || car2Flag) {
    // send data to server
    theData.nodeID = 50;
    theData.sensorID = 3;
    theData.var1_usl = millis();
    theData.var2_float = car1This;
    theData.var3_float = car2This;		
    radio.send(GATEWAYID, (const void*)(&theData), sizeof(theData));
  }
 */  
  // ********************************************************* door status **********************************************  

  if (digitalRead(HALLSENSOR0) == true) {  // if closed sensor is on
    if (STATUS == STATUS_OPEN) {            // if status is open
      setStatus(STATUS_CLOSED, 1);      // change status to closed
      openWarning = 0;
      //Serial.println("stupid");
    } 
  } else {                                 // if closed sensor is off
    if (STATUS == STATUS_CLOSED) {          // if status is closed
      setStatus(STATUS_OPEN, 1);           // set status to open
      openStartTime = millis();            // start open timer
    } 
    
    if (openWarning == 0) {
      openCurrentTime = millis();
      if (openCurrentTime - openStartTime > openWarningDelay) {
        openWarning = 1;
        setStatus(STATUS_OPEN, 1);
      } else {
        openWarning = 0;
      }
    }
  }
    
    
  // ********************************************************** control door ************************************************************************* 
  if (radio.receiveDone()) {
    
      byte newStatus=STATUS;
      boolean reportStatusRequest=false;
      //DEBUG('[');
      //DEBUG(radio.SENDERID, DEC);
      //DEBUG("] ");
      //if (promiscuousMode) {
      //  DEBUG("to [");
      //  DEBUG(radio.TARGETID, DEC);
      //  DEBUG("] ");
      //}
      //DEBUG();
  
      if (radio.DATALEN != sizeof(Payload)) {
        Serial.println(F("Invalid payload received, not matching Payload struct!"));
        //Serial.println(sizeof(Payload));
        //Serial.println(radio.DATALEN);
        //theData = *(Payload*)radio.DATA;
        //Serial.println(theData.var2_float);
      } else {
        theData = *(Payload*)radio.DATA; //assume radio.DATA actually contains our struct and not something else
        Serial.println("Payload received");
        //save it for i2c:
        SensorNode.nodeID = theData.nodeID;
        SensorNode.sensorID = theData.sensorID;
        SensorNode.var1_usl = theData.var1_usl;
        SensorNode.var2_float = theData.var2_float;
        SensorNode.var3_float = theData.var3_float;
        SensorNode.var4_int = radio.RSSI;
  
        DEBUG("Received Device ID = ");
        DEBUG(SensorNode.sensorID);  
        DEBUG ("    ToggleRelay = ");
        DEBUG (SensorNode.var1_usl);
        DEBUG ("    RSSI ");
        DEBUG (SensorNode.var4_int);
  
  
      }
  
   /*
      if (radio.ACK_REQUESTED)
      {
        byte theNodeID = radio.SENDERID;
        radio.sendACK();
  
        // When a node requests an ACK, respond to the ACK
        // and also send a packet requesting an ACK (every 3rd one only)
        // This way both TX/RX NODE functions are tested on 1 end at the GATEWAY
       if (ackCount++%3==0)
        {
          //Serial.print(" Pinging node ");
          //Serial.print(theNodeID);
          //Serial.print(" - ACK...");
          //delay(3); //need this when sending right after reception .. ?
          //if (radio.sendWithRetry(theNodeID, "ACK TEST", 8, 0))  // 0 = only 1 attempt, no retries
          //  Serial.print("ok!");
          //else Serial.print("nothing");
        }
      }//end if radio.ACK_REQESTED */
     
  
        //check for an OPEN/CLOSE/STATUS request
  
        if (SensorNode.var1_usl == 1)
        {
          SensorNode.var1_usl = 0;
          theData.var1_usl = 0;
          pulseRelay();
          
          // clear openHAB switch
          theData.nodeID = 50;
          theData.sensorID = 3;
          theData.var1_usl = 0;
          theData.var2_float = 1; // this will reset the switch
          theData.var3_float = 0;		
          radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
          
          Serial.print("sending clear = ");
          Serial.println(theData.var2_float);
          
          delay(5000);
          
          theData.nodeID = 50;
          theData.sensorID = 3;
          theData.var1_usl = 0;
          theData.var2_float = 0; // clear reset signal
          theData.var3_float = 0;		
          radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
          //else radio.Send(requester, "INVALID", 7);
          
          Serial.print("sending clear = ");
          Serial.println(theData.var2_float);
        }
  
      //first send any ACK to request
      DEBUG("   [RX_RSSI:");DEBUG(radio.RSSI);DEBUG("]");
      if (radio.ACKRequested())
      {
        radio.sendACK();
        DEBUG(" - ACK sent.");
      }
      
      //now take care of the request, if not invalid  
      if (reportStatusRequest)
      {
        reportStatus();
      }
        
      DEBUGln();
  }//end if radio.receive 
  // ********************************************************** door control end **************************************************************
 
 delay(100);
  
} //************************************** END LOOP ***********************************************
/*
//returns TRUE if magnet is next to sensor, FALSE if magnet is away
boolean digitalRead(HALLSENSOR1)(byte which)
{
  //while(millis()-lastStatusTimestamp<STATUS_CHANGE_MIN);
  digitalWrite(which ? HALLSENSOR2_EN : HALLSENSOR1_EN, HIGH); //turn sensor ON
  delay(1); //wait a little
  byte reading = digitalRead(which ? HALLSENSOR2 : HALLSENSOR1);
  digitalWrite(which ? HALLSENSOR2_EN : HALLSENSOR1_EN, LOW); //turn sensor OFF
  return reading==0;
}
*/
void setStatus(byte newSTATUS, boolean reportStatusRequest)
{
  if (STATUS != newSTATUS) lastStatusTimestamp = millis();
  STATUS = newSTATUS;
  if (STATUS != STATUS_OPEN) {
    openStartTime = 0;
    openWarning = 0;
  }
  DEBUGln(STATUS==STATUS_CLOSED ? "CLOSED" : STATUS==STATUS_CLOSING ? "CLOSING" : STATUS==STATUS_OPENING ? "OPENING" : STATUS==STATUS_OPEN ? "OPEN" : "UNKNOWN");
  if (reportStatusRequest)
    reportStatus();
}

boolean reportStatus()
{
  if (lastRequesterNodeID == 0) return false;
  char buff[10];
  sprintf(buff, STATUS==STATUS_CLOSED ? "CLOSED" : STATUS==STATUS_CLOSING ? "CLOSING" : STATUS==STATUS_OPENING ? "OPENING" : STATUS==STATUS_OPEN ? "OPEN" : "UNKNOWN");
  
  // byte len = strlen(buff);
  // return radio.sendWithRetry(lastRequesterNodeID, buff, len);
  
  
  // updated code to work with mqtt server
  theData.nodeID = 50;
  theData.sensorID = 1;
  theData.var1_usl = millis();
  theData.var2_float = STATUS;
  theData.var3_float = openWarning;		
  return radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
}

void pulseRelay()
{
  digitalWrite(RELAYPIN1, HIGH);
  digitalWrite(RELAYPIN2, HIGH);
  delay(RELAY_PULSE_MS);
  digitalWrite(RELAYPIN1, LOW);
  digitalWrite(RELAYPIN2, LOW);
}

void Blink(byte PIN, byte DELAY_MS)
{
  pinMode(PIN, OUTPUT);
  digitalWrite(PIN,HIGH);
  delay(DELAY_MS);
  digitalWrite(PIN,LOW);
}

long microsecondsToCentimeters(long microseconds)
{
  // The speed of sound is 340 m/s or 29 microseconds per centimeter.
  // The ping travels out and back, so to find the distance of the
  // object we take half of the distance travelled.
  return microseconds / 29 / 2;
}



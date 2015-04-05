// **********************************************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this code/library but please abide with the CCSA license:
// http://creativecommons.org/licenses/by-sa/4.0/
// **********************************************************************************

// part of this code was taken from an example teaching how to use the accelerometers used in this project:

/* FILE:    ARD_HMC5803L_GY273_Example
   DATE:    23/10/13
   VERSION: 0.1

This is an example of how to use the Hobby Components GY-273 module (HCMODU0036) which 
uses a Honeywell HMC5883L 3-Axis Digital Compass IC. The IC uses an I2C interface to 
communicate which is compatible with the standard Arduino Wire library.

This example demonstrates how to initialise and read the module in single shot 
measurement mode. It will continually trigger single measurements and output the 
results for the 3 axis to the serial port.


CONNECTIONS:

MODULE    ARDUINO
VCC       3.3V
GND       GND
SCL       A5
SDA       A4
DRDY      N/A


You may copy, alter and reuse this code in any way you like, but please leave
reference to HobbyComponents.com in your comments if you redistribute this code.
This software may not be used for the purpose of promoting or selling products 
that directly compete with Hobby Components Ltd's own range of products.

THIS SOFTWARE IS PROVIDED "AS IS". HOBBY COMPONENTS MAKES NO WARRANTIES, WHETHER
EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED TO, IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, ACCURACY OR LACK OF NEGLIGENCE.
HOBBY COMPONENTS SHALL NOT, IN ANY CIRCUMSTANCES, BE LIABLE FOR ANY DAMAGES,
INCLUDING, BUT NOT LIMITED TO, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY
REASON WHATSOEVER. */

/* Include the standard Wire library */
#include <Wire.h>

/* The I2C address of the module */
#define HMC5803L_Address 0x1E

/* Register address for the X Y and Z data */
#define X 3
#define Y 7
#define Z 5

//RFM69  --------------------------------------------------------------------------------------------------
#include <RFM69.h>
#include <SPI.h>
#define NODEID        40    //unique for each node on same network
#define NETWORKID     101  //the same on all nodes that talk to each other
#define GATEWAYID     1
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
//#define FREQUENCY   RF69_433MHZ
//#define FREQUENCY   RF69_868MHZ
#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "1111111111111111" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define ACK_TIME      30 // max # of ms to wait for an ack
#define LED           9  // Moteinos have LEDs on D9
#define SERIAL_BAUD   9600  //must be 9600 for GPS, use whatever if no GPS

boolean debug = 1;

//struct for wireless data transmission
typedef struct {		
  int               nodeID; 		//node ID (1xx, 2xx, 3xx);  1xx = basement, 2xx = main floor, 3xx = outside
  int               deviceID;		//sensor ID (2, 3, 4, 5)
  unsigned long     var1_usl; 		//uptime in ms
  float             var2_float;   	//sensor data?
  float             var3_float;		//battery condition?
} Payload;
Payload theData;

char buff[20];
byte sendSize=0;
boolean requestACK = false;
RFM69 radio;

// *********** WASHER VARS *************
int yArrayW[20] = {0};
int zArrayW[20] = {0};
int yThisW = 0;
int yLastW = 0;
int yminW = 10000;
int ymaxW = -10000;
int zThisW = 0;
int zLastW = 0;
int zminW = 10000;
int zmaxW = -10000;

int washerStatus = 0;
int washerLastStatus = 0;
int washerStateChange = 1;

// ********* DRYER VARS *************
int yArrayD[20] = {0};
int zArrayD[20] = {0};
int yThisD = 0;
int yLastD = 0;
int yminD = 10000;
int ymaxD = -10000;
int zThisD = 0;
int zLastD = 0;
int zminD = 10000;
int zmaxD = -10000;

int dryerStatus = 0;
int dryerLastStatus = 0;
int dryerStateChange = 1;
// ******* OTHER VARS ***********
int switcher = 0;
int mux0 = 4;
int count = 0;
//int mux1 = 5;

void setup() 
{
  Serial.begin(115200);
  Serial.println("begin setup");

  //RFM69-------------------------------------------
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  #ifdef IS_RFM69HW
    radio.setHighPower(); //uncomment only for RFM69HW!
  #endif
  radio.encrypt(ENCRYPTKEY);
  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  theData.nodeID = NODEID;  //this node id should be the same for all devices in this node
  //end RFM--------------------------------------------

  /* Initialise the Wire library */
  Wire.begin();
  delay(200);
  
  /* Initialise the module */ 
  Init_HMC5803L();
  
  delay(500);
  
  pinMode(mux0, OUTPUT);
  
  Serial.println("setup complete");
}

void loop() 
{
  if (count > 19) {
    count = 0;
  }

  // set which gyro to read from
  if (switcher == 1) { // ******************************** washer ************************************
    digitalWrite(mux0, HIGH);
    delay(100);
    yArrayW[count] = HMC5803L_Read(Y);
    zArrayW[count] = HMC5803L_Read(Z);
  
  if (count == 19) {
    
    
    for (int i = 0; i < 20; i++) {
      ymaxW = max(yArrayW[i], ymaxW);
      yminW = min(yArrayW[i], yminW);
      zmaxW = max(zArrayW[i], zmaxW);
      zminW = min(zArrayW[i], zminW);
      delay(10);
    }
    
      yLastW = yThisW;
      zLastW = zThisW;
      yThisW = ymaxW-yminW;
      zThisW = zmaxW-zminW;  
      Serial.print("yLastW: ");
      Serial.print(yLastW);
      Serial.print(", yThisW: ");
      Serial.print(yThisW);
      Serial.print(", zLastW: ");
      Serial.print(zLastW);
      Serial.print(", zThisW: ");
      Serial.println(zThisW);
  
      // set washer status
      if (washerStatus == 0) {
        if (yThisW > 150 && yLastW > 150 && zThisW > 75 && zLastW > 75) {
          Serial.println("washer running");
          washerStatus = 1;
          washerStateChange = 1;
        }
      } else {
        if (yThisW < 12 && yLastW < 12 && zThisW < 25 && zLastW < 25) {
          Serial.println("washer stopped");
          washerStatus = 0;
          washerStateChange = 1;
        }
      }

      if (washerStateChange == 1) {
        theData.deviceID = 1;
        theData.var1_usl = millis();
        theData.var2_float = washerStatus;
        theData.var3_float = 0;		//null value;
        radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
      }

      ymaxW = -10000;
      yminW = 10000;
      zmaxW = -10000;
      zminW = 10000;
      
      switcher = 0;
      Serial.println("switcher 0");
      washerStateChange = 0;  
      count = 0; 
  }
  
  
  } else { // *********************************************** dryer ***************************************
      digitalWrite(mux0, LOW);
      delay(100);
      yArrayD[count] = HMC5803L_Read(Y);
      zArrayD[count] = HMC5803L_Read(Z);
    
    if (count == 19) {
      
      
      for (int i = 0; i < 20; i++) {
        ymaxD = max(yArrayD[i], ymaxD);
        yminD = min(yArrayD[i], yminD);
        zmaxD = max(zArrayD[i], zmaxD);
        zminD = min(zArrayD[i], zminD);
        delay(10);
      }
      
        yLastD = yThisD;
        zLastD = zThisD;
        yThisD = ymaxD-yminD;
        zThisD = zmaxD-zminD;  
        Serial.print("yLastD: ");
        Serial.print(yLastD);
        Serial.print(", yThisD: ");
        Serial.print(yThisD);
        Serial.print(", zLastD: ");
        Serial.print(zLastD);
        Serial.print(", zThisD: ");
        Serial.println(zThisD);
    
        // set dryer status
        if (dryerStatus == 0) {
          if (yThisD > 200 && yLastD > 200 && zThisD > 225 && zLastD > 225) {
            Serial.println("dryer running");
            dryerStatus = 1;
            dryerStateChange = 1;
          }
        } else {
          if (yThisD < 150 && yLastD < 150 && zThisD < 175 && zLastD < 175) {
            Serial.println("dryer stopped");
            dryerStatus = 0;
            dryerStateChange = 1;
          }
        }
  
        if (dryerStateChange == 1) {
          theData.deviceID = 2;
          theData.var1_usl = millis();
          theData.var2_float = dryerStatus;
          theData.var3_float = 0;		//null value;
          radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));

          dryerStateChange = 0;
        }
  
        ymaxD = -10000;
        yminD = 10000;
        zmaxD = -10000;
        zminD = 10000;
        
        switcher = 1;
        Serial.println("switcher 1");
      }
    
    }
  
  count = count+1;  
  delay(100);


} // end loop


/* This function will initialise the module and only needs to be run once
   after the module is first powered up or reset */
void Init_HMC5803L(void)
{
  // init first gyro
  digitalWrite(mux0, LOW);
  delay(100);
  
  /* Set the module to 8x averaging and 15Hz measurement rate */
  Wire.beginTransmission(HMC5803L_Address);
  Wire.write(0x00);
  Wire.write(0x70);
          
  /* Set a gain of 5 */
  Wire.write(0x01);
  Wire.write(0xA0);
  Wire.endTransmission();
  
  // init second gyro
  digitalWrite(mux0, HIGH);
  delay(100);
  
  /* Set the module to 8x averaging and 15Hz measurement rate */
  Wire.beginTransmission(HMC5803L_Address);
  Wire.write(0x00);
  Wire.write(0x70);
          
  /* Set a gain of 5 */
  Wire.write(0x01);
  Wire.write(0xA0);
  Wire.endTransmission();
}


/* This function will read once from one of the 3 axis data registers
and return the 16 bit signed result. */
int HMC5803L_Read(byte Axis)
{
  int Result;
  
  /* Initiate a single measurement */
  Wire.beginTransmission(HMC5803L_Address);
  Wire.write(0x02);
  Wire.write(0x01);
  Wire.endTransmission();
  delay(6);
  
  /* Move modules the resiger pointer to one of the axis data registers */
  Wire.beginTransmission(HMC5803L_Address);
  Wire.write(Axis);
  Wire.endTransmission();
   
  /* Read the data from registers (there are two 8 bit registers for each axis) */  
  Wire.requestFrom(HMC5803L_Address, 2);
  Result = Wire.read() << 8;
  Result |= Wire.read();

  return Result;
}



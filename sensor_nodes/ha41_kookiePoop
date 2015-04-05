// **********************************************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this code/library but please abide with the CCSA license:
// http://creativecommons.org/licenses/by-sa/4.0/
// **********************************************************************************

//RFM69  --------------------------------------------------------------------------------------------------
#include <RFM69.h>
#include <SPI.h>
#define NODEID        41    //unique for each node on same network
#define NETWORKID     101  //the same on all nodes that talk to each other
#define GATEWAYID     1
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
//#define FREQUENCY   RF69_433MHZ
//#define FREQUENCY   RF69_868MHZ
#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "1111111111111111" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define ACK_TIME      30 // max # of ms to wait for an ack

//#define SERIAL_BAUD   115200  //must be 9600 for GPS, use whatever if no GPS
//deviceID's

typedef struct {		
  int                   nodeID; 
  int			deviceID;
  unsigned long         var1_usl;
  float                 var2_float;
  float			var3_float;
} Payload;
Payload theData;

char buff[20];
byte sendSize=0;
boolean requestACK = false;
RFM69 radio;

// ADC setup ----------------------------------------------------------------------------------------------------------------------
#define ADC_CLOCK 5
#define ADC_DATA  6

#define NUM_SAMPLES_LOG2 8
#define NUM_SAMPLES (1 << NUM_SAMPLES_LOG2)

#define ADC_BIT_PERIOD_MICROS 1

int32_t getADCReading()
{
  int32_t v = 0;

  while (digitalRead(ADC_DATA) == HIGH);
  
  noInterrupts();
  
  for (int i=0; i<24; i++)
  {
    digitalWrite(ADC_CLOCK, 1);
    delayMicroseconds(ADC_BIT_PERIOD_MICROS);
    
    v <<= 1;
    v |= (digitalRead(ADC_DATA) == HIGH) ? 1 : 0;
    
    digitalWrite(ADC_CLOCK, 0);
    delayMicroseconds(ADC_BIT_PERIOD_MICROS);    
  }
  
  digitalWrite(ADC_CLOCK, 1);
  delayMicroseconds(ADC_BIT_PERIOD_MICROS);
  digitalWrite(ADC_CLOCK, 0);
  
  interrupts();
  
  v |= (v & 0x00800000) ? 0xff000000 : 0x00000000; // sign extend
  
  return v;
}

int32_t getPreciseADCReading()
{
  int32_t v = 0;
  for (int i=0; i<NUM_SAMPLES; i++)
  {
    v += getADCReading();
  }
  return (v >> NUM_SAMPLES_LOG2);
}


// program variable declaration -----------------------------------------------------------------------------------------------
int32_t calibration = 0;
int32_t reading = 0;
float weight = 0;
float lastWeight = 0;
float weightDiff = 0;
float emptyBox = 0; // box without cat
float emptyBox_1 = 0;
float emptyBox_2 = 0;
float catBox = 0; // box with cat
float lastCatBox = 0;
float poopBox = 0; // poop left in box
float diffDetect = 1.5; // number to use to detect change in weight
float minCatWeight = 10; // resonable min weight so we can assume cat is in box
unsigned long startPoop = 0;
unsigned long endPoop = 0;
float poopTime = 0;
int counter1 = 0;
int calLoop = 0;
int getReading = 0;

int button = 7;
int led = 9;

int Step = 0;
/* step number explanation
step 0: no cat in box
step 1: cat entering box
step 2: cat settled in box
step 3: cat leaving box
step 4: cat out of box (only the poop remains)
*/

void setup() {
  Serial.begin(9600);
  Serial.println("catpoop v0.1");

  pinMode(ADC_CLOCK, OUTPUT);
  pinMode(ADC_DATA, INPUT);
  
  pinMode(button, INPUT);
  pinMode(led, OUTPUT);
  
  calibration = getPreciseADCReading();
  calLoop = 1;
  Serial.print("initial calibration: ");
  Serial.println(calibration);
  
  //RFM69-------------------------------------------
  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  #ifdef IS_RFM69HW
    radio.setHighPower(); //uncomment only for RFM69HW!
  #endif
  radio.encrypt(ENCRYPTKEY);
  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
  theData.nodeID = 10;  //this node id should be the same for all devices in this node
  
  Serial.println("setup complete");
}

void loop() 
{
  if (Step == 5) {
    calLoop = 1;
    poopBox = 0;
    catBox = 0;
    delay(3600000);
  }
  
  // check for button push to recalibrate
  if (digitalRead(button) == HIGH || calLoop == 1) {
    int x = 0;
    while (x < 3) { // blink light 3 times
      digitalWrite(led, HIGH);
      delay(500);
      digitalWrite(led, LOW);
      delay(500);
      x++;
    }
    delay(240000); // set this to box change time
    calibration = getPreciseADCReading();
    Step = 0;
    counter1 = 0;
    calLoop = 1;
    
    if (Step == 5) {
      Step = 0;
    }
  }
  
  
  // take reading
  int32_t v = getPreciseADCReading();
  reading = v - calibration;
  Serial.print("v: ");
  Serial.println(reading);
  
  // remember last reading
  lastWeight = weight;
  
  // translate reading to lbs
  weight = (reading + 1845)/10726.68;
  Serial.print(weight);
  Serial.println(" lbs");
  
  // calculate weight diff
  weightDiff  = weight - lastWeight;
  Serial.print("diff");
  Serial.println(weightDiff);
  
  if (calLoop == 0) {
    // determine step number
    if (Step == 0) { // cat has not entered litter box
      if (weight > minCatWeight) { // cat is in box
        startPoop = millis(); // record start poop time
        Step = 1;
        Serial.println("step 1");
      } else { 
        // emptyBox = weight; // set empty box weight
        emptyBox = emptyBox_1;
        emptyBox_1 = emptyBox_2;
        emptyBox_2 = weight;
      }
    }
      
    if (Step == 1) { 
      if (weight < 1) { //cat out of box
        endPoop = millis();
        poopTime = (endPoop - startPoop)/1000;
        counter1 = 0;
        Serial.println("step 2");
        delay(240000); // delay to give load cells time to settle
        Step = 2;
        getReading = 0;
        
      } else {
        if (weight > minCatWeight) { // check to make sure cat still in box
          if (counter1 < 5) { // take 3 weight readings to ensure sensors have adjusted to weight
            lastCatBox = catBox;
            catBox = max(weight - emptyBox, lastCatBox);
            counter1 = counter1 + 1;
          }
        }
      }
    }

    if (Step == 2) {
      if (getReading == 1) {
        poopBox = weight - emptyBox;
        Serial.print("poopy is ");
        Serial.print(poopBox);
        Serial.println(" lbs");
        
        Serial.print("cat was ");
        Serial.print(catBox);
        Serial.println(" lbs");
        
        Serial.print("poop time was ");
        Serial.print(poopTime);
        Serial.println(" s");
        
        //send data
        theData.nodeID = 41; 
        theData.deviceID = 1;
        theData.var1_usl = poopTime;
        theData.var2_float = poopBox;
        theData.var3_float = catBox;
        radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
          
        Step = 5;
        
      } else {
        getReading = 1;
      }
    }
  } else {
    calLoop = 0;   
  } 
} // end of loop





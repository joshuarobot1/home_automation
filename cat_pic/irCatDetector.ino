// **********************************************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this code/library but please abide with the CCSA license:
// http://creativecommons.org/licenses/by-sa/4.0/
// **********************************************************************************

int sensorPinR = A0;   // select the input pin for the potentiometer
int sensorPinL = A1;   // select the input pin for the potentiometer
int ledPin = 13;      // select the pin for the LED
int sensorValueR = 0;  // variable to store the value coming from the sensor
int sensorValueL = 0;  // variable to store the value coming from the sensor
int counter1 = 0;
int rangeArray[10] = {0};
int lightArray[10] = {0};
int totalR = 0;
int totalL = 0;

int outPinE = 5; // set this pin to trigger an eating picture
int outPinD = 6; // set this pin to trigger a drinking picture

int step1 = 0;
int step2 = 0;
int stepCount = 0;

void setup() {
  // declare the ledPin as an OUTPUT:
  pinMode(outPinE, OUTPUT);
  pinMode(outPinD, OUTPUT);  
  Serial.begin(115200);
}

void loop() {
  
  if (counter1 > 9) {
    counter1 = 0;
  }
  
  totalR = totalR - rangeArray[counter1];
  totalL = totalL - lightArray[counter1];
  
  // read the value from the sensor:
  rangeArray[counter1] = analogRead(sensorPinR);
  lightArray[counter1] = analogRead(sensorPinL);  
  
  totalR = totalR + rangeArray[counter1];
  totalL = totalL + lightArray[counter1];
  
  sensorValueR = totalR/10;
  sensorValueL = totalL/10;
  
  // turn the ledPin on
  Serial.print("range: ");
  Serial.println(sensorValueR);  
  Serial.print("light: ");
  Serial.println(sensorValueL); 
  Serial.println(counter1); 
  Serial.println(step1); 
  Serial.println(""); 
  
  //if there is enough light to take a picture
  if (sensorValueL > 300) {
    
    // if kookie is in eating area
    if (sensorValueR > 300 && step1 == 0) {
      counter1 = 10;
      step1 = 1;
    }

    // we have ensured an averaged reading
    if (step1 == 1 && counter1 == 9) {
      if ((sensorValueR > 300) && (sensorValueR < 500)) {
        digitalWrite(outPinE, HIGH);
        Serial.println("eating"); 
        delay(1000);
        digitalWrite(outPinE, LOW);
        step1 = 0;
        // if kookie is in drinking area
      } else if(sensorValueR >= 500) {      
        digitalWrite(outPinD, HIGH);
        Serial.println("drinking"); 
        delay(1000);
        digitalWrite(outPinD, LOW);
        step1 = 0;
      } else {
        step1 = 0;
      }
    }
  }
    
  // stop the program for for <sensorValue> milliseconds:
  delay(1000);

  counter1 = counter1 + 1;  
}

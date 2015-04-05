// **********************************************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this code/library but please abide with the CCSA license:
// http://creativecommons.org/licenses/by-sa/4.0/
// **********************************************************************************

// this code uses a DHT11 to monitor temperature and humidity

//Celsius to Fahrenheit conversion
double Fahrenheit(double celsius)
{
	return 1.8 * celsius + 32;
}

#include <dht11.h>

dht11 DHT11;

#define DHT11PIN 5


//RFM69  --------------------------------------------------------------------------------------------------
#include <RFM69.h>
#include <SPI.h>
#define NODEID        15    //unique for each node on same network
#define NETWORKID     101  //the same on all nodes that talk to each other
#define GATEWAYID     1
//Match frequency to the hardware version of the radio on your Moteino (uncomment one):
//#define FREQUENCY   RF69_433MHZ
//#define FREQUENCY   RF69_868MHZ
#define FREQUENCY     RF69_915MHZ
#define ENCRYPTKEY    "1111111111111111" //exactly the same 16 characters/bytes on all nodes!
#define IS_RFM69HW    //uncomment only for RFM69HW! Leave out if you have RFM69W!
#define ACK_TIME      30 // max # of ms to wait for an ack

#define SERIAL_BAUD   115200  //must be 9600 for GPS, use whatever if no GPS
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


void setup()
{
  Serial.begin(115200);
  Serial.println("DHT11 Squirtle Terrarium ");
  Serial.print("LIBRARY VERSION: ");
  Serial.println(DHT11LIB_VERSION);
  Serial.println();
  
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
  
  // initialize sensor
  Serial.println("\n");

  int chk = DHT11.read(DHT11PIN);

  Serial.print("Read sensor: ");
  switch (chk)
  {
    case DHTLIB_OK: 
		Serial.println("OK"); 
		break;
    case DHTLIB_ERROR_CHECKSUM: 
		Serial.println("Checksum error"); 
		break;
    case DHTLIB_ERROR_TIMEOUT: 
		Serial.println("Time out error"); 
		break;
    default: 
		Serial.println("Unknown error"); 
		break;
  }

  Serial.print("Humidity (%): ");
  float humidity = (DHT11.humidity);
  Serial.println(humidity);
  float temp = Fahrenheit(DHT11.temperature);
  Serial.print("Temperature (°F): ");
  Serial.println(temp);
  
  delay(5000);
  

  Serial.println("finished setup");
}

void loop()
{
  Serial.println("\n");

  int chk = DHT11.read(DHT11PIN);

  Serial.print("Read sensor: ");
  switch (chk)
  {
    case DHTLIB_OK: 
		Serial.println("OK"); 
		break;
    case DHTLIB_ERROR_CHECKSUM: 
		Serial.println("Checksum error"); 
		break;
    case DHTLIB_ERROR_TIMEOUT: 
		Serial.println("Time out error"); 
		break;
    default: 
		Serial.println("Unknown error"); 
		break;
  }

  Serial.print("Humidity (%): ");
  float humidity = (DHT11.humidity);
  Serial.println(humidity);
  float temp = Fahrenheit(DHT11.temperature);
  Serial.print("Temperature (°F): ");
  Serial.println(temp);
  
  //send data
  theData.nodeID = NODEID; 
  theData.deviceID = 1;
  theData.var1_usl = millis();
  theData.var2_float = humidity;
  theData.var3_float = (analogRead(A3))*3.30/1023.00*2.00;
  radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
  
  Serial.println((analogRead(A3))*3.30/1023.00*2.00);
  delay(10000);
  
  theData.nodeID = NODEID; 
  theData.deviceID = 2;
  theData.var1_usl = millis();
  theData.var2_float = temp;
  theData.var3_float = (analogRead(A3))*3.30/1023.00*2.00;
  radio.sendWithRetry(GATEWAYID, (const void*)(&theData), sizeof(theData));
  
  
  Serial.println("data sent to server");
  delay(3600000);
}
//
// END OF FILE
//

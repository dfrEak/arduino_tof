/*
This example takes range measurements with the VL53L1X and displays additional 
details (status and signal/ambient rates) for each measurement, which can help
you determine whether the sensor1 is operating normally and the reported range is
valid. The range is in units of mm, and the rates are in units of MCPS (mega 
counts per second).
*/



#include <LinkedList.h>
#include <Wire.h>
#include <VL53L1X.h>
#include <time.h>

#include "listTable.h"
#include "peak.h"

//////////////////////////////////////////////////////////////
//Multi sensor////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

//ref: https://forum.pololu.com/t/vl53l0x-maximum-sensor1s-on-i2c-arduino-bus/10845/7
//to change addresses so I can connect more than 1 sensor1, 6 sensor1s in example

//bug address can not be changed: https://github.com/sparkfun/SparkFun_VL53L1X_Arduino_Library/issues/7

//#define XSHUT_pin6 not required for address change - mega setting
//#define XSHUT_pin5 9
//#define XSHUT_pin4 8
//#define XSHUT_pin3 7
//#define XSHUT_pin2 6
#define XSHUT_pin1 5
#define XSHUT_pin2 4

//ADDRESS_DEFAULT 0b0101001 or 41
//#define sensor1_newAddress 41 not required address change - mega setting
//#define sensor1_newAddress (uint8_t)22
//#define sensor2_newAddress (uint8_t)25
//#define sensor3_newAddress 43
//#define sensor4_newAddress 44
//#define sensor5_newAddress 45
//#define sensor6_newAddress 46

VL53L1X sensor;
//VL53L1X sensor2;
//VL53L1X sensor3;
//VL53L1X sensor4;
//VL53L1X sensor5;
//VL53L1X sensor6;

//set pin the 1st sensor at beginning
int pin=XSHUT_pin1;

//read status
bool read=true;

//////////////////////////////////////////////////////////////
//Var/////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////


// led
#define LED_1 12 //GREEN
#define LED_2 11 //YELLOW
#define LED_3 10 //RED
#define LED_4 9  //GREEN2

// communication
char incomingByte = 0;

// SETTINGS
// check status
// true if the range data MUST be valid to be processed
bool VALID_CHECK=false;

int range_1=0;
int range_2=0;

/////Setting
//default 5
const int pdLag=20;
//window size
const int pdWindowSize=10;
//default 3.5
const float pdThreshold=20;
//default 0.5
const float pdInfluence=0.5; 

//using peak.h
peak peak1;
peak peak2;
// listTable
listTable table= listTable();

//////////////////////////////////////////////////////////////
//Setup///////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void setup()
{
  // setup pin
  pinMode(LED_1, OUTPUT); //LED_1 
  pinMode(LED_2, OUTPUT); //LED_2 
  pinMode(LED_3, OUTPUT); //LED_3 
  pinMode(LED_4, OUTPUT); //LED_4 

  // setup vl53l1x
  setupVl53l1x();
}

//////////////////////////////////////////////////////////////
//Loop////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void loop()
{
  // read incoming byte
  readIncomingByte();

/*
 //checking device
  int count=0;
  for (byte i = 1; i < 120; i++)
  {
    Wire.beginTransmission (i);
    if (Wire.endTransmission () == 0)
    {
      Serial.print ("Found address: ");
      Serial.print (i, DEC);
      Serial.print (" (0x");
      Serial.print (i, HEX);
      Serial.println (")");
      count++;
    } // end of good response
  } // end of for loop
  Serial.println ("Done.");
  Serial.print ("Found ");
  Serial.print (count, DEC);
  Serial.println (" device(s).");
*/
  
  if(read)
  {
    get_and_print_measurement();
    /*
    sensor1.read();
    Serial.print("range: ");
    Serial.print(sensor1.ranging_data.range_mm);
    
    sensor2.read();
    Serial.print("\trange2: ");
    Serial.println(sensor2.ranging_data.range_mm);
    */
  }
}

//////////////////////////////////////////////////////////////
//Function////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void setupVl53l1x()
{
//////////////////////////////////////////////////////////////
//Multi sensor////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
  /*WARNING*/
  //Shutdown pins of VL53L0X ACTIVE-LOW-ONLY NO TOLERANT TO 5V will fry them
  //pinMode(XSHUT_pin0, OUTPUT);
  //set xhut low
  pinMode(XSHUT_pin1, OUTPUT);
  pinMode(XSHUT_pin2, OUTPUT);
  digitalWrite(XSHUT_pin1, LOW);
  digitalWrite(XSHUT_pin2, LOW);
  delay(100);
  
  Wire.begin();
  Serial.begin(115200);
  Wire.setClock(400000); // use 400 kHz I2C

  //start with first sensor 
  pin=XSHUT_pin1;
  digitalWrite(pin, HIGH);

  //initialize the i2c bis and configure the sensor
  //sensor.init(true);
  Serial.print("init done\n");
  
  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1);
  }
  sensor.startContinuous(100);
  digitalWrite(pin, LOW);
  // Use long distance mode and allow up to 50000 us (50 ms) for a measurement.
  // You can change these settings to adjust the performance of the sensor1, but
  // the minimum timing budget is 20 ms for short distance mode and 33 ms for
  // medium and long distance modes. See the VL53L1X datasheet for more
  // information on range and timing limits.
  sensor.setDistanceMode(VL53L1X::Long);
  //sensor1.setDistanceMode(VL53L1X::Short);
  // default to long range, 50 ms timing budget
  sensor.setMeasurementTimingBudget(50000);
  Serial.print("setting done\n");
  
  //start setup pd
  setupPD();
}

//////////Peak detection (pd) -begin
void setupPD()
{
  Serial.println("SetupPD");
  peak1=peak(pdThreshold,pdInfluence);
  peak2=peak(pdThreshold,pdInfluence);
  // read ToF sensor
  readPin();

  //looping pdLag * 2 times
  int i=0;
  while(i<pdLag*2) 
  {
    //check current signal
    if(sensor.ranging_data.range_status==sensor.RangeValid or not VALID_CHECK)
    {
      if(pin==XSHUT_pin1)
        peak1.addSignalSetup(sensor.ranging_data.range_mm);
      else
        peak2.addSignalSetup(sensor.ranging_data.range_mm);
      //pdFilteredWindow1.add(sensor1.ranging_data.range_mm);
      //pdFilteredWindow2.add(sensor2.ranging_data.range_mm);
      i++;
    Serial.println(sensor.ranging_data.range_mm);
    }
  }

  //after loop then calculate avg std
  //set defaultDistance
  //defaultDistance=sensor1.ranging_data.range_mm;
  //setup for lag+window size is complete
  LEDCode(2);
  //calculate avg and std
  pdCalculateAvgStd(true);
  //setup list table
  table.setupTable(peak1.avgSetup,peak2.avgSetup);
  //copy pdFilteredWindow to pdSetupWindow
  //pdCloneSetupWindow();
  Serial.println("SetupPD : finish");
}


void get_and_print_measurement()
{
  time_t t_start = time(NULL);
  readPin();
  int stability1=0;
  int stability2=0;

  //check stability
  if(pin==XSHUT_pin1)
  {
    stability1=peak1.checkStability(sensor.ranging_data.range_mm);
    if (stability1==0)
    {
      LEDCode(3);
    }
    else
    {
      LEDCode(4);
      //add peak1 avg
      //table.addData(1,peak1.avgFilter);
      table.addData(1,sensor.ranging_data.range_mm);
    }
    //update avg
    peak1.calculateAvgStd(false);
  }
  else
  {
    stability2=peak2.checkStability(sensor.ranging_data.range_mm);
    if (stability2==0)
    {
      LEDCode(5);    
    }
    else
    {
      LEDCode(6);
      //add peak2 avg
      //table.addData(2,peak2.avgFilter);
      table.addData(2,sensor.ranging_data.range_mm);
    }
    //update avg
    peak1.calculateAvgStd(false);
  }
  
  //update table
  table.updateTable();
  //print();
  printPlotter(stability1,stability2);
  //printInfo();
  /*
  //Serial.println("sensor on pin: %d\tvalue: %d\ttime: %f" , (pin, distance_in_mm,(double)(clock() - start)/CLOCKS_PER_SEC) );
  Serial.print("sensor on pin: ");
  Serial.print(pin);
  Serial.print("\tvalue: ");
  Serial.print(distance_in_mm);
  Serial.print("\ttime: ");
  time_t t_end = time(NULL);
  Serial.print(t_end - t_start);
  Serial.println();


  Serial.print("sensor 5: ");
  Serial.print(range_1);
  Serial.print("\tsensor 4: ");
  Serial.print(range_2);
  Serial.println();
  */
}

void readPin()
{
    // toogle pin
    pin = toggle_pin(pin);
    // Start ranging, 1 = Short Range, 2 = Medium Range, 3 = Long Range
    digitalWrite(pin, HIGH);
    delay(100);
    sensor.startContinuous(100);
    sensor.read();
    if(pin==XSHUT_pin1)
      range_1=sensor.ranging_data.range_mm;
    else
      range_2=sensor.ranging_data.range_mm;
    //int distance_in_mm = sensor.ranging_data.range_mm;
    //sensor.stopContinuous();
    digitalWrite(pin, LOW);
}

int toggle_pin(int pin)
{
    int new_pin;
    if (pin == XSHUT_pin2)
        new_pin = XSHUT_pin1;
    else
        new_pin = XSHUT_pin2;
    return new_pin;
}

void pdCalculateAvgStd(bool first)
{
  //using peak.h
    peak1.calculateAvgStd(first);
    peak2.calculateAvgStd(first);
   
}

void printPlotter(int stability1,int stability2)
{
  Serial.print("range: ");
  Serial.print(range_1);
  
  Serial.print("\trange2: ");
  Serial.print(range_2);

  Serial.print("\tstability2: ");
  Serial.print(1000+500*stability1);
  
  Serial.print("\tstability2: ");
  Serial.print(1000+500*stability2);

  Serial.println();
}

void printInfo()
{
  Serial.print("range: ");
  Serial.print(range_1);
  
  Serial.print("\trange2: ");
  Serial.print(range_2);
  
  Serial.print("\tavg1: ");
  Serial.print(peak1.avgFilter);
  //Serial.print("\tstd1: ");
  //Serial.print(peak1.stdFilter);
  Serial.print("\tavg2: ");
  Serial.print(peak2.avgFilter);
  //Serial.print("\tstd2: ");
  //Serial.print(peak2.stdFilter);
  
  Serial.print("\tlist: ");
  Serial.print(table.objectList.size());
  Serial.print("/(");
  for(int i=0;i<table.objectList.size();i++)
  {
    Serial.print(table.objectList.get(i).sensor);
    Serial.print("_");
    Serial.print(table.objectList.get(i).object);
    //Serial.print(table.objectList.get(i).timed);
    Serial.print(",");
  }
  Serial.print(")");

  
  Serial.print("\tcount: ");
  Serial.print(table.movementList.size());

  Serial.print("/(");
  for(int i=0;i<table.movementList.size();i++)
  {
    Serial.print(table.movementList.get(i));
    Serial.print(",");
  }
  Serial.print(")");
  
  //Serial.print("range: ");
  //Serial.print(sensor.ranging_data.range_mm);

  /*
  Serial.print("range2: ");
  Serial.print(sensor2.ranging_data.range_mm);
  
  Serial.print("\tpdAvgFilter: ");
  Serial.print(pdAvgFilter);

  Serial.print("\ta: ");
  float a=pdAvgFilter+pdThreshold*pdStdFilter;
  Serial.print(a);

  Serial.print("\tb: ");
  float b=pdAvgFilter-pdThreshold*pdStdFilter;
  Serial.print(b);
  
  Serial.print("\tpdStat: ");
  int c=pdStat*500+1000;
  Serial.print(c);
  Serial.println();
  */
  Serial.println();
  
}

void LEDCode(int code)
{
  switch(code)
  {
    case 0: // turn off
        digitalWrite(LED_1,LOW);
        digitalWrite(LED_2,LOW);
        digitalWrite(LED_3,LOW);
        digitalWrite(LED_4,LOW);
        //Serial.println("LED Code: LED off");
      break;
    case 1: // turn on
        digitalWrite(LED_1,HIGH);
        digitalWrite(LED_2,HIGH);
        digitalWrite(LED_3,HIGH);
        digitalWrite(LED_4,HIGH);
        //Serial.println("LED Code: LED on");
      break;
    case 2: // setup finish
        for(int i=0;i<3;i++)
        {
          digitalWrite(LED_1,HIGH);
          digitalWrite(LED_2,HIGH);
          digitalWrite(LED_3,HIGH);
          digitalWrite(LED_4,HIGH);
          delay(100);
          digitalWrite(LED_1,LOW);
          digitalWrite(LED_2,LOW);
          digitalWrite(LED_3,LOW);
          digitalWrite(LED_4,LOW);
          delay(100);
        }
        //Serial.println("LED Code: SetupPD done");
      break;
    case 3: // sensor 1 stable
        digitalWrite(LED_2,LOW);
      break;
    case 4: // sensor 1 unstable
        digitalWrite(LED_2,HIGH);
      break;
    case 5: // sensor 2 stable
        digitalWrite(LED_3,LOW);
      break;
    case 6: // sensor 2 unstable
        digitalWrite(LED_3,HIGH);
      break;
    default : 
        //Serial.println("LED Code: code not found");
      break;
  }
}


void readIncomingByte()
{
  // if receive data
  if (Serial.available() > 0)
  {
    // read the incoming byte:
    incomingByte = Serial.read();
    
    // say what you got:
    Serial.print("I received: ");
    Serial.println(incomingByte);

    if(incomingByte=='p')
    {
      Serial.println("Paused");
      read=false;
    }
    else if(incomingByte=='r')
    {
      Serial.println("Resumed");
      read=true;
    }
  }
}

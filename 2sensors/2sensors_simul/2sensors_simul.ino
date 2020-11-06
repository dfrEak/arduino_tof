/*
This example takes range measurements with the VL53L1X and displays additional 
details (status and signal/ambient rates) for each measurement, which can help
you determine whether the sensor1 is operating normally and the reported range is
valid. The range is in units of mm, and the rates are in units of MCPS (mega 
counts per second).
*/



#include <Wire.h>
#include <VL53L1X.h>

#include <LinkedList.h>
#include <math.h>

#include "peak.h"
#include "listTable.h"

//////////////////////////////////////////////////////////////
//Multi sensor////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

//ref: https://forum.pololu.com/t/vl53l0x-maximum-sensor1s-on-i2c-arduino-bus/10845/7
//to change addresses so I can connect more than 1 sensor, 6 sensors in example

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
#define sensor1_newAddress (uint8_t)22
#define sensor2_newAddress (uint8_t)25
//#define sensor3_newAddress 43
//#define sensor4_newAddress 44
//#define sensor5_newAddress 45
//#define sensor6_newAddress 46

VL53L1X sensor1;
VL53L1X sensor2;
//VL53L1X sensor3;
//VL53L1X sensor4;
//VL53L1X sensor5;
//VL53L1X sensor6;

int sensor1_read;
int sensor2_read;

bool simul=true;

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

// sensor read status
bool read=true;

// SETTINGS
// check status
// true if the range data MUST be valid to be processed
bool VALID_CHECK=false;

//////////Peak detection (pd) -begin

/////Setting
//default 5
const int pdLag=10;
//window size
const int pdWindowSize=10;
//default 3.5
const float pdThreshold=8;
//default 0.5
const float pdInfluence=0.7; 

/////Param
/*
//first (lag) window
//LinkedList<int> pdSetupWindow1 = LinkedList<int>();
int pdSumSetup1=-1;
float pdAvgSetup1=-1;
float pdStdSetup1=-1;

//moving filter
LinkedList<int> pdFilteredWindow1 = LinkedList<int>();
float pdAvgFilter1=-1;
float pdStdFilter1=-1;


//second (lag) window
//LinkedList<int> pdSetupWindow2 = LinkedList<int>();
int pdSumSetup2=-1;
float pdAvgSetup2=-1;
float pdStdSetup2=-1;

//moving filter
LinkedList<int> pdFilteredWindow2 = LinkedList<int>();
float pdAvgFilter2=-1;
float pdStdFilter2=-1;
*/

//using peak.h
peak peak1;
peak peak2;
int divider=25;
// listTable
listTable table= listTable(divider);

int pdStat=0;
int pdSetupStat=0;

//////////Peak detection (pd) -end


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

/**
 * //checking device
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

  
  delay(3000);
**/
  
  // check sensor1 read status
  if(read)
  {
    //readsensor();
    //delay(200);

    /*
    sensor1.read();
    Serial.print("range: ");
    Serial.print(sensor1.ranging_data.range_mm);
    
    sensor2.read();
    Serial.print("\trange2: ");
    Serial.println(sensor2.ranging_data.range_mm);
    */

    readsensor();
    //print();
    
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
//  pinMode(XSHUT_pin0, OUTPUT);
  //set xhut low
  pinMode(XSHUT_pin1, OUTPUT);
  pinMode(XSHUT_pin2, OUTPUT);
  digitalWrite(XSHUT_pin1, LOW);
  digitalWrite(XSHUT_pin2, LOW);
  //pinMode(XSHUT_pin3, OUTPUT);
  //pinMode(XSHUT_pin4, OUTPUT);
  //pinMode(XSHUT_pin5, OUTPUT);

  
  delay(500);
  Wire.begin();
  Serial.begin(115200);
  Wire.setClock(400000); // use 400 kHz I2C
  
  //Change address of sensor and power up next one
  //sensor6.setAddress(sensor6_newAddress);
  //pinMode(XSHUT_pin5, INPUT);
  //delay(10); //For power-up procedure t-boot max 1.2ms "Datasheet: 2.9 Power sequence"
  //sensor5.setAddress(sensor5_newAddress);
  //pinMode(XSHUT_pin4, INPUT);
  //delay(10);
  //sensor4.setAddress(sensor4_newAddress);
  //pinMode(XSHUT_pin3, INPUT);
  //delay(10);
  //sensor3.setAddress(sensor3_newAddress);
  //pinMode(XSHUT_pin2, INPUT);
  //delay(10);

  //set 1st xshut high then turn on the 1st sensor and set the address
  digitalWrite(XSHUT_pin1, HIGH);
  delay(150);
  Serial.println("00");
  sensor1.init(true);

  Serial.println("01");
  delay(100);
  sensor1.setAddress(sensor1_newAddress);
  Serial.println("02");
  
  //set 2nd xshut high then turn on the 2nd sensor and set the address
  digitalWrite(XSHUT_pin2, HIGH);
  delay(150);
  sensor2.init(true);
  Serial.println("03");
  
  delay(100);
  sensor2.setAddress(sensor2_newAddress);
  Serial.println("04");

  Serial.print("init: ");
//////////////////////////////////////////////////////////////
//move to each sensor part
  //sensor2.init();
  Serial.print("2: ");
  //sensor1.init();
  //sensor3.init();
  //sensor4.init();
  //sensor5.init();
  //sensor6.init();
  
  Serial.print("init done: ");
  
  sensor1.setTimeout(500);
  sensor2.setTimeout(500);
  //sensor3.setTimeout(500);
  //sensor4.setTimeout(500);
  //sensor5.setTimeout(500);
  //sensor6.setTimeout(500);

  // Start continuous back-to-back mode (take readings as
  // fast as possible).  To use continuous timed mode
  // instead, provide a desired inter-measurement period in
  // ms (e.g. sensor.startContinuous(100)).
  if(simul)
    sensor1.startContinuous(100);
  else
    sensor1.startContinuous(80);
  sensor2.startContinuous(100);
  //sensor3.startContinuous();
  //sensor4.startContinuous();
  //sensor5.startContinuous();
  //sensor6.startContinuous();
  
  // Use long distance mode and allow up to 50000 us (50 ms) for a measurement.
  // You can change these settings to adjust the performance of the sensor1, but
  // the minimum timing budget is 20 ms for short distance mode and 33 ms for
  // medium and long distance modes. See the VL53L1X datasheet for more
  // information on range and timing limits.
  //sensor1.setDistanceMode(VL53L1X::Long);
  //sensor1.setDistanceMode(VL53L1X::Short);
  // default to long range, 50 ms timing budget
  //sensor1.setMeasurementTimingBudget(50000);
  //sensor1.setMeasurementTimingBudget(100000);
  //sensor1.setMeasurementTimingBudget(300000);
  // 20ms to 1000ms
  //sensor1.setMeasurementTimingBudget(200000);
  
  // 4m max
  //sensor1.setMeasurementTimingBudget(140000);
  
  // max 140ms
  //sensor1.setMeasurementTimingBudget(1000000);

  // Start continuous readings at a rate of one measurement every 50 ms (the
  // inter-measurement period). This period should be at least as long as the
  // timing budget.
  //sensor1.startContinuous(50);
  //sensor1.startContinuous(100);
  //sensor1.startContinuous(140);

  //start setup pd
  setupPD();
  
}

void readsensor()
{
  // read ToF sensor1
  sensor1.read();
  // read ToF sensor2
  sensor2.read();

  
  // first use
  //if(pdAvgSetup1==-1) // at the begining
  //{
    //if(sensor1.ranging_data.range_status==sensor1.RangeValid)
    //{
      //defaultDistance=sensor1.ranging_data.range_mm;
      //LEDCode(1);
    //}
    //setupPD();
  //}
  //else // normal run
  //{
  //checkStability();
  //}

  //check stability
  int stability1=peak1.checkStability(sensor1.ranging_data.range_mm);
  int stability2=peak2.checkStability(sensor2.ranging_data.range_mm);

  if (stability1==0)
  {
    LEDCode(3);
  }
  else
  {
    LEDCode(4);
    //add peak1 avg
    //table.addData(1,peak1.avgFilter);
    table.addData(1,sensor1.ranging_data.range_mm);
  }
    
  if (stability2==0)
  {
    LEDCode(5);    
  }
  else
  {
    LEDCode(6);
    //add peak2 avg
    //table.addData(2,peak2.avgFilter);
    table.addData(2,sensor2.ranging_data.range_mm);
  }
  
  //update avg
  pdCalculateAvgStd(false);
  //update table
  table.updateTable();
  //print();
  //printPlotter(stability1,stability2);
  printInfo();
  //printData();
}

//////////Peak detection (pd) -begin
void setupPD()
{
  Serial.println("SetupPD");
  peak1=peak(pdThreshold,pdInfluence);
  peak2=peak(pdThreshold,pdInfluence);
  // read ToF sensor1
  sensor1.read();
  // read ToF sensor2
  sensor2.read();

  //looping pdLag times
  int i=0;
  while(i<pdLag)
  {
    //check current signal
    if(sensor1.ranging_data.range_status==sensor1.RangeValid or not VALID_CHECK)
    {
        peak1.addSignalSetup(sensor1.ranging_data.range_mm);
        peak2.addSignalSetup(sensor2.ranging_data.range_mm);
      //pdFilteredWindow1.add(sensor1.ranging_data.range_mm);
      //pdFilteredWindow2.add(sensor2.ranging_data.range_mm);
      i++;
  Serial.println(sensor1.ranging_data.range_mm);
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


void pdCalculateAvgStd(bool first)
{
  //using peak.h
    peak1.calculateAvgStd(first);
    peak2.calculateAvgStd(first);
   
}


//////////Peak detection (pd) -end



void print()
{
  Serial.print("range1: ");
  Serial.print(sensor1.ranging_data.range_mm);
  Serial.print("\trange2: ");
  Serial.print(sensor2.ranging_data.range_mm);
  
  Serial.println();
  Serial.print("sensor1: ");
  peak1.printInfo();
  Serial.println();
  Serial.print("sensor2: ");
  peak2.printInfo();
  Serial.println();

  
  /*
  Serial.print("\tstatus: ");
  Serial.print(VL53L1X::rangeStatusToString(sensor1.ranging_data.range_status));
  Serial.print("\tpeak signal: ");
  Serial.print(sensor1.ranging_data.peak_signal_count_rate_MCPS);
  Serial.print("\tambient: ");
  Serial.print(sensor1.ranging_data.ambient_count_rate_MCPS);
*/
  //Serial.print("\tdelta: ");
  //Serial.print(deltaDistance);
  //Serial.print("\tavg: ");
  //Serial.print(pdAvgFilter);
  //Serial.print("\tstd: ");
  //Serial.print(pdStdFilter);
  //Serial.print("\tfiltered y: ");

  /*
  for(int i = 0; i < pdFilteredWindow.size(); i++){
    Serial.print(pdFilteredWindow.get(i));
    Serial.print(", ");
  }
  */

  /*
  Serial.print("\tstability: ");
  Serial.print(stability);
  Serial.print("\tduration: ");
  Serial.print(stabilityDuration);
  Serial.print(" , ");
  Serial.print(stabilityStat);
  Serial.print(" , ");
  Serial.print(defaultDistance);
  */
  
  Serial.println();
}

void printPlotter(int stability1,int stability2)
{
  Serial.print("range: ");
  Serial.print(sensor1.ranging_data.range_mm);
  
  Serial.print("\trange2: ");
  Serial.print(sensor2.ranging_data.range_mm);

  Serial.print("\tstability2: ");
  Serial.print(1000+500*stability1);
  
  Serial.print("\tstability2: ");
  Serial.print(1000+500*stability2);

  Serial.println();
}

void printData()
{
  Serial.print(sensor1.ranging_data.range_mm);
  Serial.print("\t");
  Serial.print(sensor2.ranging_data.range_mm);
  Serial.println();
  
}


void printInfo()
{
  Serial.print("range: ");
  Serial.print(sensor1.ranging_data.range_mm);
  
  Serial.print("\trange2: ");
  Serial.print(sensor2.ranging_data.range_mm);
  
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
    Serial.print(table.objectList.get(i).object*divider);
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

  /*
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
  */
  Serial.println();
  
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

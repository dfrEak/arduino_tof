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

// sensor1 read status
bool read=true;

// default distance (no object)
int defaultDistance = 0;

// sensor1 threshold 50 mm
const int sensor1Threshold = 50;

// SETTINGS
// check status
// true if the range data MUST be valid to be processed
bool VALID_CHECK=false;

int deltaDistance = 0;

/* prev version

int prevDeltaDistance;

int stability = 0;

const int stabilityThreshold = 10;

int stabilityDuration = 0;

const int stabilityDurationThreshold = 10;

const int stabilityDefaultThreshold = 50;

bool stabilityStat = false;
*/

//////////Peak detection (pd) -begin

/////Setting
//default 5
const int pdLag=20;
//window size
const int pdWindowSize=10;
//default 3.5
const float pdThreshold=5;
//default 0.5
const float pdInfluence=0.1; 

/////Param
//first (lag) window
LinkedList<int> pdSetupWindow = LinkedList<int>();
int pdSumSetup=0;
float pdAvgSetup = 0;
float pdStdSetup = 0;

//moving filter
LinkedList<int> pdFilteredWindow = LinkedList<int>();
float pdAvgFilter=0;
float pdStdFilter=0;
//float pdAvgFilter2=0;
//float pdStdFilter2=0;
//LinkedList<float> pdAvgFilterArray = LinkedList<float>();
//LinkedList<float> pdStdFilterArray = LinkedList<float>();

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
    //readsensor1();
    //delay(200);

    
    sensor1.read();
    Serial.print("range: ");
    Serial.print(sensor1.ranging_data.range_mm);
    
    sensor2.read();
    Serial.print("\trange2: ");
    Serial.println(sensor2.ranging_data.range_mm);
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
  sensor1.startContinuous(100);
  sensor2.startContinuous(100);
  //sensor3.startContinuous();
  //sensor4.startContinuous();
  //sensor5.startContinuous();
  //sensor6.startContinuous();
  
//////////////////////////////////////////////////////////////
//sensor1/////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

  //sensor1.setTimeout(500);
  //if (!sensor1.init())
  //{
    //Serial.println("Failed to detect and initialize sensor1!");
    //while (1);
  //}
  //sensor2.setTimeout(500);
  //if (!sensor2.init())
  //{
    //Serial.println("Failed to detect and initialize sensor2!");
    //while (1);
  //}
  
  // Use long distance mode and allow up to 50000 us (50 ms) for a measurement.
  // You can change these settings to adjust the performance of the sensor1, but
  // the minimum timing budget is 20 ms for short distance mode and 33 ms for
  // medium and long distance modes. See the VL53L1X datasheet for more
  // information on range and timing limits.
  sensor1.setDistanceMode(VL53L1X::Long);
  //sensor1.setDistanceMode(VL53L1X::Short);
  // default to long range, 50 ms timing budget
  sensor1.setMeasurementTimingBudget(50000);
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
  sensor1.startContinuous(50);
  //sensor1.startContinuous(100);
  //sensor1.startContinuous(140);

  defaultDistance = 0; //first set 0
  
}

void readsensor1()
{
  // read ToF sensor1
  sensor1.read();
  
  // first use
  if(defaultDistance==0)
  {
    //if(sensor1.ranging_data.range_status==sensor1.RangeValid)
    //{
      //defaultDistance=sensor1.ranging_data.range_mm;
      //LEDCode(1);
    //}
    setupPD();
  }
  else
  {
    checkStability();
  }
  
  //print();
  printPlotter();

  // calculate deltaDistance
  deltaDistance=defaultDistance-sensor1.ranging_data.range_mm;


  // check if pass threshold
  //if(deltaDistance>sensor1Threshold)
  //{
    //there is an object  
  //}
  
  /*
  // turn the led on if error
  if(sensor1.ranging_data.range_status==sensor1.RangeValid
    //or sensor1.ranging_data.range_status==sensor1.SignalFail
    )
  {
    digitalWrite(LED,LOW);
  }
  else
  {
    digitalWrite(LED,HIGH);
  }
  */

  //the last time update everything
  //prevDeltaDistance=deltaDistance;
}

//////////Peak detection (pd) -begin
void setupPD()
{
  //Serial.println("SetupPD");
  bool stat=false;
  //checking lag
  if(pdFilteredWindow.size()==pdLag+pdWindowSize-1)
  {
    stat=true;
  }

  //check current signal
  if(sensor1.ranging_data.range_status==sensor1.RangeValid or not VALID_CHECK)
  {
    pdFilteredWindow.add(sensor1.ranging_data.range_mm);
    
    //set defaultDistance
    if(stat)
    {
      defaultDistance=sensor1.ranging_data.range_mm;
      //setup for lag+window size is complete
      LEDCode(2);
      //calculate avg and std
      pdCalculateAvgStd();
      //copy pdFilteredWindow to pdSetupWindow
      pdCloneSetupWindow();
    }
  }
}

void pdCloneSetupWindow()
{
  pdSetupWindow.clear();
  pdSumSetup=0;
  for(int i=0;i<pdLag;i++)
  {
    //add
    pdSetupWindow.add(pdFilteredWindow.get(0));
    pdSumSetup+=pdFilteredWindow.get(0);
    //pop filtered window
    pdFilteredWindow.shift();
  }
  //Serial.println((String)"Setup　　"+pdSetupWindow.size()+" "+pdFilteredWindow.size());
  //calculate average
  pdAvgSetup=pdSumSetup/pdLag;
  //calculate standard deviation
  pdStdSetup=0;
  for(int i=0;i<pdLag;i++)
  {
    pdStdSetup+=pow((pdSetupWindow.get(i)-pdAvgSetup) , 2);
  }
  pdStdSetup=sqrt( pdStdSetup / pdLag );
  
  //Serial.print("sum :");
  //Serial.println(pdSumSetup);
  //Serial.print("avg :");
  //Serial.println(pdAvgSetup);
  //Serial.print("std :");
  //Serial.println(pdStdSetup);
}

void pdCalculateAvgStd()
{
  //avg
  pdAvgFilter=0;
  //pdAvgFilter2=pdSumSetup;
  for(int i=0;i<pdFilteredWindow.size();i++)
  {
    pdAvgFilter+=pdFilteredWindow.get(i);
    //pdAvgFilter2+=pdFilteredWindow.get(i);
  }
  pdAvgFilter=pdAvgFilter/pdFilteredWindow.size();
  //pdAvgFilter2=pdAvgFilter2/(pdFilteredWindow.size()+pdSetupWindow.size());


  //std
  //pdStdFilter=sqrt( pow(pdFilteredWindow.get(pdFilteredWindow.size()-1) , 2)/pdFilteredWindow.size() );
  pdStdFilter=0;
  //pdStdFilter2=0;
  //for(int i=0;i<pdSetupWindow.size();i++)
  //{
    //pdStdFilter2+=pow((pdSetupWindow.get(i)-pdAvgFilter2) , 2);
  //}
  for(int i=0;i<pdFilteredWindow.size();i++)
  {
    pdStdFilter+=pow((pdFilteredWindow.get(i)-pdAvgFilter) , 2);
    //pdStdFilter2+=pow((pdFilteredWindow.get(i)-pdAvgFilter2) , 2);
  }
  //Serial.println((String)"std total "+pdStdFilter);
  pdStdFilter=sqrt( pdStdFilter / pdFilteredWindow.size() );
  //pdStdFilter2=sqrt( pdStdFilter2 / (pdFilteredWindow.size()+pdSetupWindow.size()) );
}

void pdAddSignal(int y)
{
    //remove first
    pdFilteredWindow.shift();
    //add last
    pdFilteredWindow.add(y);
}

void checkStability()
{
  /* previous version
  // check if previous and current delta distance pass threshold
  if(abs(prevDeltaDistance-deltaDistance)<stabilityThreshold)
  {
    stabilityDuration++;
    if (stabilityDuration==stabilityDurationThreshold)
      stabilityStat = true;
    if (stabilityDuration==stabilityDefaultThreshold)
    {
      defaultDistance=sensor1.ranging_data.range_mm;
      LEDCode(1);
    }
  }
  else
  {
    stabilityDuration=0;
    stability = 0;
    stabilityStat = false;
  }
  */

  //https://stackoverflow.com/questions/22583391/peak-signal-detection-in-realtime-timeseries-data
  //with some modifications
  
  if(abs(sensor1.ranging_data.range_mm - pdAvgFilter) > pdThreshold*pdStdFilter) //stream data change too much
  {
    if(sensor1.ranging_data.range_mm > pdAvgFilter)
    {
      pdStat=1;
      //Serial.println("Signal +1");
      LEDCode(0);
      digitalWrite(LED_4,HIGH);
      //LEDCode(1);
    }
    else
    {
      pdStat=-1;
      //Serial.println("Signal -1");
      LEDCode(0);
      digitalWrite(LED_2,HIGH);
      //LEDCode(1);
    }
    
    float pdFiltered=pdInfluence*sensor1.ranging_data.range_mm + (1-pdInfluence)*pdFilteredWindow.get(pdFilteredWindow.size()-1);
    pdAddSignal((int)pdFiltered);
  }
  else //stream data not change that much
  {
    pdStat=0;
    //Serial.println("Signal 0");
    LEDCode(0);
    digitalWrite(LED_3,HIGH);
    pdAddSignal(sensor1.ranging_data.range_mm);

    //check
    //if(abs(sensor1.ranging_data.range_mm-defaultDistance)>defaultDistance+100)
      //digitalWrite(LED_1,HIGH);
      
  }

  //calculate new avg and std
  pdCalculateAvgStd();
}


//////////Peak detection (pd) -end



void print()
{
  Serial.print("range: ");
  Serial.print(sensor1.ranging_data.range_mm);
  /*
  Serial.print("\tstatus: ");
  Serial.print(VL53L1X::rangeStatusToString(sensor1.ranging_data.range_status));
  Serial.print("\tpeak signal: ");
  Serial.print(sensor1.ranging_data.peak_signal_count_rate_MCPS);
  Serial.print("\tambient: ");
  Serial.print(sensor1.ranging_data.ambient_count_rate_MCPS);
*/
  Serial.print("\tdelta: ");
  Serial.print(deltaDistance);
  Serial.print("\tavg: ");
  Serial.print(pdAvgFilter);
  Serial.print("\tstd: ");
  Serial.print(pdStdFilter);
  Serial.print("\tfiltered y: ");
  
  for(int i = 0; i < pdFilteredWindow.size(); i++){
    Serial.print(pdFilteredWindow.get(i));
    Serial.print(", ");
  }
  

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

void printPlotter()
{
  Serial.print("range: ");
  Serial.print(sensor1.ranging_data.range_mm);
  
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
    default : 
        //Serial.println("LED Code: code not found");
      break;
  }
}

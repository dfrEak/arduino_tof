/*
This example takes range measurements with the VL53L1X and displays additional 
details (status and signal/ambient rates) for each measurement, which can help
you determine whether the sensor is operating normally and the reported range is
valid. The range is in units of mm, and the rates are in units of MCPS (mega 
counts per second).
*/

#include <Wire.h>
#include <VL53L1X.h>

#include <LinkedList.h>
#include <math.h>


VL53L1X sensor;




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

// default distance (no object)
int defaultDistance = 0;

// sensor threshold 50 mm
const int sensorThreshold = 50;

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
const float pdThreshold=10;
//default 0.5
const float pdInfluence=1.0; 

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

  // check sensor read status
  if(read)
  {
    readsensor();
    //delay(200);

    
    
  }
}

//////////////////////////////////////////////////////////////
//Function////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void setupVl53l1x()
{
  
  Serial.begin(115200);
  Wire.begin();
  Wire.setClock(400000); // use 400 kHz I2C
//////////////////////////////////////////////////////////////
//sensor/////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

  sensor.setTimeout(500);
  if (!sensor.init())
  {
    Serial.println("Failed to detect and initialize sensor!");
    while (1);
  }
  //if (!sensor2.init())
  //{
    //Serial.println("Failed to detect and initialize sensor2!");
    //while (1);
  //}
  
  // Use long distance mode and allow up to 50000 us (50 ms) for a measurement.
  // You can change these settings to adjust the performance of the sensor, but
  // the minimum timing budget is 20 ms for short distance mode and 33 ms for
  // medium and long distance modes. See the VL53L1X datasheet for more
  // information on range and timing limits.
  sensor.setDistanceMode(VL53L1X::Long);
  //sensor.setDistanceMode(VL53L1X::Short);
  // default to long range, 50 ms timing budget
  sensor.setMeasurementTimingBudget(50000);
  //sensor.setMeasurementTimingBudget(100000);
  //sensor.setMeasurementTimingBudget(300000);
  // 20ms to 1000ms
  //sensor.setMeasurementTimingBudget(200000);
  
  // 4m max
  //sensor.setMeasurementTimingBudget(140000);
  
  // max 140ms
  //sensor.setMeasurementTimingBudget(1000000);

  // Start continuous readings at a rate of one measurement every 50 ms (the
  // inter-measurement period). This period should be at least as long as the
  // timing budget.
  sensor.startContinuous(50);
  //sensor.startContinuous(100);
  //sensor.startContinuous(140);

  defaultDistance = 0; //first set 0
  
}

void readsensor()
{
  // read ToF sensor
  sensor.read();
  
  // first use
  if(defaultDistance==0)
  {
    //if(sensor.ranging_data.range_status==sensor.RangeValid)
    //{
      //defaultDistance=sensor.ranging_data.range_mm;
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
  deltaDistance=defaultDistance-sensor.ranging_data.range_mm;


  // check if pass threshold
  //if(deltaDistance>sensorThreshold)
  //{
    //there is an object  
  //}
  
  /*
  // turn the led on if error
  if(sensor.ranging_data.range_status==sensor.RangeValid
    //or sensor.ranging_data.range_status==sensor.SignalFail
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
  if(sensor.ranging_data.range_status==sensor.RangeValid or not VALID_CHECK)
  {
    pdFilteredWindow.add(sensor.ranging_data.range_mm);
    
    //set defaultDistance
    if(stat)
    {
      defaultDistance=sensor.ranging_data.range_mm;
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
      defaultDistance=sensor.ranging_data.range_mm;
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
  
  if(abs(sensor.ranging_data.range_mm - pdAvgFilter) > pdThreshold*pdStdFilter) //stream data change too much
  {
    if(sensor.ranging_data.range_mm > pdAvgFilter)
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
    
    float pdFiltered=pdInfluence*sensor.ranging_data.range_mm + (1-pdInfluence)*pdFilteredWindow.get(pdFilteredWindow.size()-1);
    pdAddSignal((int)pdFiltered);
  }
  else //stream data not change that much
  {
    pdStat=0;
    //Serial.println("Signal 0");
    LEDCode(0);
    digitalWrite(LED_3,HIGH);
    pdAddSignal(sensor.ranging_data.range_mm);

    //check
    //if(abs(sensor.ranging_data.range_mm-defaultDistance)>defaultDistance+100)
      //digitalWrite(LED_1,HIGH);
      
  }

  //calculate new avg and std
  pdCalculateAvgStd();
}


//////////Peak detection (pd) -end



void print()
{
  Serial.print("range: ");
  Serial.print(sensor.ranging_data.range_mm);
  /*
  Serial.print("\tstatus: ");
  Serial.print(VL53L1X::rangeStatusToString(sensor.ranging_data.range_status));
  Serial.print("\tpeak signal: ");
  Serial.print(sensor.ranging_data.peak_signal_count_rate_MCPS);
  Serial.print("\tambient: ");
  Serial.print(sensor.ranging_data.ambient_count_rate_MCPS);
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
  Serial.print(sensor.ranging_data.range_mm);
  
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

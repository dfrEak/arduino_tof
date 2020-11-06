//https://www.arduino.cc/en/Hacking/LibraryTutorial
#include <LinkedList.h>
#include "peak.h"
#include <math.h>
#include "Arduino.h"


peak::peak()
{
}

peak::peak(float threshold,float influence)
{
  this->threshold=threshold;
  this->influence=influence;
}

void peak::addSignalSetup(int newdata)
{
    filteredWindow.add(newdata);
}

void peak::addSignal(int newdata)
{
    //remove first
    filteredWindow.shift();
    //add last
    filteredWindow.add(newdata);
}

void peak::calculateAvgStd(bool first=false)
{
  //avg
  avgFilter=0;
  for(int i=0;i<filteredWindow.size();i++)
  {
    avgFilter+=filteredWindow.get(i);
  }
  if(first)  sumSetup=avgFilter;
  avgFilter=avgFilter/filteredWindow.size();
  if(first)  avgSetup=avgFilter;
  
  //std
  stdFilter=0;
  for(int i=0;i<filteredWindow.size();i++)
  {
    stdFilter+=pow((filteredWindow.get(i)-avgFilter) , 2);
  }
  //Serial.println((String)"std total "+pdStdFilter);
  stdFilter=sqrt( stdFilter / filteredWindow.size() );
  if(first)  stdSetup=stdFilter;
}

void peak::printInfo()
{
  Serial.print("avg :");
  Serial.print(avgFilter);
  Serial.print("\tstd :");
  Serial.print(stdFilter);
}

int peak::calculateStatus(int range)
{
  //https://stackoverflow.com/questions/22583391/peak-signal-detection-in-realtime-timeseries-data
  //with some modifications
  int retval;
  if(abs(range - avgFilter) > threshold*stdFilter) //stream data change too much
  {
    if(range > avgFilter)
    {
      retval=1;
      //Serial.println("Signal +1");
    } 
    else
    {
      retval=-1;
      //Serial.println("Signal -1");
    }
    
    float pdFiltered=influence*range + (1-influence)*filteredWindow.get(filteredWindow.size()-1);
    addSignal((int)pdFiltered);
  }
  else //stream data not change that much
  {
    retval=0;
    addSignal((int)range);
    //Serial.println("Signal 0");
  }
  return retval;
}

int peak::checkStability(int range)
{
  int newStat=calculateStatus(range);
  return newStat;
  if(stat != 0 && newStat == 0)
  {
    stat=newStat;
    return 1;
  }
  else
  {
    stat=newStat;
    return 0;
  }
}

//https://www.arduino.cc/en/Hacking/LibraryTutorial
#include <LinkedList.h>
#include "listTable.h"
#include "Arduino.h"
#include <math.h>

listTable::listTable()
{
  
}

void listTable::updateTable()
{
  for(int i=0;i<objectList.size();i++)
  {
    obj data=objectList.get(i);
    data.timed++;
    objectList.set(i,data);
  }

  //check if the data already expired or not
  while(true)
  {
    if(objectList.get(0).timed>timeThreshold)
    {
      objectList.remove(0);
    }
    else
      break; //stop the looping
  }
  
}

void listTable::setupTable(float avg1,float avg2)
{
  diff2=(int) avg2-avg1;
}

void listTable::addData(int sensor, int newData)
{
  //normalize
  //if(sensor==2)
  //{
  //  newData-=diff2;
  //}
  //Serial.print(sensor);
  //Serial.print("->");
  //Serial.println(newData);
  
  int objectDistance=(int) floor(newData/divider);
  int found=-1;

  //desc
  for(int i=objectList.size()-1;i>=0;i--)
  {
    if(objectList.get(i).sensor == 3 - sensor) //check for the other sensor // HARDCODED
    {
      // -1,0,1
      if(abs(objectDistance-objectList.get(i).object)<2)
      {
        found=i;
        break;
      }
    }
  }

  //if found
  if(found!=-1)
  {
    //remove from object
    objectList.remove(found);
    //put on movement
    movementList.add(sensor);
  }
  else
  {
    obj data;
    data.object=objectDistance;
    data.sensor=sensor;
    objectList.add(data);
  }
  
  //objectList.add(objectDistance);
}

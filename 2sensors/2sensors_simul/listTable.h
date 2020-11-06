//https://www.arduino.cc/en/Hacking/LibraryTutorial
#ifndef listTable_h
#define listTable_h

class listTable
{
  public:
  int divider=25; //threshold = 3 x divider = 75
  struct obj {
    int object;
    int sensor;
    int timed=0;
  };
  LinkedList<obj> objectList = LinkedList<obj>();
  LinkedList<int> movementList = LinkedList<int>();

  int timeThreshold=50;
  int diff2=0;

  listTable();
  listTable(int divider);

  void updateTable();
  void setupTable(float avg1,float avg2);
  void addData(int sensor,int newData);
  
  //private:
};

#endif

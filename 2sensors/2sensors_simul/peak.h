//https://www.arduino.cc/en/Hacking/LibraryTutorial
#ifndef peak_h
#define peak_h

class peak
{
  public:
    //first (lag) window
    //LinkedList<int> pdSetupWindow = LinkedList<int>();
    int sumSetup=-1;
    float avgSetup=-1;
    float stdSetup=-1;
    //moving filter
    LinkedList<int> filteredWindow = LinkedList<int>();
    float avgFilter=-1;
    float stdFilter=-1;
    //parameter
    float threshold=5;
    float influence=0.1;
    int stat=0;
    
    peak();
    peak(float threshold,float influence);
    
    void addSignalSetup(int newdata);
    void addSignal(int newdata);
    void calculateAvgStd(bool first=false);
    void printInfo();
    int calculateStatus(int range);
    int checkStability(int range);
  //private:
};

#endif

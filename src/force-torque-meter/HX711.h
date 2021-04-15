#define tDoutFallingEdgeToSckrigingEdge 1 //us
#define tSckRisingEdgeToDoutDataReady 0.1 //us
#define tSckHighTime 1                    //us
#define tSckLowTime 1                     //us

#include "Arduino.h"

class HX711 {
  public:
    HX711(int dout, int sck);
    ~HX711();
    void setGain(int mul);
    void clrData();
    void readDOUT();
    bool dataReady();
    int32_t getRawData();
    int32_t readData();
    int32_t readAverageData(int numIter = 10);
    void tare(int numIter = 50);
//    void calibration(int weight, int numIter = 10);
  private:
    int DOUT;
    int PD_SCK;
    int gainMode;
    int32_t raw;
    int32_t offset;
//    float scale;
};

HX711::HX711(int dout, int sck){
  DOUT = dout;
  PD_SCK = sck;
  pinMode(DOUT, INPUT);
  pinMode(PD_SCK, OUTPUT);
  gainMode = 1; //default gain is 128
  raw = 0;
  offset = 0;
//  scale = 1;
}

HX711::~HX711(){
  
}

void HX711::setGain(int mul){
  switch(mul){
    case 128:
      gainMode = 1;
      break;
    case 32:
      gainMode = 2;
      break;
    case 64:
      gainMode = 3;
      break;
    default:
      gainMode = 0;
      break;
  }
}

void HX711::clrData(){
  raw = 0;
}

void HX711::readDOUT(){
  clrData();
  while(!dataReady()) delayMicroseconds(1);
  delayMicroseconds(tDoutFallingEdgeToSckrigingEdge);
  cli();
  for(int i=0; i<24; i++){
    digitalWrite(PD_SCK, HIGH);
    delayMicroseconds(tSckHighTime);
    digitalWrite(PD_SCK, LOW);
    delayMicroseconds(tSckLowTime);
    raw = raw << 1;
    raw |= digitalRead(DOUT);
  }
  for(int i=0; i<gainMode; i++){
    digitalWrite(PD_SCK, HIGH);
    delayMicroseconds(tSckHighTime);
    digitalWrite(PD_SCK, LOW);
    delayMicroseconds(tSckLowTime);    
  }
  sei();
  if(raw & 0x00800000) raw |= 0xFF000000;
}

bool HX711::dataReady(){
  if(!digitalRead(DOUT))return true;
  else return false;
}

int32_t HX711::getRawData(){
  readDOUT();
  return raw;
}

int32_t HX711::readData(){
  return (getRawData()-offset);//*scale;
}

int32_t HX711::readAverageData(int numIter = 10){
  int32_t tmp = 0;
  for(int i=0; i<numIter; i++){
    tmp += getRawData()/(int32_t)numIter;
  }
  return tmp-offset;
}

void HX711::tare(int numIter = 50){
  offset = 0;
  for(int i=0; i<numIter; i++){
    offset += getRawData()/numIter;
  }
}

//void HX711::calibration(int weight, int numIter = 10){
//  scale = weight/getAverageRawData(numIter);
//}

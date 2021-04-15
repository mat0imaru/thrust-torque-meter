#include "Arduino.h"

struct HX711{
  int DOUT;
  int PD_SCK;
  int32_t offset;
  int32_t scale;
  int32_t raw;
  struct HX711* next;
};

class HX711_ARRAY{
  public:
    HX711_ARRAY();
    ~HX711_ARRAY();
    void addSensor(int dout, int sck);
    void setGain(int mul);
    void clrData();
    void setSCK(int state);
    void readBit();
    void rawSignExtend();
    bool dataReady();
    void readDOUT();
    void getRawData(uint32_t* data);
    int32_t getData();
    int32_t getRawDatum(int dout);
    int32_t getDatum(int dout);
    int howManySensor();
  private:
    struct HX711* head;
    int gainMode = 1;
    int sensors = 0;

};

HX711_ARRAY::HX711_ARRAY(){
  head = NULL;
}

HX711_ARRAY::~HX711_ARRAY(){
  struct HX711* delPtr;
  while(head != NULL){
    delPtr = head;
    head = head->next;
    free(delPtr);
  }
}

void HX711_ARRAY::addSensor(int dout, int sck){
  struct HX711* newSensor = malloc(sizeof(struct HX711));
  newSensor->DOUT = dout;
  newSensor->PD_SCK = sck;
  pinMode(newSensor->DOUT, INPUT);
  pinMode(newSensor->PD_SCK, OUTPUT);
  newSensor->offset = 0;
  newSensor->scale = 0;
  newSensor->raw = 0;
  newSensor->next = head;
  head = newSensor;
  sensors += 1;
}

void HX711_ARRAY::setGain(int mul){
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

void HX711_ARRAY::clrData(){
  struct HX711* clrPtr = head;
  while(clrPtr != NULL){
    Serial.println(clrPtr->raw);
    clrPtr->raw = 0;
    clrPtr = clrPtr->next;
  }
  Serial.println("Clear!");
}

void HX711_ARRAY::setSCK(int state){
  struct HX711* clkPtr;
  while(clkPtr != NULL){
    digitalWrite(clkPtr, state);
    clkPtr = clkPtr->next;
  }
}

void HX711_ARRAY::readBit(){
  struct HX711* rdPtr = head;
  while(rdPtr != NULL){
    rdPtr->raw = rdPtr->raw << 1;
    rdPtr->raw |= digitalRead(rdPtr->DOUT);
    rdPtr = rdPtr->next;
  }
}

void HX711_ARRAY::rawSignExtend(){
  struct HX711* sePtr = head;
  while(sePtr != NULL){
    if(sePtr->raw & 0x00800000) sePtr->raw |= 0xFF000000;
    sePtr = sePtr->next;
  }
}

bool HX711_ARRAY::dataReady(){
  struct HX711* drPtr = head;
  while(drPtr != NULL){
      if(digitalRead(drPtr->DOUT))return false;
      drPtr = drPtr->next;
  }
  return true;
}

void HX711_ARRAY::readDOUT(){
  clrData();
  while(!dataReady()) delayMicroseconds(1);
  cli();
  for(int i=0; i<24; i++){
    setSCK(HIGH);
    delayMicroseconds(1);
    readBit();
    setSCK(LOW);
    delayMicroseconds(1);    
  }
  for(int i=0; i<gainMode; i++){
    setSCK(HIGH);
    delayMicroseconds(1);
    setSCK(LOW);
    delayMicroseconds(1);
  }
  sei();
  rawSignExtend();
}

void HX711_ARRAY::getRawData(uint32_t* data){
  readDOUT();
}

int32_t HX711_ARRAY::getData(){

}

int32_t HX711_ARRAY::getRawDatum(int dout){
  
}

int32_t HX711_ARRAY::getDatum(int dout){
  
}

int HX711_ARRAY::howManySensor(){
  return sensors;
}

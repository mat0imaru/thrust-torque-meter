#define SET_REG_BIT(p, b) (p |= (0x01 << b))
#define CLR_REG_BIT(p, b) (p &= ~(0x01 << b))

#define clkPin 8
#define forcePin1 7
#define forcePin2 6
#define forcePin3 5
#define torquePin1 10
#define torquePin2 11

#include "hx711.hpp"
//#include "HX711.h"
#include <string.h>
#include <EEPROM.h>

int logState = 0;
int currMotorSpeed = 0;

float tau = 0.8;
float ts = 1.0 - tau;

//int32_t prevforce = 0;
//int32_t prevTorque = 0;

//int32_t force1 = 0;
//int32_t force2 = 0;
//int32_t force3 = 0;
//int32_t torque1 = 0;
//int32_t torque2 = 0;

float forceScale = 1;
float torqueScale = 1;

Hx711 hx711;
int forceNode1;
int forceNode2;
int forceNode3;
int torqueNode1;
int torqueNode2;

void setup()
{
  Serial.begin(115200);

  // PWM setup
  SET_REG_BIT(DDRD, 3);
  SET_REG_BIT(PORTD, 3);
  // Fast PWM mode
  CLR_REG_BIT(TCCR2B, 3);
  SET_REG_BIT(TCCR2A, 1);
  SET_REG_BIT(TCCR2A, 0);
  // clk/128 (prescaler), 1clk/8microSec.
  SET_REG_BIT(TCCR2B, 2);
  CLR_REG_BIT(TCCR2B, 1);
  SET_REG_BIT(TCCR2B, 0);
  // non-inverting mode
  SET_REG_BIT(TCCR2A, 5);
  CLR_REG_BIT(TCCR2A, 4);
  // PWM initialize 0
  OCR2B = 0;

  // node initialization
  forceNode1 = hx711.InsertNode(forcePin1, clkPin);
  forceNode2 = hx711.InsertNode(forcePin2, clkPin);
  forceNode3 = hx711.InsertNode(forcePin3, clkPin);
  torqueNode1 = hx711.InsertNode(torquePin1, clkPin);
  torqueNode2 = hx711.InsertNode(torquePin2, clkPin);

  hx711.PrintNodeInfo();
  // tare once on boot
  hx711.UpdateOffset(HX711_GAIN_128, 100);
}

void loop()
{

  // read signal from all nodes
  hx711.UpdateAllNode(HX711_GAIN_128);

  int32_t average_force = (hx711.GetNodeData(forceNode1) + hx711.GetNodeData(forceNode2) + hx711.GetNodeData(forceNode3)) / 3;
  int32_t average_torque = (hx711.GetNodeData(torqueNode1) + hx711.GetNodeData(torqueNode2)) / 2;
  //
  //  Serial.print(average_force);
  //  Serial.print(", ");
  //  Serial.print(average_torque);
  //  Serial.println();

  //  // low pass filter
  //  int32_t force = tau*prevforce + ts*average_force;
  //  int32_t torque = tau*prevTorque + ts*average_torque;
  //
  //  prevforce = force;
  //  prevTorque = torque;

  if (logState == 1)
  {
    // data out
    Serial.print(currMotorSpeed);
    Serial.print(",");
    //Serial.print("force:");
    Serial.print(average_force * forceScale);
    Serial.print(",");
    //Serial.print("torque:");
    Serial.println(average_torque * torqueScale);
  }

  //  Serial.print("force1:");
  //  Serial.print(hx711.GetNodeData(forceNode1));//, BIN);
  //  Serial.print(",");
  //  Serial.print("force2:");
  //  Serial.print(hx711.GetNodeData(forceNode2));//, BIN);
  //  Serial.print(",");
  //  Serial.print("force3:");
  //  Serial.print(hx711.GetNodeData(forceNode3));//, BIN);
  //  Serial.print(",");
  //  Serial.print("torque1:");
  //  Serial.print(hx711.GetNodeData(torqueNode1));//, BIN);
  //  Serial.print(",");
  //  Serial.print("torque2:");
  //  Serial.println(hx711.GetNodeData(torqueNode2));//, BIN);

  // user interface
  if (Serial.available())
  {
    String userCommand = Serial.readString();
    char userCommandParse[userCommand.length()];
    userCommand.toCharArray(userCommandParse, userCommand.length());
    char *head = strtok(userCommandParse, " ");
    //Serial.println(head);
    if (strcmp(head, "tare") == 0)
    {
      int numIter = atoi(strtok(NULL, " "));
      if (numIter <= 0)
      {
        Serial.println(",,Invalid iteration number, give me integer over 0");
      }
      hx711.UpdateOffset(HX711_GAIN_128, numIter);
    }
    else if (strcmp(head, "calibrate") == 0)
    {
      char *option = strtok(NULL, " ");
      //Serial.println(option);
      if (strcmp(option, "force") == 0)
      {
        float balanceForce = atoi(strtok(NULL, " "));
        int numIter = 20;
        int32_t average_force = 0;
        for (int i = 0; i < numIter; i++)
        {
          hx711.UpdateAllNode(HX711_GAIN_128);
          average_force += ((hx711.GetNodeData(forceNode1) + hx711.GetNodeData(forceNode2) + hx711.GetNodeData(forceNode3)) / 3) / numIter;
        }
        forceScale = balanceForce / average_force;
      }
      else if (strcmp(option, "torque") == 0)
      {
        float balanceTorque = atoi(strtok(NULL, " "));
        int numIter = 20;
        int32_t average_torque = 0;
        for (int i = 0; i < numIter; i++)
        {
          hx711.UpdateAllNode(HX711_GAIN_128);
          average_torque += ((hx711.GetNodeData(torqueNode1) + hx711.GetNodeData(torqueNode2)) / 2) / numIter;
        }
        torqueScale = balanceTorque / average_torque;
      }
      else
      {
        Serial.println("Invalid option, give me option as force of torque");
      }
    }
    else if (strcmp(head, "throttle") == 0)
    {
      int motorSpeed = atoi(strtok(NULL, " "));
      if (motorSpeed > 100 || motorSpeed < 0)
      {
        Serial.println("Out of speed range, give me interger between 0 to 100");
      }
      else
      {
        //Serial.print("motor throttle setted to ");
        //Serial.println(motorSpeed);
        currMotorSpeed = map(motorSpeed, 0, 100, 125, 250);
        OCR2B = currMotorSpeed;
      }
    }
    else if (strcmp(head, "save") == 0)
    {
      char *option = strtok(NULL, " ");
      if (strcmp(option, "force") == 0)
      {
        EEPROM.write(0, 1);
        byte tmp[4];
        memcpy(&tmp, &forceScale, 4);
        for (int i = 1; i < 5; i++)
        {
          EEPROM.write(i, tmp[i - 1]);
        }
        Serial.println("force scale saved");
      }
      else if (strcmp(option, "torque") == 0)
      {
        EEPROM.write(5, 1);
        byte tmp[4];
        memcpy(&tmp, &torqueScale, 4);
        for (int i = 6; i < 10; i++)
        {
          EEPROM.write(i, tmp[i - 6]);
        }
        Serial.println("torque scale saved");
      }
      else
      {
        Serial.println("Invalid option, give me option as force of torque");
      }
    }
    else if (strcmp(head, "load") == 0)
    {
      char *option = strtok(NULL, " ");
      if (strcmp(option, "force") == 0)
      {
        if (EEPROM.read(0) == 1)
        {
          byte tmp[4];
          for (int i = 1; i < 5; i++)
          {
            tmp[i - 1] = EEPROM.read(i);
          }
          memcpy(&forceScale, &tmp, 4);
          Serial.println("force scale loaded");
        }
        else
        {
          Serial.println("there is no save of force scale");
        }
      }
      else if (strcmp(option, "torque") == 0)
      {
        if (EEPROM.read(5) == 1)
        {
          byte tmp[4];
          for (int i = 6; i < 10; i++)
          {
            tmp[i - 6] = EEPROM.read(i);
          }
          memcpy(&torqueScale, &tmp, 4);
          Serial.println("torque scale loaded");
        }
        else
        {
          Serial.println("there is no save of torque scale");
        }
      }
      else
      {
        Serial.println("Invalid option, give me option as force of torque");
      }
    }
    else if (strcmp(head, "log") == 0)
    {
      char *option = strtok(NULL, " ");
      if (strcmp(option, "on") == 0)
      {
        logState = 1;
      }
      else if (strcmp(option, "off") == 0)
      {
        logState = 0;
      }
    }
  }
  delay(100);
}

// 옛날 코드의 잔재들

//#define gainMode 1  // 1 for 128(Ch.A), 2 for 32(Ch.B) and 3 for 64(Ch.?)
//                    // 게인 값 64에 해당하는 채널은 데이터시트에 A와 B가 둘다 적혀있음. A로 보는것이 맞을 듯?
//                    // input pulses = (24 + gainMode)

//int pulses = 24 + gainMode;

//HX711 forceNode1(forcePin1, clkPin);
//HX711 forceNode2(forcePin1, clkPin);
//HX711 forceNode3(forcePin1, clkPin);
//HX711 torqueNode1(torquePin1, clkPin);
//HX711 torqueNode2(torquePin2, clkPin);

//  forceNode1.tare(10);
//  forceNode2.tare(10);
//  forceNode3.tare(10);
//  torqueNode1.tare(10);
//  torqueNode2.tare(10);

//  Serial.print("force1:");
//  Serial.print(forceNode1.readAverageData());
//  Serial.print(",");
//  Serial.print("force2:");
//  Serial.print(forceNode2.readAverageData());
//  Serial.print(",");
//  Serial.print("force3:");
//  Serial.print(forceNode3.readAverageData());
//  Serial.print(",");
//  Serial.print("torque1:");
//  Serial.print(torqueNode1.readAverageData());
//  Serial.print(",");
//  Serial.print("torque2:");
//  Serial.println(torqueNode2.readAverageData());

//    /*
//    switch(head){
//      case 'T':
//        hx711.UpdateOffset(HX711_GAIN_128, Serial.parseInt());
//        break;
//      case 'M':
//        OCR2B = map(Serial.parseInt(),0,100,125,250);
//      case '':
//      case '':
//      default:
//        break;
//    */

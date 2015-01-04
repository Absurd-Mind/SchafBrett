// ShiftPWM uses timer1 by default. To use a different timer, before '#include <ShiftPWM.h>', add
#define SHIFTPWM_USE_TIMER2  // for Arduino Uno and earlier (Atmega328)
// #define SHIFTPWM_USE_TIMER3  // for Arduino Micro/Leonardo (Atmega32u4)

// Clock and data pins are pins from the hardware SPI, you cannot choose them yourself.
// Data pin is MOSI (Uno and earlier: 11, Leonardo: ICSP 4, Mega: 51, Teensy 2.0: 2, Teensy 2.0++: 22) 
// Clock pin is SCK (Uno and earlier: 13, Leonardo: ICSP 3, Mega: 52, Teensy 2.0: 1, Teensy 2.0++: 21)

// You can choose the latch pin yourself.
const int ShiftPWM_latchPin=5;

// ** uncomment this part to NOT use the SPI port and change the pin numbers. This is 2.5x slower **
 #define SHIFTPWM_NOSPI
 const int ShiftPWM_dataPin = 3;
 const int ShiftPWM_clockPin = 6;

int program = 0;
int freq = 10;
int brig = 0;


// If your LED's turn on if the pin is low, set this to true, otherwise set it to false.
const bool ShiftPWM_invertOutputs = false;

// You can enable the option below to shift the PWM phase of each shift register by 8 compared to the previous.
// This will slightly increase the interrupt load, but will prevent all PWM signals from becoming high at the same time.
// This will be a bit easier on your power supply, because the current peaks are distributed.
const bool ShiftPWM_balanceLoad = true;

#include <ShiftPWM.h>   // include ShiftPWM.h after setting the pins!


// Here you set the number of brightness levels, the update frequency and the number of shift registers.
// These values affect the load of ShiftPWM.
// Choose them wisely and use the PrintInterruptLoad() function to verify your load.
unsigned char maxBrightness = 255;
unsigned char pwmFrequency = 40;
unsigned int numRegisters = 3;

#include "EEPROM.h"
#include "cc1101.h"

#define LEDOUTPUT        4       // Connect led to digital pin 4
#define RFCHANNEL        0       // Let's use channel 0
#define SYNCWORD1        0xB5    // Synchronization word, high byte
#define SYNCWORD0        0x47    // Synchronization word, low byte
#define SOURCE_ADDR      6       // Sender address
#define DESTINATION_ADDR 4       // Receiver address

bool debug = false;
bool debugSerial = false;

CC1101 cc1101;      // radio object
CCPACKET txPacket;  // packet object
byte count = 0;

/**
 * This function is called whenever a wireless packet is received
 */
void rfPacketReceived(void)
{
  CCPACKET packet;
    
  // Disable wireless reception interrupt
  detachInterrupt(0);
  if(cc1101.receiveData(&packet) > 0)
  {
//    Serial.println("receiving data");
    if (packet.crc_ok/* && packet.length > 1*/)
    {
      if (packet.data[0] == SOURCE_ADDR) {
          
      
      program = 0;
      if (packet.data[1] == 1) {
        ShiftPWM.SetOne(packet.data[2], packet.data[3]);        
      } else if (packet.data[1] == 2) {
        int value = packet.data[2];
        int n = packet.data[3];
         for (int i = 0; i < n; i++) {
           ShiftPWM.SetOne(packet.data[i+4], value);
         } 
      } else if (packet.data[1] == 3) {
        ShiftPWM.SetOne(packet.data[2], packet.data[5]);
        ShiftPWM.SetOne(packet.data[3], packet.data[6]);
        ShiftPWM.SetOne(packet.data[4], packet.data[7]);
      } else if (packet.data[1] == 4) {
          int r = packet.data[2];
          int g = packet.data[3];
          int b = packet.data[4];
          
          int n = packet.data[5];
          for (int i = 0; i < n; i++) {
            ShiftPWM.SetOne(packet.data[6 + i*3], r);
            ShiftPWM.SetOne(packet.data[7 + i*3], g);
            ShiftPWM.SetOne(packet.data[8 + i*3], b);
          }
      } else if (packet.data[1] == 5) {
          freq = packet.data[2];
          brig = packet.data[3];
          program = 1;
      } else if (packet.data[1] == 6) {
         for (int i = 0; i < 24; i++) {
            ShiftPWM.SetOne(packet.data[i+1], 0);
         } 
      }
      } else {
        Serial.println("invalid packet");
      }
      if (debugSerial) {
        Serial.println("packet");
        Serial.println(packet.data[1]);
        Serial.println(packet.data[2]);
        Serial.println(packet.data[3]);
        Serial.println(packet.data[4]);
        Serial.println(packet.data[5]);
        Serial.println(packet.data[6]);
        Serial.println(packet.data[7]);
        Serial.println(packet.data[8]);
      }
    }
  }

  // Enable wireless reception interrupt
  attachInterrupt(0, rfPacketReceived, FALLING);
}



void setup(){
  if (debugSerial) {
    while(!Serial){
      delay(100); 
    }
    Serial.begin(9600);
  }

  // Sets the number of 8-bit registers that are used.
  ShiftPWM.SetAmountOfRegisters(numRegisters);

  // SetPinGrouping allows flexibility in LED setup. 
  // If your LED's are connected like this: RRRRGGGGBBBBRRRRGGGGBBBB, use SetPinGrouping(4).
  ShiftPWM.SetPinGrouping(1); //This is the default, but I added here to demonstrate how to use the funtion
  
  ShiftPWM.Start(pwmFrequency,maxBrightness);
  ShiftPWM.SetOne(8, 255);
  ShiftPWM.SetOne(9, 255);
//  ShiftPWM.SetOne(10, 255);
//  ShiftPWM.SetOne(11, 255);
  
  // Init RF IC
  cc1101.init();
  cc1101.setChannel(RFCHANNEL, false);
  byte sw[] = {SYNCWORD1, SYNCWORD0};
  cc1101.setSyncWord(sw, false);
  cc1101.setDevAddress(SOURCE_ADDR, false);

  // Let's disable address check for the current project so that our device
  // will receive packets even not addressed to it.
  cc1101.disableAddressCheck();
  if (!debug) {
    attachInterrupt(0, rfPacketReceived, FALLING);
  }
}
int i = 0;

void loop()
{
  
  if (program == 1) {
      ShiftPWM.SetOne(8, brig);
      ShiftPWM.SetOne(9, brig);
      ShiftPWM.SetOne(10, brig);
      ShiftPWM.SetOne(11, brig);
      delay(freq);
      ShiftPWM.SetOne(8, 0);
      ShiftPWM.SetOne(9, 0);
      ShiftPWM.SetOne(10, 0);
      ShiftPWM.SetOne(11, 0);
      delay(freq);
  } else {
    delay(1000);
  }
  if (debug) {
    if (i == 0) {
      i=255;
    } else {
      i=0;
    }
    
    for (int x = 0; x < 8*numRegisters; x++) {
      ShiftPWM.SetOne(x, i);
    }
    
    return; 
  }
}



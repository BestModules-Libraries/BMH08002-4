/*************************************************
File:       	     BMH08002-4.h
Author:              BESTMODULE
Description:         Define classes and required variables
Version:             V1.0.2   --2023-09-06

**************************************************/
#ifndef _BMH08002_4_H__
#define _BMH08002_4_H__

#include <Arduino.h>
#include <SoftwareSerial.h>

#define START_MEASURE   0x00
#define STOP_MEASURE    0x01
#define ENTER_HALT      0x02
#define CHECK_OK        0
#define CHECK_ERROR     1
#define TIMEOUT_ERROR   2

/*  Oximetry data */

class BMH08002_4
{
public:
   BMH08002_4(uint16_t enPin,HardwareSerial *theSerial = &Serial);
   BMH08002_4(uint16_t enPin,uint16_t rxPin,uint16_t txPin);
   void begin();
   uint8_t beginMeasure();
   uint8_t endMeasure();
   uint8_t sleep();
   void powerDown();
   uint8_t requestInfoPackage(uint8_t buff[]);
   bool isInfoAvailable();
   void readInfoPackage(uint8_t buff[]);
   uint8_t updateWave();
   uint8_t calSensitivity();
   uint16_t getTimeInterval();
   uint8_t getModeConfig();   
   uint8_t setTimeInterval(uint16_t Time);
   uint8_t setModeConfig(uint8_t mode_code);
   
private:
   HardwareSerial *_serial = NULL;
   SoftwareSerial *_softSerial = NULL;
   uint16_t _rxPin;
   uint16_t _txPin;
   uint16_t _enPin;
   uint16_t _checksum;
   uint8_t _recBuf[15] = {0}; // Array for storing received data
   uint8_t setWorkStatus(uint8_t workStatus);
   void getChecksum(uint8_t buff[],uint8_t lenght);
   uint8_t writeEEPROM(uint8_t addr,uint8_t highByte,uint8_t lowByte);
   uint16_t readEEPROM(uint8_t addr);
   void writeBytes(uint8_t wbuf[], uint8_t wlen);
   uint8_t readBytes(uint8_t rbuf[], uint8_t rlen, uint16_t timeout);  
};

#endif

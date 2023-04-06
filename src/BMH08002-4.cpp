/*************************************************
File:       			BMH08002-4.cpp
Author:           BESTMODULE
Description:      UART communication with the BMH08002-4 and obtain the corresponding value
History：			    
  V1.0.1	 -- initial version；2023-02-15；Arduino IDE :v1.8.13

**************************************************/
#include "BMH08002-4.h"

/*************************************************
Description:  Constructor
Parameters:    *theSerial  : Wire object if your board has more than one UART interface            
Return:             
Others:             
*************************************************/
BMH08002_4::BMH08002_4(uint16_t enPin,HardwareSerial *theSerial)
{
  _enPin = enPin;
  pinMode(_enPin, OUTPUT);
  digitalWrite(_enPin, HIGH);
  _softSerial = NULL;
  _serial = theSerial;
}
/************************************************
Description:  Constructor
Parameters:    intPin :INT Output pin connection with Arduino, 
                      the INT will be pulled down when an object approaches
              rxPin : Receiver pin of the UART            
Return:            
Others:           
*************************************************/
BMH08002_4::BMH08002_4(uint16_t enPin,uint16_t rxPin,uint16_t txPin)
{

  _serial = NULL;
  _enPin = enPin;
  _rxPin = rxPin;
  _txPin = txPin;
  pinMode(_enPin, OUTPUT);
  digitalWrite(_enPin, HIGH);
  _softSerial = new SoftwareSerial(_rxPin,_txPin);
}
/*************************************************
Description:  Module Initial,default start measure
Parameters:                      
Return:             
Others:             
*************************************************/
void BMH08002_4::begin()
{
  if (_softSerial != NULL)
  {
    _softSerial->begin(38400); 
  }
  else
  {
    _serial->begin(38400); 
  }
  delay(100);
}
/*************************************************
Description:  beginMeasure
Parameters:   none
Return:     
        0 - Success
        1 - Failure       
Others:           
*************************************************/
uint8_t BMH08002_4::beginMeasure()
{
  digitalWrite(_enPin,HIGH);
  uint8_t i=0;
  i=setWorkStatus(0x00);
  return i;
}
/*************************************************
Description:  endMeasure
Parameters:   none
Return:     
        0 - Success
        1 - Failure       
Others:           
*************************************************/
uint8_t BMH08002_4::endMeasure()
{
  uint8_t i=0;
  i=setWorkStatus(0x01);
  return i;
}
/*************************************************
Description:  sleep
Parameters:   none
Return:     
        0 - Success
        1 - Failure       
Others:           
*************************************************/
uint8_t BMH08002_4::sleep()
{
  uint8_t i=0;
  i=setWorkStatus(0x02);
  return i;
}
/*************************************************
Description: powerDown  EN=0
Parameters:  none
Return: none
Others:  none         
*************************************************/
void BMH08002_4::powerDown()
{
  digitalWrite(_enPin,LOW);
}

/*************************************************
Description:  Judge the module measurement status and read the data.      
Parameters:   BUFF[]:10 byte
Return:       0: No measurement data
              1: Generating data
              2: Data generation is completed and can be read
Others:
*************************************************/
uint8_t BMH08002_4::requestInfoPackage(uint8_t buff[])
{
  uint8_t temp,readFlag = 1;
  temp = readEEPROM(0x01);    //get work status,Timing send mode and waveform mode do not require sending commands
  uint8_t rxBuf[15]={0};
  if (bitRead(temp, 2) == 0 && bitRead(temp, 1) == 1)
  {
      uint8_t txBuf[8]={0x55,0xb1,0x03,0x00,0x00,0x00,0x00,0xaa};
      getChecksum(txBuf,8);
      writeBytes(txBuf, 8);
      delay(50);
  } 
  
    readFlag = readBytes(rxBuf, 15, 30);
    if (readFlag==0x00 && rxBuf[0] == 0x55 && rxBuf[14] == 0xAA)
    {
          for(uint8_t i=0;i<10;i++)
          {
            buff[i]=rxBuf[i+3];
          }
          return rxBuf[2];
    }
    return 3;
}

/**********************************************************
Description: Query whether the 15-byte data sent by the module is received
Parameters: None
Return: true(1): 15-byte data received
        false(0): 15-byte data not received
Others: Only used in the mode of Tx Auto Output Info
**********************************************************/
bool BMH08002_4::isInfoAvailable()
{
  uint8_t header[2] = {0x55, 0xB0}; // Fixed code for first 2 bytes of 15-byte data
  uint8_t recBuf[15] = {0}, recLen = 15;
  uint8_t i, num = 0, readCnt = 0, failCnt = 0, checkCode = 0;
  bool isHeader = false, result = false;

  /* Select hardSerial or softSerial according to the setting */
  if (_softSerial != NULL)
  {
    num = _softSerial->available();
  }
  else if (_serial != NULL)
  {
    num = _serial->available();
  }

  /* Serial buffer contains at least one 15-byte data */
  if (num >= recLen)
  {
    while (failCnt < 2) // Didn't read the required data twice, exiting the loop
    {
      /* Find 2-byte data header */
      for (i = 0; i < 2;)
      {
        if (_softSerial != NULL)
        {
          recBuf[i] = _softSerial->read();
        }
        else if (_serial != NULL)
        {
          recBuf[i] = _serial->read();
        }

        if (recBuf[i] == header[i])
        {
          isHeader = true; // Fixed code is correct
          i++;             // Next byte
        }
        else if (recBuf[i] != header[i] && i > 0)
        {
          isHeader = false; // Next fixed code error
          failCnt++;
          break;
        }
        else if (recBuf[i] != header[i] && i == 0)
        {
          readCnt++; // 0x55 not found, continue
        }
        if (readCnt >= (num - 2))
        {
          readCnt = 0;
          isHeader = false; // Fixed code not found
          break;
        }
      }
      /* Find the correct fixed code */
      if (isHeader)
      {
        checkCode = recBuf[1]; // Sum checkCode
        for (i = 2; i < recLen; i++) // Read subsequent 13-byte data
        {
          if (_softSerial != NULL)
          {
            recBuf[i] = _softSerial->read();
          }
          else if (_serial != NULL)
          {
            recBuf[i] = _serial->read();
          }
          checkCode += recBuf[i]; // Sum checkCode
        }
        checkCode = checkCode - recBuf[recLen - 1] - recBuf[recLen - 2];

        /* Compare whether the check code is correct */
        if (checkCode == recBuf[recLen - 2])
        {
          for (i = 0; i < recLen; i++)
          {
            _recBuf[i] = recBuf[i]; // True, assign data to _recBuf[]
          }
          result = true;
          break; // Exit "while (failCnt < 2)" loop
        }
        else
        {
          failCnt++; // Error, failCnt plus 1, return "while (failCnt < 2)" loop
          checkCode = 0;
        }
      }
    }
  }
  return result;
}


/*************************************************
Description:  Updating waveform
Parameters:   none
Return:     
        0 - Success
        1 - Failure       
Others:           
*************************************************/
uint8_t BMH08002_4::updateWave()
{
    uint8_t rxBuf[8]={0};
    uint8_t txBuf[8]={0x55,0xb1,0x02,0x00,0x00,0x00,0x00,0xaa};
    getChecksum(txBuf,8);
    writeBytes(txBuf, 8);
    delay(50);
    if (readBytes(rxBuf, 8, 30)==0x00&&rxBuf[1] == 0xB1 && rxBuf[2] == 0x02)
        {
            return 0;
        }
    else
        {
            return 1;
        }
}
   
/*************************************************
Description:  Calibration detects finger sensitivity        
Parameters:                   
Return:            
Others:       Place the finger in the position that needs to be sensed and keep the finger relatively stable. 
              After sending this command, the calibration can be completed. 
              The detection distance should not exceed 2CM, otherwise it will affect the measurement.          
*************************************************/
uint8_t BMH08002_4::calSensitivity()
{
  uint8_t rxBuf[8]={};
  uint8_t txBuf[8]={0x55,0xb1,0x06,0x00,0x00,0x00,0x00,0xaa};
  getChecksum(txBuf,8);
  writeBytes(txBuf, 8);
  delay(50);
  if (readBytes(rxBuf, 8, 30)==0x00&&rxBuf[1] == 0xB1 && rxBuf[2] == 0x06)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
/*************************************************
Description:  Gets the timed send interval
Parameters:   none
Return:     
        intTime - the timed send interval
        0 - Failure       
Others:           
*************************************************/
uint16_t  BMH08002_4::getTimeInterval()
{
    uint8_t rxBuf[8]={};
    uint8_t txBuf[8]={0x55,0xb1,0x05,0x0A,0x00,0x00,0x00,0xaa};
    getChecksum(txBuf,8);
    writeBytes(txBuf, 8);
    delay(50);
    if (readBytes(rxBuf, 8, 30)==0x00&&rxBuf[1] == 0xB1 && rxBuf[2] == 0x05)
    {
      uint16_t intTime = (rxBuf[4]<<8) + rxBuf[5];
      return intTime;
    }
    return 0;
  
}

/*************************************************
Description:  get the ModeConfig
Parameters:   none
Return:     
        rxBuf[5] -  ModeConfig
                        0x00:Timed transmission mode, red light off when finger is detected
                        0x01:Timed transmission mode, red light on when finger is detected
                        0x02:Ask-response mode, red light off when finger is detected
                        0x03:Ask-response mode, red light on when finger is detected
                        0x04:Continuous upload mode, red light off when finger is detected
                        0x05:Continuous upload mode, red light on when finger is detected
                        0x06:Timed transmission mode, red light off when finger is detected
                        0x07:Timed transmission mode, red light on when finger is detected    
        8 - Failure       
Others:           
*************************************************/
uint8_t  BMH08002_4::getModeConfig()
{
    uint8_t rxBuf[8]={};
    uint8_t txBuf[8]={0x55,0xb1,0x05,0x01,0x00,0x00,0x00,0xaa};
    getChecksum(txBuf,8);
    writeBytes(txBuf, 8);
    delay(50);
    if (readBytes(rxBuf, 8, 30)==0x00&&rxBuf[1] == 0xB1 && rxBuf[2] == 0x05)
    {
      return rxBuf[5];
    }
    else
    {
      return 8;
    }
}
/**********************************************************
Description: Read data through UART
Parameters: rbuf: Used to store received data
            rlen: Length of data to be read
            timeout：timeout time
Return:  0:OK
         1:CHECK_ERROR
         2: timeout error
Others: None
**********************************************************/
uint8_t BMH08002_4::readBytes(uint8_t rbuf[], uint8_t rlen, uint16_t timeout)
{
  uint8_t i = 0, delayCnt = 0,checkSum=0;
/* Select SoftwareSerial Interface */
  if (_softSerial != NULL)
  {
    for (i = 0; i < rlen; i++)
    {
      delayCnt = 0;
      while (_softSerial->available() == 0)
      {
        if (delayCnt > timeout)
        {
          return TIMEOUT_ERROR; // Timeout error
        }
        delay(1);
        delayCnt++;
      }
      rbuf[i] = _softSerial->read();
    }
  }
/* Select HardwareSerial Interface */
  else
  {
    for (i = 0; i < rlen; i++)
    {
      delayCnt = 0;
      while (_serial->available() == 0)
      {
        if (delayCnt > timeout)
        {
          return TIMEOUT_ERROR; // Timeout error
        }
        delay(1);
        delayCnt++;
      }
      rbuf[i] = _serial->read();
    }
  }
  /* check Sum */
  for (i = 1; i < (rlen - 2); i++)
  {
    checkSum += rbuf[i];
  }
  if (checkSum == rbuf[rlen - 2])
  {
    return CHECK_OK; // Check correct
  }
  else
  {
    return CHECK_ERROR; // Check error
  }
}

/**********************************************************
Description: Read the 15-byte data of sent by the module
Parameters: array: The array for storing the 15-byte module information
                  (refer to datasheet for meaning of each bit)
Return: None
Others: Use after isInfoAvailable()
**********************************************************/
void BMH08002_4::readInfoPackage(uint8_t buff[])
{
  for (uint8_t i = 0; i < 15; i++)
  {
    buff[i] = _recBuf[i];
  }
}
/*************************************************
Description:  set Time Interval
Parameters:   Time:Time Interval,1=4ms
Return:     
        0 - Success
        1 - Failure       
Others:           
*************************************************/
uint8_t  BMH08002_4::setTimeInterval(uint16_t Time)
{
    uint8_t Time_H = Time>>8;
    uint8_t Time_L = Time &0xFF;
    uint8_t rxBuf[8]={};
    uint8_t txBuf[8]={0x55,0xb1,0x04,0x0A,Time_H,Time_L,0x00,0xaa};
    getChecksum(txBuf,8);
    writeBytes(txBuf, 8);
    delay(50);
    if (readBytes(rxBuf, 8, 30)==0x00&&rxBuf[1] == 0xB1 && rxBuf[2] == 0x04)
    {
      return 0;
    }
    else
    {
      return 1;
    }
}

/*************************************************
Description:  set Mode Config
Parameters:   mode_code:0x00:Timed transmission mode, red light off when finger is detected
                        0x01:Timed transmission mode, red light on when finger is detected
                        0x02:Ask-response mode, red light off when finger is detected
                        0x03:Ask-response mode, red light on when finger is detected
                        0x04:Continuous upload mode, red light off when finger is detected
                        0x05:Continuous upload mode, red light on when finger is detected
                        0x06:Timed transmission mode, red light off when finger is detected
                        0x07:Timed transmission mode, red light on when finger is detected    
Return:     
             
Others:           
*************************************************/
uint8_t BMH08002_4::setModeConfig(uint8_t mode_code)
{
  if(0x00 < mode_code  && mode_code > 0x0F)//CTRL_LOGIC=0
  {
      return 1;
  }
   uint8_t i = writeEEPROM(0x01,0x00,mode_code);
   return i;
}

/*************************************************
Description:  setWorkStatus
Parameters:   Start measuring (0x00),
End measurement (0x01)
Go to sleep (0x02)
Return:     
        0 - Success
        1 - Failure       
Others:           
*************************************************/
uint8_t BMH08002_4::setWorkStatus(uint8_t workStatus)
{
  uint8_t rxBuf[8]={0};
  uint8_t txBuf[8]={0x55,0xb1,0x01,0x00,0x00,workStatus,0x00,0xaa};
  getChecksum(txBuf,8);
  writeBytes(txBuf, 8);
  delay(50);
  if (readBytes(rxBuf, 8, 30)==0x00&&rxBuf[1] == 0xB1 && rxBuf[5] == workStatus)
      {
          return 0;
      }
  else
      {
          return 1;
      }
}
/*************************************************
Description:  get the checksum
Parameters:  The checksum are stored in txBuf[lenght-2]
Return: 
Others:           
*************************************************/
void BMH08002_4::getChecksum(uint8_t buff[],uint8_t lenght)
{
  for (uint8_t i = 1; i < (lenght - 2); i++)
  {
    buff[lenght - 2] +=buff[i];
  }
}
/*************************************************
Description:  Write EEPROM
Parameters:   EEPROM address,write value            
Return:       1：write fail  0：Write success
Others:       data_HighByte Writes the EEPROM address addr
              data_LowByte Writes the EEPROM address addr+1
*************************************************/
uint8_t BMH08002_4::writeEEPROM(uint8_t addr,uint8_t highByte,uint8_t lowByte)
{
  uint8_t rxBuf[8]={};
  uint8_t txBuf[8]={0x55,0xb1,0x04,addr,highByte,lowByte,0x00,0xaa};
  getChecksum(txBuf,8);
  writeBytes(txBuf, 8);
  delay(50);
  if (readBytes(rxBuf, 8, 30)==0x00&&rxBuf[1] == 0xB1 && rxBuf[2] == 0x04)
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
/*************************************************
Description:  Read EEPROM
Parameters:   EEPROM address       
Return:       EEPROM address value
Others:       HighByte is the data for addr
              LowByte is the data for addr+1
*************************************************/
uint16_t BMH08002_4::readEEPROM(uint8_t addr)
{
  uint8_t rxBuf[8]={0};
  uint8_t txBuf[8]={0x55,0xb1,0x05,addr,0x00,0x00,0x00,0xaa};
  uint16_t temp=0;
  getChecksum(txBuf,8);
  writeBytes(txBuf, 8);
  delay(50);
  if (readBytes(rxBuf, 8, 30)==0x00&&rxBuf[0]==0x55 && rxBuf[7] == 0xAA && rxBuf[1] == 0xB1)
      {         
        temp = rxBuf[4] << 8 | rxBuf[5];
      }
  return temp;
}
/**********************************************************
Description: Write data through uart
Parameters: wbuf:The array for storing Data to be sent
            wlen:Length of data sent
Return: None
Others: None
**********************************************************/
void BMH08002_4::writeBytes(uint8_t wbuf[], uint8_t wlen)
{
  /* Select SoftwareSerial Interface */
  if (_softSerial != NULL)
  {
    while (_softSerial->available() > 0)
    {
      _softSerial->read();
    }
    _softSerial->write(wbuf, wlen);
  }
  /* Select HardwareSerial Interface */
  else
  {
    while (_serial->available() > 0)
    {
      _serial->read();
    }
    _serial->write(wbuf, wlen);
  }
}

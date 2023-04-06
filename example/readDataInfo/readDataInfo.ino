/*************************************************
File:       		  readDataInfo
Description:      1.SoftwareSerial interface (BAUDRATE 38400) is used to communicate with mySpo2.
                  2.hardware Serial (BAUDRATE 9600) is used to communicate with Serial port monitor. 
Note:
**************************************************/
#include <BMH08002-4.h>
//BMH08002_4 mySpo2(2,5,4);       //enPin,rxPin,txPin,Please comment out this line of code if you don't use SW Serial
BMH08002_4 mySpo2(22,&Serial1);  //Please uncomment out this line of code if you use HW Serial1 on BMduino
//BMH08002_4 mySpo2(25,&Serial2);  //Please uncomment out this line of code if you use HW Serial2 on BMduino
// BMH08002_4 mySpo2(2,&Serial3);  //Please uncomment out this line of code if you use HW Serial3 on BMduino
// BMH08002_4 mySpo2(2,&Serial4);  //Please uncomment out this line of code if you use HW Serial4 on BMduino
uint8_t Mode=0;
uint8_t rBuf[15]={0};
uint8_t Status=0;
uint8_t flag=0;

void setup()
{
  Serial.begin(9600); // start serial for output
  mySpo2.begin();
  mySpo2.setModeConfig(0x01);//Timed transmission mode, red light on when finger is detected
  mySpo2.setTimeInterval(300);
  Serial.println("Please place your finger"); 
  delay(2000);  //Wait for finger placement
  mySpo2.beginMeasure();

  Mode = mySpo2.getModeConfig();
  if(Mode == 0x02 || Mode == 0x03)
  {
    Mode = 1;
  }
  else Mode = 0;
}
void loop()
{
  
  switch(Mode)
  {
    case 1:
      Mode_ask();
      break;
    default:
      Mode_continuous_timing();
  }
}

void Mode_ask ()
{
    Status= mySpo2.requestInfoPackage(rBuf);
    if (Status==0x02)
    {
      Serial.println("Measurement completed,Can remove fingers"); 
      Serial.print("SpO2:"); 
      Serial.print(rBuf[0],DEC);
      Serial.println("%"); 
      Serial.print("Heart rate:"); 
      Serial.print(rBuf[1],DEC);
      Serial.println("%"); 
      Serial.print("PI:"); 
      Serial.print((float)rBuf[2] / 10);
      Serial.println("%");
      mySpo2.endMeasure(); //stop Measure
      mySpo2.sleep();   //enter Halt
    }
    if (Status==0x01&&flag!=1)
    {
        Serial.println("Don't move your fingers");
        flag=1;
    }
    if (Status==0x00&&flag!=0)
    {
        Serial.println("Please reposition your fingers");
        flag=0;
    }
}

void Mode_continuous_timing()
{    
  if (mySpo2.isInfoAvailable() == true) // Scaning the serial port received buffer to receive the information sent by the module
  {
    mySpo2.readInfoPackage(rBuf);
    for(uint8_t i=0;i<15;i++)
    {
      Serial.print(i);
      Serial.print(":");
      Serial.println(rBuf[i]);  
    }
  }
}

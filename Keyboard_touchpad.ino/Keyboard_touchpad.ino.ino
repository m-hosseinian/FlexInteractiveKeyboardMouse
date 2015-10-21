#include "mpr121.h"
#include "i2c.h"

double pressThrs1 = 0.90;
double pressThrs2 = 0.6;
double pressThrs3 = 0.55;
double pressThrs4 = 0.45;

int baseVals[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int vals[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint16_t touches;
uint16_t presses = 0x0000;
uint16_t lastPresses = 0x0000;
uint16_t lastTouches = 0x0000;

void setup()
{
  //configure serial out
  Serial.begin(115200);
  
  // initalize I2C bus. Wiring lib not used. 
  i2cInit();
  
  // initialize mpr121
  mpr121QuickConfig();
    
  Serial.println("Ready...");
  
  delay(100);
  collectBaseVals();
}

void loop()
{
  long ratio;
  populateInteractions();
  int value;
  for (int i=0; i<12; i++) {
    //populateInteractions();
    if ((presses & (1 << i)) && !(lastPresses & (1 << i))) {
      value = vals[i];
      Serial.print(i);Serial.print(" pressed");
      Serial.print(": value=");Serial.print(value);
      Serial.print(", base=");Serial.print(baseVals[i]);
      ratio = (value * 10) / (baseVals[i] / 10);
      Serial.print(", ratio=%");Serial.println(ratio);
    } 
    
    if ((touches & (1 << i)) && !(lastTouches & (1 << i))) {
      value = vals[i];
      Serial.print(i);;Serial.print(" touched");
      Serial.print(": value=");Serial.print(value);
      Serial.print(", base=");Serial.print(baseVals[i]);
      ratio = (value * 10) / (baseVals[i] / 10);
      Serial.print(", ratio=%");Serial.println(ratio);
    } 
  }
  lastPresses = presses;
  lastTouches = touches;
 // delay(100);
}

void collectBaseVals()
{
  int maxBaseVals[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  
  for (int i=0; i<12; i++) {
    for (int j=0; j<20; j++) {
      maxBaseVals[i] = max(maxBaseVals[i], getSensorMeasurement(i));
      //delay(30);
    }
    baseVals[i] = maxBaseVals[i];
  }
}

void populateInteractions()
{
  int value,temp;
  boolean touch;
  boolean pressed;
   for (int i=0; i<12; i++) {
      touch = false;
      pressed = false;
      temp = 10000;
      value = getSensorMeasurement(i);
      if (value < (baseVals[i] * pressThrs1)) { // 85%
        touch = true;
        //delay(50);
        do {
          temp = getSensorMeasurement(i);
          value = min(temp, value); // store minimum value
        } while(temp < (baseVals[i] * pressThrs1) && 
                temp > (baseVals[i] * pressThrs2));
        
        if (value < (baseVals[i] * pressThrs2)) { // 70%
          touch = false;
          pressed = true;
          do{
             temp = getSensorMeasurement(i);
             value = min(temp, value); // store minimum value
          }
          while(temp < baseVals[i] * pressThrs1);
          //for(int j = pressThrs2 * 10; j>0; j--){
           // if (value < (baseVals[i] * (j / 10))){
            //  delay(10);
             // temp = min(getSensorMeasurement(i), value);
             // if(temp > value) {break;}
             // value = temp;
            //}
          //}
         //delay(10); 
         //while(getSensorMeasurement(i) < baseVals[i] * pressThrs1);
        }
      }
     if(touch) {
       touches = touches | (1 << i); ////////
       presses = presses & ~(1 << i);
       vals[i] = value;
     } else {
       touches = touches & ~(1 << i);
     }
     if (pressed){
       presses = presses | (1 << i); ////////
       touches = touches & ~(1 << i);
       vals[i] = value;
     } else {
       presses = presses & ~(1 << i);
     } 
     
   }
}

int getSensorMeasurement(byte sensorNumber)
{
  int value = mpr121Read2Bytes(0x04 + (sensorNumber << 1));  
  return value;
}

void printvalues()
{
  for (int i=0; i<12; i++) {
    Serial.print(i);Serial.print(": ");Serial.print(getSensorMeasurement(i)); 
    Serial.print("\t");
  }
  Serial.println();
}

void mpr121QuickConfig(void)
{
  // reset (in case already running)
  mpr121Write(0x80, 0x63);

  // auto config off  
  mpr121Write(ATO_CFG0, 0x00);
  
  // big sensors, use max charge current
  // FFI = 00 (default)
  // CDC = 111111
  mpr121Write(0x5C, 0x3F);

  // CDT=011 charge time, use the one that fits the size of your sensor best
  // SFI=00 (default)
  // ESI=100 (default)
 // mpr121Write(0x5D, 0x24); // CDT=001
  mpr121Write(0x5D, 0x44); // CDT=010
  //mpr121Write(0x5D, 0x64); // CDT=011
  //mpr121Write(0x5D, 0x84); // CDT=100
  
  // Electrode Configuration
  mpr121Write(ELE_CFG, 0x0C);	// Enables all 12 Electrodes
  //mpr121Write(ELE_CFG, 0x01);  // Enable first electrode only
}

#include "mpr121.h"
#include "i2c.h"

double touchThreshold = 0.95;
double pressThreshold = 0.55;

boolean touch[12] = {false,false,false,false,false,false,false,false,false,false,false,false};
boolean pressed[12] = {false,false,false,false,false,false,false,false,false,false,false,false};
int temp=0;
int value[12] = {10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000};
int baseVals[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int vals[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint16_t touches;
uint16_t presses = 0x0000;
uint16_t lastHolds = 0x0000;
//uint16_t lastTouches = 0x0000;

unsigned long pinPressedMillis[8] = {0, 0, 0, 0, 0, 0, 0, 0};
boolean hold[8] = {false,false,false,false,false,false,false,false};
int timeOut = 800;
uint16_t holds = 0x0000;

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
    if ((presses & (1 << i))) {
      value = vals[i];
      Serial.print(i);Serial.print(" pressed");
      Serial.print(": value=");Serial.print(value);
      Serial.print(", base=");Serial.print(baseVals[i]);
      ratio = (value * 10) / (baseVals[i] / 10);
      Serial.print(", ratio=%");Serial.println(ratio);
    } 
    
    if ((touches & (1 << i))) {
      value = vals[i];
      Serial.print(i);Serial.print(" touched");
      Serial.print(": value=");Serial.print(value);
      Serial.print(", base=");Serial.print(baseVals[i]);
      ratio = (value * 10) / (baseVals[i] / 10);
      Serial.print(", ratio=%");Serial.println(ratio);
    } 
    
    if ((holds & (1 << i)) && !(lastHolds & (1 << i))) {
      Serial.print(i);Serial.println(" hold");
    }
    
    if ((lastHolds & (1 << i)) && !(holds & (1 << i))) {
      Serial.print(i);Serial.println(" unhold");
    }
  }
  lastHolds = holds;
  //lastTouches = touches;
 // delay(100);
}

void collectBaseVals()
{
  int maxBaseVals[8] = {0, 0, 0, 0, 0, 0, 0, 0};
  
  for (int i=0; i<8; i++) {
    for (int j=0; j<20; j++) {
      maxBaseVals[i] = max(maxBaseVals[i], getSensorMeasurement(i));
      //delay(30);
    }
    baseVals[i] = maxBaseVals[i];
  }
}



void populateInteractions()
{
   for (int i=0; i<8; i++) {
      temp = getSensorMeasurement(i);
      if (temp < (baseVals[i] * touchThreshold)) { // 85%
        if(!pressed[i])
          {
            touch[i] = true;
          }
         value[i] = min(temp, value[i]);
         if(temp < (baseVals[i] * touchThreshold) && 
                  temp > (baseVals[i] * pressThreshold))
           {
              continue;
           }
    
         if (temp < (baseVals[i] * pressThreshold)) { // 70%
         
            touch[i] = false;
            pressed[i] = true;
            
            if (pinPressedMillis[i] == 0) {
              pinPressedMillis[i] = millis();
            }
            
            if (!hold[i] && ((millis() - pinPressedMillis[i]) > timeOut)) { // pin is hold
              hold[i] = true;
              fireHold(i);
            }
            
            continue;
          }
      }
      
       if(touch[i]) {
         touches = touches | (1 << i); 
         presses = presses & ~(1 << i);
         vals[i] = value[i];
       } else {
         touches = touches & ~(1 << i);
       }
       
       if (pressed[i]) {
        
         if (!hold[i]) {
           presses = presses | (1 << i); 
           touches = touches & ~(1 << i);
           vals[i] = value[i];
         } else { // if was pressed and hold, on release: 
           fireUnhold(i);
         }
         
         pinPressedMillis[i] = 0; 
         
      } else {
         presses = presses & ~(1 << i);
      } 
       
       touch[i] = false;
       pressed[i] = false;
       hold[i] = false;
       value[i] = 10000;
   }
}

void fireHold(int pin) 
{
   holds = holds | (1 << pin);  
}

void fireUnhold(int pin) 
{
   hold[pin] = false;
   holds = holds & ~(1 << pin);
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

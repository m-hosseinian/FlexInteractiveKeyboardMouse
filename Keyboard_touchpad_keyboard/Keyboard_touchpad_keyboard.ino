#include "mpr121.h"
#include "i2c.h"

double touchThreshold = 0.95;
double pressThreshold = 0.55; 

unsigned long newPressMillis = 0;
unsigned long lastPressMillis = 0;

boolean touch[12] = {false,false,false,false,false,false,false,false,false,false,false,false};
boolean pressed[12] = {false,false,false,false,false,false,false,false,false,false,false,false};
int temp=0;
int value[12] = {10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000, 10000};
int baseVals[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
int vals[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint16_t touches;
uint16_t presses = 0x0000;
uint16_t lastPresses = 0x0000;
uint16_t lastTouches = 0x0000;

/* Keyboard variables */
int pressWin[2] = {-1, -1};
int pressIndex = -1;

char keyboardMap[4][4] = {
  {',','0','.',';'}, 
  {'1','2','3','+'}, 
  {'4','5','6','-'}, 
  {'7','8','9','*'}
};

void setup()
{
  //configure serial out
  Serial.begin(115200);
  
  // initalize I2C bus. Wiring lib not used. 
  i2cInit();
  
  // initialize mpr121
  mpr121QuickConfig();
    
  Serial.println("Ready...");
  Keyboard.begin();
  delay(100);
  collectBaseVals();
}

void loop()
{  
    populateInteractions();
  
    /* Keyboard code */
    for (int i = 0; i < 8; i++) {
      if (presses & (1 << i)) {
        newPressMillis = millis();
        if (newPressMillis - lastPressMillis > 400) {
          resetKeyboardWin(); 
        }
        pressIndex = (pressIndex + 1) % 2;
        pressWin[pressIndex] = i;
        if (pressIndex == 1) {
          if(pressWin[1] > pressWin[0]){
            keyboardPrintChar(pressWin[0], pressWin[1] - 4);
          } else {
            keyboardPrintChar(pressWin[1], pressWin[0] - 4);
          }
        }
        lastPressMillis = newPressMillis;
     }
  }
}

/* Keyboard library */
void keyboardPrintChar(int row, int col) {
  if( row < 0 || col < 0 || row > 3)
  {
    return;
  }
  Serial.print(row);Serial.print(", ");Serial.println(col);
  Keyboard.print(keyboardMap[row][col]);
  resetKeyboardWin();
}

void resetKeyboardWin(){
  pressIndex = -1;
}


/* touch-press compbo library */
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
   for (int i=0; i<12; i++) {
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
            if(temp < baseVals[i] * touchThreshold)
            {
              continue;
            }
          }
      }
      
       if(touch[i]) {
         touches = touches | (1 << i); ////////
         presses = presses & ~(1 << i);
         vals[i] = value[i];
       } else {
         touches = touches & ~(1 << i);
       }
       if (pressed[i]){
         presses = presses | (1 << i); ////////
         touches = touches & ~(1 << i);
         vals[i] = value[i];
       } else {
         presses = presses & ~(1 << i);
       } 
       
       touch[i] = false;
       pressed[i] = false;
       value[i] = 10000;
   }
}

int getSensorMeasurement(byte sensorNumber)
{
  int value = mpr121Read2Bytes(0x04 + (sensorNumber << 1));  
  return value;
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

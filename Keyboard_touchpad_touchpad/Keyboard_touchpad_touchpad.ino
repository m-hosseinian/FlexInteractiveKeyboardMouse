#include "mpr121.h"
#include "i2c.h"

double touchThreshold = 0.95;
double pressThreshold = 0.55;

int mouseStep = 30;

int timeOut = 800;

int ROW_NUM = 4;
int COL_NUM = 3;

unsigned long lastTouchMilis = 0;

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

int mouseMap[4][4] = {
  {0,  1, 2, 3}, 
  {4,  5, 6, 7}, 
  {8,  9,10,11}, 
  {12,13,14,15}
};

int moveMap[16][16] = {
  /* 0 = Up, 1 = UpRight, 2 = Right, 3 = UpLeft
  /* 4 = Down, 5 = DownLeft 6 = Left , 7 = DownRight
       /*0,   1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15*/
  /*00*/{-1,  2,  2,  2,  0,  1, -1, -1,  0, -1,  1, -1,  0, -1, -1,  1}, 
  /*01*/{-1, -1,  2,  2,  3,  0,  1, -1, -1,  0, -1, -1, -1,  0, -1, -1}, 
  /*02*/{-1, -1, -1,  2, -1,  3,  0,  1,  3, -1,  0, -1, -1, -1,  0, -1}, 
  /*03*/{-1, -1, -1, -1, -1, -1,  3,  0,  3,  3, -1,  0, -1, -1, -1,  0},
  /*04*/{-1, -1, -1, -1, -1,  2,  2,  2,  0,  1, -1, -1,  0, -1,  1,  1},
  /*05*/{-1, -1, -1, -1, -1, -1,  2,  2,  3,  0,  1, -1, -1,  0, -1, -1}, 
  /*06*/{-1, -1, -1, -1, -1, -1, -1,  2,  3,  3,  0,  1,  3, -1,  0, -1}, 
  /*07*/{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  3,  0, -1,  3, -1,  0},
  /*08*/{-1, -1, -1, -1, -1, -1, -1, -1, -1,  2,  2,  2,  0,  1, -1, -1},
  /*09*/{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  2,  2,  3,  0,  1, -1}, 
  /*10*/{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  2, -1,  3,  0,  1},
  /*11*/{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  3,  0},
  /*12*/{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  2,  2,  2}, 
  /*13*/{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  2,  2}, 
  /*14*/{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  2}, 
  /*15*/{-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}
};

void setup()
{
  //configure serial out
  Serial.begin(115200);
  Mouse.begin();
  // initalize I2C bus. Wiring lib not used. 
  i2cInit();
  
  // initialize mpr121
  mpr121QuickConfig();
    
  Serial.println("Ready...");
  
  delay(100);
  collectBaseVals();
}

int win[2] = {-1, -1};
int moveWin[2] = {-1, -1};

int index = -1;
int indexMove = -1;

unsigned long newTouchMilis;

void loop()
{
  
  populateInteractions();
  
  /* mouse code */
  for (int i = 0; i < 8; i++) {
    if (touches & (1 << i)){
      index = (index + 1) % 2;
      win[index] = i;
      if (index == 1) {
        if(win[1] > win[0]){
          handleTouchKeys(win[0], win[1] - 4);
        } else {
          handleTouchKeys(win[1], win[0] - 4);
        }
      }
    }
  }
  
}

void handleTouchKeys(int row, int col)
{
    if(row < 0 || col < 0 || row == col + 4 || row >= ROW_NUM) {
      return;
    }
   processTouch(mouseMap[row][col]);
   
}

void resetmoveWin()
{
  indexMove = -1;
  moveWin[0] = -1; moveWin[1] = -1;
}

void processTouch(int input)
{
  newTouchMilis = millis();
  //Serial.println(newTouchMilis - lastTouchMilis);
  if (lastTouchMilis != 0 && (newTouchMilis - lastTouchMilis) > timeOut) {
     resetmoveWin();
     //Serial.print("/*");Serial.print(newTouchMilis - lastTouchMilis);Serial.println("*/");
     Serial.println("");
  }
  
  indexMove = (indexMove + 1) % 2;
  moveWin[indexMove] = input;
  Serial.print("w[");Serial.print(indexMove);Serial.print("]="); Serial.print(input);Serial.print(", ");
  if (indexMove == 1) {
    mouseMove(moveWin[0], moveWin[1]);
  }
  lastTouchMilis = newTouchMilis;
}

int movement;
unsigned long newMoveMillis;
unsigned long lastMoveMillis;

void mouseMove(int source, int dest) 
{
  if (source < 0 || dest < 0) {
    indexMove = -1;
    return; 
  }
  if (source == dest) {
    indexMove = 0;
    return; 
  }
  if (source < dest) {
    movement = moveMap[source][dest];
  } else {
    movement = moveMap[dest][source];
    if (movement > -1) {
      movement += 4; // reverse
    }
  }
  newMoveMillis = millis();
  
  switch(movement) {
    case 0://Up
      Serial.println("Up");
      Mouse.move(0, -1 * mouseStep, 0);
      break;
    case 1://Up Right
          Serial.println("Up Right");
          Mouse.move(50, -1 * mouseStep, 0);
      break;
    case 2://Right 
          Serial.println("Right");
          Mouse.move(mouseStep, 0, 0);
      break;
    case 3://Up Left
          Serial.println("Up Left");
          Mouse.move(-1 * mouseStep, -1 * mouseStep, 0);
      break;
    case 4://Down
          Serial.println("Down");
          Mouse.move(0, mouseStep, 0);
      break;
    case 5://Down Left
          Serial.println("Down Left");
          Mouse.move(-1 * mouseStep, mouseStep, 0);
      break;
    case 6://Left
         Serial.println("Left");
         Mouse.move(-1 * mouseStep, 0, 0);
      break;
    case 7://Down Right
          Serial.println("Down Right");
          Mouse.move(mouseStep, mouseStep, 0);
      break;
    default:
          resetmoveWin();
      return;
  }
  lastMoveMillis = newMoveMillis;
  shiftMoveWindow();
}

void shiftMoveWindow() {
  moveWin[0] = moveWin[1];
  moveWin[1] = -1;
  indexMove = 0;
}

void collectBaseVals()
{
  int maxBaseVals[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  
  for (int i=0; i<12; i++) {
    for (int j=0; j<20; j++) {
      maxBaseVals[i] = max(maxBaseVals[i], getSensorMeasurement(i));
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

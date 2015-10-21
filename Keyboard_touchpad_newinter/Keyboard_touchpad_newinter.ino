#include "mpr121.h"
#include "i2c.h"




#define isHold(i) (holds & (1 << i)) && !(lastHolds & (1 << i)) 
#define isUnhold(i) (lastHolds & (1 << i)) && !(holds & (1 << i))
#define holdTimeOut  ((millis() - lastHoldMillis) > timeOut)
#define mouseMoveTimeOut  ((millis() - lastMouseMove) > timeOut)

/* Library variables */
int ROW_NUM = 4;
int COL_NUM = 3;

double touchThreshold = 0.98;
double pressThreshold = 0.38;

boolean touch[12] = {false,false,false,false,false,false,false,false};
boolean pressed[12] = {false,false,false,false,false,false,false,false};
boolean hold[8] = {false,false,false,false,false,false,false,false};
boolean isHold[8] = {false,false,false,false,false,false,false,false};

unsigned long pinPressedMillis[8] = {0, 0, 0, 0, 0, 0, 0, 0};

int temp=0;
int baseVals[12] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
uint16_t touches = 0x0000;
uint16_t presses = 0x0000;
uint16_t holds = 0x0000;
uint16_t lastHolds = 0x0000;

/* General variables */
int timeOut = 800;

/* Keyboard variables */
int pressWin[2] = {-1, -1};
char eventWin[2] = {'\0', '\0'};
int pressIndex = -1;

unsigned long newPressMillis = 0;
unsigned long lastPressMillis = 0;

String keyboardMap[4][4] = {
    {"0", "1", "2", "3"}, 
    {"4", "5", "6", "7"}, 
    {"8", "9", "10", "11"}, 
    {"12", "13", "14", "15"}
};

/* Mouse variables */
int mouseStep = 30;
int touchWin[2] = {-1, -1};
int moveWin[2] = {-1, -1};

int touchIndex = -1;
int moveIndex = -1;

unsigned long newButtonTouchMilis;
unsigned long lastButtonTouchMilis = 0;

unsigned long newPinTouchMilis;
unsigned long lastPinTouchMilis = 0;

unsigned long lastMouseMove = 0;

int movement;

int mouseMap[4][4] = {
  {0,   1,  2,  3}, 
  {4,   5,  6,  7}, 
  {8,   9, 10, 11}, 
  {12, 13, 14, 15}
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

/* Extra interactions variables */
boolean keyboardClosed = false;
boolean lowerHalfHold = false;
boolean upperHalfHold = false;
boolean gHold = false;
unsigned long lastHoldMillis = 0;

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

void loop()
{
    if (!holdTimeOut || !mouseMoveTimeOut || gHold) {
        pressThreshold = 0.1;
    } else {
        pressThreshold = 0.38;
    }
    
    populateInteractions();

    for (int i = 0; i < 8; i++) {
        if(!keyboardClosed) {
            handleMouse(i);
            handleKeyboard(i);
        }
        handleHold(i);
    }
    
    if(!keyboardClosed) {
        checkLowerHalfHold();
        checkUpperHalfHold();
        checkLowerHalfReleased();
        checkUpperHalfReleased();
    }
  
    checkAnyHold();
    checkKeyboardClosed();
    checkKeyboardOpen();
    lastHolds = holds;
}

void handleMouse(int pin) {
    /* mouse code */
    if (touches & (1 << pin)){
        newPinTouchMilis = millis();
        if ((newPinTouchMilis - lastPinTouchMilis) > timeOut) {
            resetMouseWins();
        }
        touchIndex = (touchIndex + 1) % 2;
        touchWin[touchIndex] = pin;
        if (touchIndex == 1) {
            //Serial.println("mouse window full");
            resetKeyboardWin();
            if(touchWin[1] > touchWin[0]){
                processTouchedPins(touchWin[0], touchWin[1] - 4);
            } else {
                processTouchedPins(touchWin[1], touchWin[0] - 4);
            }
        }
        lastPinTouchMilis = newPinTouchMilis;
    }
}

void handleKeyboard(int pin) {
  
      /* Keyboard code */
      if (presses & (1 << pin) || touches & (1 << pin)) {
          newPressMillis = millis();
          if (newPressMillis - lastPressMillis > 400) {
              resetKeyboardWin();
          }
          pressIndex = (pressIndex + 1) % 2;
          
          /* instead of simply: pressWin[pressIndex] = i 
           * we populate a window that contains at least 
           * one press event: {p, p}, {p, t}, {t, p} are
           * valid windows
           */
          if(pressIndex == 0) {
              pressWin[pressIndex] = pin;
              if (presses & (1 << pin)) {
                  eventWin[pressIndex] = 'p';
              } else {
                  eventWin[pressIndex] = 't';
              }
          } else { // pressIndex == 1
              if (presses & (1 << pin)) {
                  pressWin[pressIndex] = pin;
                  eventWin[pressIndex] = 'p';
              } else { // touches & (1 << i)
                  if (eventWin[pressIndex - 1] == 'p') { // previous event is press
                      pressWin[pressIndex] = pin;
                      eventWin[pressIndex] = 't';
                  } else { // previous event is also touch: dicard everything
                      resetKeyboardWin();
                  }
              }
          }
          if (pressIndex == 1) {
              resetMouseWins();
              if(pressWin[1] > pressWin[0]){
                  keyboardPrintChar(pressWin[0], pressWin[1] - 4);
              } else {
                  keyboardPrintChar(pressWin[1], pressWin[0] - 4);
              }
          }
          lastPressMillis = newPressMillis; 
      }    
}

void handleHold(int pin) {
  
    /* Hold code */
    if (isHold(pin)) {
        isHold[pin] = true;
        
    }
    
    if (isUnhold(pin)) {
        isHold[pin] = false;
    }
}

void checkAnyHold()
{
    if(holds == 0) {
        if (gHold)
            lastHoldMillis = millis();
        gHold = false;
        
    } else {  
        gHold = true;
    }
}


/* Extra Interaction library */
void checkKeyboardClosed()
{
    if(isHold[0] && isHold[3] && !keyboardClosed) {
        Serial.println("Keyboard Closed.");
        keyboardClosed = true;
    }
}

void checkKeyboardOpen()
{
    if((!isHold[0] || !isHold[3]) && keyboardClosed) {
        Serial.println("Keyboard Open.");
        keyboardClosed = false;
    }
}

void checkLowerHalfHold()
{
    if(isHold[0] && isHold[1] && !lowerHalfHold) {
        Serial.println("Lower Half Hold.");
        lowerHalfHold = true;
    }
}
void checkUpperHalfHold()
{
    if(isHold[2] && isHold[3] && !upperHalfHold) {
        Serial.println("Upper Half Hold.");
        upperHalfHold = true;
    }
}
void checkLowerHalfReleased()
{
    if((!isHold[0] || !isHold[1]) && lowerHalfHold) {
        Serial.println("Lower Half Released.");
        lowerHalfHold = false;
    }
}
void checkUpperHalfReleased()
{
    if((!isHold[2] || !isHold[3]) && upperHalfHold) {
        Serial.println("Upper Half Released.");
        upperHalfHold = false;
    }
}

/* Mouse library */
void resetMouseWins()
{
    moveIndex = -1; touchIndex = -1;
    //Serial.println("mouse TO");
}


void processTouchedPins(int row, int col)
{
    if(row < 0 || col < 0 || row == col + 4 || row >= ROW_NUM) {
        return;
    }
   processTouchedKeys(mouseMap[row][col]);
   
}

void processTouchedKeys(int input)
{
    moveIndex = (moveIndex + 1) % 2;
    moveWin[moveIndex] = input;
    if (moveIndex == 1) {
        Serial.print("("); ;Serial.print(moveWin[0]);Serial.print(", ");
        Serial.print(moveWin[1]);Serial.print(")");
        moveMouse(moveWin[0], moveWin[1]);
    } 
}

void moveMouse(int source, int dest) 
{
    if (source == dest) {
        moveIndex = 0;
        return; 
    }
    if (source < dest) {
        movement = moveMap[source][dest];
    } else {
        movement = moveMap[dest][source];
        if (movement > -1) {
            movement += 4; // reverse
        } // else: error, it's caught in the following switch/case
    }
    
    
      switch(movement) {
        case 0://Up
            if(gHold == true || !holdTimeOut){
                Serial.println(" => Multitouch Up. ");
                Mouse.move(0, 0, mouseStep);
                lastHoldMillis = millis();
            }
            else{
                Serial.println(" => Up. ");
                Mouse.move(0, -1 * mouseStep, 0);
                lastMouseMove = millis();
            }
        break;
        case 1://Up Right
            if(gHold == true || !holdTimeOut){
                Serial.println(" => Multitouch Up Right. ");
                lastHoldMillis = millis();
            }
            else{
                Serial.println(" => Up Right. ");
                Mouse.move(mouseStep, -1 * mouseStep, 0);
                lastMouseMove = millis();
            }
        break;
        case 2://Right 
            if(gHold == true || !holdTimeOut){
                Serial.println(" => Multitouch Right. ");
                Keyboard.print("=");
                lastHoldMillis = millis();
            }
            else{
                Serial.println(" => Right. ");
                Mouse.move(mouseStep, 0, 0);
                lastMouseMove = millis();
            }
        break;
        case 3://Up Left
            if(gHold == true || !holdTimeOut){
                Serial.println(" => Multitouch Up Left. ");
                lastHoldMillis = millis();
            }
            else{
                Serial.println(" => Up Left. ");
                Mouse.move(-1 * mouseStep, -1 * mouseStep, 0);
                lastMouseMove = millis();
            }
        break;
        case 4://Down
            if(gHold == true || !holdTimeOut){
                Serial.println(" => Multitouch Down. ");
                Mouse.move(0, 0, -1 * mouseStep);
                lastHoldMillis = millis();
            }
            else{
                Serial.println(" => Down. ");
                Mouse.move(0, mouseStep, 0);
                lastMouseMove = millis();
            }
        break;
        case 5://Down Left
            if(gHold == true || !holdTimeOut){
                Serial.println(" => Multitouch Down Left. ");
                lastHoldMillis = millis();
            }
            else{
                Serial.println(" => Down Left. ");
                Mouse.move(-1 * mouseStep, mouseStep, 0);
                lastMouseMove = millis();
            }
        break;
        case 6://Left
            if(gHold == true || !holdTimeOut){
                Serial.println(" => Multitouch Left. ");
                Keyboard.print("-");
                lastHoldMillis = millis();
            }
            else{
                Serial.println(" => Left. ");
                Mouse.move(-1 * mouseStep, 0, 0);
                lastMouseMove = millis();
            }
        break;
        case 7://Down Right
            if(gHold == true || !holdTimeOut){
                Serial.println(" => Multitouch Down Right. ");
                lastHoldMillis = millis();
            }
            else{
                Serial.println(" => Down Right. ");
                Mouse.move(mouseStep, mouseStep, 0);
                lastMouseMove = millis();
            }
        break;
        default:
            resetMouseWins();
        return;
    }
    shiftMoveWindow();
}

void shiftMoveWindow() {
    moveWin[0] = moveWin[1];
    moveWin[1] = -1;
    moveIndex = 0;
}

/* Keyboard library */
void keyboardPrintChar(int row, int col) {
    if( row < 0 || col < 0 || row > 3)
    {
        return;
    }
    Serial.print("(");Serial.print(row);Serial.print(", ");Serial.print(col);
    Serial.print(") => ");Serial.println(keyboardMap[row][col]);

    Keyboard.print(keyboardMap[row][col]);
    resetKeyboardWin();
}

void resetKeyboardWin(){
    pressIndex = -1;
    //Serial.println("keyboard TO");
}

/* touch-press compbo library */
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
    for (int i=0; i<8; i++) {
    temp = getSensorMeasurement(i);
        if (temp <= (baseVals[i] * touchThreshold)) { 
        
            if(!pressed[i]) {
                touch[i] = true;
            }
            
            if(temp <= (baseVals[i] * touchThreshold) && 
                    temp >= (baseVals[i] * pressThreshold))
            {
                continue;
            }
            
            if (temp <= (baseVals[i] * pressThreshold)) {
                touch[i] = false;
                pressed[i] = true;
              
                if (pinPressedMillis[i] == 0) {
                    pinPressedMillis[i] = millis();
                }
              
                if (!hold[i] && ((millis() - pinPressedMillis[i]) > (timeOut / 3))) { // pin is hold
                    hold[i] = true;
                    fireHold(i);
                }
              
                continue;
            }
        }
        
        if(touch[i]) {
               touches = touches | (1 << i); 
               presses = presses & ~(1 << i);
        } else {
               touches = touches & ~(1 << i);
        }
        
        if (pressed[i]) {
            if (!hold[i]) {
                presses = presses | (1 << i); 
            } else { // it was pressed and hold, on release: 
                fireUnhold(i);
            }
            
            pinPressedMillis[i] = 0; 
           
        } else {
            presses = presses & ~(1 << i);
        } 
           
        touch[i] = false;
        pressed[i] = false;
        hold[i] = false;
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

/* I2C raw data read */
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

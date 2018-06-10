/* --------------------------------------------------------------------
 *  StepperBotLidar2
 * --------------------------------------------------------------------
 * Bluepill board STM32F103C8T6
 * 
 * RX1 Pin PA10 (31), 5V tolerant -> TX TF Mini
 * TX1 Pin PA9 (30), 5V tolerant -> RX TF Mini
 * RX3 Pin B11 (22) -> TX5 Beaglebone
 * TX3 Pin B10 (21) -> RX5 Beaglebone
 * Stepper DIR -> PA1
 * Stepper STEP -> PA2
 * Stepper SLEEP -> PA3 Permanently HIGH! Stepper motor needs holding force against slip ring!
 *
 * Stepper motor 200 steps/rev
 * belt drive 30 teeth to 54 teeth
 * => 360 motor steps = turntable 1 revolution
 * LIDAR has to turn counter-clockwise
 */

//--------------------------------------------------------------------------------
// ToF Sensor
//--------------------------------------------------------------------------------
#include "TFMini.h"
#define pinOn  PA4
#define pinOff PA5
TFMini tfmini;
uint16_t distance;
uint16_t strength;
int angle;

//--------------------------------------------------------------------------------
// Stepper
//--------------------------------------------------------------------------------
#include <Arduino.h>
#include "BasicStepperDriver.h"

// Motor steps per revolution. Most steppers are 200 steps or 1.8 degrees/step
#define MOTOR_STEPS 200

// All the wires needed for full functionality
#define DIR PA1
#define STEP PA2
#define ENABLE PA3  // sleep is a keyword ;o)

// Since microstepping is set externally, make sure this matches the selected mode
// 1=full step, 2=half step etc.
#define MICROSTEPS 1

// 2-wire basic config, microstepping is hardwired on the driver
BasicStepperDriver stepper(MOTOR_STEPS, DIR, STEP);

//--------------------------------------------------------------------------------
// command interface
//--------------------------------------------------------------------------------
const char del = '#';   // delimiter
String cmd  = "stop";	// dummy command
String scan = "scan"; 	// start 360 degree scan
String info = "info"; 	// show version info
String ID   = "StepperBotLidar2.ino V2.0";
// auto switch between USB Serial und Serial
boolean USB = true;
int countdown = 2000;

//--------------------------------------------------------------------------------
void setup() {

  // Set target motor RPM
  stepper.setRPM(60);

  // stepper sleep pin
  pinMode(ENABLE, OUTPUT);
  digitalWrite(ENABLE,HIGH);

  // Bluepill check serial connection
  Serial.begin(115200);
  while (!Serial)
    { 
      delay(5);
      countdown--;
      if (countdown == 0)
        {
          USB = false;
          break; 
        }
    }
  if (!USB)
    {
      Serial3.begin(57600);
      while (!Serial3)
        { delay(1); }
    }

  // TF Mini: Initialize the data rate for the Serial port
  Serial1.begin(TFMINI_BAUDRATE);
  pinMode(pinOn, OUTPUT);
  pinMode(pinOff, OUTPUT);

  // Initialize the TF Mini sensor
  tfmini.begin(&Serial1);
  delay(1000);    
}

//--------------------------------------------------------------------------------
void loop() {

  //----------------------------------------------------------------
  // scanning
  //----------------------------------------------------------------
  if (cmd.equals(scan))
  {
    // power on LIDAR
    powerLidar(true);
    delay(100);
    tfmini.setSingleScanMode();

    // workaround for first measurement running wild
    // result is ignored
    tfmini.externalTrigger();
    distance = tfmini.getDistance();
    strength = tfmini.getRecentSignalStrength();
    
    for (angle = 0; angle < 360; angle = angle + 1)
    {
      // Take single measurement 
      tfmini.externalTrigger();
      distance = tfmini.getDistance();
      strength = tfmini.getRecentSignalStrength();
      
      // Display the measurement
      if (USB)
      {
        Serial.print(angle);
        Serial.print(",");
        Serial.print(distance);
        Serial.print(",");
        Serial.println(strength);
      }
      else
      {
        Serial3.print(angle);
        Serial3.print(",");
        Serial3.print(distance);      
        Serial3.print(",");
        Serial3.println(strength);      
      }
      // move turntable 1 degree counter-clockwise
      stepper.move(1*MICROSTEPS);
    }
    cmd = "stop";
    // power off LIDAR
    powerLidar(false);
  }

  //----------------------------------------------------------------
  // version info 
  //----------------------------------------------------------------
  else if (cmd.equals(info))
  {
     if (USB)
     {
       Serial.println(ID);
       Serial.println("Interface: Serial(USB)");
     }
     else
     {
       Serial3.println(ID);
       Serial3.println("Interface: Serial3");
     }
     cmd = "stop"; 
  }
  
  //----------------------------------------------------------------
  // wait for command 
  //----------------------------------------------------------------
  else
  { 
    delay(10);
    if (USB)
      { cmd = Serial.readStringUntil(del); }
    else
      { cmd = Serial3.readStringUntil(del); }
    cmd = cmd.substring(0,4);
  }
}

//--------------------------------------------------------------------------------
void powerLidar(boolean trigger) {
  if (trigger) {
    digitalWrite(pinOn, HIGH);
    delay(10);
    digitalWrite(pinOn, LOW);
  }
  else {
    digitalWrite(pinOff, HIGH);
    delay(10);
    digitalWrite(pinOff, LOW);    
  }  
}


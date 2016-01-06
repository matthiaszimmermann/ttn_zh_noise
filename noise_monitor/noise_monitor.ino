#include "LoRa.h"

// node info
#define NODE_INFO "'info': 'ttn-zh-noise'"

// version string
#define VERSION "'version': 'v0.1'"

// true: print debug info on serial
// false: don't print debug info to serial
#define PRINT_DEBUG false

// analog input noise pin
#define PIN_NOISE A0

// size of inner loop to get min max levels
#define CNT_MAX 2000

// number of loops to acuumulate noise sampling
#define CNT_ACC 350

// gobal vars for sound level
int cnt, val_min, val_max, diff;

// global vars for accumulated noise measures
int acc_cnt, acc_sum, acc_max;
long acc_start;

// the lora lib
LoRa LoraLib;

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  
  initLora();  
  initSensor();
  resetSensorValues();
  resetNoiseLevelValues();
}

void loop() {  
  // update noise level vars from sensor value
  updateSensorValues(analogRead(PIN_NOISE));
  
  // update/print current noise level
  if(cnt == CNT_MAX) {
    diff = smoothDiff(val_max - val_min);
    printDiff(diff);
    updateNoiseLevelValues(diff);
    resetSensorValues();

    // print/send noise data for last time window
    if(acc_cnt == CNT_ACC) {
      printNoiseLevel();
      sendLora();
      resetNoiseLevelValues();
    }
  }
}

// configure lora module
void initLora() {
  // serial to communicate with lora chip
  Serial1.begin(57600); 
  delay(3000);

  if(PRINT_DEBUG) {
    Serial.println("--------------------------");
    Serial.println("Configuring LoRa module...");
  }
  
  LoraLib.LoRaConfig();
  
  if(PRINT_DEBUG) {
    Serial.println("LoRa module ready.");
    Serial.println("--------------------------");
  }
}

void sendLora() {
  LoraLib.LoRaSendAndReceive(getJson());
}

// assemble data package for lora
String getJson() {
  String s = String(NODE_INFO) + ", ";
  s += String(VERSION) + ", ";
  s += "'acc': '" + String(getNoiseLevelNormalized()) + "', ";
  s += "'max': '" + String(acc_max) + "'";
  return s;
}

// get normalizes accumulated noise
double getNoiseLevelNormalized() {
  return acc_sum / ((millis() - acc_start) / 1000.0);
}

// set reference to internal to increase sensitivity
void initSensor() {
  analogReference(INTERNAL);
}

void resetSensorValues() {
  cnt = 0;
  val_min = 2000;
  val_max = 0;
  diff = 0;
}

void resetNoiseLevelValues() {
  acc_cnt = 0;
  acc_sum = 0;
  acc_max = 0;
}

void updateSensorValues(int val) {
  if (val < val_min) { val_min = val; }
  if (val > val_max) { val_max = val; }
  
  cnt++;
}

void updateNoiseLevelValues(int d) {
  if(acc_cnt == 0) {
    acc_start = millis();
  }
  
  if (diff > acc_max) { 
    acc_max = d; 
  }

  acc_sum += d;
  acc_cnt++;
}

// diff values of 2 and below don't seem to be meaningful
int smoothDiff(int d) {
  if(d >= 2) {
    return d - 2;
  }
  
  if(d >= 1) {
    return d - 1;
  }

  return d;
}

// print current noise level
void printDiff(int d) {
  if(PRINT_DEBUG) {
    Serial.print(getDiffString(d));   
    Serial.print(" ");   
    Serial.print(d);   
    Serial.print(" (min ");   
    Serial.print(val_min);   
    Serial.print(" max ");   
    Serial.print(val_max);   
    Serial.println(")");
  }
}

// converts diff value into a string for printing on serial
String getDiffString(int d) {
  int noise = d / 2;
  int pos = 1;
  String bar;

  if(diff == 0) { bar = "."; }
  else          { bar = "#"; }
  
  for(;pos < 40; pos++) {    
    if(pos <= noise) { bar += "#"; }
    else             { bar += " "; }
  }

  return bar;
}

// print accumulated noise info
void printNoiseLevel() {
  if(PRINT_DEBUG) {
    Serial.print("--- noise ");   
    Serial.print(getNoiseLevelNormalized());
    Serial.print(" (sum ");   
    Serial.print(acc_sum);   
    Serial.print(" max ");   
    Serial.print(acc_max);   
    Serial.print(") json ");
    Serial.println(getJson());
  }
}


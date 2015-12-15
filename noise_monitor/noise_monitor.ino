#include "LoRa.h"

// node info
#define NODE_INFO "'info': 'ttn-zh-noise'"

// version string
#define VERSION "'version': 'v0.1'"

// microphone input pin
#define PIN_AUDIO A0

// number of loops for inner time window (measuring min/max mic levels)
#define CNT_MAX_INNER 2000

// number of loops fo outer time window (accumulated mic level differences ~= noise)
#define CNT_MAX_OUTER 30

// measurements for inner time frame
// min and max volt levels
int val_min = 2000;
int val_max = 0;
int cnt_inner = 0;

// measurements for outer time frame
// accumulated differences/max/min over inner time frame
// accumulated difference seems to correspond nicely to accumulated noise
int diff_sum = 0;
int diff_min = 100;
int diff_max = 0;
int cnt_outer = 0;

// accumulated and normalized noise
// variables to measure time window for normalization
double noise_acc_norm;
unsigned long time_since;
unsigned long time_now;
unsigned long time_delta;

LoRa LoRa_Lib;

void setup() {
  Serial.begin(9600);   // serial monitor
  Serial1.begin(57600); // lora communication serial
  
  initLora();
  initMeasuredData();
}

void loop() {  
  updateInnerValues(analogRead(PIN_AUDIO)); // read the input pin (dc pin of spw2430)

  // check if inner time frame is completed
  if(cnt_inner == CNT_MAX_INNER) {
    updateOuterValues(val_min, val_max);

    // check if outer time frame is completed
    if(cnt_outer == CNT_MAX_OUTER) {
      updateMeasuredData();
      printDebugInfo();
      sendLoRaData();
      
      resetOuterValues();
    }
    
    resetInnerValues();
  }
}

void initLora() {
  delay(3000);
  Serial.println("--------------------------");
  Serial.println("Configuring LoRa module...");
  LoRa_Lib.LoRaConfig();
  Serial.println("LoRa module ready.");
  Serial.println("--------------------------");
}

void initMeasuredData() {
  time_since = millis();
  time_now = 0;
  time_delta = 0;
  noise_acc_norm = 0.0;
}

void updateMeasuredData() {
  time_now = millis();
  time_delta = time_now - time_since;
  noise_acc_norm = 1000.0 * diff_sum / time_delta;
}

void resetInnerValues() {
  cnt_inner = 0;
  val_min = 2000;
  val_max = 0;
}

void updateInnerValues(int val) {
  if (val < val_min) { val_min = val; }
  if (val > val_max) { val_max = val; }
  
  cnt_inner++;
}

void resetOuterValues() {
  cnt_outer = 0;
  diff_sum = 0;
  diff_min = 100;
  diff_max = 0;

  // reset after sending data via lora
  time_since = time_now;  
}

void updateOuterValues(int vmin, int vmax) {
  int diff = vmax - vmin;

  diff_sum += diff;
  if(diff < diff_min) { diff_min = diff; }
  if(diff > diff_max) { diff_max = diff; }
  
  cnt_outer++;
}
  
void sendLoRaData() {
  LoRa_Lib.LoRaSendAndReceive(getJsonData());
}

void printDebugInfo() {
  Serial.println("--------------------------");
  Serial.print("sum: ");
  Serial.print(diff_sum);
  Serial.print(" time_delta: ");
  Serial.print(time_delta);
  Serial.print(" noise_acc_norm: ");
  Serial.print(noise_acc_norm);
  Serial.print(" min: ");
  Serial.print(diff_min);
  Serial.print(" max: ");
  Serial.print(diff_max);
  Serial.print(" jsonData: ");
  Serial.print(getJsonData());
  Serial.println();
  Serial.println("--------------------------");
}

String getJsonData() {
  String s = String(NODE_INFO) + ", ";
  s += String(VERSION) + ", ";
  s += "'acc': '" + String(noise_acc_norm) + "', ";
  s += "'max': '" + String(diff_max) + "'";
  return s;
}


#include "LoRa.h"

// LORA_ENABLE true: connect & send via lora
// LORA_ENABLE false: switch lora communication off
#define LORA_ENABLE true

// PRINT_DEBUG true: print debug info on serial
// PRINT_DEBUG false: don't print debug info to serial
#define PRINT_DEBUG true

// number of bytes for a lora message
#define LORA_MESSAGE_SIZE 12

// analog input noise pin
#define PIN_NOISE A0

// size of inner loop to get min max levels
#define CNT_MAX 2000

// number of samples used to calibrate min and max
#define CALIBRATION_SAMPLES 30

// size of diff histo for calibration
#define CALIBRATION_HISTO_SIZE 10

#define LINE "--------------------------"

// lora end-device address (DevAddr)
// for ttn zurich, please use the address space 5A4801xx
// you need to set the devaddr in file LoRa.cpp
// use an individual address for each device when using the ttn prototype infrasructure
// "mac set devaddr 0x5A48012E\r\n"

// network key and application key are also set in file LoRa.cpp
// "mac set nwkskey 2B7E151628AED2A6ABF7158809CF4F3C\r\n"
// "mac set appskey 2B7E151628AED2A6ABF7158809CF4F3C\r\n"

// number of seconds to accumulate until tranmitting data
// transmissions start every 20 seconds. over time the interval between transmissions is increasing to 120 seconds
static const float SAMPLE_INTERVAL[] = {20.0, 20.0, 20.0, 30.0, 30.0, 30.0, 30.0, 60.0, 60.0, 120.0};
static const int SAMPLE_INTERVAL_MAX_INDEX = 9;

// the lora lib
LoRa LoraLib;

// gobal vars for sound level
int cnt, val_min, val_max, diff;

// global vars for accumulated noise measures
int acc_max, acc_sum, acc_cnt, samples_cnt, calibration_cnt;
long acc_start;

// global vars for calibration
int calibration_diff[CALIBRATION_SAMPLES];
int calibration_histo[CALIBRATION_HISTO_SIZE];

// minimum difference for noise reading (updated during calibration)
int diff_min;

// variable to check if node is in transmission mode
boolean nodeIsTransmitting = false;
boolean dataReadyToSend = false;

// char buffer for lora payload data
uint8_t loraData[] = "Hello, world!";

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);

  if (PRINT_DEBUG) {
    Serial.println(LINE);
    Serial.println("Starting setup ...");
  }
  
  initLora();  
  initSensor();
  resetRawSensorValues();
  resetNoiseLevelValues();

  if (PRINT_DEBUG) {
    Serial.println(LINE);
  }
}

void loop() {  
    updateRawSensorValues(analogRead(PIN_NOISE));
    
    // update/print current noise level
    if (cnt == CNT_MAX) {
      updateNoiseLevelCalibration();
      diff = val_max - val_min < diff_min? 0: val_max - val_min - diff_min;
      printDiff(diff, acc_cnt);
      updateNoiseLevelValues(diff);
      resetRawSensorValues();
    }

    if(sampleTime() >= SAMPLE_INTERVAL[samples_cnt]) {
      sendLora();
    }
}

// configure lora module
void initLora() {
  if (!LORA_ENABLE) {
    if (PRINT_DEBUG) {
      Serial.println("WARNING: LoRA disabled, not connecting");
    }

    return;
  }

  if (PRINT_DEBUG) {
    Serial.println("Configuring LoRa module...");
  }
  
  // serial to communicate with lora chip
  Serial1.begin(57600); 
  delay(3000);
  
  LoraLib.LoRaConfig();
  
  if(PRINT_DEBUG) {
    Serial.println("LoRa module ready.");
  }
}

void sendLora() {
  printNoiseLevel();
  prepareDataForTransmission();
  resetNoiseLevelValues();

  if (PRINT_DEBUG) {
    Serial.println(LINE);
    if (!LORA_ENABLE) {
      Serial.println("WARNING: Not sending data any data, LoRA is disabled");
    }
  }

  if (LORA_ENABLE && dataReadyToSend) {
    if (PRINT_DEBUG) {
      Serial.println("Sending data via LoRA");
      Serial.println("aaaabbbbcccc (int values encoded in hex format: a=max, b=sum, c=cnt)");
      Serial.println((char*)loraData);
      Serial.println(LINE);
    }
    
    nodeIsTransmitting = true;
    LoraLib.LoRaSendAndReceive((char*)loraData);
    nodeIsTransmitting = false;
  }

  // update sample counting
  samples_cnt++;

  if (samples_cnt >= SAMPLE_INTERVAL_MAX_INDEX) {
    samples_cnt = SAMPLE_INTERVAL_MAX_INDEX;
  }
  
}

// encode acc_max, acc_sum, acc_cnt to loraData array in hex format
// loraData then contains the 0 terminated string aaaabbbbcccc (int values encoded in hex format: a=max, b=sum, c=cnt)
void prepareDataForTransmission() {
  byte high, low;
  high = highByte(acc_max);
  low = lowByte(acc_max);
  loraData[0] = highNibble(high);
  loraData[1] = lowNibble(high);
  loraData[2] = highNibble(low);
  loraData[3] = lowNibble(low);

  high = highByte(acc_sum);
  low = lowByte(acc_sum);
  loraData[4] = highNibble(high);
  loraData[5] = lowNibble(high);
  loraData[6] = highNibble(low);
  loraData[7] = lowNibble(low);

  high = highByte(acc_cnt);
  low = lowByte(acc_cnt);
  loraData[8] = highNibble(high);
  loraData[9] = lowNibble(high);
  loraData[10] = highNibble(low);
  loraData[11] = lowNibble(low);

  loraData[12] = 0;
}

byte highNibble(byte value) {
  byte b = value >> 4;
  return b > 9 ? b + 0x37 : b + 0x30;
}

byte lowNibble(byte value) {
  byte b = value & 0xF;
  return b > 9 ? b + 0x37 : b + 0x30;
}

void initSensor() {
  // set reference to internal to increase sensor reading sensitivity
  analogReference(INTERNAL);

  // set pin for microphone
  pinMode(PIN_NOISE, INPUT);

  // init min noise reading differences
  diff_min = 0;

  // reset number of samples sent
  samples_cnt = 0;

  // reset calibration sample count
  calibration_cnt = 0;
  
  for (int i = 0; i < CALIBRATION_HISTO_SIZE; i++) {
    calibration_histo[i] = 0;
  }
}

// reset variables for raw noise measurments
void resetRawSensorValues() {
  cnt = 0;
  val_min = 2000;
  val_max = 0;
  diff = 0;
}

// reset variables for accumulated noise measurments
void resetNoiseLevelValues() {
  acc_cnt = 0;
  acc_sum = 0;
  acc_max = 0;
  acc_start = millis();
}

// update variables for raw noise measurments
void updateRawSensorValues(int val) {
  if (val < val_min) {
    val_min = val;
  }
  if (val > val_max) {
    val_max = val;
  }

  cnt++;
}

// update calibration of noise level differences
// assumtion is that most frequently observed difference 
// between max and min value represents silence.
void updateNoiseLevelCalibration() {

  // continuous sampling
  calibration_diff[calibration_cnt++ % CALIBRATION_SAMPLES] = val_max - val_min;
  
  if(calibration_cnt == CALIBRATION_SAMPLES) {
    calibration_cnt = 0;
  }
  
  // update histogram for calibration
  for (int i = 0; i < CALIBRATION_SAMPLES; i++) {
    if (calibration_diff[i] < CALIBRATION_HISTO_SIZE) {
      calibration_histo[calibration_diff[i]]++;
    }
  }

  // reset diff_min
  diff_min = 0;

  // find best diff_min.
  // assumpion: most frequent noise diff value represents silence
  for (int i = 1; i < CALIBRATION_HISTO_SIZE; i++) {
    if (calibration_histo[i] > calibration_histo[diff_min]) {
      diff_min = i;
    }
  }
}

// update variables for accumulated noise measurments
void updateNoiseLevelValues(int d) {
  if (diff > acc_max) {
    acc_max = d;
  }

  acc_sum += d;
  acc_cnt++;

  dataReadyToSend = true;
}

// returns the accumulated noise normalized by sampling time
double getNoiseLevelNormalized() {
  return acc_sum / sampleTime();
}

// returns the time (seconds) since sampling started
double sampleTime() {
  return (millis() - acc_start) / 1000.0;
}

// print current noise level
void printDiff(int d, int cnt) {
  if (PRINT_DEBUG) {
    Serial.print(getDiffString(d));
    Serial.print(" ");
    Serial.print(d);
    Serial.print(" (min ");
    Serial.print(val_min);
    Serial.print(" max ");
    Serial.print(val_max);
    Serial.print(" diff_min ");
    Serial.print(diff_min);
    Serial.print(") cnt=");
    Serial.print(cnt);
    Serial.print(" time=");
    Serial.println(sampleTime());
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
    Serial.println(LINE);
    Serial.print("Noise ");
    Serial.print(getNoiseLevelNormalized());
    Serial.print(" (max ");
    Serial.print(acc_max);
    Serial.print(" sum ");
    Serial.print(acc_sum);
    Serial.print(" cnt ");
    Serial.print(acc_cnt);
    Serial.print(")");
    Serial.println();
  }
}


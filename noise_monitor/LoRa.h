#ifndef LoRa_h
#define LoRa_h

#include <avr/pgmspace.h>

#include <Arduino.h>

class LoRa
{
  private:
// interfered with a0 analogread for noise level measurements
//    int red = A3; //this sets the red led pin
//    int green = A2; //this sets the green led pin
//    int blue = A0; //this sets the blue led pin
    int led = 13;
    
    void LoRaBlink()
    {
//      analogWrite(red, 0);
//      analogWrite(blue, 255);
//      analogWrite(green, 255);
      digitalWrite(led, HIGH);
      delay(50);
//      analogWrite(red, 255);
//      analogWrite(blue, 255);
//      analogWrite(green, 255);
      digitalWrite(led, LOW);
      delay(50);
    }

    void LoRaBlinkOff()
    {
//      analogWrite(red, 255);
//      analogWrite(blue, 0);
//      analogWrite(green, 0);
      digitalWrite(led, LOW);
    }

      

  public:
    void LoRaConfig();
    void LoRaSendAndReceive(String message);
};
#endif

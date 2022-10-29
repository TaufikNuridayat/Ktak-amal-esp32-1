#include <Arduino.h>
#include "function.h"

void setup()
{
  Serial.begin(115200);
  setting_up();
  ss.begin(9600, SERIAL_8N1, 16, 17);
  vibra.init(INPUT);
  door_pin.init(INPUT_PULLUP);
  Selenoid.init(OUTPUT);

  Selenoid.write(LOW);
  Serial.println("HI");
  delay(5000);
  // Buzz.write(HIGH);
  Buzz.init(OUTPUT);
  Buzz.write(LOW);
  Selenoid.write(HIGH);
}

void loop()
{
  gps_run();
  getFingerprintID();
  logic();
  static unsigned long blynk_run = millis();
  
  if (millis() - blynk_run > 5000)
  {
    runn_blynk();
    count_tampilan++;
    if(count_tampilan > 1) count_tampilan = 0;
    blynk_run = millis();
  }
  distance =
      TinyGPSPlus::distanceBetween(
          lat,
          lng,
          m_lat,
          m_lng);
  delay(50);
}

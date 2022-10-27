#include <Arduino.h>
// #include <SoftwareSerial.h>
#include <ArduinoJson.h>

// UTILITY
bool blynk_secure_state = false;
bool flag_door = 0;
bool buzz_confirm = false;
bool buzz_wrong = false;
bool door_state = false;
bool door_secure_flag = false;
bool terbuka = !LOW;
bool tertutup = !HIGH;
bool vibration = false;
bool door = false;
int satelite = 0;
double distance;
uint8_t count_tampilan = 0;
float lat, lng, m_lat, m_lng;
String gpsInfo = "";

#include <TinyGPSPlus.h>
TinyGPSPlus gps;
HardwareSerial ss(2);

// Blynk
#define BLYNK_PRINT Serial
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

char auth[]   = "X3KvZwuw7Icufiky_qXRS6x2eFx7unw2";
char ssid[]   = "T. N. H";
char pass[]   = "diadanaku";
char server[] = "iot.serangkota.go.id";

WidgetMap myMap(V0);
WidgetLED led(V3);
WidgetLCD lcd_blynk(V4);

BLYNK_WRITE(V5)
{
  GpsParam mobile(param);
  m_lat = mobile.getLat();
  m_lng = mobile.getLon();
}

void gps_run()
{
  while (ss.available() > 0)
    gps.encode(ss.read());
  if (gps.location.isValid())
  {
    lng = gps.location.lng();
    lat = gps.location.lat();
    satelite = gps.satellites.value();
    gpsInfo = true;
  }
  else if (gps.location.isUpdated())
  {
    gpsInfo = false;
  }
}

// Pins
class PIN
{
public:
  PIN(uint8_t pin)
  {
    init_pin = pin;
  }
  void init(uint8_t mode)
  {
    pinMode(init_pin, mode);
  }

  bool read()
  {
    return digitalRead(init_pin);
  }
  void write(bool state)
  {
    digitalWrite(init_pin, state);
  }

private:
  int init_pin;
};

PIN Buzz(27);
PIN Selenoid(14);
PIN vibra(12);
PIN door_pin(13);

// FINGERPRINT
#include <Adafruit_Fingerprint.h>
HardwareSerial mySerial(1);
Adafruit_Fingerprint finger(&mySerial);

void setting_up()
{
  Blynk.begin(auth, ssid, pass, server, 8080);
  Blynk.run();
  lcd_blynk.print(0, 0, "Init sensor");
  lcd_blynk.print(0, 1, "Fingerprint");

  // Finger print
  mySerial.begin(57600, SERIAL_8N1, 4, 2);

  delay(5);
  if (finger.verifyPassword())
  {
    lcd_blynk.clear();
    lcd_blynk.print(0, 0, "Sensor finger");
    lcd_blynk.print(0, 1, "Terdeteksi");
    Serial.println("Found fingerprint sensor!");
  }
  else
  {
    lcd_blynk.clear();
    lcd_blynk.print(0, 0, "Sensor tidak");
    lcd_blynk.print(0, 1, "Terdeteksi");
    Serial.println("Did not find fingerprint sensor :(");
    while (1)
    {
      delay(1);
    }
  }

  Serial.println(F("Reading sensor parameters"));
  finger.getParameters();
  Serial.print(F("Status: 0x"));
  Serial.println(finger.status_reg, HEX);
  Serial.print(F("Sys ID: 0x"));
  Serial.println(finger.system_id, HEX);
  Serial.print(F("Capacity: "));
  Serial.println(finger.capacity);
  Serial.print(F("Security level: "));
  Serial.println(finger.security_level);
  Serial.print(F("Device address: "));
  Serial.println(finger.device_addr, HEX);
  Serial.print(F("Packet len: "));
  Serial.println(finger.packet_len);
  Serial.print(F("Baud rate: "));
  Serial.println(finger.baud_rate);

  finger.getTemplateCount();

  if (finger.templateCount == 0)
  {
    Serial.print("Sensor doesn't contain any fingerprint data. Please run the 'enroll' example.");
  }
  else
  {
    Serial.println("Waiting for valid finger...");
    Serial.print("Sensor contains ");
    Serial.print(finger.templateCount);
    Serial.println(" templates");
  }
  delay(1000);
}

void logic()
{
  door = door_pin.read();
  vibration = !vibra.read();

  if (buzz_confirm == true)
  {
    door_state = true;
    Buzz.write(HIGH);
    delay(1000);
    Buzz.write(LOW);
    buzz_confirm = false;
  }

  if (buzz_wrong == true)
  {
    Buzz.write(HIGH);
    delay(250);
    Buzz.write(LOW);
    delay(250);
    Buzz.write(HIGH);
    delay(250);
    Buzz.write(LOW);
    delay(250);
    buzz_wrong = false;
  }

  if (door_secure_flag)
  {
    if (door == tertutup)
    {
      Selenoid.write(LOW);
      flag_door = 1;
      Serial.println("Pintu terbuka");
      led.on();
    }
  }
  else
  {
    if (door == tertutup)
    {
      Serial.println("Pintu tertutup");
      Selenoid.write(HIGH);
      flag_door = 0;
      led.off();
    }
  }

  if (door == terbuka)
  {
    door_secure_flag = false;
  }

  if (door == terbuka && !door_secure_flag && flag_door == 0)
  {
    Blynk.notify("Kotak amal terbuka dengan paksa");
    Serial.println("pintu terbuka paksa");
  }

  if (vibration)
  {
    Blynk.notify("Ada getaran terdeteksi");
    Serial.println("Ada getaran terdeteksi");
  }
}

uint8_t getFingerprintID()
{
  uint8_t p = finger.getImage();
  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.println("Image taken");
    break;
  case FINGERPRINT_NOFINGER:
    // Serial.println("No finger detected");
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return p;
  case FINGERPRINT_IMAGEFAIL:
    Serial.println("Imaging error");
    return p;
  default:
    Serial.println("Unknown error");
    return p;
  }

  // OK success!

  p = finger.image2Tz();
  switch (p)
  {
  case FINGERPRINT_OK:
    Serial.println("Image converted");
    break;
  case FINGERPRINT_IMAGEMESS:
    Serial.println("Image too messy");
    return p;
  case FINGERPRINT_PACKETRECIEVEERR:
    Serial.println("Communication error");
    return p;
  case FINGERPRINT_FEATUREFAIL:
    Serial.println("Could not find fingerprint features");
    return p;
  case FINGERPRINT_INVALIDIMAGE:
    Serial.println("Could not find fingerprint features");
    return p;
  default:
    Serial.println("Unknown error");
    return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK)
  {
    // Match
    lcd_blynk.print(0, 0, "Jari Terdeteksi");
    lcd_blynk.print(0, 1, "Pintu terbuka");
    Serial.println("Found a print match!");
    buzz_confirm = true;
    door_secure_flag = true;
    delay(2000);
    lcd_blynk.clear();
  }
  else if (p == FINGERPRINT_PACKETRECIEVEERR)
  {
    Serial.println("Communication error");
    return p;
  }
  else if (p == FINGERPRINT_NOTFOUND)
  {
    // Not match
    lcd_blynk.clear();
    lcd_blynk.print(0, 0, "Jari Terdeteksi");
    lcd_blynk.print(0, 1, "Tidak Terdaftar");
    Serial.println("Did not find a match");
    Blynk.notify("Sidik jari tidak terdaftar mencoba membuka kotak");
    Serial.println("Sidik jari tidak terdaftar mencoba membuka kotak");
    // buzz_wrong = true;
    delay(2000);
    lcd_blynk.clear();
    return p;
  }
  else
  {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);

  return finger.fingerID;
}

void runn_blynk()
{
  Blynk.run();
  Blynk.virtualWrite(V1, lat);
  Blynk.virtualWrite(V2, lng);
  myMap.location(1, lat, lng, "Kotak amal");
  Blynk.virtualWrite(V7, satelite);
  Blynk.virtualWrite(V6, distance/1000);
  switch (count_tampilan)
  {
  case 0:
    lcd_blynk.clear();
    lcd_blynk.print(0, 0, "Masjid Darul");
    lcd_blynk.print(0, 1, "Ulum Amban");
    break;

  case 1:
    lcd_blynk.clear();
    lcd_blynk.print(0, 0, "LAT:");
    lcd_blynk.print(4, 0, lat);
    lcd_blynk.print(0, 1, "LNG:");
    lcd_blynk.print(4, 1, lng);
    break;
  }
  if (!Blynk.connected())
  {
    Blynk.begin(auth, ssid, pass, server, 8080);
  }
}
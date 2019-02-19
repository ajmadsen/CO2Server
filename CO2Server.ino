#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>

#include <ACROBOTIC_SSD1306.h>
#include <Wire.h>

#include "mhz19b.h"

const char *networks[] = {
    "MadNet",
    "augurycheeseadvertisedpl4nk",
    "HartGuest",
    "GoHart!2018",
};
ESP8266WiFiMulti multi;
const int led = LED_BUILTIN;
int lastError = 0;

uint16_t minReading = 0xffff, maxReading = 0;

void connect()
{
  oled.clearDisplay();
  oled.putString("Scanning:\n");
  int i;
  for (i = 0; i < (sizeof networks / sizeof networks[0]) / 2; ++i)
  {
    oled.setTextXY(i + 1, 0);
    oled.putChar(' ');
    oled.putString(networks[i * 2]);
    multi.addAP(networks[i * 2], networks[i * 2 + 1]);
  }
  oled.setTextXY(i + 1, 0);

  // Wait for connection
  while (multi.run() != WL_CONNECTED)
  {
    delay(500);
    oled.putChar('.');
  }
}

void displayConnection()
{
  oled.clearDisplay();
  oled.putString("Connected:");
  oled.setTextXY(1, 1);
  oled.putString(WiFi.SSID());
  oled.setTextXY(2, 0);
  oled.putString("IP Address:");
  oled.setTextXY(3, 1);
  oled.putString(WiFi.localIP().toString());
  oled.setTextXY(4, 0);
  oled.putString("Listening at:");
  oled.setTextXY(5, 1);
  oled.putString("http://esp8266.local/");
}

inline void putTime(char **buf, uint32_t m, char sfx)
{
  if (!buf || !*buf || m == 0 || m >= 100)
    return;
  const char u = m / 10, l = m % 10;
  if (u)
  {
    **buf = u + '0';
    ++*buf;
  }
  **buf = l + '0';
  ++*buf;
  **buf = sfx;
  ++*buf;
}

String uptime()
{
  uint32_t seconds = millis() / 1000;
  uint32_t minutes = seconds / 60;
  seconds %= 60;
  uint32_t hours = minutes / 60;
  minutes %= 60;
  uint32_t days = hours / 24;
  hours %= 24;

  char i, uptime[13] = {0}, *u = uptime;
  putTime(&u, days, 'd');
  putTime(&u, hours, 'h');
  putTime(&u, minutes, 'm');
  putTime(&u, seconds, 's');

  return String(uptime);
}

void setupDisplay()
{
  oled.clearDisplay();
  oled.setTextXY(0, 0);
  oled.putString("CO2 Level:");
  oled.setTextXY(2, 0);
  oled.putString("Min:");
  oled.setTextXY(3, 0);
  oled.putString("Max:");
  oled.setTextXY(4, 0);
  oled.putString("Uptime:");
}

void displayCO2()
{
  if (lastError > 0)
  {
    oled.setTextXY(7, 0);
    do
    {
      oled.putChar(' ');
    } while (--lastError > 0);
  }

  uint16_t concentration = MHZ19B::readCO2();
  if (concentration == MHZ19B::E_TimedOut)
  {
    oled.setTextXY(7, 0);
    oled.putString("timed out");
    lastError = 9;
  }
  else if (concentration == MHZ19B::E_CksumFail)
  {
    oled.setTextXY(7, 0);
    oled.putString("cksum fail");
    lastError = 10;
  }
  else if (concentration > 5000)
  {
    // skip since the concentration is out of range
  }
  else
  {
    oled.setTextXY(1, 1);
    oled.putString(String(concentration));
    oled.putString(" PPM   ");

    if (minReading > concentration)
    {
      minReading = concentration;
      oled.setTextXY(2, 5);
      oled.putString(String(minReading));
      oled.putString(" PPM  ");
    }
    if (concentration > maxReading)
    {
      maxReading = concentration;
      oled.setTextXY(3, 5);
      oled.putString(String(maxReading));
      oled.putString(" PPM  ");
    }
  }

  oled.setTextXY(5, 1);
  oled.putString(uptime());
}

void setup(void)
{
  pinMode(led, OUTPUT);
  digitalWrite(led, 1);

  Serial.begin(9600, SERIAL_8N1);
  Serial.pins(15, 13);

  WiFi.mode(WIFI_STA);

  Wire.begin();
  oled.init();

  // connect();
  // displayConnection();

  setupDisplay();

  MHZ19B::setRange(MHZ19B::PpmRange::PPM_5000);
}

void loop(void)
{
  displayCO2();
  delay(2000);
}

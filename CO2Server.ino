#include <SoftwareSerial.h>
#include <Wire.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>

#define OLED_RESET (-1)
#define SCREEN_ADDRESS (0x3C)
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Adafruit_BME280.h>

#include <InfluxDbClient.h>

#include "mhz19b.h"
#include "secrets.h"

#define TZ_INFO "CST6CDT"

#define cw_ (6)
#define ch_ (8)
#define ccpos(x, y) ((x)*cw_), ((y)*ch_)
#define clrLine(x, y, w) oled.fillRect(ccpos(x, y), (w)*cw_, ch_, BLACK)
Adafruit_SSD1306 oled(128, 64, &Wire, OLED_RESET);

ESP8266WiFiMulti multi;
const int led = LED_BUILTIN;
int lastError = 0;

uint16_t minReading = 0xffff, maxReading = 0;

SoftwareSerial debug;
MHZ19B::device_t co2_sensor;

Adafruit_BME280 bme;
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_hum = bme.getHumiditySensor();

InfluxDBClient client(INFLUXDB_SERVER, INFLUXDB_ORG, INFLUXDB_BUCKET, INFLUXDB_TOKEN);
Point sensor("environment");

#define WARMUP_MILLIS (90 * 1000)
#define READ_TIMEOUT (500L)

#if 1
void connect()
{
  oled.clearDisplay();
  oled.print("Scanning:\n");
  oled.display();
  int i;
  for (i = 0; i < num_networks; ++i)
  {
    oled.setCursor(ccpos(0, i + 1));
    oled.print(' ');
    oled.print(networks[i * 2]);
    multi.addAP(networks[i * 2], networks[i * 2 + 1]);
  }
  oled.setCursor(ccpos(0, i + 1));
  oled.display();

  // Wait for connection
  while (multi.run() != WL_CONNECTED)
  {
    oled.print('.');
    oled.display();
  }
}

void displayConnection()
{
  oled.clearDisplay();
  oled.setCursor(ccpos(0, 0));
  oled.print("Connected:");
  oled.setCursor(ccpos(1, 1));
  oled.print(WiFi.SSID());
  oled.setCursor(ccpos(0, 2));
  oled.print("IP Address:");
  oled.setCursor(ccpos(1, 3));
  oled.print(WiFi.localIP().toString());
}
#endif

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
  oled.setCursor(ccpos(0, 0));
  oled.print("CO2 Level:");
  oled.setCursor(ccpos(0, 2));
  oled.print("Min:");
  oled.setCursor(ccpos(0, 3));
  oled.print("Max:");
  oled.setCursor(ccpos(0, 4));
  oled.print("Uptime:");
}

void printUptime()
{
  oled.setCursor(ccpos(1, 7));
  String time = uptime();
  clrLine(1, 7, 1 + time.length());
  oled.print(time);
}

void displayCO2()
{
  if (lastError > 0)
  {
    clrLine(0, 7, lastError);
    lastError = 0;
  }

  digitalWrite(led, 0);
  printf("reading concentration\n");
  int16_t concentration = co2_sensor.read();
  // uint16_t concentration = 0;
  printf("%d\n", concentration);
  digitalWrite(led, 1);

  if (concentration == MHZ19B::E_TimedOut)
  {
    oled.setCursor(ccpos(0, 7));
    oled.print("timed out");
    lastError = 9;
    printf("timed out\n");
  }
  else if (concentration == MHZ19B::E_CksumFail)
  {
    oled.setCursor(ccpos(0, 7));
    oled.print("cksum fail");
    lastError = 10;
    printf("cksum fail\n");
  }
  /*else if (concentration > 5000)
  {
    // skip since the concentration is out of range
    oled.setCursor(ccpos(0, 7));
    oled.print("too high");
    lastError = 8;
    printf("too high\n");
  }*/
  else
  {
    oled.setCursor(ccpos(1, 1));
    clrLine(1, 1, 8);
    oled.printf("%d PPM", concentration);

    if (minReading > concentration)
    {
      minReading = concentration;
      oled.setCursor(ccpos(5, 2));
      clrLine(5, 2, 8);
      oled.printf("%d PPM", minReading);
    }
    if (concentration > maxReading)
    {
      maxReading = concentration;
      oled.setCursor(ccpos(5, 3));
      clrLine(5, 3, 8);
      oled.printf("%d PPM", maxReading);
    }
  }

  printUptime();
}

void setup(void)
{
  pinMode(led, OUTPUT);

  Serial.begin(115200, SERIAL_8N1);
  Serial.setDebugOutput(true);
  Serial.printf("hello world, serial is up\n");

  // d8 blue -- perif rx
  // d7 green -- perif tx
  pinMode(D7, INPUT);
  pinMode(D8, OUTPUT);
  debug.begin(9600, SWSERIAL_8N1, 13, 15);
  co2_sensor.reset(&debug);

  WiFi.mode(WIFI_STA);

  Wire.begin(SDA, SCL);
  oled.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS, false, false);

  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.cp437(true);

  oled.clearDisplay();
  oled.display();

  connect();
  displayConnection();
  oled.display();

  timeSync(TZ_INFO, "pool.ntp.org");

  if (client.validateConnection())
  {
    Serial.print("Connected to influx server: ");
    Serial.println(client.getServerUrl());
  }
  else
  {
    Serial.print("Connecting to influx failed: ");
    Serial.println(client.getLastErrorMessage());
  }

  // setupDisplay();
  // oled.display();

  Serial.printf("setting CO2 sensor range to 5000 PPM\n");
  co2_sensor.setRange(MHZ19B::PpmRange::PPM_2000);
  Serial.printf("setting CO2 ABC mode enabled\n");
  co2_sensor.setABC(false);

  if (!bme.begin(BME280_ADDRESS_ALTERNATE))
    Serial.println("Could not find BME sensor");

  bme_temp->printSensorDetails();
  bme_pressure->printSensorDetails();
  bme_hum->printSensorDetails();
}

void loop(void)
{
  sensors_event_t e_temp, e_press, e_hum;
  float altitude;
  int16_t concentration;
  time_t now;
  unsigned long m_time = millis();

  digitalWrite(led, 0);
  now = time(nullptr);
  bme_temp->getEvent(&e_temp);
  bme_pressure->getEvent(&e_press);
  bme_hum->getEvent(&e_hum);
  concentration = co2_sensor.read(READ_TIMEOUT);
  digitalWrite(led, 1);

  altitude = 44330.0 * (1.0 - pow(e_press.pressure / 1013.25F, 0.1903));

  Serial.printf("\nThe current time is %s", ctime(&now));
  Serial.printf("CO2 concentration = %d PPM\n", concentration);
  Serial.printf("Temperature = %.2f *C\n", e_temp.temperature);
  Serial.printf("Humidity = %.1f%%\n", e_hum.relative_humidity);
  Serial.printf("Pressure = %f hPa\n", e_press.pressure);
  Serial.printf("Altitude = %f m\n", altitude);

  sensor.clearFields();

  if (concentration >= 0 && m_time > WARMUP_MILLIS)
  {
    sensor.addField("CO2", concentration);
  }
  else if (m_time <= WARMUP_MILLIS)
  {
    Serial.println("Waiting for CO2 sensor to warm up before reporting");
  }
  sensor.addField("temperature", e_temp.temperature);
  sensor.addField("humidity", e_hum.relative_humidity);
  sensor.addField("pressure", e_press.pressure, 4);
  sensor.addField("altitude", altitude, 2);

  // displayCO2();
  // oled.display();
  displayConnection();
  printUptime();
  oled.display();

  if (Serial.available() > 0)
  {
    String cmd = Serial.readStringUntil('\n');
    if (cmd == "zero")
    {
      Serial.printf("zeroing co2 sensor...\n");
      co2_sensor.setZero();
      Serial.printf("done\n");
    }
    else
    {
      Serial.printf("unknown command '%s'\n", cmd);
    }
  }

  if (multi.run() != WL_CONNECTED)
  {
    Serial.printf("unable to connect to WiFi\n");
    goto out;
  }

  if (!client.writePoint(sensor))
  {
    Serial.print("unable to write measurement:");
    Serial.println(client.getLastErrorMessage());
  }

out:
  delay(5000);
}

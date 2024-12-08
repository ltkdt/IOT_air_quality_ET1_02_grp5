#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <Wire.h>
#include <WiFi.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>
#include <MQ135.h>
#include <math.h>

#define BLYNK_TEMPLATE_ID "TMPL6zqrXlz6W"
#define BLYNK_TEMPLATE_NAME "Air Quality"
#define BLYNK_AUTH_TOKEN "SNPHQianuE-xPmTP4OHLnaC3QxpR3hIC"

#include <arduino_secrets.h>


#define I2C_ADDRESS 0x3C
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

// SDA A4 | SCL A5

int measurePin = A2;
int MQ135Pin = A0;
int ledPower = 7;
int dht22_data = 4;


Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
DHT_Unified dht(dht22_data, DHT22);
MQ135 mq135_sensor(MQ135Pin);

unsigned int samplingTime = 280;
unsigned int deltaTime = 40;
unsigned int sleepTime = 9680;

float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

float temperature = 0;
float heat_index = 0;
float relative_humidity = 0;

float co2_ppm = 0;

void getDataDHT(float &temp, float &hi, float &rh){
  delay(1000);
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  temp = event.temperature - 4.0;
  rh = dht.humidity().getEvent(&event);
  rh = event.relative_humidity - 4.0;
}

void printText(int x, int y, String text){
  display.setCursor(x, y);
  display.print(text);
}

void printText(int x, int y, float value){
  display.setCursor(x, y);
  display.print(value);
}

void printText(int x, int y, int value){
  display.setCursor(x, y);
  display.print(value);
}

void DSDataCollect(float &vo_measured, float &calc_voltage, float &dust_density){
  delay(1000);
  digitalWrite(ledPower,LOW);
  delayMicroseconds(samplingTime);

  voMeasured = analogRead(measurePin);

  delayMicroseconds(deltaTime);
  digitalWrite(ledPower,HIGH);
  delayMicroseconds(sleepTime);

  calcVoltage = voMeasured*(5.0/1024);
  dustDensity = 170*calcVoltage-0.1;

  if ( dustDensity < 0)
  {
    dustDensity = 0.00;
  }
}

float calc_aqi_epa(float concentration){
  float aqi;
  float c_low[8] = {0, 12.1, 35.5, 55.5, 150.5, 250.5, 350.5, 500.5};
  float c_high[8] = {12, 35.4, 55.4, 150.4, 250.4, 350.4, 500.4, 1000.0};
  float i_low[8] = {0, 51, 101, 151, 201, 301, 401, 501};
  float i_high[8] = {50, 100, 150, 200, 300, 400, 500, 9999};

  for(int i = 0; i < 8; i++){
    if ( concentration >= c_low[i] && concentration <= c_high[i]){
      aqi = ( ( i_high[i] - i_low[i]) / (c_high[i] - c_low[i]) ) * (concentration - c_low[i]) + i_low[i];
      return ceil(aqi);
    }
  }

  if (concentration > c_high[7]){
    aqi = ( ( i_high[7] - i_low[7]) / (c_high[7] - c_low[7]) ) * (concentration - c_low[7]) + i_low[7];
    return ceil(aqi);
  }
  else{
    return 0.0;
  }
}

void get_co2_ppm(float &CO2_ppm){
  delay(1000);
  float correctedRZero = mq135_sensor.getCorrectedRZero(temperature, relative_humidity);
  float resistance = mq135_sensor.getResistance();
  co2_ppm = mq135_sensor.getCorrectedPPM(temperature, relative_humidity);
}

void status_display(){
  delay(1000);
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  
  display.drawFastHLine(0,32, 128, SH110X_WHITE);
  display.drawFastVLine(90, 0, 64, SH110X_WHITE);

  // PRINT AQI
  printText(16, 4, "PM2.5 AQI:");
  printText(40, 16, static_cast<int>(calc_aqi_epa(dustDensity)));

  printText(16, 36, "CO2 (ppm):");
  printText(40, 48, co2_ppm);


  printText(94, 8, "T(oC)");
  printText(94, 20, temperature);
  
  printText(94, 36, "%RH");
  printText(94, 48, relative_humidity);
}

void setup(){
  Serial.begin(9600);

  pinMode(ledPower,OUTPUT);

  dht.begin();

  display.begin(I2C_ADDRESS, true);
  display.display();
  display.clearDisplay();
}

void loop(){
  DSDataCollect(voMeasured, calcVoltage, dustDensity);
  getDataDHT(temperature, heat_index, relative_humidity);
  get_co2_ppm(co2_ppm);

  //Serial.println(voMeasured);
  //Serial.println(calcVoltage);
  //Serial.println(dustDensity);
  //Serial.println(temperature);
  //Serial.println("  -  ");

  status_display();
  display.display();
}
#include <Wire.h>
#include <Adafruit_ADS1015.h>
#include "PCF8574.h"
#include <ESP8266WiFi.h>
#include "U8g2lib.h"
#include "Adafruit_TCS34725.h"

Adafruit_ADS1115 ads;
PCF8574 pcf8574(0x20);

//OLED u8g2
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);
#define u8g2_logo_width 16
#define u8g2_logo_height 16
#define u8g2_result_width 24
#define u8g2_result_height 24

static const unsigned char u8g2_logo_food[] U8X8_PROGMEM = {
  0x00, 0x00, 0x18, 0x33, 0x8C, 0x19, 0x84, 0x08, 0x18, 0x33, 0x10, 0x22, 0x18, 0x33, 0x08, 0x11,
  0x00, 0x00, 0xFC, 0x3F, 0x04, 0x20, 0x04, 0x20, 0x0C, 0x30, 0x38, 0x1C, 0xE0, 0x07, 0x00, 0x00
};

static const unsigned char u8g2_logo_drink[] U8X8_PROGMEM = {
  0x00, 0x00, 0xE0, 0x0E, 0x20, 0x02, 0x30, 0x03, 0x10, 0x01, 0x00, 0x00, 0xFE, 0x0F, 0x02, 0x38,
  0x02, 0x28, 0x02, 0x28, 0x02, 0x38, 0x06, 0x0C, 0x0C, 0x06, 0xF8, 0x03, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char u8g2_logo_smile[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x80, 0x81, 0x01, 0x40, 0x00, 0x02, 0x20, 0x00, 0x04,
  0x20, 0x42, 0x04, 0x10, 0xA5, 0x08, 0x10, 0xE7, 0x08, 0x10, 0x00, 0x08, 0x10, 0x00, 0x08, 0x10,
  0x00, 0x08, 0x10, 0x81, 0x08, 0x20, 0x42, 0x04, 0x20, 0x3C, 0x04, 0x40, 0x00, 0x02, 0x80, 0x81,
  0x01, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const unsigned char u8g2_logo_sad[] U8X8_PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x7E, 0x00, 0x80, 0x81, 0x01, 0x40, 0x00, 0x02, 0x20, 0x00, 0x04,
  0x20, 0x66, 0x04, 0x10, 0x66, 0x08, 0x10, 0x00, 0x08, 0x10, 0x00, 0x08, 0x10, 0x00, 0x08, 0x10,
  0x00, 0x08, 0x10, 0x3C, 0x08, 0x20, 0x42, 0x04, 0x20, 0x81, 0x04, 0x40, 0x00, 0x02, 0x80, 0x81,
  0x01, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

//OLED  end

//TCS34725
Adafruit_TCS34725 tcs = Adafruit_TCS34725(TCS34725_INTEGRATIONTIME_700MS, TCS34725_GAIN_16X);
//TCS end

//led rgb
#define rgb D7


//switch
#define switch_cal D4
#define switch_start D3
#define switch_select D5
#define LED D6

#define interval_battery 6000
#define interval_checkSwitch 300

int device_state = 0, sugar_state = 0; //sugar_state => teaspoons
float cal_sugar_940, start_sugar_940, cal_sugar_940_2, start_sugar_940_2, cal_sugar_940_3, start_sugar_940_3, sugar_result;

//rgb data
uint16_t calr = 1, calg = 1, calb = 1, calc = 1, calcolorTemp, callux;
uint16_t startr, startg, startb, startc, startcolorTemp, startlux;
//rgb end

bool checkDF = 0; //check food 0  drink 1
bool checkDFswitch = 0;
unsigned long previousMillisDF = 0, presentMillisDF;

bool checkBattery = 0;
unsigned long previousMillisBattery = 0, presentMillisBattery, previousMillisCal = 0, presentMillisCal;
bool check940 = 0;
unsigned long previousMillisCheck940 = 0, presentMillisCheck940;
int count,rgbcount;
unsigned long previousMillisSum940 = 0, presentMillisSum940;

void check_device_state();
void device_state0();
void device_state1();
void device_state2();
void device_state3();

void check_battery();

//calbrate state
bool calibrate_state = 0;
void calibrate();



void setup() {
  ads.setGain(GAIN_ONE);        // 1x gain   +/- 4.096V  1 bit = 0.125mV

  Serial.begin(9600);
  Serial.println("Test");
  ads.begin();
  //pcf8574.begin();
  Wire.begin();
  u8g2.begin();
  tcs.begin();

  pinMode(switch_cal, INPUT_PULLUP);
  pinMode(switch_start, INPUT_PULLUP);
  pinMode(switch_select, INPUT_PULLUP); //switch select food/drink;
  pinMode(rgb, OUTPUT);
  digitalWrite(rgb, LOW);

  pinMode(LED, OUTPUT); //led light=error blink=battery
  digitalWrite(LED, LOW); 
  //Serial.println("Test2");

  Serial.println("welcome");
  u8g2.firstPage();
  do {
    u8g2.drawLine(1, 32, 127, 32);
    u8g2.drawRFrame(1, 1, 127, 63, 1);
    u8g2.setCursor(10, 25);
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.print("EZglucose");
    u8g2.drawUTF8(10, 55, "Welcome");
  } while ( u8g2.nextPage() );
  delay(2000);
  u8g2.firstPage();
  do {
    u8g2.drawLine(1, 32, 127, 32);
    u8g2.drawRFrame(1, 1, 127, 63, 1);
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    u8g2.drawXBMP(56, 11, u8g2_logo_width, u8g2_logo_height, u8g2_logo_drink);
    
  } while ( u8g2.nextPage() );
}

void loop() {
  check_device_state();
  //Serial.print("loop device state ");
  //Serial.println(device_state);
}

void check_device_state() {
  //wait for pressing button
  if (device_state == 0) {
    device_state0();
  }

  //calibrate
  if (device_state == 1) {
    device_state1();
  }

  //start testing
  if (device_state == 2) {
    device_state2();
  }

  //show result
  if (device_state == 3) {
    device_state3();
  }

  //check Battery
  presentMillisBattery = millis();
  if (presentMillisBattery - previousMillisBattery >= interval_battery) {
    previousMillisBattery = millis();
    check_battery();
  }
}

void device_state0() {
  //check switch press
  if (digitalRead(switch_select) == 0) {
    //Serial.println("digitalRead(switch_select) == 0");
    if (checkDFswitch == 0) {
      previousMillisDF = millis();
      checkDFswitch = 1;
    }
  }
  if (checkDFswitch == 1) {
    //Serial.println("checkDFswitch == 1");
    presentMillisDF = millis();
    if (presentMillisDF - previousMillisDF >= interval_checkSwitch) {
      if (checkDF == 0) {
        u8g2.firstPage();
        do {
          u8g2.drawLine(1, 32, 127, 32);
          u8g2.drawRFrame(1, 1, 127, 63, 1);
          u8g2.setFont(u8g2_font_unifont_t_symbols);
          u8g2.drawXBMP(56, 11, u8g2_logo_width, u8g2_logo_height, u8g2_logo_food);
        } while ( u8g2.nextPage() );
        digitalWrite(LED, LOW);
        checkDF = 1;
      }
      else {
        u8g2.firstPage();
        do {
          u8g2.drawLine(1, 32, 127, 32);
          u8g2.drawRFrame(1, 1, 127, 63, 1);
          u8g2.setFont(u8g2_font_unifont_t_symbols);
          u8g2.drawXBMP(56, 11, u8g2_logo_width, u8g2_logo_height, u8g2_logo_drink);
        } while ( u8g2.nextPage() );
        checkDF = 0;
      }
      checkDFswitch = 0;
    }
  }
  if (digitalRead(switch_cal) == 0) {
    //Serial.println("digitalRead(switch_cal) == 0");
    device_state = 1;
    calr = 0;
    calb = 0;
    calg = 0;
    calc = 0;

    check940 = 0;
    previousMillisCheck940 = 0;
    count = 0;
    cal_sugar_940 = 0;
    cal_sugar_940_2 = 0;
    cal_sugar_940_3 = 0;

    digitalWrite(LED, LOW);

    u8g2.firstPage();
    do {
      u8g2.drawLine(1, 32, 127, 32);
      u8g2.drawRFrame(1, 1, 127, 63, 1);
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      if (checkDF == 0)u8g2.drawXBMP(56, 11, u8g2_logo_width, u8g2_logo_height, u8g2_logo_drink);
      else u8g2.drawXBMP(56, 11, u8g2_logo_width, u8g2_logo_height, u8g2_logo_food);
      u8g2.drawUTF8(10, 55, "Processing");
    } while ( u8g2.nextPage() );
  }
  if (digitalRead(switch_start) == 0) {
    //Serial.println("digitalRead(switch_start) == 0");
    device_state = 2;
    startr = 0;
    startb = 0;
    startg = 0;
    startc = 0;

    check940 = 0;
    previousMillisCheck940 = 0;
    count = 0;
    start_sugar_940 = 0;
    sugar_result = 0;
    sugar_state = 0;

    digitalWrite(LED, LOW); 

    u8g2.firstPage();
    do {
      u8g2.drawLine(1, 32, 127, 32);
      u8g2.drawRFrame(1, 1, 127, 63, 1);
      u8g2.setFont(u8g2_font_unifont_t_symbols);
      if (checkDF == 0)u8g2.drawXBMP(56, 11, u8g2_logo_width, u8g2_logo_height, u8g2_logo_drink);
      else u8g2.drawXBMP(56, 11, u8g2_logo_width, u8g2_logo_height, u8g2_logo_food);
      u8g2.drawUTF8(10, 55, "Processing");
    } while ( u8g2.nextPage() );
  }
}

float ad0, ad2, ad3;

void device_state1() {
  if (check940 == 0) {
    digitalWrite(rgb, HIGH);
    tcs.getRawData(&calr, &calg, &calb, &calc);
    //previousMillisCheck940 = millis();
    check940 = 1;
    digitalWrite(rgb, LOW);
  }
  if (check940 == 1) {
    presentMillisSum940 = millis();
    if (presentMillisSum940 - previousMillisSum940 >= 10) {
      ad0 = ads.readADC_SingleEnded(0) * 0.125 ;
      ad2 = ads.readADC_SingleEnded(2) * 0.125;
      ad3 = ads.readADC_SingleEnded(3) * 0.125;
      cal_sugar_940 += ad0 ;
      cal_sugar_940_2 += ad2 ;
      cal_sugar_940_3 += ad3 ;
      //Serial.println(previousMillisSum940);
      //Serial.println(presentMillisSum940);
      //Serial.println(cal_sugar_940_3);
      //Serial.println(cal_sugar_940);
      //Serial.println(cal_sugar_940_2);

      previousMillisSum940 = presentMillisSum940;
      count++;
    }
    //presentMillisCheck940 = millis();
    if (count == 100) {
      //Serial.println(previousMillisCheck940);
      //Serial.println(presentMillisCheck940);
      //Serial.println(count);
      cal_sugar_940 = cal_sugar_940 / count;
      cal_sugar_940_2 = cal_sugar_940_2 / count;
      cal_sugar_940_3 = cal_sugar_940_3 / count;
      Serial.print(cal_sugar_940_3);
      Serial.print("\t");
      Serial.print(cal_sugar_940);
      Serial.print("\t");
      Serial.println(cal_sugar_940_2);
      previousMillisCal = millis();
      device_state = 0;

      u8g2.firstPage();
      do {
        u8g2.setCursor(1, 15);
        u8g2.setFont(u8g2_font_t0_11_tf);
        u8g2.print("940nm-0 ");
        u8g2.print(String(cal_sugar_940_3));
        u8g2.setCursor(1, 30);
        u8g2.print("940nm-1 ");
        u8g2.print(String(cal_sugar_940));
        u8g2.setCursor(1, 45);
        u8g2.print("940nm-3 ");
        u8g2.print(String(cal_sugar_940_2) );
        u8g2.setCursor(1, 60);
        u8g2.print("rgb ");
        u8g2.print(String(calr * 255 / 65535));
        u8g2.setCursor(45, 60);
        u8g2.print(String(calg * 255 / 65535));
        u8g2.setCursor(65, 60);
        u8g2.print(String(calb * 255 / 65535));
        u8g2.setCursor(85, 60);
        u8g2.print(String(calc * 255 / 65535));
      } while ( u8g2.nextPage() );
    }
  }
}

void device_state2() {
  /*if (calr == 0 && calg == 0 && calb == 0 && calc == 0) {
    device_state = 3;
    sugar_state = 3;
  }*/
  //else {
    if (check940 == 0) {
      digitalWrite(rgb, HIGH);
      tcs.getRawData(&startr, &startg, &startb, &startc);
      // previousMillisCheck940 = millis();
      check940 = 1;
      digitalWrite(rgb, LOW);
    }

    if (check940 == 1) {
      presentMillisSum940 = millis();
      if (presentMillisSum940 - previousMillisSum940 >= 10) {
        start_sugar_940 += ads.readADC_SingleEnded(0) * 0.125;
        start_sugar_940_2 += ads.readADC_SingleEnded(2) * 0.125;
        start_sugar_940_3 += ads.readADC_SingleEnded(3) * 0.125;
        previousMillisSum940 = presentMillisSum940;
        count++;
      }

      //presentMillisCheck940 = millis();
      if (count == 100) {
        start_sugar_940 = start_sugar_940 / count;
        start_sugar_940_2 = start_sugar_940_2 / count;
        start_sugar_940_3 = start_sugar_940_3 / count;

        //calculate sugar from mV
        if (checkDF == 0)sugar_result = sugar_result * 236.59;
        else sugar_result = sugar_result * 400;

        if (start_sugar_940 > 1700) {
          sugar_state = 2;
        }
        else {
          sugar_state = 1;
        }
        device_state = 3;
      }
    }
  //}
}

void device_state3() {

  u8g2.firstPage();
  do {
    u8g2.drawLine(1, 32, 127, 32);
    u8g2.drawRFrame(1, 1, 127, 63, 1);
    u8g2.setFont(u8g2_font_unifont_t_symbols);
    if (checkDF == 0)u8g2.drawXBMP(56, 11, u8g2_logo_width, u8g2_logo_height, u8g2_logo_drink);
    else u8g2.drawXBMP(56, 11, u8g2_logo_width, u8g2_logo_height, u8g2_logo_food);

    if (sugar_state == 1)u8g2.drawXBMP(52, 36, u8g2_result_width, u8g2_result_height, u8g2_logo_smile);
    else if (sugar_state == 2)u8g2.drawXBMP(52, 36, u8g2_result_width, u8g2_result_height, u8g2_logo_sad);
    else {
      u8g2.drawUTF8(54, 55, "ERROR!");
      pcf8574.digitalWrite(P0, HIGH);
    }
  } while ( u8g2.nextPage() );
  device_state = 0;
}

void check_battery() {
  //define battery low
  if (ads.readADC_SingleEnded(1) * 0.125 < 3000) {
    while (1) {
      digitalWrite(LED, HIGH);
      delay(500);
      digitalWrite(LED, LOW);
      delay(500);
    }
  }
  	Serial.print("device state ");
  	Serial.println(device_state);
  	Serial.print("battery ");
  	Serial.println(ads.readADC_SingleEnded(1));
  //
  //	Serial.print("cal red ");
  //	Serial.print(calr);
  //	Serial.print(" blue ");
  //	Serial.print(calb);
  //	Serial.print(" green ");
  //	Serial.println(calg);
  //
  //	Serial.print("start red ");
  //	Serial.print(startr);
  //	Serial.print(" blue ");
  //	Serial.print(startb);
  //	Serial.print(" green ");
  //	Serial.println(startg);
  //
  //	Serial.print("cal sugar_940 ");
  //	Serial.println(cal_sugar_940);
  //	Serial.print("start sugar_940 ");
  //	Serial.println(start_sugar_940);
  //	Serial.print("sugar state ");
  //	Serial.println(sugar_state);
}

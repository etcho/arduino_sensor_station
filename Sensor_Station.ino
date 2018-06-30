#include <Wire.h>
#include <Adafruit_BMP085.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>
#include <TinyGPS.h>
#include <DHT.h>
#include <EEPROM.h>

LiquidCrystal_I2C lcd(0x3F, 16, 4);
SoftwareSerial ss(10, 11);
TinyGPS gps;
DHT dht(9, DHT22);
Adafruit_BMP085 bmp180;

uint8_t arrow_n[8] = {0x4, 0xe, 0x1f, 0xe, 0xe, 0xe, 0x0, 0x0};
uint8_t arrow_ne[8] = {0xf, 0x7, 0xf, 0x1d, 0x18, 0x0, 0x0, 0x0};
uint8_t arrow_e[8] = {0x0, 0x4, 0x1e, 0x1f, 0x1e, 0x4, 0x0, 0x0};
uint8_t arrow_se[8] = {0x0, 0x0, 0x0, 0x18, 0x1d, 0xf, 0x7, 0xf};
uint8_t arrow_s[8] = {0x0, 0x0, 0xe, 0xe, 0xe, 0x1f, 0xe, 0x4};
uint8_t arrow_sw[8] = {0x0, 0x0, 0x0, 0x3, 0x17, 0x1e, 0x1c, 0x1e};
uint8_t arrow_w[8] = {0x0, 0x4, 0xf, 0x1f, 0xf, 0x4, 0x0, 0x0};
uint8_t arrow_nw[8] = {0x1e, 0x1c, 0x1e, 0x17, 0x3, 0x0, 0x0, 0x0};
float alt1, alt2, alt_gps1, alt_gps2 = 0;
int btn = 8;
int btn_pressed_count = 0;
int addr_alt_baro_div = 0;
int addr_alt_baro_mod = 1;
int addr_alt_gps_div = 2;
int addr_alt_gps_mod = 3;
int loops = 0;

static void smartdelay(unsigned long ms);
static void print_float(float val, float invalid, int len, int prec);
static void print_int(unsigned long val, unsigned long invalid, int len);
static void print_date(TinyGPS &gps);
static void print_str(const char *str, int len);

void setup() {
  Serial.begin(9600);
  ss.begin(9600);
  lcd.init();
  lcd.backlight();
  lcd.begin(16, 4);
  lcd.createChar(5, arrow_n);
  lcd.createChar(6, arrow_ne);
  lcd.createChar(7, arrow_e);
  lcd.createChar(8, arrow_se);
  lcd.createChar(9, arrow_s);
  lcd.createChar(10, arrow_sw);
  lcd.createChar(11, arrow_w);
  lcd.createChar(12, arrow_nw);
  dht.begin();
  if (!bmp180.begin()) {
    Serial.println("Sensor nao encontrado !!");
  }
  pinMode(btn, INPUT_PULLUP);
}
   
void loop(){
  //lendo o estado do botão
  if (digitalRead(btn) == LOW){
    btn_pressed_count++;
  } else {
    btn_pressed_count = 0;
  }

  //temperatura e humidade
  float humidity = dht.readHumidity();
  if (humidity > 99){
    humidity = 99;
  }
  float temperature;
  if (loops % 6 > 2){
    temperature = dht.readTemperature();
  } else {
    temperature = bmp180.readTemperature();
  }
  lcd.setCursor(0, 0);
  if (temperature < 10 && temperature >= 0){
    lcd.print(" ");
  }
  print_float_lcd(temperature, -1, 1);
  lcd.print((char)223);
  if (loops % 6 > 2){
    lcd.print("  ");
  } else {
    lcd.print("b ");
  }
  if (humidity < 10){
    lcd.print(" ");
  }
  lcd.print((int)humidity);
  lcd.print("% ");
  
  if (altitudes_are_defined() && loops % 10 == 0){
    lcd.print("*");
  } else {
    lcd.print(" ");
  }

  //altitude do barômetro
  float seaLevelPressure = 102520;
  float alt_original = bmp180.readAltitude(seaLevelPressure);
  if (btn_pressed_count == 2){
    EEPROM.write(addr_alt_baro_div, (int)alt_original / 255);
    EEPROM.write(addr_alt_baro_mod, (int)alt_original % 255);
  } else if (btn_pressed_count == 4){
    EEPROM.write(addr_alt_baro_div, 0);
    EEPROM.write(addr_alt_baro_mod, 0);
  }
  float alt;
  if (altitudes_are_defined() && loops % 10 == 0){
    alt = alt_original;
  } else {
    alt = alt_original - (255 * EEPROM.read(addr_alt_baro_div) + EEPROM.read(addr_alt_baro_mod));
  }
  if (alt < 10 && alt >= 0){
    lcd.print(" ");
  }
  if (alt < 100 && alt > -10){
    lcd.print(" ");
  }
  if (alt < 1000 && alt > -100){
    lcd.print(" ");
  }
  print_float_lcd(alt, -1, 0);
  lcd.print("m ");
  alt = alt_original - (255 * EEPROM.read(addr_alt_baro_div) + EEPROM.read(addr_alt_baro_mod));
  if (alt > alt1 && alt1 > alt2){
    lcd.write(5);
  } else if (alt < alt1 && alt1 < alt2){
    lcd.write(9);
  } else {
    lcd.print("=");
  }
  lcd.print(" ");
  alt2 = alt1;
  alt1 = alt;

  //pressão atmosférica
  lcd.setCursor(0, 1);
  if (bmp180.readPressure() / 100 < 1000){
    lcd.print(" ");
  }
  float atm = ((float)bmp180.readPressure())/101325;
  lcd.print(bmp180.readPressure() / 100);
  lcd.print(" hPa / ");
  lcd.print(atm);
  lcd.print(" atm");

  //GPS
  float flat, flon;
  unsigned long age;
  gps.f_get_position(&flat, &flon, &age);
  if (age > 5000){
    lcd.setCursor(0, 2);
    lcd.print("                    ");
    lcd.setCursor(0, 3);
    lcd.print("                    ");
  } else {
    //número de satélites
    lcd.setCursor(0, 2);
    int sats = gps.satellites();
    if (sats == 255){
      lcd.print("   ");
    } else {
      lcd.print("#");
      lcd.print(sats);
      if (sats < 10){
        lcd.print(" ");
      }
    }
    //velocidade
    lcd.print(" ");
    float kmh = gps.f_speed_kmph();
    if (kmh == -1){
      lcd.print("         ");
    } else{
      if (kmh < 10){
        lcd.print(" ");
      }
      if (kmh < 100){
        lcd.print(" ");
      }
      print_float_lcd(kmh, -1, 1);
      lcd.print("km/h");
    }
    //hora
    int year;
    byte month, day, hour, minute, second, hundredths;
    unsigned long age = 0;
    gps.crack_datetime(&year, &month, &day, &hour, &minute, &second, &hundredths, &age);
    int hour_int = (int) hour;
    if ((int) hour > 23){
      lcd.print("     ");
    } else {
      hour_int -= 3;
      if (hour_int < 0){
        hour_int = 24 + hour_int;
      }
      lcd.print("  ");
      if (hour_int < 10){
        lcd.print("0");
      }
      lcd.print(hour_int);
      lcd.print(":");
      if (minute < 10){
        lcd.print("0");
      }
      lcd.print(minute);
    }
    //altitude
    lcd.setCursor(0, 3);
    alt_original = gps.f_altitude();
    if (alt_original == 1000000){
      lcd.print("      ");
    } else {
      if (btn_pressed_count == 2){
        EEPROM.write(addr_alt_gps_div, (int)alt_original / 255);
        EEPROM.write(addr_alt_gps_mod, (int)alt_original % 255);
      } else if (btn_pressed_count == 4){
        EEPROM.write(addr_alt_gps_div, 0);
        EEPROM.write(addr_alt_gps_mod, 0);
      }
      if (altitudes_are_defined() && loops % 10 == 0){
        lcd.print("*");
        alt = alt_original;
      } else {
        lcd.print(" ");
        alt = alt_original - (255 * EEPROM.read(addr_alt_gps_div) + EEPROM.read(addr_alt_gps_mod));
      }    
      if (alt < 10 && alt >= 0){
        lcd.print(" ");
      }
      if (alt < 100 && alt > -10){
        lcd.print(" ");
      }
      if (alt < 1000 && alt > -100){
        lcd.print(" ");
      }
      print_float_lcd(alt, -1, 0);
      lcd.print("m");
      if (alt > alt_gps1 && alt_gps1 > alt_gps2){
        lcd.write(5);
      } else if (alt < alt_gps1 && alt_gps1 < alt_gps2){
        lcd.write(9);
      } else {
        lcd.print("=");
      }
      lcd.print(" ");
      alt_gps2 = alt_gps1;
      alt_gps1 = alt;
    }
    //bússola
    lcd.print("  ");
    float course = gps.f_course();
    if (course > 360){
      lcd.print("         ");
    } else {
      if (course < 10){
        lcd.print(" ");
      }
      if (course < 100){
        lcd.print(" ");
      }
      print_float_lcd(course, -1, 0);
      lcd.print((char)223);
      int i;
      int course_inverso = 360 - course;
      if (course_inverso > 337 || course_inverso < 23){
        lcd.write(5);
        lcd.print(" N ");
      } else {
        int pos = 5;
        for (i=23; i<359; i+=45){
          pos++;
          if (course_inverso >= i && course_inverso < i + 45){
            lcd.write(pos);
            break;
          }
        }
        pos = 5;
        for (i=23; i<360; i+=45){
          pos++;
          if (course >= i && course < i + 45){
            lcd.print(" ");
            switch (pos){
              case 6: lcd.print("NE"); break;
              case 7: lcd.print("E "); break;
              case 8: lcd.print("SE"); break;
              case 9: lcd.print("S "); break;
              case 10: lcd.print("SO"); break;
              case 11: lcd.print("O "); break;
              case 12: lcd.print("NO"); break;
            }
          }
        }
        lcd.print(" ");
      }
    }
  }

  loops++;
  if (loops == 1000){
    loops = 0;
  }
  smartdelay(1000);
}

static void smartdelay(unsigned long ms){
  unsigned long start = millis();
  do {
    while (ss.available()){
      gps.encode(ss.read());
    }
  } while (millis() - start < ms);
}

static void print_float_lcd(float val, int len, int prec){
  lcd.print(val, prec);
  if (len >= 0){
    int vi = abs((int)val);
    int flen = prec + (val < 0.0 ? 2 : 1); // . and -
    flen += vi >= 1000 ? 4 : vi >= 100 ? 3 : vi >= 10 ? 2 : 1;
    for (int i=flen; i<len; ++i){
      lcd.print(' ');
    }
  }
  smartdelay(0);
}

static boolean altitudes_are_defined(){
  return EEPROM.read(addr_alt_baro_div) != 0 || 
         EEPROM.read(addr_alt_baro_mod) != 0 || 
         EEPROM.read(addr_alt_gps_div) != 0 || 
         EEPROM.read(addr_alt_gps_mod) != 0;
}


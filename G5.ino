//#include <AltSoftSerial.h>
#include "SoftwareSerial.h"
#include <LiquidCrystal.h>

// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

//define LCD Key button

int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

//Intial Panteng G5 data structure
struct _panteng {
        unsigned char len[2];
        unsigned char pm1_cf1[2];
        unsigned char pm2_5_cf1[2];
        unsigned char pm10_0_cf1[2];
        unsigned char pm1_0[2];
        unsigned char pm2_5[2];
        unsigned char pm10_0[2];
        unsigned char d[20];
} panteng;

// read the buttons
int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // read the value from the sensor 
 // my buttons when read are centered at these valies: 0, 144, 329, 504, 741
 // we add approx 50 to those values and check to see if we are close
 if (adc_key_in > 1000) return btnNONE; // We make this the 1st option for speed reasons since it will be the most likely result
 // For V1.1 us this threshold
 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 195)  return btnUP; 
 if (adc_key_in < 380)  return btnDOWN; 
 if (adc_key_in < 555)  return btnLEFT;
 if (adc_key_in < 790)  return btnSELECT;  

 return btnNONE;  // when all others fail, return this...
}


void setup()
{
        lcd.begin(16,2);                      // initialize the lcd 
        lcd.setCursor(0,0);
        lcd.print("Push the buttons"); // print a simple message
        Serial1.begin(9600);        //Initial Serial Port
}

void loop()
{
        unsigned char c;
        char str[100];
        static int state = 0;
        static int count = 0;
        static int time=0;
        int pm1_0, pm2_5, pm10_0;        //PM1.0、PM2.5、PM10
        int i;

        if (Serial1.available()) {
                c = Serial1.read();
                switch (state) {
                case 0:
                        if (0x42 == c)
                                state = 1;
                        break;
                case 1:
                        if (0x4d == c) {
                                state = 2;
                                count = 0;
                                //Serial.println(' ');
                        }
                        break;
                case 2:
                        ((unsigned char *) &panteng)[count++] = c;
                        sprintf(str, "%02X ", c);
                        //Serial.print(str);
                        if (count > 28) {
                                state = 0;
                                pm1_0 = panteng.pm1_0[0] * 256 + panteng.pm1_0[1];
                                pm2_5 = panteng.pm2_5[0] * 256 + panteng.pm2_5[1];
                                pm10_0 = panteng.pm10_0[0] * 256 + panteng.pm10_0[1];

                                sprintf(str, "%d\t%d\t%d\t%d", time++,pm1_0, pm2_5, pm10_0);
                                Serial.println(str);

                                snprintf(str,16, "PM2.5=%d    ", pm2_5);
                                //lcd.clear();
                                for (i = 0; i < strlen(str); i++) {
                                        lcd.setCursor(i, 0);
                                        lcd.print(&str[i]);
                                }
                        }
                        break;
                default:
                        break;
                }
        }
}


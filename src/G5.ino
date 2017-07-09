//#include <AltSoftSerial.h>
//#include "SoftwareSerial.h"
#include <LiquidCrystal.h>
#include <assert.h>

// select the pins used on the LCD panel
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

byte readCO2[] = {0xFE, 0x04, 0x00, 0x03, 0x00, 0x01, 0xD5, 0xC5};  //Command packet to read Co2 (see app note) //<FE> <04> <00> <03> <00> <01> <D5> <C5>
byte response[] = {0,0,0,0,0,0,0};  //create an array to store the response

//multiplier for value. default is 1. set to 3 for K-30 3% and 10 for K-33 ICB
//int valMultiplier = 1;

//define LCD Key button

int lcd_key     = 0;
int adc_key_in  = 0;
bool stopbutton = false;
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
        unsigned char c_03[2];
        unsigned char c_05[2];
        unsigned char c_1[2];
        unsigned char c_25[2];
        unsigned char c_5[2];
        unsigned char c_10[2];
        unsigned char hcho[2];
        unsigned char d[18];
} panteng;

//// Intial S8 Sensor Response Structure 
//struct _s8 {
//        unsigned char s8fe[1];
//        unsigned char s8begin[2];
//        unsigned char s8co2[2];
//        unsigned char s8end[2];
//} s8;


char pm25title[6]="PM25";
char CO2title[6]="CO2";
char hchotitle[6]="HCHO";

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
 if (adc_key_in < 790)  
 {
  stopbutton=true;
  return btnSELECT;
  }  

 return btnNONE;  // when all others fail, return this...
}


void setup()
{
        lcd.begin(16,2);                      // initialize the lcd 
        lcd.setCursor(0,0);
        lcd.print("Push the buttons"); // print a simple message
        Serial1.begin(9600);        //Initial Serial Port for PM 2.5
        Serial2.begin(9600);        //Initial Serial Port for CO2
}

  
void displayData()
{
        unsigned char c;
        char str[100];
        static int state = 0;
        static int count = 0;
        static int time=0;
        int pm1_0, pm2_5, hcho;        //PM1.0、PM2.5、HCHO
        int i;

        while (!stopbutton){
          lcd_key=read_LCD_buttons();
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

                        if (count > 28) {
                                state = 0;
                                //pm1_0 = panteng.pm1_0[0] * 256 + panteng.pm1_0[1];
                                pm2_5 = panteng.pm2_5[0] * 256 + panteng.pm2_5[1];
                                hcho=panteng.hcho[0]*256+panteng.hcho[1];

                                lcd.setCursor(0,1);            // move cursor to second line "1" 
                                lcd.print("                "); // clear second line
                                lcd.setCursor(0,1);            // move cursor to second line "1" 
                                lcd.print(pm2_5);
                                lcd.setCursor(6,1);
                                lcd.print(hcho);
                                sendRequest(readCO2);
                                unsigned long valCO2 = getValue(response);
                                lcd.setCursor(11,1);
                                lcd.println(valCO2);
                                }
                        
                        break;
                default:
                        break;
                }
        }
    }
 
}

void loop()
{
  
 
 lcd_key = read_LCD_buttons();  // read the buttons
 switch (lcd_key)               // depending on which button was pushed, we perform an action
 {
   case btnRIGHT:
   {

    lcd.clear();
    lcd.setCursor(0,0);            // move cursor to first line "1" and print title
    lcd.print(pm25title);
    lcd.setCursor(6,0);
    lcd.print(hchotitle);
    lcd.setCursor(11,0);
    lcd.print(CO2title);
    displayData();
    stopbutton=false;
    break;
     }
   case btnLEFT:
     {
      stopbutton=false;
      break;
     }
   case btnUP:
     {
      stopbutton=false;
      break;
     }
   case btnDOWN:
     {
      stopbutton=false;
      break;
     }
   case btnSELECT:
     {
     lcd.clear();
     lcd.setCursor(0,0);
     lcd.print("Push the buttons"); // print a simple message
     break;
     }
     case btnNONE:
     {
     break;
     }
 }
}

void sendRequest(byte packet[])
{
  while(!Serial2.available())  //keep sending request until we start to get a response
  {
    Serial2.write(readCO2,sizeof(readCO2));
    delay(50);
  }
  
  int timeout=0;  //set a timeoute counter
  while(Serial2.available() < 7 ) //Wait to get a 7 byte response
  {
    timeout++;  
    if(timeout > 10)    //if it takes to long there was probably an error
      {
        while(Serial2.available())  //flush whatever we have
          Serial2.read();
          
          break;                        //exit and try again
      }
      delay(50);
  }
  
  for (int i=0; i < 7; i++)
  {
    response[i] = Serial2.read();
  }  
}

unsigned long getValue(byte packet[])
{
    int high = packet[3];                        //high byte for value is 4th byte in packet in the packet
    int low = packet[4];                         //low byte for value is 5th byte in the packet

    unsigned long val = high*256 + low;                //Combine high byte and low byte with this formula to get value
    return val;
}


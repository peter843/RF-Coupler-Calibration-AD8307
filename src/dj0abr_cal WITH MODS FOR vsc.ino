/*
  code used to calibare DJ0ABR board/

  >>>>>VERY IMPORTANT NOTES FOR VSC PLATFORMIO ARDUINO IDE EXTENSIONS use

  This has been modified from the original arduino ide sketch i made by:
  1) Adding arduino #include at beginning so that Visual Studio Code VSC understand the arduino departurews from real 
  C++ like defining pins
  2) All functions (subroutines) need to be declalred and written in full before thay are called at the top of the program before 
  void setup (not at end of program that I did before).
  3) All the librarty files are stored in the VSC project dir lib  not in central arduino libraries folder in the arduino 
  directory file.

  
  Dig write 3 low = atu (no bypass) high = bypass
  dig write 2 low =atu ant 1 gig wrie 2 high = ant 2 (if dig write 3 bypass is low)
  no power to atu both relays off low gives ant 1

  Nextion connection with Arduino Uno/Nano:
  +5V = 5V
  TX  = pin 0 (RX)
  RX  = pin 1 (TX)
  GND = GND
  SDA -> A4
  SCL -> A5

  Note Nextion shares same bus (in Nano) as usb so disonnect usb at least tx pin

*/

// for lcd
#include <Arduino.h>
#include <Wire.h>                   // #include <Wire.h> default to Wiring for Nano /Uno; SCL = A5, SDA = A4, VCC = 5V+, GND = GND
#include <LiquidCrystal_I2C.h>      // The Arduino I2C is hard coded SDA = pin A4 & SCL = pin A5
#include <PWM.h>                    // PWM library - allows frq changes to PWM - choose high no audio freq so you cant hear it in the fan
LiquidCrystal_I2C lcd(0x27, 20, 4); //  serial 20 X 4 LCD Display      //  Address  A2    A1    A0      //    0x26   Hi    Hi    Lo

int AnalogMeterPin = 5; // pwm out for anlog meter on D5
int am = 0;             // analog meter value to pwm

int V12Pin = A3; //
int V5Pin = A2;  //
int VrPin = A1;  //
int VfPin = A0;  //

int ProgressBarValue; // This variable stores the value that we are going to send to the progress bar on the display.
int CurrentPage = 0;  //Page number of Nextion display
int GaugeValue;
int PowerValuef;
int PowerValuer;
int VSWRValue;
int directivity;

float Vf; // reading from AD
float Vr;
float Pfwd;  // power dbm
float Prev;  //power dbm
float Pfwdw; // power in Watts
float Prevw;
float VSWR;
float VSWRmeter;
float VSWRi;
int adf;
int adr;

//for lcd//
float Directivity;
float V12;
float V5;

void Computeimmediatevswr()
{
  Vf = 0;
  for (int i = 0; i < 5; i++)
  {
    Vf = Vf + analogRead(VfPin);
    delay(50);
  }
  Vf = Vf / 5;

  Vr = analogRead(VrPin);

  // new equatiin for DJ0ABR board
  Prev = 0.2544 * Vr - 95.267 + 52.6;  // Convert AD measurment to  dBm with equation derived from calibration in Excel. 53.3dB is total attenulation to AD8307
  Pfwd = 0.252 * Vf - 95.131 + 52.6;   // old equation Prev = 0.2372 * Vr - 93.166 + 53.3 but changed due errors and now recalibrated see AD8307 Calibration - PH xls file..
  Pfwd = constrain(Pfwd, -30.0, 60.0); // 60 dBm is 1 kW m well over max possible with my currnet ldmos amps
  Prev = constrain(Prev, -30.0, 60.0);

  Pfwdw = pow(10, (Pfwd / 10.0)) / 1000.0; // change dBm to Watts
  Prevw = pow(10, (Prev / 10.0)) / 1000.0; // change dBm to Watts

  float fpi = sqrt(Prevw / Pfwdw);
  VSWRi = (1 + fpi) / (1 - fpi);

  //VSWRi = abs(VSWRi); // sometimes measured revers power is greater than fowrard power need to catch this and flag hi vswr
  VSWRi = constrain(VSWRi, 1.0, 99.0); //

  directivity = abs(Pfwd - Prev);
  return;
}

/*int VSWRMeterWrite() // using immediate value not averaged vf and pr
{
  // VSWRmeter = log10(VSWRi);
  VSWRmeter = VSWRi * 100;
  GaugeValue = map(VSWRmeter, 100, 500, 1, 100);
  if (VSWRmeter > 500)
  {
    GaugeValue = 100;
  }
  if (VSWRmeter > 200) // if vswr is >2:1 go red
  {
    Serial.print("j2.pco="); // set text colour pco to Red
    Serial.print(63488);
    Serial.write(0xff); // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("j2.val=");  // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.print(GaugeValue); // This is the value you want to send to that object and atribute mention before.
    Serial.write(0xff);       // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }

  else
  {

    Serial.print("j2.pco="); // set text colour pco to green
    Serial.print(1024);
    Serial.write(0xff); // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("j2.val=");  //
    Serial.print(GaugeValue); //
    Serial.write(0xff);       //
    Serial.write(0xff);
    Serial.write(0xff);
  }
  return;
}

int AnalogMeterWrite()
{
  VSWRi = VSWRi * 10000;
  //constrain (VSWRi, 1, 99);
  am = map(VSWRi, 10000, 100000, 0, 255);

  analogWrite(AnalogMeterPin, am); //  analogWrite values from 0 to 255
  VSWRi = VSWRi / 10000;
  return;
}

int VSWRWrite()
{
  VSWRValue = VSWRi * 10;
  // using nextion Xfloat widget. Send the value in the same way as number, but you need to change the parameters "vvs0" and "vvs1" on nextion editor,
  //so the widget adjust the value.
  //For example if you wanna show the value 2.53, multiply it by 100 and then set the parameter "vvs0" to 1 and "vvs1" to 2.

  // VSWRValue = map (VSWRi, 1000, 99000, 10, 990);// to sort out int float values in integer dispaly value

  Serial.print("x4.val="); // write abs value
  Serial.print(VSWRValue); //
  Serial.write(0xff);      //
  Serial.write(0xff);
  Serial.write(0xff);
  return;
}

int PowerDisplayWatts() // power values only no progress bar
{

  if (Vf > 270)
  {
    Serial.print("t2.txt="); // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.print("\"");      // Since we are sending text we need to send double quotes before and after the actual text.
    Serial.print("ON AIR");  // This is the text we want to send to that object and atribute mention before.
    Serial.print("\"");      // Since we are sending text we need to send double quotes before and after the actual text.
    Serial.write(0xff);      // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    PowerValuef = map(Pfwdw, 0, 1000, 0, 1000);
    Serial.print("n2.val=");   //
    Serial.print(PowerValuef); //
    Serial.write(0xff);        //
    Serial.write(0xff);
    Serial.write(0xff);
  }

  else
  {
    Serial.print("t2.txt="); // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.print("\"");      // Since we are sending text we need to send double quotes before and after the actual text.
    Serial.print("Receive"); // This is the text we want to send to that object and atribute mention before.
    Serial.print("\"");      // Since we are sending text we need to send double quotes before and after the actual text.
    Serial.write(0xff);      // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("n2.val="); // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.print(0);         // if in rx send 0 watts to n3
    Serial.write(0xff);      // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }

  return;
}

int SWRWarning() // High SWR condition display
{

  if (VSWRi > 2) // set red text in vswr
  {

    Serial.print("t0.pco"); // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.print(63488);
    Serial.write(0xff); // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);

    Serial.print("t0.txt="); // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
    Serial.print("\"");      // Since we are sending text we need to send double quotes before and after the actual text.
    Serial.print("SWR!!");   // This is the text we want to send to that object and atribute mention before.
    Serial.print("\"");      // Since we are sending text we need to send double quotes before and after the actual text.
    Serial.write(0xff);      // We always have to send this three lines after each command sent to the nextion display.
    Serial.write(0xff);
    Serial.write(0xff);
  }
  else // leave as initial value white
    Serial.print("t0.pco=");
  Serial.print(65535);
  Serial.write(0xff); // We always have to send this three lines after each command sent to the nextion display.
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.print("t0.txt="); // This is sent to the nextion display to set what object name (before the dot) and what atribute (after the dot) are you going to change.
  Serial.print("\"");      // Since we are sending text we need to send double quotes before and after the actual text.
  Serial.print("VSWR");    // This is the text we want to send to that object and atribute mention before.
  Serial.print("\"");      // Since we are sending text we need to send double quotes before and after the actual text.
  Serial.write(0xff);      // We always have to send this three lines after each command sent to the nextion display.
  Serial.write(0xff);
  Serial.write(0xff);

  return;
}
*/

void LCDDisplay() //
{
  // print ad values
  lcd.setCursor(0, 0); // go to Column, Row
  lcd.print("adf ");
  lcd.setCursor(4, 0);
  lcd.print(Vf, 0);
  lcd.setCursor(10, 0); // go to Column, Row
  lcd.print("adr ");
  lcd.setCursor(14, 0);
  lcd.print(Vr, 0);

  lcd.setCursor(0, 1); // go to Column, Row
  lcd.print("FwdBm ");
  lcd.setCursor(6, 1);
  lcd.print(Pfwd, 1);
  lcd.setCursor(0, 2); // go to Column, Row
  lcd.print("RvdBm ");
  lcd.setCursor(6, 2);
  lcd.print(Prev, 1);

  lcd.setCursor(14, 1); // go to Column, Row
  lcd.print("     ");
  lcd.setCursor(14, 1);
  lcd.print(Pfwdw, 1);
  lcd.setCursor(19, 1); // go to Column, Row
  lcd.print("W");

  lcd.setCursor(14, 2); // go to Column, Row
  lcd.print("     ");
  lcd.setCursor(14, 2);
  lcd.print(Prevw, 1);
  lcd.setCursor(19, 2); // go to Column, Row
  lcd.print("W");

  lcd.setCursor(0, 3); // go to Column, Row
  lcd.print("           ");
  lcd.setCursor(0, 3); // go to Column, Row
  lcd.print("VSWRi= ");
  lcd.print(VSWRi, 1);

  lcd.setCursor(11, 3); // go to Column, Row
  lcd.print("Dir ");
  lcd.print(directivity, 1);

  // compute directivirt

  return;
}



void setup()
{
  Serial.begin(9600);          // Nextion baud rate must match - nextion defaults to 9600 if not set in nextion // other speeds dont work!!
  Serial.print("baud=256000"); //Display nextion prende questo comando solo se acceso in contemporanea con arduino
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.end();
  delay(100);
  Serial.begin(256000); // seems to be max that works. Wanted to reduce display lag for swr tuning and maximise updates

  lcd.init();          //  initialize the LCD
  lcd.begin(20, 4);    // defines the column row of lcd NB MUST COME AFTER INIT
  lcd.backlight();     // Turn on the blacklight
  lcd.clear();         // clear all
  lcd.setCursor(0, 0); // go to Column, Row
  lcd.print("ATU");    // displays text
  lcd.setCursor(0, 1); // go to Column, Row
  lcd.print("G8KWX "); // displays text
  lcd.setCursor(0, 3); // go to Column, Row
  lcd.print(" ");      // displays text
  delay(10);           // delay  n milli seconds
  lcd.clear();         // clear all

  pinMode(2, OUTPUT);              // Ant relay high for ant 2
  pinMode(3, OUTPUT);              //Ant relay bypass / atu high for bypass
  digitalWrite(3, LOW);            // Set bypass off
  digitalWrite(2, LOW);            // Set ANT 1 by default
  pinMode(AnalogMeterPin, OUTPUT); // sets the pin as output for anlog meter
  analogWrite(AnalogMeterPin, 0);  // set to zero initially
}

void loop()

{
  Computeimmediatevswr(); // read immediate and calculate instantaneous vswr
                          // VSWRWrite();//write vswri to X4
                          // PowerDisplayWatts(); // write integer data to power windows and sliders
  //SWRWarning();
  // VSWRMeterWrite(); // immediate vswri to sliderj2
  LCDDisplay(); // used for devlelopment / re load libraries if needed
  //AnalogMeterWrite();// pwm for analog meter if needed readoutComputePower

}

/*
  Serial.print("AD Fwd VSWRi Pfwd ");
  Serial.print(adf);
  Serial.print(" ");
  Serial.print(VSWRi);
  Serial.print(" ");
  Serial.print(Pfwd);
  Serial.println();
*/

// Called functions to tidy up program



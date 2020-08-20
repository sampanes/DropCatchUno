/*

PIN 2   RX (goes to TX on BT)
PIN 3   TX (goes to RX on BT, and goes to GND through voltage divider)
PIN 4   

PIN A4  SDA (same on LCD)
PIN A5  SCL (same on LCD)

PIN GND GND BT and GND LCD and GND 16-MUX. also TX to RX with voltage divided
PIN 5V  VCC BT and VCC LCD and VCC 16-MUX

PIN A0 used for 6 position switch input, tied to GND with 100K resistor
PIN A1 used as "Start" button, attached to GND with 10K resistor (5V to btn+, btn- to A1 directly, btn- to ground via 10k res)

*/

#define DEBUG 0
#include <SoftwareSerial.h> //FOR BLUETOOTH
#include <Wire.h>           //FOR LCD
#include <LiquidCrystal_I2C.h>  //FOR LCD

//-----------------------------------------------------------
//        LCD
//-----------------------------------------------------------
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); // FOR LCD


//-----------------------------------------------------------
//        BT
//-----------------------------------------------------------
SoftwareSerial mySerial(2, 3); // RX (goes to HC-05 TX), TX (goes to HC-05 RX) // FOR BLUETOOTH
String received_msg;            // for bluetooth received msg

//-----------------------------------------------------------
//        6 Position Switch
//-----------------------------------------------------------
int analogPin = A0; // reads switch resistance
int clickySwitchValue = 0; //Value read by A0

//-----------------------------------------------------------
//        16 Channel Mux
//-----------------------------------------------------------
const int SIG = 13;// SIG pin used to be 2  
const int EN = 8;// Enable pin used to be 7
const int controlPin[4] = {9, 10, 11, 12}; // 4 pins used for binary output, used to be 3 4 5 6
const int MUX_VCC = 7; //TURNED HIGH TO SUPPLY 5v?
const int muxTable[16][4] = {
  // s0, s1, s2, s3     channel
    {0,  0,  0,  0}, // 0
    {1,  0,  0,  0}, // 1
    {0,  1,  0,  0}, // 2
    {1,  1,  0,  0}, // 3
    {0,  0,  1,  0}, // 4
    {1,  0,  1,  0}, // 5
    {0,  1,  1,  0}, // 6
    {1,  1,  1,  0}, // 7
    {0,  0,  0,  1}, // 8
    {1,  0,  0,  1}, // 9
    {0,  1,  0,  1}, // 10
    {1,  1,  0,  1}, // 11
    {0,  0,  1,  1}, // 12
    {1,  0,  1,  1}, // 13
    {0,  1,  1,  1}, // 14
    {1,  1,  1,  1}  // 15
};

//-----------------------------------------------------------
//        sw: MODES
//-----------------------------------------------------------
int drop_mode_switch;       //SWITCH CASE SEE WHAT MODE IS SELECTED
int last_dms;               //Previous state, to see if LCD should be changed
int start_button_pin = A1;
int start_button_state = 0;
int start_button_123; //Attached to button, when 3, start using mode selected
#define MANUAL_MODE 0
#define EASIEST_MODE 1
#define FRESHMAN_MODE 2
#define SENIOR_MODE 3
#define MASTER_MODE 4
#define IMPOSSIBLE 5
#define OBSCURE_SWITCH_VALUE 10
#define RANGE 45
#define res_val_MANUAL 90
#define res_val_EASIEST 236
#define res_val_FRESHMAN 510
#define res_val_SENIOR 695
#define res_val_MASTER 840
#define res_val_IMPOSSIBLE 1002

//-----------------------------------------------------------
//        sw: TIMING
//-----------------------------------------------------------
long int light_on_early_millis; //HOW EARLY TO TURN LIGHT ON
#define MANUAL_LIGHT_EARLY 500
#define EASIEST_LIGHT_EARLY 1000
#define FRESHMAN_LIGHT_EARLY 750
#define SENIOR_LIGHT_EARLY 500
#define MASTER_LIGHT_EARLY 250
#define IMPOSSIBLE_LIGHT_EARLY 175
#define RELAY_OFF_DELAY_MILLIS 150
int last_dropTime = 0;      //used in non-manual modes
int rand_int_dropDelay = 0; //used in non-manual modes
int stick_quantity = 0;
long int loopStart = 0;     //used for timing in start of loops
long int big_number = 0;

//-----------------------------------------------------------
//        sw: Stick Structure and alphanumeric map
//-----------------------------------------------------------
typedef struct {
  //char stickLetter;         //a, b, c, d, e, f, g
  long int stickDrop;          // millis when to activate latchlatch
  bool dropValueWasReceived;// some sticks don't get dropped, is this one getting dropped?
} stickStruct;
stickStruct mySticks[7];        //array of 7 struct objects

char letterOf[] = {'a', 'b', 'c', 'd', 'e', 'f', 'g'};

//-----------------------------------------------------------
//        SETUP
//-----------------------------------------------------------
void setup()
{
  //Arduino Setup
  //drop_mode_switch = MANUAL_MODE; //this is for now, should check switch values
  last_dms = -1;
  start_button_123 = 0;
  randomSeed(analogRead(2)); //seed random dropping for non-manual modes
  

  //-----------16 CH-MUX----------------------
  for(int i=0; i<4; i++)
  {
    pinMode(controlPin[i], OUTPUT);// set pin as output
    digitalWrite(controlPin[i], LOW); // set initial state as low, used to be HIGH     
  }
  pinMode(SIG, OUTPUT);     // set SIG pin as output
  digitalWrite(SIG, HIGH);  // set SIG sends what should be the output
                            // for low trigger relay it should be LOW
                            // for HIGH trigger device it should be HIGH
  pinMode(EN, OUTPUT);      // set EN pin as output
  digitalWrite(EN, LOW);    // set EN in (enable pin) Low to activate the chip

  pinMode(MUX_VCC, OUTPUT); // set an output pin as always high, makes it a VCC source.
  digitalWrite(MUX_VCC, HIGH);
  
  //-----------BLUETOOTH----------------------
  // Open serial (BLUETOOTH) communications and wait for port to open:
  Serial.begin(9600); 

  mySerial.begin(9600);
  mySerial.println("Hello, world?");  // send to BT

  //-----------LCD-----------------------------
  pinMode(13,OUTPUT);
    
  lcd.begin(16,2);               // initialize the lcd 

  lcd.home ();                   // go home
  lcd.print("Begin Program");  
  lcd.setCursor ( 0, 1 );        // go to the next line
  lcd.print ("Auth: Sampanes");
  
  delay ( 3000 );
}

void loop()
{
  //--------- LET's READ AND ANALYZE THE 6-POSITION SWITCH -------------
  clickySwitchValue = analogRead(analogPin);
  drop_mode_switch = getDropModeByValue(clickySwitchValue);
  if (drop_mode_switch != last_dms && drop_mode_switch != OBSCURE_SWITCH_VALUE)
  {
    #if DEBUG
    Serial.print(last_dms);
    Serial.print(" changed to ");
    Serial.println(drop_mode_switch);
    #endif
    printLCDMode(drop_mode_switch);
    last_dms = drop_mode_switch;
    start_button_123 = 0;
  }

  //--------- LET's READ AND ANALYZE THE START BUTTON -----------------
  start_button_state = analogRead(start_button_pin);
  if (start_button_state > 512)
  {
    start_button_123 = 2;
  }
  else
  {
    start_button_123 = start_button_123 * 2;
  }
  //--------- LET's START DROPPING STUFF IF START BUTTON RELEASED! ---------
  if (start_button_123 > 2)
  {
    #if DEBUG
    Serial.print("Start Button Activated! In Mode: ");
    Serial.println(drop_mode_switch);
    #endif
    start_button_123 = 0;
    switch(drop_mode_switch) {
      case MANUAL_MODE:
        #if DEBUG
        Serial.println("Manual Mode");
        #endif
        light_on_early_millis = MANUAL_LIGHT_EARLY;
        while (mySerial.available())
        {
          delay(2); // delay to allow byte to arrive in input buffer
          char c = char(mySerial.read());
          received_msg += c;
          #if DEBUG
          Serial.print(c);
          #endif
        }
        if (received_msg.length() > 0) {
          if (received_msg.charAt(0) == '[' && received_msg.charAt(1) != ']') {
            for (int ii = 0; ii < 7; ii++) {
              mySticks[ii] = newStickFromLetterAndString(letterOf[ii], received_msg);
              #if DEBUG
              Serial.print("\n");
              Serial.println(String(ii) + " : " + String(mySticks[ii].dropValueWasReceived) + " -> " + String(mySticks[ii].stickDrop));
              #endif
            }
            lcd.clear();
            lcd.print("GET READY");
            delay(3000);
            long int loopStart = millis() + light_on_early_millis;
            lcd.clear();
            #if DEBUG
            Serial.println("ii\tStart\tii:drop\tst+drp\tmillis");
            #endif
            while (anyToBeDropped(mySticks)){
              for (int ii = 0; ii < 7; ii++) {
                //-------------- EACH STICK (1-7) WILL BE CHECKED TO SEE IF IT DROPS ---------------
                if (mySticks[ii].dropValueWasReceived){
                  //------------ EACH DROPPING STICK WILL HAVE A WARNING SOUND+LIGHT ---------------
                  if (        millis() > loopStart + mySticks[ii].stickDrop - light_on_early_millis &&
                              millis() < loopStart + mySticks[ii].stickDrop) { //TEST IF TIME TO TURN ON LIGHT
                    channelControl(7-ii); //TODO turn on indicator light
                                          //pins 1 through 7 are LED/SOUND, 6 maps to 1, 5 maps to 2, .... 7 - ii = LED/SOUND #
                    #if DEBUG
                    lcd.setCursor(ii+1,0);
                    lcd.print(String(ii+1));
                    Serial.print(ii+1);
                    #endif
                  }
                  //------------ EACH DROPPING STICK GETS DROPPED AT THE RIGHT TIME ----------------
                  else if ( millis() > loopStart + mySticks[ii].stickDrop &&
                            millis() < loopStart + mySticks[ii].stickDrop + RELAY_OFF_DELAY_MILLIS) { //TEST IF TIME TO ACTIVATE RELAY AND TURN OFF LIGHT
                    channelControl(14-ii);  //TODO turn on relay to drop stick (this also turns off indicator light)
                                            //pins 8 thorugh 14 are sticks, 13 is stick 1, 12 is stick 2 ... 14 - ii = stick
                    #if DEBUG
                    lcd.setCursor(ii+1,1);
                    lcd.print(String(ii+1));
                    Serial.print(ii+1);
                    #endif
                  }
                  //------------ MUST ALSO DEACTIVATE THE RELAYS SO THE SIGNAL ISN'T HIGH FOREVER --
                  else if ( millis() > loopStart + mySticks[ii].stickDrop + RELAY_OFF_DELAY_MILLIS) { //TEST IF TIME TO DEACTIVATE RELAY
                    channelControl(0); //TODO turn off relay to drop stick
                    mySticks[ii].dropValueWasReceived = false;
                  }
                  //------------ CATCH ALL OF SOME SORT FROM MY DAYS OF UNIT TESTING ---------------
                  else { //TODO: CREATE CASES? after light on/off and after relay on/off
                    //mySticks[ii].dropValueWasReceived = false; THIS WAS MOVED IT WAS CAUSING PROBLEMS lol
                  }
                  #if DEBUG
                  Serial.println(String(ii) + "\t" + String(loopStart) + "\t" + String(mySticks[ii].stickDrop) + "\t" +String(loopStart + mySticks[ii].stickDrop) + "\t" + String(millis()));
                  #endif
                }
              }
            }
            lcd.clear();
            lcd.print("HOW'D YOU DO?!");
          } else {
            lcd.clear();
            lcd.print(received_msg);
          }
          received_msg="";
        } else {
          lcd.clear();
          lcd.print("Use App to send");
        }
        break;
      case EASIEST_MODE:
        light_on_early_millis = EASIEST_LIGHT_EARLY;
        last_dropTime = 0;      
        rand_int_dropDelay = 0; 
        for (int ii=0; ii < 7; ii++)
        {
          mySticks[ii].stickDrop = last_dropTime + rand_int_dropDelay;
          last_dropTime = last_dropTime + rand_int_dropDelay;
          rand_int_dropDelay = (int)random(2500,3500);
          mySticks[ii].dropValueWasReceived = true;
        }
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        lcd.clear();
        lcd.print("GET READY");
        delay(3000);
        loopStart = millis() + light_on_early_millis;
        lcd.clear();
        #if DEBUG
        Serial.println("ii\tStart\tii:drop\tst+drp\tmillis");
        #endif
        while (anyToBeDropped(mySticks)){
          for (int ii = 0; ii < 7; ii++) {
            //-------------- EACH STICK (1-7) WILL BE CHECKED TO SEE IF IT DROPS ---------------
            if (mySticks[ii].dropValueWasReceived){
              //------------ EACH DROPPING STICK WILL HAVE A WARNING SOUND+LIGHT ---------------
              if (        millis() > loopStart + mySticks[ii].stickDrop - light_on_early_millis &&
                    millis() < loopStart + mySticks[ii].stickDrop) { //TEST IF TIME TO TURN ON LIGHT
              channelControl(7-ii); //TODO turn on indicator light
                          //pins 1 through 7 are LED/SOUND, 6 maps to 1, 5 maps to 2, .... 7 - ii = LED/SOUND #
              #if DEBUG
              lcd.setCursor(ii+1,0);
              lcd.print(String(ii+1));
              Serial.print(ii+1);
              #endif
              }
              //------------ EACH DROPPING STICK GETS DROPPED AT THE RIGHT TIME ----------------
              else if ( millis() > loopStart + mySticks[ii].stickDrop &&
                  millis() < loopStart + mySticks[ii].stickDrop + RELAY_OFF_DELAY_MILLIS) { //TEST IF TIME TO ACTIVATE RELAY AND TURN OFF LIGHT
              channelControl(14-ii);  //TODO turn on relay to drop stick (this also turns off indicator light)
                          //pins 8 thorugh 14 are sticks, 13 is stick 1, 12 is stick 2 ... 14 - ii = stick
              #if DEBUG
              lcd.setCursor(ii+1,1);
              lcd.print(String(ii+1));
              Serial.print(ii+1);
              #endif
              }
              //------------ MUST ALSO DEACTIVATE THE RELAYS SO THE SIGNAL ISN'T HIGH FOREVER --
              else if ( millis() > loopStart + mySticks[ii].stickDrop + RELAY_OFF_DELAY_MILLIS) { //TEST IF TIME TO DEACTIVATE RELAY
              channelControl(0); //TODO turn off relay to drop stick
              mySticks[ii].dropValueWasReceived = false;
              }
              //------------ CATCH ALL OF SOME SORT FROM MY DAYS OF UNIT TESTING ---------------
              else { //TODO: CREATE CASES? after light on/off and after relay on/off
              //mySticks[ii].dropValueWasReceived = false; THIS WAS MOVED IT WAS CAUSING PROBLEMS lol
              }
              #if DEBUG
              Serial.println(String(ii) + "\t" + String(loopStart) + "\t" + String(mySticks[ii].stickDrop) + "\t" +String(loopStart + mySticks[ii].stickDrop) + "\t" + String(millis()));
              #endif
            }
          }
        }
        lcd.clear();
        lcd.print("HOW'D YOU DO?!");
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        break;
      case FRESHMAN_MODE:
        light_on_early_millis = FRESHMAN_LIGHT_EARLY;
        last_dropTime = 0;
        rand_int_dropDelay = 0;
        for (int ii=0; ii < 7; ii++)
        {
          mySticks[ii].stickDrop = last_dropTime + rand_int_dropDelay; //first drop at 0, from then on it is lastdrop + random
          last_dropTime = last_dropTime + rand_int_dropDelay;          //set prev to this one
          rand_int_dropDelay = (int)random(1500,4500);                 //generate new random
          mySticks[ii].dropValueWasReceived = true;           //be sure to let system know that this is good to drop
        }
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        lcd.clear();
        lcd.print("GET READY");
        delay(3000);
        loopStart = millis() + light_on_early_millis;
        lcd.clear();
        #if DEBUG
        Serial.println("ii\tStart\tii:drop\tst+drp\tmillis");
        #endif
        while (anyToBeDropped(mySticks)){
          for (int ii = 0; ii < 7; ii++) {
            //-------------- EACH STICK (1-7) WILL BE CHECKED TO SEE IF IT DROPS ---------------
            if (mySticks[ii].dropValueWasReceived){
              //------------ EACH DROPPING STICK WILL HAVE A WARNING SOUND+LIGHT ---------------
              if (        millis() > loopStart + mySticks[ii].stickDrop - light_on_early_millis &&
                    millis() < loopStart + mySticks[ii].stickDrop) { //TEST IF TIME TO TURN ON LIGHT
              channelControl(7-ii); //TODO turn on indicator light
                          //pins 1 through 7 are LED/SOUND, 6 maps to 1, 5 maps to 2, .... 7 - ii = LED/SOUND #
              #if DEBUG
              lcd.setCursor(ii+1,0);
              lcd.print(String(ii+1));
              Serial.print(ii+1);
              #endif
              }
              //------------ EACH DROPPING STICK GETS DROPPED AT THE RIGHT TIME ----------------
              else if ( millis() > loopStart + mySticks[ii].stickDrop &&
                  millis() < loopStart + mySticks[ii].stickDrop + RELAY_OFF_DELAY_MILLIS) { //TEST IF TIME TO ACTIVATE RELAY AND TURN OFF LIGHT
              channelControl(14-ii);  //TODO turn on relay to drop stick (this also turns off indicator light)
                          //pins 8 thorugh 14 are sticks, 13 is stick 1, 12 is stick 2 ... 14 - ii = stick
              #if DEBUG
              lcd.setCursor(ii+1,1);
              lcd.print(String(ii+1));
              Serial.print(ii+1);
              #endif
              }
              //------------ MUST ALSO DEACTIVATE THE RELAYS SO THE SIGNAL ISN'T HIGH FOREVER --
              else if ( millis() > loopStart + mySticks[ii].stickDrop + RELAY_OFF_DELAY_MILLIS) { //TEST IF TIME TO DEACTIVATE RELAY
              channelControl(0); //TODO turn off relay to drop stick
              mySticks[ii].dropValueWasReceived = false;
              }
              //------------ CATCH ALL OF SOME SORT FROM MY DAYS OF UNIT TESTING ---------------
              else { //TODO: CREATE CASES? after light on/off and after relay on/off
              //mySticks[ii].dropValueWasReceived = false; THIS WAS MOVED IT WAS CAUSING PROBLEMS lol
              }
              #if DEBUG
              Serial.println(String(ii) + "\t" + String(loopStart) + "\t" + String(mySticks[ii].stickDrop) + "\t" +String(loopStart + mySticks[ii].stickDrop) + "\t" + String(millis()));
              #endif
            }
          }
        }
        lcd.clear();
        lcd.print("HOW'D YOU DO?!");
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        break;
      case SENIOR_MODE:
        #if DEBUG
        Serial.println("Entered SENIOR MODE");
        #endif
        light_on_early_millis = SENIOR_LIGHT_EARLY;
        last_dropTime = 0;
        rand_int_dropDelay = 0;
        for (int ii=0; ii < 7; ii++)
        {
          mySticks[ii].dropValueWasReceived = false;
        }
        big_number = random(13245670,76543219);
        #if DEBUG
        Serial.print("Senior Mode Big Number: ");
        Serial.println(big_number);
        #endif
        stick_quantity = 0;
        for (int ii=0; ii < 8; ii++) //iterate through all digits, see line # current - 5
        {
          #if DEBUG
          Serial.println(big_number);
          #endif
          int stick = big_number%10 - 1;
          if(stick > -1 && stick < 7 && mySticks[stick].dropValueWasReceived == false)
          {
            stick_quantity = stick_quantity + 1;
            mySticks[stick].stickDrop = last_dropTime + rand_int_dropDelay;
            last_dropTime = last_dropTime + rand_int_dropDelay;
            rand_int_dropDelay = (int)random(1000,5000);
            mySticks[stick].dropValueWasReceived = true;
          }
          #if DEBUG
          else {
            Serial.print("Stick: ");
            Serial.print(stick);
            Serial.println(" not a valid stick");
          }
          #endif
          big_number = (long int)big_number / (long int)10;
        }
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        lcd.clear();
        lcd.print("GET READY");
        lcd.setCursor(0,1);
        //_________0123456789012345_______
        lcd.print("Catch   sticks");
        lcd.setCursor(6,2);
        lcd.print(stick_quantity); 
        delay(3000);
        loopStart = millis() + light_on_early_millis;
        lcd.clear();
        #if DEBUG
        Serial.println("ii\tStart\tii:drop\tst+drp\tmillis");
        #endif
        while (anyToBeDropped(mySticks)){
          for (int ii = 0; ii < 7; ii++) {
            //-------------- EACH STICK (1-7) WILL BE CHECKED TO SEE IF IT DROPS ---------------
            if (mySticks[ii].dropValueWasReceived){
              //------------ EACH DROPPING STICK WILL HAVE A WARNING SOUND+LIGHT ---------------
              if (        millis() > loopStart + mySticks[ii].stickDrop - light_on_early_millis &&
                    millis() < loopStart + mySticks[ii].stickDrop) { //TEST IF TIME TO TURN ON LIGHT
              channelControl(7-ii); //TODO turn on indicator light
                          //pins 1 through 7 are LED/SOUND, 6 maps to 1, 5 maps to 2, .... 7 - ii = LED/SOUND #
              #if DEBUG
              lcd.setCursor(ii+1,0);
              lcd.print(String(ii+1));
              Serial.print(ii+1);
              #endif
              }
              //------------ EACH DROPPING STICK GETS DROPPED AT THE RIGHT TIME ----------------
              else if ( millis() > loopStart + mySticks[ii].stickDrop &&
                  millis() < loopStart + mySticks[ii].stickDrop + RELAY_OFF_DELAY_MILLIS) { //TEST IF TIME TO ACTIVATE RELAY AND TURN OFF LIGHT
              channelControl(14-ii);  //TODO turn on relay to drop stick (this also turns off indicator light)
                          //pins 8 thorugh 14 are sticks, 13 is stick 1, 12 is stick 2 ... 14 - ii = stick
              #if DEBUG
              lcd.setCursor(ii+1,1);
              lcd.print(String(ii+1));
              Serial.print(ii+1);
              #endif
              }
              //------------ MUST ALSO DEACTIVATE THE RELAYS SO THE SIGNAL ISN'T HIGH FOREVER --
              else if ( millis() > loopStart + mySticks[ii].stickDrop + RELAY_OFF_DELAY_MILLIS) { //TEST IF TIME TO DEACTIVATE RELAY
              channelControl(0); //TODO turn off relay to drop stick
              mySticks[ii].dropValueWasReceived = false;
              }
              //------------ CATCH ALL OF SOME SORT FROM MY DAYS OF UNIT TESTING ---------------
              else { //TODO: CREATE CASES? after light on/off and after relay on/off
              //mySticks[ii].dropValueWasReceived = false; THIS WAS MOVED IT WAS CAUSING PROBLEMS lol
              }
              #if DEBUG
              Serial.println(String(ii) + "\t" + String(loopStart) + "\t" + String(mySticks[ii].stickDrop) + "\t" +String(loopStart + mySticks[ii].stickDrop) + "\t" + String(millis()));
              #endif
            }
          }
        }
        lcd.clear();
        lcd.print("HOW'D YOU DO?!");
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        break;
      case MASTER_MODE:
        #if DEBUG
          Serial.println("Entered MASTER MODE");
        #endif
        light_on_early_millis = MASTER_LIGHT_EARLY;
        last_dropTime = 0;
        rand_int_dropDelay = 0;
        for (int ii=0; ii < 7; ii++)
        {
          mySticks[ii].dropValueWasReceived = false;
        }
        big_number = random(1324567000,7654321999);
        #if DEBUG
        Serial.print("Senior Mode Big Number: ");
        Serial.println(big_number);
        #endif
        stick_quantity = 0;
        for (int ii=0; ii < 8; ii++) //iterate through all digits, see line # current - 5
        {
          #if DEBUG
          Serial.println(big_number);
          #endif
          int stick = big_number%10 - 1;
          if(stick > -1 && stick < 7 && mySticks[stick].dropValueWasReceived == false)
          {
            stick_quantity = stick_quantity + 1;
            mySticks[stick].stickDrop = last_dropTime + rand_int_dropDelay;
            last_dropTime = last_dropTime + rand_int_dropDelay;
            rand_int_dropDelay = (int)random(700,5750);
            mySticks[stick].dropValueWasReceived = true;
          }
          #if DEBUG
          else {
            Serial.print("Stick: ");
            Serial.print(stick);
            Serial.println(" not a valid stick");
          }
          #endif
          big_number = (long int)big_number / (long int)10;
        }
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        lcd.clear();
        lcd.print("GET READY");
        lcd.setCursor(0,1);
        //_________0123456789012345_______
        lcd.print("Catch   sticks");
        lcd.setCursor(6,2);
        lcd.print(stick_quantity); 
        delay(3000);
        loopStart = millis() + light_on_early_millis;
        lcd.clear();
        #if DEBUG
        Serial.println("ii\tStart\tii:drop\tst+drp\tmillis");
        #endif
        while (anyToBeDropped(mySticks)){
          for (int ii = 0; ii < 7; ii++) {
            //-------------- EACH STICK (1-7) WILL BE CHECKED TO SEE IF IT DROPS ---------------
            if (mySticks[ii].dropValueWasReceived){
              //------------ EACH DROPPING STICK WILL HAVE A WARNING SOUND+LIGHT ---------------
              if (        millis() > loopStart + mySticks[ii].stickDrop - light_on_early_millis &&
                    millis() < loopStart + mySticks[ii].stickDrop) { //TEST IF TIME TO TURN ON LIGHT
              channelControl(7-ii); //TODO turn on indicator light
                          //pins 1 through 7 are LED/SOUND, 6 maps to 1, 5 maps to 2, .... 7 - ii = LED/SOUND #
              #if DEBUG
              lcd.setCursor(ii+1,0);
              lcd.print(String(ii+1));
              Serial.print(ii+1);
              #endif
              }
              //------------ EACH DROPPING STICK GETS DROPPED AT THE RIGHT TIME ----------------
              else if ( millis() > loopStart + mySticks[ii].stickDrop &&
                  millis() < loopStart + mySticks[ii].stickDrop + RELAY_OFF_DELAY_MILLIS) { //TEST IF TIME TO ACTIVATE RELAY AND TURN OFF LIGHT
              channelControl(14-ii);  //TODO turn on relay to drop stick (this also turns off indicator light)
                          //pins 8 thorugh 14 are sticks, 13 is stick 1, 12 is stick 2 ... 14 - ii = stick
              #if DEBUG
              lcd.setCursor(ii+1,1);
              lcd.print(String(ii+1));
              Serial.print(ii+1);
              #endif
              }
              //------------ MUST ALSO DEACTIVATE THE RELAYS SO THE SIGNAL ISN'T HIGH FOREVER --
              else if ( millis() > loopStart + mySticks[ii].stickDrop + RELAY_OFF_DELAY_MILLIS) { //TEST IF TIME TO DEACTIVATE RELAY
              channelControl(0); //TODO turn off relay to drop stick
              mySticks[ii].dropValueWasReceived = false;
              }
              //------------ CATCH ALL OF SOME SORT FROM MY DAYS OF UNIT TESTING ---------------
              else { //TODO: CREATE CASES? after light on/off and after relay on/off
              //mySticks[ii].dropValueWasReceived = false; THIS WAS MOVED IT WAS CAUSING PROBLEMS lol
              }
              #if DEBUG
              Serial.println(String(ii) + "\t" + String(loopStart) + "\t" + String(mySticks[ii].stickDrop) + "\t" +String(loopStart + mySticks[ii].stickDrop) + "\t" + String(millis()));
              #endif
            }
          }
        }
        lcd.clear();
        lcd.print("HOW'D YOU DO?!");
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        break;
      case IMPOSSIBLE:
        #if DEBUG
          Serial.println("Entered IMPOSSIBLE MODE");
        #endif
        light_on_early_millis = IMPOSSIBLE_LIGHT_EARLY;
        last_dropTime = 0;
        rand_int_dropDelay = 0;
        for (int ii=0; ii < 7; ii++)
        {
          mySticks[ii].dropValueWasReceived = false;
        }
        big_number = random(13245670000,76543219999);
        #if DEBUG
        Serial.print("Senior Mode Big Number: ");
        Serial.println(big_number);
        #endif
        stick_quantity = 0;
        for (int ii=0; ii < 8; ii++) //iterate through all digits, see line # current - 5
        {
          #if DEBUG
          Serial.println(big_number);
          #endif
          int stick = big_number%10 - 1;
          if(stick > -1 && stick < 7 && mySticks[stick].dropValueWasReceived == false)
          {
            stick_quantity = stick_quantity + 1;
            mySticks[stick].stickDrop = last_dropTime + rand_int_dropDelay;
            last_dropTime = last_dropTime + rand_int_dropDelay;
            rand_int_dropDelay = (int)random(400,750);
            mySticks[stick].dropValueWasReceived = true;
          }
          #if DEBUG
          else {
            Serial.print("Stick: ");
            Serial.print(stick);
            Serial.println(" not a valid stick");
          }
          #endif
          big_number = (long int)big_number / (long int)10;
        }
        for (int ii = 0; ii < 7; ii++)
        {
          if (mySticks[ii].dropValueWasReceived == false)
          {
            stick_quantity = stick_quantity + 1;
            mySticks[ii].stickDrop = last_dropTime + rand_int_dropDelay;
            last_dropTime = last_dropTime + rand_int_dropDelay;
            rand_int_dropDelay = (int)random(400,750);
            mySticks[ii].dropValueWasReceived = true;
          }
        }
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        lcd.clear();
        lcd.print("GET READY");
        lcd.setCursor(0,1);
        //_________0123456789012345_______
        lcd.print("Catch   sticks");
        lcd.setCursor(6,2);
        lcd.print(stick_quantity); 
        delay(3000);
        loopStart = millis() + light_on_early_millis;
        lcd.clear();
        #if DEBUG
        Serial.println("ii\tStart\tii:drop\tst+drp\tmillis");
        #endif
        while (anyToBeDropped(mySticks)){
          for (int ii = 0; ii < 7; ii++) {
            //-------------- EACH STICK (1-7) WILL BE CHECKED TO SEE IF IT DROPS ---------------
            if (mySticks[ii].dropValueWasReceived){
              //------------ EACH DROPPING STICK WILL HAVE A WARNING SOUND+LIGHT ---------------
              if (        millis() > loopStart + mySticks[ii].stickDrop - light_on_early_millis &&
                    millis() < loopStart + mySticks[ii].stickDrop) { //TEST IF TIME TO TURN ON LIGHT
              channelControl(7-ii); //TODO turn on indicator light
                          //pins 1 through 7 are LED/SOUND, 6 maps to 1, 5 maps to 2, .... 7 - ii = LED/SOUND #
              #if DEBUG
              lcd.setCursor(ii+1,0);
              lcd.print(String(ii+1));
              Serial.print(ii+1);
              #endif
              }
              //------------ EACH DROPPING STICK GETS DROPPED AT THE RIGHT TIME ----------------
              else if ( millis() > loopStart + mySticks[ii].stickDrop &&
                  millis() < loopStart + mySticks[ii].stickDrop + RELAY_OFF_DELAY_MILLIS) { //TEST IF TIME TO ACTIVATE RELAY AND TURN OFF LIGHT
              channelControl(14-ii);  //TODO turn on relay to drop stick (this also turns off indicator light)
                          //pins 8 thorugh 14 are sticks, 13 is stick 1, 12 is stick 2 ... 14 - ii = stick
              #if DEBUG
              lcd.setCursor(ii+1,1);
              lcd.print(String(ii+1));
              Serial.print(ii+1);
              #endif
              }
              //------------ MUST ALSO DEACTIVATE THE RELAYS SO THE SIGNAL ISN'T HIGH FOREVER --
              else if ( millis() > loopStart + mySticks[ii].stickDrop + RELAY_OFF_DELAY_MILLIS) { //TEST IF TIME TO DEACTIVATE RELAY
              channelControl(0); //TODO turn off relay to drop stick
              mySticks[ii].dropValueWasReceived = false;
              }
              //------------ CATCH ALL OF SOME SORT FROM MY DAYS OF UNIT TESTING ---------------
              else { //TODO: CREATE CASES? after light on/off and after relay on/off
              //mySticks[ii].dropValueWasReceived = false; THIS WAS MOVED IT WAS CAUSING PROBLEMS lol
              }
              #if DEBUG
              Serial.println(String(ii) + "\t" + String(loopStart) + "\t" + String(mySticks[ii].stickDrop) + "\t" +String(loopStart + mySticks[ii].stickDrop) + "\t" + String(millis()));
              #endif
            }
          }
        }
        lcd.clear();
        lcd.print("HOW'D YOU DO?!");
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        break;
      default:
        break;
    }//end switch
    //TODO: OUTSIDE OF SWITCH CASE
  }
}

/*
 * @brief Parse segment of input from Bluetooth device and create stick struct
 * @param stickChar (character representing a stick), recieved_msg (the rest of the bluetooth transmission)
 * @return stickStruct (return a struct representing a stick with various properties)
 */
stickStruct newStickFromLetterAndString(char stickChar, String received_msg){
  stickStruct tempStickItem;
  int letter_found_at = received_msg.indexOf(stickChar);
  if (letter_found_at != -1){                               // we found the letter we were looking for
    tempStickItem.dropValueWasReceived = true;
    int comma_found_at = received_msg.indexOf(',',letter_found_at + 1);
    if (comma_found_at != -1) {                             // we found a comma after that letter
      String time_temp = received_msg.substring(letter_found_at + 1, comma_found_at);
      long int dropTime = time_temp.toInt();
      tempStickItem.stickDrop = dropTime;
    } else {                                                // if -1, then there is no comma after letter, prolly last like ...c32523]
      int end_bracket_found_at = received_msg.indexOf(']',letter_found_at + 1);
      if (end_bracket_found_at != letter_found_at + 1) {    //there is at least one digit between letter and ]
        String time_temp = received_msg.substring(letter_found_at + 1, end_bracket_found_at);
        long int dropTime = time_temp.toInt();
        tempStickItem.stickDrop = dropTime;
      } else {                                              //the letter is immediately followed by ]
        tempStickItem.stickDrop = 0;
      }
    }
  } else {                                                  // if -1, then stick letter was not sent via bluetooth
    tempStickItem.stickDrop   = -1;
    tempStickItem.dropValueWasReceived = false;
  }
  return tempStickItem;
}

/*
 * @brief See if any sticks are left To Be Dropped
 * @param mySticks[7] (an array of structs, representing sticks)
 * @return boolean value answering "Are there sticks left to be dropped?"
 */
bool anyToBeDropped(stickStruct mySticks[7]) {
  for (int ii = 0; ii < 7; ii++) {
    if (mySticks[ii].dropValueWasReceived) {
      return true;
    }
  }
  return false;
}

/*
 * @brief change LCD printout based on mode selected
 * @param input (0-5 manual, easiest... to hardest)
 * @return void
 */
void printLCDMode(int input) {
  switch(input)
  {
    case MANUAL_MODE:
      lcd.clear();                  //clear screen
      lcd.home();                   //go home
      //_________0123456789012345_______
      lcd.print("Manual Mode");  
      lcd.setCursor ( 0, 1 );       // go to the next line
      lcd.print("Start = BT");
      break;
    case EASIEST_MODE:
      lcd.clear();                  //clear screen
      lcd.home();                   //go home
      //_________0123456789012345_______
      lcd.print("1: Easiest Mode");  
      lcd.setCursor ( 0, 1 );       // go to the next line
      lcd.print("Start to begin");
      break;
    case FRESHMAN_MODE:
      lcd.clear();                  //clear screen
      lcd.home();                   //go home
      //_________0123456789012345_______
      lcd.print("2: Freshman Mode");  
      lcd.setCursor ( 0, 1 );       // go to the next line
      lcd.print("Start to begin");
      break;
    case SENIOR_MODE:
      lcd.clear();                  //clear screen
      lcd.home();                   //go home
      //_________0123456789012345_______
      lcd.print("3: Senior Mode");  
      lcd.setCursor ( 0, 1 );       // go to the next line
      lcd.print("Start to begin");
      break;
    case MASTER_MODE:
      lcd.clear();                  //clear screen
      lcd.home();                   //go home
      //_________0123456789012345_______
      lcd.print("4: Master Mode");  
      lcd.setCursor ( 0, 1 );       // go to the next line
      lcd.print("Start to begin");
      break;
    case IMPOSSIBLE:
      lcd.clear();                  //clear screen
      lcd.home();                   //go home
      //_________0123456789012345_______
      lcd.print("IMPOSSIBLE MODE");  
      lcd.setCursor ( 0, 1 );       // go to the next line
      lcd.print("not even fun...");
      break;
  }
}

/*
 * @brief TODO
 * @param input TODO
 * @return 1 TODO
 */
int getDropModeByValue(int input) {
  /*#if DEBUG
  Serial.println(input);
  #endif*/
  if ((input > -1) && (input < res_val_MANUAL + RANGE))
  {
    return MANUAL_MODE;
  }
  else if ((input > res_val_EASIEST - RANGE) && (input < res_val_EASIEST + RANGE))
  {
    return EASIEST_MODE;
  }
  else if ((input > res_val_FRESHMAN - RANGE) && (input < res_val_FRESHMAN + RANGE))
  {
    return FRESHMAN_MODE;
  }
  else if ((input > res_val_SENIOR - RANGE) && (input < res_val_SENIOR + RANGE))
  {
    return SENIOR_MODE;
  }
  else if ((input > res_val_MASTER - RANGE) && (input < res_val_MASTER + RANGE))
  {
    return MASTER_MODE;
  }
  else if ((input > res_val_IMPOSSIBLE - RANGE) && (input < res_val_IMPOSSIBLE + RANGE))
  {
    return IMPOSSIBLE;
  }
  return OBSCURE_SWITCH_VALUE;
}

/*
 * @brief Control the MUX outputs (one at a time)
 * @param muxOutput (integer, 1-7 are relays, 8-14 are alarms, alerts, warnings)
 * @return void
 */
void channelControl(int muxOutput)
{
  digitalWrite(controlPin[0], muxTable[muxOutput][0]);
  digitalWrite(controlPin[1], muxTable[muxOutput][1]);
  digitalWrite(controlPin[2], muxTable[muxOutput][2]);
  digitalWrite(controlPin[3], muxTable[muxOutput][3]);
  #if DEBUG
  Serial.print(muxOutput);
  Serial.print (": ");
  Serial.print(muxTable[muxOutput][3]);
  Serial.print(muxTable[muxOutput][2]);
  Serial.print(muxTable[muxOutput][1]);
  Serial.println(muxTable[muxOutput][0]);
  #endif
}

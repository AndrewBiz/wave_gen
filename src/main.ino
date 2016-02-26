/* AD9850 DDS based wave generator
   inspired by project of Richard Visokey AD7C - www.ad7c.com
   Andrew Bizyaev (ANB) github.com/andrewbiz
*/

#include <LiquidCrystal.h>
#include <EEPROM.h>
#include <Logging.h>

#define LOGLEVEL LOG_LEVEL_DEBUG //see Logging.h for options
#define LOG_PRINT_TS true  // print time stamp in logging
#define LOG_AUTO_LN  true  // print auto LN (CR) after each call

#define WAVE_GEN_VERSION "0.9.0"
#define DDS_DEVICE "AD9850"

// LCD keypad ARDUINO pins mapping:
#define D4     4 //LCD data
#define D5     5 //LCD data
#define D6     6 //LCD data
#define D7     7 //LCD data
#define RS     8 //LCD RS
#define ENABLE 9 //LCD ENABLE
#define D10   10 // LCD Backlight control

LiquidCrystal lcd( RS, ENABLE, D4, D5, D6, D7 );

// LCD keypad button pins mapping
#define btnNONE  0 // originally btnNONE
#define btnMEMO1 1 // originally btnSELECT
#define btnMEMO2 2 // originally btnLEFT
#define btnUP    3 // originally btnUP
#define btnDOWN  4 // originally btnDOWN
#define btnDELTA 5 // originally btnRIGHT
#define btnERROR 99 //

// AD9850 module pins
#define DATA  A1 // connect to serial data load pin (DATA)
#define W_CLK A2 // connect to word load clock pin (CLK)
#define FQ_UD A3 // connect to freq update pin (FQ)
#define RESET A4 // connect to reset pin (RST)

#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }

#define MIN_FREQUENCY 1
#define MAX_FREQUENCY 20000000
#define DEF_FREQUENCY 1000 //default freq
#define MAX_FREQUENCY_INDEX 6
#define DEF_FREQUENCY_INDEX 3 //default freq index
#define SAVE_TO_M0_INTERVAL 7000 //7 sec after the key was pressed current frequency will be saved to EEPROM
#define LONG_KEY_PRESS_INTERVAL 1000 //1 sec is considered long keypress
#define REPEAT_KEY_PRESS_INTERVAL 300 //0,3 sec is considered to start autorepeat

struct MemoryRecord
{
  uint32_t frequency;
  byte frequency_delta_index;
};

uint32_t frequency = DEF_FREQUENCY; //frequency of VFO
byte frequency_delta_index = DEF_FREQUENCY_INDEX;
const uint32_t frequency_delta[] = { 1, 10, 100, 1000, 10000, 100000, 1000000 };
const byte EEPROM_address[] = { 0, 5, 10 };
bool need_save_to_m0 = false;
bool state_btn_pressed = false;
bool state_btn_repeat = false;
byte btn_pressed = btnNONE;
unsigned long time_btn_pressed = 0;
unsigned long time_btn_released = 0;

void setup()
{
  Log.Init(LOGLEVEL, 38400L, LOG_PRINT_TS, LOG_AUTO_LN);
  Log.Info(F("Starting DDS " DDS_DEVICE " Wave_Gen, version " WAVE_GEN_VERSION));

  // LCD setup
  lcd.begin(16, 2);
  pinMode(D10, OUTPUT);   // backlight pin
  digitalWrite(D10, HIGH); // backlight is ON when reset

  // Initial screen
  LCD_show_line(0, F("AndrewBiz (c)"));
  LCD_show_line(1, F("Wave_Gen v" WAVE_GEN_VERSION));
  delay(1000);

  init_memory();

  //read from default memory slot
  read_from_memory(0);

  LCD_show_frequency();
  LCD_show_frequency_delta(" ");

  // setup AD9850
  pinMode(DATA,  OUTPUT);
  pinMode(W_CLK, OUTPUT);
  pinMode(FQ_UD, OUTPUT);
  pinMode(RESET, OUTPUT);
  pulseHigh(RESET);
  pulseHigh(FQ_UD);  // this pulse enables serial mode on the AD9850 - Datasheet page 12.
  // sent initial frequency to DDS device
  set_frequency();
  set_frequency(); //experimentally found out - need to set 2 times
}

void loop()
{
  delay(50);

  if( state_btn_pressed ){
    //key was being pressed in the last cycle
    switch(read_LCD_buttons()){
      case btnUP:
        // the key is kept pressed by the user
        if( (millis() - time_btn_pressed) >= REPEAT_KEY_PRESS_INTERVAL){
          state_btn_repeat = true;
          Log.Debug(F("Key btnUP repeat"));
          LCD_show_frequency_delta("+");
          frequency_inc();
          LCD_show_frequency();
        }
        break; //casebtnUP

      case btnDOWN:
        // the key is kept pressed by the user
        if( (millis() - time_btn_pressed) >= REPEAT_KEY_PRESS_INTERVAL){
          state_btn_repeat = true;
          Log.Debug(F("Key btnDOWN repeat"));
          LCD_show_frequency_delta("-");
          frequency_dec();
          LCD_show_frequency();
        }
        break; // case btnDOWN

      case btnNONE:
        // we have the key was pressed down and then released
        state_btn_pressed = false;
        time_btn_released = millis();
        switch(btn_pressed){
          case btnMEMO1:
            if( (time_btn_released - time_btn_pressed) < LONG_KEY_PRESS_INTERVAL){
              // it was short key press
              Log.Debug(F("Key btnMEMO1 short pressed"));
              read_from_memory(1);
              set_frequency();
              need_save_to_m0 = true;
              LCD_show_frequency();
              LCD_show_frequency_delta(" ");
            }
            else {
              // it was long key press
              Log.Debug(F("Key btnMEMO1 long pressed"));
              save_to_memory(1);
            }
            break;

          case btnMEMO2:
            if( (time_btn_released - time_btn_pressed) < LONG_KEY_PRESS_INTERVAL){
              // it was short key press
              Log.Debug(F("Key btnMEMO2 short pressed"));
              read_from_memory(2);
              set_frequency();
              need_save_to_m0 = true;
              LCD_show_frequency();
              LCD_show_frequency_delta(" ");
            }
            else {
              // it was long key press
              Log.Debug(F("Key btnMEMO2 long pressed"));
              save_to_memory(2);
            }
            break;

          case btnUP:
            if(!state_btn_repeat){ // in repeate mode will not trigger btn unpress function
              Log.Debug(F("Key btnUP pressed"));
              LCD_show_frequency_delta("+");
              frequency_inc();
              LCD_show_frequency();
            }
            break;

          case btnDOWN:
            if(!state_btn_repeat){ // in repeate mode will not trigger btn unpress function
              Log.Debug(F("Key btnDOWN pressed"));
              LCD_show_frequency_delta("-");
              frequency_dec();
              LCD_show_frequency();
            }
            break;

          case btnDELTA:
            Log.Debug(F("Key btnDELTA pressed"));
            frequency_delta_index++;
            if (frequency_delta_index > MAX_FREQUENCY_INDEX) {
              frequency_delta_index = 0;
            }
            Log.Info(F("Delta = %l Hz"), frequency_delta[frequency_delta_index]);
            LCD_show_frequency_delta(" ");
            need_save_to_m0 = true;
            break;

          case btnERROR:
            Log.Debug(F("Key error"));
            lcd.print(F("BTN ERROR!"));
            break;

        } // switch
        state_btn_repeat = false;
        break; // case btnNONE
    } // switch global
  }
  else { // no keys was pressed in the last cycle
    // saving to the memory M0 if needed
    if(((millis() - time_btn_released) > SAVE_TO_M0_INTERVAL) and need_save_to_m0){
      save_to_memory(0);
      need_save_to_m0 = false;
    }
    btn_pressed = read_LCD_buttons();
    if( (btn_pressed != btnNONE) and (btn_pressed != btnERROR) ){
      state_btn_pressed = true;
      time_btn_pressed = millis();
    }
  }
} // loop

// set frequency into DDS_DEVICE
void set_frequency()
{
  // datasheet page 8: frequency = <sys clock> * <frequency tuning word>/2^32
  // DDS_DEVICE allows an output frequency resolution of 0.0291 Hz with a 125 MHz reference clock applied
  // double freq_tuning_word_d = frequency * 4294967295.0/125000000.0;  // note 125 MHz clock on 9850. You can make 'slight' tuning variations here by adjusting the clock frequency.
  int32_t freq_tuning_word = frequency * 4294967295.0/125000000.0;  // note 125 MHz clock on 9850. You can make 'slight' tuning variations here by adjusting the clock frequency.
  Log.Info(F("Setting frequency to " DDS_DEVICE ": %l Hz"), frequency);
  Log.Debug(F("Tuning word = %l"), freq_tuning_word);

  for (int b=0; b<4; b++, freq_tuning_word>>=8) {
    transfer_byte(freq_tuning_word & 0xFF);
  }
  transfer_byte(0x00); // Final control byte, all 0 for 9850 chip
  pulseHigh(FQ_UD);  // Done!  Should see output
}

// transfers a byte, a bit at a time, LSB first to the 9850 via serial DATA line
void transfer_byte(byte data)
{
  for (int i=0; i<8; i++, data>>=1) {
    digitalWrite(DATA, data & 0x01);
    pulseHigh(W_CLK); //after each bit sent, CLK is pulsed high
  }
}

// Show frequency
void LCD_show_frequency()
{
  String lcd_info = String(frequency) + " Hz"; //!!
  LCD_show_line(0, lcd_info);
}

void LCD_show_frequency_delta(String prefix)
{
  String lcd_info = prefix + String(frequency_delta[frequency_delta_index]) + " Hz"; //!!
  LCD_show_line(1, lcd_info);
}

// Show info on LCD screen
void LCD_show_line(byte line_number, String info)
{
  lcd.setCursor(0, line_number);  // move to the begining of the line
  lcd.print(F("                "));
  lcd.setCursor(0, line_number);  // move to the begining of the line
  lcd.print(info);
}

 // read the buttons
byte read_LCD_buttons()
{
  int key = analogRead(A0);   // read the value from the sensor
  // buttons when read are centered at these valies: 0, 144, 329, 504, 741
  // Serial.println(key);
  if (key > 1000) return btnNONE;
  if (key < 50)   return btnDELTA;
  if (key < 250)  return btnUP;
  if (key < 450)  return btnDOWN;
  if (key < 650)  return btnMEMO2;
  if (key < 850)  return btnMEMO1;
  return btnERROR;             // when all others fail, return this.
}

//init memory
void init_memory()
{
  for (byte i=0; i<3; i++) {
    Log.Info(F("Initializing M%d"), i);
    MemoryRecord m = { 0, 99};
    EEPROM.get(EEPROM_address[i], m);
    uint32_t f = long(m.frequency);
    if(isnan(f)) f = 0;
    if(isinf(f)) f = 0;
    byte fdi = byte(m.frequency_delta_index);
    if(isnan(fdi)) fdi = 0;
    if(isinf(fdi)) fdi = 0;
    Log.Info(F("Stored values: %l Hz, delta = %l Hz"), f, frequency_delta[fdi]);
    if((f <= MIN_FREQUENCY) or (f >= MAX_FREQUENCY)){
      m.frequency = DEF_FREQUENCY;
      Log.Debug(F("Saving new frequency: %l Hz"), m.frequency);
    }
    else {
      m.frequency = f;
    }
    if((fdi < 0) or (fdi > MAX_FREQUENCY_INDEX)){
      m.frequency_delta_index = DEF_FREQUENCY_INDEX;
      Log.Debug(F("Saving new delta: %l Hz"), frequency_delta[m.frequency_delta_index]);
    }
    else {
      m.frequency_delta_index = fdi;
    }
    EEPROM.put(EEPROM_address[i], m);
  }
}

// working with memory
void read_from_memory(byte memory_slot)
{
  MemoryRecord m;
  EEPROM.get(EEPROM_address[memory_slot], m);
  frequency = long(m.frequency);
  frequency_delta_index = byte(m.frequency_delta_index);
  Log.Info(F("Read from EEPROM M%i: %l Hz, delta %l Hz"), memory_slot, frequency, frequency_delta[m.frequency_delta_index]);
  lcd.setCursor(14,0);
  lcd.print("M"); lcd.print(memory_slot);
  delay(800);
  lcd.setCursor(14,0);
  lcd.print("  ");
 }

void save_to_memory(byte memory_slot)
{
  MemoryRecord m = {
    frequency,
    frequency_delta_index
  };
  EEPROM.put(EEPROM_address[memory_slot], m);
  Log.Info(F("Saved to EEPROM M%i: %l Hz, delta %l Hz"), memory_slot, frequency, frequency_delta[m.frequency_delta_index]);
  for (byte i=0; i<3; i++) {
    lcd.setCursor(14,0);
    lcd.print("M"); lcd.print(memory_slot);
    delay(400);
    lcd.setCursor(14,0);
    lcd.print("  ");
    delay(500);
  }
}

void frequency_inc()
{
  if((frequency + frequency_delta[frequency_delta_index]) <= MAX_FREQUENCY) {
    frequency = frequency + frequency_delta[frequency_delta_index];
    set_frequency();
    need_save_to_m0 = true;
  }
}

void frequency_dec()
{
  if((frequency - frequency_delta[frequency_delta_index]) >= MIN_FREQUENCY) {
    frequency = frequency - frequency_delta[frequency_delta_index];
    set_frequency();
    need_save_to_m0 = true;
  }
}

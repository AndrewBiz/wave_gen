/* AD9850 DDS based wave generator
   header file
   Andrew Bizyaev (ANB) github.com/andrewbiz
*/
#ifndef WAVE_GEN_H
#define WAVE_GEN_H
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Logging.h>

#define LOGLEVEL LOG_LEVEL_DEBUG //see Logging.h for options
#define LOG_PRINT_TS true  // print time stamp in logging
#define LOG_AUTO_LN  true  // print auto LN (CR) after each call
#define DDS_DEVICE "AD9850"

// LCD keypad ARDUINO pins mapping:
#define D4     4 //LCD data
#define D5     5 //LCD data
#define D6     6 //LCD data
#define D7     7 //LCD data
#define RS     8 //LCD RS
#define ENABLE 9 //LCD ENABLE
#define D10   10 // LCD Backlight control

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

const uint32_t frequency_delta[] = { 1, 10, 100, 1000, 10000, 100000, 1000000 };
extern LiquidCrystal lcd;
extern uint32_t frequency; //frequency of VFO
extern byte frequency_delta_index;
extern bool need_save_to_m0;
extern bool state_btn_pressed;
extern bool state_btn_repeat;
extern byte btn_pressed;
extern uint32_t time_btn_pressed;
extern uint32_t time_btn_released;

// set frequency into DDS_DEVICE
void transfer_byte(byte);
void set_frequency();
void frequency_inc();
void frequency_dec();

// Show frequency
void LCD_show_frequency();
void LCD_show_frequency_delta(String prefix);
// Show info on LCD screen
void LCD_show_line(byte line_number, String info);
 // read the buttons
byte read_LCD_buttons();

// working with memory
void init_memory();
void read_from_memory(byte memory_slot);
void save_to_memory(byte memory_slot);

#endif

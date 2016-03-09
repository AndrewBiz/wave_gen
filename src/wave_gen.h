/* AD9850 DDS based wave generator
   header file
   Andrew Bizyaev (ANB) github.com/andrewbiz
*/
#ifndef WAVE_GEN_H
#define WAVE_GEN_H
#include <Arduino.h>
#include <LiquidCrystal.h>
#include <Logging.h>

#define LOGLEVEL LOG_LEVEL_DEBUG // see Logging.h for options
// #define LOGLEVEL LOG_LEVEL_VERBOSE // see Logging.h for options
#define LOG_PRINT_TS true  // print time stamp in logging
#define LOG_AUTO_LN  true  // print auto LN (CR) after each call
#define DDS_DEVICE "AD9850"

// LCD keypad button pins mapping
#define btnNONE  0 // originally btnNONE
#define btnMEMO1 1 // originally btnSELECT
#define btnMEMO2 2 // originally btnLEFT
#define btnUP    3 // originally btnUP
#define btnDOWN  4 // originally btnDOWN
#define btnDELTA 5 // originally btnRIGHT
#define btnERROR 99 //

#define MIN_FREQUENCY 1
#define MAX_FREQUENCY 20000000
#define DEF_FREQUENCY 1000 //default freq
#define MAX_FREQUENCY_INDEX 6
#define DEF_FREQUENCY_INDEX 3 //default freq index
#define MIN_PHASE 0
#define MAX_PHASE 31
#define DEF_PHASE 0
#define SAVE_TO_M0_INTERVAL 7000 //7 sec after the key was pressed current frequency will be saved to EEPROM
#define LONG_KEY_PRESS_INTERVAL 1000 //1 sec is considered long keypress
#define REPEAT_KEY_PRESS_INTERVAL 300 //0,3 sec is considered to start autorepeat

const uint32_t frequency_delta[] = { 1, 10, 100, 1000, 10000, 100000, 1000000 };
extern LiquidCrystal lcd;
extern uint32_t frequency; //frequency of VFO
extern byte frequency_delta_index;
extern byte phase;  // phase shift (5 bit)

// set frequency into DDS_DEVICE
void AD9850_init();
void AD9850_set_frequency();

// Show frequency
void LCD_backlight(byte level);
void LCD_show_frequency();
void LCD_show_frequency_delta(String prefix);
void LCD_show_line(byte line_number, String info);
byte LCD_read_buttons();

// working with memory
void MEMORY_init();
void MEMORY_read(byte memory_slot);
void MEMORY_save(byte memory_slot);

#endif

/* AD9850 DDS based wave generator
   inspired by project of Richard Visokey AD7C - www.ad7c.com
   Andrew Bizyaev (ANB) github.com/andrewbiz
*/
#include "wave_gen.h"
#define WAVE_GEN_VERSION "0.9.3"

// external global vars definitions
uint32_t frequency = DEF_FREQUENCY; //frequency of VFO
byte frequency_delta_index = DEF_FREQUENCY_INDEX;
bool state_btn_pressed = false;
bool state_btn_repeat = false;
byte btn_pressed = btnNONE;
uint32_t time_btn_pressed = 0;
uint32_t time_btn_released = 0;

// local vars
bool need_save_to_m0 = false;

void setup()
{
  Log.Init(LOGLEVEL, 38400L, LOG_PRINT_TS, LOG_AUTO_LN);
  Log.Info(F("Starting DDS " DDS_DEVICE " Wave_Gen, version " WAVE_GEN_VERSION));

  // LCD setup
  lcd.begin(16, 2);
  LCD_backlight(HIGH);
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
    switch(LCD_read_buttons()){
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
    btn_pressed = LCD_read_buttons();
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

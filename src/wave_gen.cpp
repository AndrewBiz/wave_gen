/* AD9850 DDS based wave generator
   inspired by project of Richard Visokey AD7C - www.ad7c.com
   Andrew Bizyaev (ANB) github.com/andrewbiz
*/
#include "wave_gen.h"
#include <Encoder.h>

#define WAVE_GEN_VERSION "0.12.3"

// external global vars definitions
uint32_t frequency = DEF_FREQUENCY; //frequency of VFO
byte frequency_delta_index = DEF_FREQUENCY_INDEX;
byte phase = DEF_PHASE;

void frequency_inc();
void frequency_dec();


// local vars
bool state_btn_pressed = false;
bool state_btn_repeat = false;
byte btn_pressed = btnNONE;
uint32_t time_btn_pressed = 0;
uint32_t time_btn_released = 0;
bool need_save_to_m0 = false;

Encoder wg_encoder(3, 2);
long oldPosition  = -999;
long newPosition;

void setup()
{
    Log.Init(LOGLEVEL, 38400L, LOG_PRINT_TS, LOG_AUTO_LN);
    Log.Info(F("Starting DDS " DDS_DEVICE " Wave_Gen, version " WAVE_GEN_VERSION));

    // LCD setup
    lcd.begin(16, 2);
    LCD_backlight(HIGH);
    LCD_show_line(0, F("AndrewBiz (c)"));
    LCD_show_line(1, F("WaveGen " WAVE_GEN_VERSION));
    delay(2000);

    MEMORY_init();
    //read from default memory slot
    MEMORY_read(0);

    LCD_show_frequency();
    LCD_show_frequency_delta(" ");

    // AD9850
    AD9850_init();

    // Encoder init
    wg_encoder.write(phase);
}

void loop()
{
    delay(50);
    // TMP start test encoder
    newPosition = wg_encoder.read();
    if (newPosition != oldPosition) {
        oldPosition = newPosition;
        Log.Debug(F("Encoder position: %l"), newPosition);
        if (newPosition > MAX_PHASE) {
            phase = MAX_PHASE;
        } else if (newPosition < MIN_PHASE) {
            phase = MIN_PHASE;
        } else {
            phase = newPosition;
            AD9850_set_frequency();
        }
        Log.Debug(F("Phase: %i"), phase);
    }
    // TMP end

    if ( state_btn_pressed ) {
        //key was being pressed in the last cycle
        switch(LCD_read_buttons()) {
            case btnMEMO1:
                // the user keeps pressing the button
                if( (millis() - time_btn_pressed) >= LONG_KEY_PRESS_INTERVAL){
                    // enough time passed for long press-n-hold
                    lcd.setCursor(14,0);
                    lcd.print("M1");
                }
                break;

            case btnMEMO2:
                // the user keeps pressing the button
                if( (millis() - time_btn_pressed) >= LONG_KEY_PRESS_INTERVAL){
                    // enough time passed for long press-n-hold
                    lcd.setCursor(14,0);
                    lcd.print("M2");
                }
                break;

            case btnUP:
                // the user keeps pressing the button
                LCD_show_frequency_delta("+");
                // the key is kept pressed by the user
                if( (millis() - time_btn_pressed) >= REPEAT_KEY_PRESS_INTERVAL) {
                    state_btn_repeat = true;
                    Log.Debug(F("Key btnUP repeat"));
                    frequency_inc();
                    LCD_show_frequency();
                }
                break; //casebtnUP

            case btnDOWN:
                // the user keeps pressing the button
                LCD_show_frequency_delta("-");
                // the key is kept pressed by the user
                if( (millis() - time_btn_pressed) >= REPEAT_KEY_PRESS_INTERVAL) {
                    state_btn_repeat = true;
                    Log.Debug(F("Key btnDOWN repeat"));
                    frequency_dec();
                    LCD_show_frequency();
                }
                break; // case btnDOWN

            case btnNONE:
                // we have the key was pressed down and then released
                state_btn_pressed = false;
                time_btn_released = millis();
                switch(btn_pressed) {
                    case btnMEMO1:
                        if( (time_btn_released - time_btn_pressed) < LONG_KEY_PRESS_INTERVAL){
                            // it was short key press
                            Log.Debug(F("Key btnMEMO1 short pressed"));
                            MEMORY_read(1);
                            AD9850_set_frequency();
                            LCD_show_frequency();
                            LCD_show_frequency_delta(" ");
                        } else {
                            // it was long key press
                            Log.Debug(F("Key btnMEMO1 long pressed"));
                            MEMORY_save(1);
                        }
                        break;

                    case btnMEMO2:
                        if( (time_btn_released - time_btn_pressed) < LONG_KEY_PRESS_INTERVAL) {
                            // it was short key press
                            Log.Debug(F("Key btnMEMO2 short pressed"));
                            MEMORY_read(2);
                            AD9850_set_frequency();
                            LCD_show_frequency();
                            LCD_show_frequency_delta(" ");
                        } else {
                            // it was long key press
                            Log.Debug(F("Key btnMEMO2 long pressed"));
                            MEMORY_save(2);
                        }
                        break;

                    case btnUP:
                        if(!state_btn_repeat) { // in repeate mode will not trigger btn unpress function
                            Log.Debug(F("Key btnUP pressed"));
                            frequency_inc();
                            LCD_show_frequency();
                        }
                        LCD_show_frequency_delta(" ");
                        break;

                    case btnDOWN:
                        if(!state_btn_repeat) { // in repeate mode will not trigger btn unpress function
                            Log.Debug(F("Key btnDOWN pressed"));
                            frequency_dec();
                            LCD_show_frequency();
                        }
                        LCD_show_frequency_delta(" ");
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
    } else { // no keys was pressed in the last cycle
        // saving to the memory M0 if needed
        if(((millis() - time_btn_released) > SAVE_TO_M0_INTERVAL) and need_save_to_m0) {
            MEMORY_save(0);
            need_save_to_m0 = false;
        }
        btn_pressed = LCD_read_buttons();
        if( (btn_pressed != btnNONE) and (btn_pressed != btnERROR) ) {
            state_btn_pressed = true;
            time_btn_pressed = millis();
        }
    }
} // function loop

void frequency_inc()
{
    if((frequency + frequency_delta[frequency_delta_index]) <= MAX_FREQUENCY) {
        frequency = frequency + frequency_delta[frequency_delta_index];
        AD9850_set_frequency();
        need_save_to_m0 = true;
    }
}

void frequency_dec()
{
    if((frequency - frequency_delta[frequency_delta_index]) >= MIN_FREQUENCY) {
        frequency = frequency - frequency_delta[frequency_delta_index];
        AD9850_set_frequency();
        need_save_to_m0 = true;
    }
}

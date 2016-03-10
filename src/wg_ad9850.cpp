/* AD9850 DDS based wave generator
   inspired by project of Richard Visokey AD7C - www.ad7c.com
   Andrew Bizyaev (ANB) github.com/andrewbiz
*/
#include "wave_gen.h"

// AD9850 module pins
#define DATA  A1 // connect to serial data load pin (DATA)
#define W_CLK A2 // connect to word load clock pin (CLK)
#define FQ_UD A3 // connect to freq update pin (FQ)
#define RESET A4 // connect to reset pin (RST)
#define pulseHigh(pin) {digitalWrite(pin, HIGH); digitalWrite(pin, LOW); }

void transfer_byte(byte);

void AD9850_init()
{
    // setup AD9850
    pinMode(DATA,  OUTPUT);
    pinMode(W_CLK, OUTPUT);
    pinMode(FQ_UD, OUTPUT);
    pinMode(RESET, OUTPUT);
    pulseHigh(RESET);
    pulseHigh(FQ_UD);  // this pulse enables serial mode on the AD9850 - Datasheet page 12.
    // sent initial frequency to DDS device
    AD9850_set_frequency();
    AD9850_set_frequency(); //experimentally found out - need to set 2 times
}

// set frequency into DDS_DEVICE
void AD9850_set_frequency()
{
    // datasheet page 8: frequency = <sys clock> * <frequency tuning word>/2^32
    // DDS_DEVICE allows an output frequency resolution of 0.0291 Hz with a 125 MHz reference clock applied
    // double freq_tuning_word_d = frequency * 4294967295.0/125000000.0;
    // note 125 MHz clock on 9850. You can make 'slight' tuning variations here by adjusting the clock frequency.
    uint32_t freq_tuning_word = frequency * 4294967295/125000000;
    uint8_t phase_control_byte = (phase << 3);

    Log.Info(F("Setting frequency to " DDS_DEVICE ": %l Hz, phase %i"), frequency, phase);
    Log.Debug(F("Tuning word: %l (%y %y %y %y)"), freq_tuning_word,\
                Byte4(freq_tuning_word), Byte3(freq_tuning_word),\
                Byte2(freq_tuning_word), Byte1(freq_tuning_word) );
    Log.Debug(F("Phase: %i (%Y), control byte %i (%Y)"),\
                phase, phase, phase_control_byte, phase_control_byte);
    Log.Debug(F("40 bits to load (right bit 1st): %y %y %y %y %y"),\
                phase_control_byte,\
                Byte4(freq_tuning_word), Byte3(freq_tuning_word),\
                Byte2(freq_tuning_word), Byte1(freq_tuning_word) );

    for (int b = 0; b < 4; b++, freq_tuning_word >>= 8) {
        transfer_byte(freq_tuning_word & 0xFF);
    }
    transfer_byte(phase_control_byte & 0xF8); // Final control byte, 3 low bits should be 0 for 9850 chip (W32,33,34)
    pulseHigh(FQ_UD);  // Done!  Should see output
}

// transfers a byte, a bit at a time, LSB first to the 9850 via serial DATA line
void transfer_byte(byte data)
{
    Log.Debug(F("....Sending byte: %y"), data);
    for (int i=0; i < 8; i++, data >>= 1) {
        digitalWrite(DATA, data & 0x01);
        #if defined(LOGLEVEL) && LOGLEVEL == LOG_LEVEL_VERBOSE
            Log.Verbose(F("........bit: %b"), data & 0x01);
        #endif
        pulseHigh(W_CLK); //after each bit sent, CLK is pulsed high
    }
}

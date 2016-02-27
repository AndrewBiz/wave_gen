/* AD9850 DDS based wave generator
   memory manipulation library
   Andrew Bizyaev (ANB) github.com/andrewbiz
*/
#include <EEPROM.h>
#include "wave_gen.h"

const byte EEPROM_address[] = { 0, 5, 10 };

struct MemoryRecord {
  uint32_t frequency;
  byte frequency_delta_index;
};

//init memory
void init_memory()
{
  for (byte i=0; i<3; i++) {
    Log.Info(F("Initializing M%d"), i);
    MemoryRecord m = {0, 99};
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

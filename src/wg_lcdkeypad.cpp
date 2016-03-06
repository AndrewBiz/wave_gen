/* AD9850 DDS based wave generator
   LCD Keypad Shield
   Andrew Bizyaev (ANB) github.com/andrewbiz
*/
#include "wave_gen.h"

/* LCD keypad upper PINS map:
     |   |   |       |     |     |   |
    D13 D12 D11     D3    D2    D1  D0
                 (int1) (int0)
*/
// LCD keypad ARDUINO pins mapping:
#define LCD_D4      4 //LCD data
#define LCD_D5      5 //LCD data
#define LCD_D6      6 //LCD data
#define LCD_D7      7 //LCD data
#define LCD_RS      8 //LCD RS
#define LCD_ENABLE  9 //LCD ENABLE
#define LCD_BL      10 // LCD Backlight control

LiquidCrystal lcd( LCD_RS, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7 );

void LCD_backlight(byte level)
{
    pinMode(LCD_BL, OUTPUT);
    if( (level == HIGH) or (level == LOW) ){
        digitalWrite(LCD_BL, level);
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
byte LCD_read_buttons()
{
    int key = analogRead(A0);   // read the value from the sensor
    // buttons when read are centered at these valies: 0, 144, 329, 504, 741
    if (key > 1000) return btnNONE;
    if (key < 50)   return btnDELTA;
    if (key < 250)  return btnUP;
    if (key < 450)  return btnDOWN;
    if (key < 650)  return btnMEMO2;
    if (key < 850)  return btnMEMO1;
    return btnERROR;             // when all others fail.
}

#include <LiquidCrystal.h>

#define CHANGE_SCREEN_IN_MS 3000

#define LCD_DATA1 22         /* LCD data DIO pin 4 */
#define LCD_DATA2 23         /* LCD data DIO pin 5 */
#define LCD_DATA3 24         /* LCD data DIO pin 6 */
#define LCD_DATA4 25         /* LCD data DIO pin 7 */
#define LCD_RESET 26         /* LCD Reset DIO pin */
#define LCD_ENABLE 27        /* LCD Enable DIO pin */
#define LCD_BACKLIGHT 28    /* LCD backlight DIO pin */

LiquidCrystal lcd(LCD_RESET, LCD_ENABLE, LCD_DATA1, LCD_DATA2, LCD_DATA3, LCD_DATA4);

void lcdSetup() {
  /* Set the correct display size (16 character, 2 line display) */
  lcd.begin(16, 2);  
}

int infoNo = 0;
long lastChangeOn = 0;
void writeInfoToLcd()
{
  if (lastChangeOn > 0 && lastChangeOn + CHANGE_SCREEN_IN_MS > millis())
    return;

  lcd.clear();
    
  if (infoNo==0) {
      lcd.setCursor(0,0); 
      lcdDateTime();
      lcd.setCursor(0, 1);
      lcd.print("profile");
      lcd.setCursor(8, 1);
      lcd.print(currentProfile);
  }
  else if (infoNo == 1) {
    lcd.setCursor(0,0);
    lcd.print("HEAT");
    lcd.setCursor(5,0);
    lcd.print("12345678");
    for (int a=0; a<SWITCH_COUNT+1; a++)
    {
        lcd.setCursor(5+a,1);
        lcd.print(switchState[a]);
    }
  }
  else if (infoNo-2<thermometersCount) {
    lcd.noDisplay();
    float tempC = sensors.getTempC(thermometers[infoNo-2]);
    lcd.display();
    lcd.setCursor(0,0);
    lcd.print(GetAddress(thermometers[infoNo-2]));
    lcd.setCursor(0,1);
//    lcd.autoscroll();
    lcd.print(tempC, 2);
  //  lcd.print((char)223);
    //lcd.print("C");
    //lcd.noAutoscroll();
  }
  else {
    infoNo = 0;
    lastChangeOn = 0;
    return;
  }
  infoNo++;
  lastChangeOn = millis();
}

void lcdDateTime()
{
  // digital clock display of the time
  lcd.setCursor(0,0); 
  lcd.print(year());
  lcd.setCursor(4,0); 
  lcd.print('-');
  lcd.setCursor(5,0); 
  lcd.print(month());
  lcd.setCursor(7,0); 
  lcd.print('-');
  lcd.setCursor(8,0); 
  lcd.print(day());
  lcd.setCursor(11,0); 
  lcd.print(hour());
  lcd.setCursor(13,0); 
  lcd.print(':');
  lcd.setCursor(14,0); 
  lcd.print(minute());
}


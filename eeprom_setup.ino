#include <EEPROM.h>
#include "EEPROMAnything.h"

void EEPROM_readConfig() {
  sensorsCount = EEPROM.read(0);
  Serial.print("Switch count: ");
  Serial.println(sensorsCount);
  
  int pos = 1;
  pos = readEepromSensorCodes(pos, sensorsConfig, sensorsCount);
  Serial.println("SENSORS");
  for (int a=0; a<sensorsCount; a++) {
	  for (int b=0; b<8; b++) {
		  Serial.print(sensorsConfig[a][b], HEX);
	  }
	  Serial.println();
  }
  
  profilesCount = EEPROM.read(pos);
  Serial.print("Profiles count: ");
  Serial.println(profilesCount);
  pos++;
  pos = readEepromProfiles(pos, profilesConfig, profilesCount, sensorsCount);
  Serial.println("PROFILES");
  for (int p=0; p<profilesCount; p++) {
	  Serial.print("PROFILE ");
	  Serial.println(p);
	  for (int b=0; b<sensorsCount; b++) {
		  for (int s=0; s<8; s++) {
			Serial.print(sensorsConfig[b][s], HEX);
		  }
		  Serial.print(" ");
		  for (int c=0; c<2; c++) {
			Serial.print(profilesConfig[p][b * 2 + c]);
			Serial.print(" ");
		  }
		  Serial.println();
	  }
  }
}
/*
#define SWITCH_COUNT 7
#define PROFILE_COUNT 4
void EEPROM_writeConfig() {
	byte profileCount = PROFILE_COUNT;
	byte switchCount = SWITCH_COUNT;
	
  byte sensors[][8] = {
    { 0x28, 0x6B, 0x72, 0x69, 0x02, 0x00, 0x00, 0x87 }, //"2807F36B02000038",
    //"102AC9CD0108002A",  // testas
    { 0x28, 0x6B, 0x72, 0x69, 0x02, 0x00, 0x00, 0x87 },
    { 0x28, 0xFB, 0xC5, 0x6B, 0x02, 0x00, 0x00, 0x2F },
    { 0x28, 0x72, 0xF9, 0x6B, 0x02, 0x00, 0x00, 0xF6 },
    { 0x28, 0xDB, 0xCC, 0x6B, 0x02, 0x00, 0x00, 0x6A },
    { 0x28, 0x5F, 0x5E, 0x69, 0x02, 0x00, 0x00, 0x6F },
    { 0x28, 0x5F, 0x5E, 0x69, 0x02, 0x00, 0x00, 0x6F }
  };

  float profiles[][MAX_SENSORS_COUNT*2] =
  {
    {  // --- In-room Profile --
      17.3, 17.5,  // Virtuve ir Tamburas
      19.5, 19.7,  // Svetaine ir Koridorius
      //21.3, 21.5,  // Testas
      20.0, 21.0,    // Vonia
      18.8, 19,    // Darbo kambarys
      18.0, 18.2,    // Jurgio kambarys
      18.3, 18.5,  // Garderobine
      18.8, 19.0   // Tevu kambarys
    }
    ,
    {  // -- Out of room Profile --
      16.0, 16.2,  // Virtuve ir Tamburas
      18.8, 19.0,  // Svetaine ir Koridorius
      //21.3, 21.5,  // Testas
      17.0, 17.3,    // Vonia
      18.3, 18.5,    // Darbo kambarys
      17.3, 17.5,    // Jurgio kambarys
      17.8, 18.0,  // Garderobine
      18.3, 18.5   // Tevu kambarys
    }
    ,
    {  // -- Out of room Profile --
      16.0, 16.2,  // Virtuve ir Tamburas
      17.5, 17.7,  // Svetaine ir Koridorius
      //21.3, 21.5,  // Testas
      17.0, 17.3,    // Vonia
      17.0, 17.2,    // Darbo kambarys
      16.0, 16.2,    // Jurgio kambarys
      16.0, 16.2,  // Garderobine
      16.0, 16.2   // Tevu kambarys
    }
    ,
    {  // -- On Holidays Profile --
      5.0, 5.2,  // Virtuve ir Tamburas
      10.5, 10.7,  // Svetaine ir Koridorius
      //21.3, 21.5,  // Testas
      8.0, 8.3,    // Vonia
      8.0, 8.2,    // Darbo kambarys
      8.0, 8.2,    // Jurgio kambarys
      8.0, 8.2,  // Garderobine
      8.0, 8.2   // Tevu kambarys
    }
  };
  
  EEPROM.write(0, SWITCH_COUNT);
  int pos = 1;
  pos = writeEepromSensorCodes(pos, sensors, switchCount);
  
  EEPROM.write(pos, profileCount);
  pos++;
  pos = writeEepromProfiles(pos, profiles, profileCount, switchCount);
}
*/
int writeEepromSensorCodes(int pos, byte sensors[][8], byte sensorsCount){
  for (int a=0; a<sensorsCount; a++) {
	  for (int b=0; b<8; b++) {
		  EEPROM.write(pos, sensors[a][b]);
		  pos++;
	  }
  }
  return pos;
}

int writeEepromProfiles(int pos, float profiles[][MAX_SENSORS_COUNT*2], byte profilesCount, byte sensorsCount) {
	for (int p=0; p < profilesCount; p++){
		for (int i=0; i<sensorsCount*2; i++) {
			int value = profiles[p][i] * 100;
			pos += EEPROM_writeAnything(pos, value);
		}
	}
	return pos;
}

int readEepromSensorCodes(int pos, byte sensorsConfig[][8], byte sensorsCount){
  for (int a=0; a<sensorsCount; a++) {
	  for (int b=0; b<8; b++) {
		  sensorsConfig[a][b] = EEPROM.read(pos);
		  pos++;
	  }
  }
  return pos;
}

int readEepromProfiles(int pos, float profilesConfig[][MAX_SENSORS_COUNT*2], byte profilesCount, byte sensorsCount) {
	int value;
	for (int p=0; p < profilesCount; p++){
		for (int i=0; i<sensorsCount*2; i++) {
			pos += EEPROM_readAnything(pos, value);
			profilesConfig[p][i] = value / 100.0;
		}
	}
	return pos;
}

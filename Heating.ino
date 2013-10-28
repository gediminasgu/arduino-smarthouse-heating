#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <SmartWire.h>
#include <RTClib.h>

#define MAX_PROFILE_COUNT 6
#define MAX_SENSORS_COUNT 8

byte sensorsConfig[MAX_SENSORS_COUNT][8];
byte sensorsCount;

float profilesConfig[MAX_PROFILE_COUNT][MAX_SENSORS_COUNT*2];
byte profilesCount;

#define WIRE_ADDRESS 0x19
#define ONE_WIRE_BUS 10
#define BOILER_OUTPUT 9
#define HOLDING_REGS_SIZE 20
unsigned int holdingRegs[HOLDING_REGS_SIZE]; // function 3 and 16 register array

long previousMillis = 0;        // will store last time LED was updated
long interval = 60000;           // interval at which to check temperature
int ledState = LOW;
byte testMode = 0;  // run test after start
int testingSwitch = 0;
byte indicateTemperatureCount = 1;
byte forceRun = 0;
byte currentProfile = 100;
byte forceProfile = 0;

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress thermometers[20];
byte switchToTermometer[MAX_SENSORS_COUNT];  // index is switch index, and item value is temperature index
byte switchState[MAX_SENSORS_COUNT+1];  // is switch on (1) or off (0). The last one is for boiler
byte thermometersCount = 0;
double tempRanges[MAX_SENSORS_COUNT*2];

RTC_DS1307 RTC;
DateTime now;

double measures[20];
int measureCount = 0;

void setup() {
  Serial.begin(9600);
  
  //EEPROM_writeConfig();
  EEPROM_readConfig();
  
  lcdSetup();

  for (int a=2; a<sensorsCount + 2; a++)
  {
    pinMode(a, OUTPUT);
    digitalWrite(a, HIGH);
    switchState[a-2] = 0;
  }

  setTempRanges();

  pinMode(BOILER_OUTPUT, OUTPUT);
  digitalWrite(BOILER_OUTPUT, HIGH);
  switchState[sensorsCount] = 0;

  SmartWire.begin(WIRE_ADDRESS, HOLDING_REGS_SIZE, holdingRegs);

  // Start up the library
  sensors.begin();
  InitDevices();
  mapSwitchIndexes();
  
  RTC.begin();
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // following line sets the RTC to the date & time this sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
}

String line;
void loop()
{
  unsigned long currentMillis = millis();
  readInput();

  if (testMode == 1 && currentMillis - previousMillis > 1000)
  {
    previousMillis = currentMillis;
    runTest();
  }

  if(previousMillis == 0 || currentMillis - previousMillis > interval || forceRun == 1) {
	now = RTC.now();
    previousMillis = currentMillis;

    setTempRanges();
    RefreshMeasures();
    SetSwitchState();

    PrintState();
    PushToWire();
    forceRun = 0;
  }

  writeInfoToLcd();
}

void readInput() {
  while (Serial.available() > 0) {
    char incomingByte = Serial.read();
    if (line.length() > 0 || incomingByte == '#')
    {
      if (incomingByte == 13 || (incomingByte == '#' && line.length() > 0))
      {
        doCommand(line.substring(1));
        line = "";
      }
      else
      {
        char c = incomingByte;
        line += c;
      }
    }
  }
}

void PrintState()
{
  for (int a=0; a<sensorsCount+1; a++)
  {
    Serial.print("#H");
    Serial.print(a);
    Serial.print('\t');
    Serial.print(switchState[a], HEX);
    Serial.println();
  }

  for (int a=0; a<thermometersCount; a++)
  {
    printData(thermometers[a]);
  }

  Serial.print("#datetime\t");
  printDateTime();
  Serial.println();

  Serial.print("#profile\t");
  Serial.print(currentProfile);
  Serial.println();
}

void PushToWire()
{
  Serial.println("Sending to wire");

  char state;
  for (char a=0; a<sensorsCount+1; a++)
    SendSmartData(0x01, a, switchState[a]);

  SendSmartData(0x00, 0x01, currentProfile);

  for (int a=0; a<thermometersCount; a++)
    pushDataToWire(thermometers[a]);
  Serial.println("Wire send Done.");
}

void SendSmartData(unsigned char type, unsigned char id, unsigned char data)
{
  /*Wire.beginTransmission(0);
  Wire.write(0x01);
  Wire.write(0x02);
  Wire.write(0x03);
  Wire.endTransmission();*/
  
  SmartWire.initEvent();
  SmartWire.writeToBuf(type);
  SmartWire.writeToBuf(id);
  SmartWire.writeToBuf(data);
  SmartWire.flush();
}

void SetSwitchState()
{
  byte boilerState = HIGH;
  byte found;
  for (int a=0; a<sensorsCount; a++)
  {
    found = 0;
    if (switchToTermometer[a] >= 0)
    {
      double temp = measures[switchToTermometer[a]];
      if (temp > 1 && temp < 70)
      {
        found = 1;
        Serial.print(temp);
        Serial.print(" E");
        Serial.println(a);
        if (temp < tempRanges[a*2])
        {
          digitalWrite(a+2, LOW);
          switchState[a] = 1;
          printWhyEnabled(a, temp, tempRanges[a*2]);
        }
        else if (temp > tempRanges[a*2+1])
        {
          digitalWrite(a+2, HIGH);
          switchState[a] = 0;
          printWhyDisabled(a, temp, tempRanges[a*2+1]);
        }
        if (switchState[a] == 1)
          boilerState = LOW;
      }
    }
    if (found == 0) // if thermometer not found or has invalid value, enable relay to heat that room every time when heater will be enabled
    {
      digitalWrite(a+2, LOW);
      switchState[a] = 1;
      Serial.print("Required thermometer for switch ");
      Serial.print(a);
      Serial.println(" wasn't found.");
    }
  }

  digitalWrite(BOILER_OUTPUT, boilerState);
  if (boilerState == HIGH)
    switchState[sensorsCount] = 0;
  else
    switchState[sensorsCount] = 1;
}

void printWhyEnabled(int a, double temp, double tempRange) {
  Serial.print("Enabling ");
  Serial.print(a);
  Serial.print(" as ");
  Serial.print(temp);
  Serial.print(" < ");
  Serial.println(tempRange);
}

void printWhyDisabled(int a, double temp, double tempRange) {
  Serial.print("Disabling ");
  Serial.print(a);
  Serial.print(" as ");
  Serial.print(temp);
  Serial.print(" > ");
  Serial.println(tempRange);
}

void RefreshMeasures()
{
  sensors.requestTemperatures();
  ClearMeasures();
  for (int a=0; a<thermometersCount; a++)
  {
    AddMeasure(sensors.getTempC(thermometers[a]));
  }
}

void ClearMeasures()
{
  measureCount = 0;
}

void AddMeasure(double value)
{
  measures[measureCount] = value;
  measureCount++;
}

void mapSwitchIndexes()
{
  for (int a=0; a < sensorsCount; a++)
  {
	  switchToTermometer[a] = -1;
    for (int t=0; t<thermometersCount; t++)
    {
      if (areArrayEqual(thermometers[t], sensorsConfig[a], 8))
      {
        switchToTermometer[a] = t;
        break;
      }
    }
  }
}

void runTest()
{
  resetSwitches();
  if (testingSwitch <= sensorsCount)
  {
    digitalWrite(testingSwitch+2, LOW);
    Serial.print("Testing ");
    Serial.println(testingSwitch);
    testingSwitch++;
  }
  else if (indicateTemperatureCount == 1)
  {
    if (thermometersCount > 0)
    {
      digitalWrite(2, LOW);
      Serial.println("Thermometers found");
    }
    else
    {
      digitalWrite(3, LOW);
      Serial.println("Thermometers NOTfound");
    }
    indicateTemperatureCount = 0;
  }
  else {
    Serial.println("Test done");
    testMode = 0;
    forceRun = 1;
  }
}

void resetSwitches()
{
  for (int a=2; a<sensorsCount + 3; a++)
  {
    digitalWrite(a, HIGH);
  }
}

//#settime:2011-10-10T22:43:00#
void doCommand(String line)
{
  Serial.print("Command: ");
  Serial.println(line);
  if (line=="refresh")
  {
    forceRun = 1;
  }
  else if (line.startsWith("profile:"))
  {
    String s = line.substring(line.indexOf(":")+1);
    forceProfile = getInt(s, "");
  }
}

int getInt(String s, String splitter)
{
  int r = -1;

  if (splitter.length() > 0)
  {
    int i = s.indexOf(splitter);
    if (i > 0)
      s = s.substring(0, i);
  }

  char buf[5];
  s.toCharArray(buf, sizeof(buf));
  r = atoi(buf);
  return r;
}

void printDateTime()
{
	Serial.print(now.year(), DEC);
	Serial.print('/');
	Serial.print(now.month(), DEC);
	Serial.print('/');
	Serial.print(now.day(), DEC);
	Serial.print(' ');
	Serial.print(now.hour(), DEC);
	Serial.print(':');
	Serial.print(now.minute(), DEC);
	Serial.print(':');
	Serial.print(now.second(), DEC);
	Serial.println();
}

void setTempRanges()
{
  int profile = 2;
  if (forceProfile == 0)
  {
    if (now.hour() >= 16 && now.hour() < 20)
      profile = 0;
    else if (now.hour() >= 5 && now.hour() < 7)
      profile = 0;
    else if (now.hour() < 5 || now.hour() >= 20)
      profile = 1;
  }
  else {
    profile = forceProfile;
  }

  if (currentProfile != profile)
  {
    currentProfile = profile;
    for (int a=0; a<sensorsCount*2; a++)
    {
      tempRanges[a] = profilesConfig[currentProfile][a];
    }
  }
}


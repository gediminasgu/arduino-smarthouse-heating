#include <OneWire.h>
#include <DallasTemperature.h>
#include <Time.h>
#include <Wire.h>
#include <SmartWire.h>

#define WIRE_ADDRESS 0x19
#define ONE_WIRE_BUS 10
#define SWITCH_COUNT 7
#define BOILER_OUTPUT 9
#define HOLDING_REGS_SIZE 20
unsigned int holdingRegs[HOLDING_REGS_SIZE]; // function 3 and 16 register array

long previousMillis = 0;        // will store last time LED was updated
long interval = 300000;           // interval at which to check temperature
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
byte switchToTermometer[SWITCH_COUNT] = {
  -1, -1, -1, -1, -1, -1, -1};  // index is switch index, and item value is temperature index
char switchState[SWITCH_COUNT+1];  // is switch on (1) or off (0). The last one is for boiler
String termoMap[SWITCH_COUNT] = {
  "286B726902000087", //"2807F36B02000038",
  //"102AC9CD0108002A",  // testas
  "286B726902000087",
  "28FBC56B0200002F",
  "2872F96B020000F6",
  "28DBCC6B0200006A",
  "285F5E690200006F",
  "285F5E690200006F"};
byte thermometersCount = 0;
double tempRanges[SWITCH_COUNT*2];

double tempProfiles[][SWITCH_COUNT*2] =
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

double measures[20];
int measureCount = 0;

void setup() {
  Serial.begin(9600);
  lcdSetup();

  for (int a=2; a<SWITCH_COUNT + 2; a++)
  {
    pinMode(a, OUTPUT);
    digitalWrite(a, HIGH);
    switchState[a-2] = '0';
  }

  setTempRanges();

  pinMode(BOILER_OUTPUT, OUTPUT);
  digitalWrite(BOILER_OUTPUT, HIGH);
  switchState[SWITCH_COUNT] = '0';

  SmartWire.begin(WIRE_ADDRESS, HOLDING_REGS_SIZE, holdingRegs);
  SmartWire.onDataReceive(receiveEvent);

  // Start up the library
  sensors.begin();
  InitDevices();
  mapSwitchIndexes();
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

  if(currentMillis - previousMillis > interval || forceRun == 1) {
    previousMillis = currentMillis;

    setTempRanges();
    RefreshMeasures();
    SetSwitchState();

    if (forceRun==1)
      PrintState();
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

void receiveEvent(){
  /*char incomingByte=0;
  while (Wire.available() > 0){
    incomingByte = Wire.read();
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
  }*/
}

void PrintState()
{
  PushToWire();
  for (int a=0; a<SWITCH_COUNT+1; a++)
  {
    Serial.print("#H");
    Serial.print(a);
    Serial.print('\t');
    Serial.print(switchState[a]);
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
  char state;
  for (char a=0; a<SWITCH_COUNT+1; a++)
    SendSmartData(0x01, a, switchState[a]);

  //Wire.write("#datetime\t");
  //pushDateTimeToWire();
  //Wire.write('\n');

  SendSmartData(0x00, 0x01, currentProfile);

  for (int a=0; a<thermometersCount; a++)
    pushDataToWire(thermometers[a]);
}

void SendSmartData(unsigned char type, unsigned char id, unsigned char data)
{
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
  for (int a=0; a<SWITCH_COUNT; a++)
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
          switchState[a] = '1';
          printWhyEnabled(a, temp, tempRanges[a*2]);
        }
        else if (temp > tempRanges[a*2+1])
        {
          digitalWrite(a+2, HIGH);
          switchState[a] = '0';
          printWhyDisabled(a, temp, tempRanges[a*2+1]);
        }
        if (switchState[a] == '1')
          boilerState = LOW;
      }
    }
    if (found == 0) // if thermometer not found or has invalid value, enable relay to heat that room every time when heater will be enabled
    {
      digitalWrite(a+2, LOW);
      switchState[a] = '1';
      Serial.print("Required thermometer for switch ");
      Serial.print(a);
      Serial.println(" wasn't found.");
    }
  }

  digitalWrite(BOILER_OUTPUT, boilerState);
  if (boilerState == HIGH)
    switchState[SWITCH_COUNT] = '0';
  else
    switchState[SWITCH_COUNT] = '1';
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
  for (int a=0; a < SWITCH_COUNT; a++)
  {
    for (int t=0; t<thermometersCount; t++)
    {
      if (GetAddress(thermometers[t]) == termoMap[a])
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
  if (testingSwitch <= SWITCH_COUNT)
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
  for (int a=2; a<SWITCH_COUNT + 3; a++)
  {
    digitalWrite(a, HIGH);
  }
}

//#settime:2011-10-10T22:43:00#
void doCommand(String line)
{
  Serial.print("Command: ");
  Serial.println(line);
  if (line.startsWith("settime:"))
  {
    String s = line.substring(line.indexOf(":")+1);
    setSystemTime(s);
  }
  else if (line=="refresh")
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

void setSystemTime(String s)
{
  if (s.indexOf('T') > 0)
  {
    String d = s.substring(0, s.indexOf('T'));
    String t = s.substring(s.indexOf('T')+1);

    int year = getInt(d, "-");
    d = d.substring(d.indexOf("-")+1);
    int month = getInt(d, "-");
    d = d.substring(d.indexOf("-")+1);
    int day = getInt(d, "");

    int hour = getInt(t, ":");
    t = t.substring(t.indexOf(":")+1);
    int minute = getInt(t, ":");
    t = t.substring(t.indexOf(":")+1);
    int second = getInt(t, "");

    setTime(hour,minute,second,month,day,year); // set time to Saturday 8:29:00am Jan 1 2011

    Serial.print("Time set: ");
    printDateTime();
    Serial.println();
  }
  else {
    Serial.print("Error time: ");
    Serial.println(line);
  }
}

void printDateTime()
{
  // digital clock display of the time
  Serial.print(year());
  Serial.print('-');
  Serial.print(month());
  Serial.print('-');
  Serial.print(day());
  Serial.print('T');
  Serial.print(hour());
  Serial.print(':');
  Serial.print(minute());
  Serial.print(':');
  Serial.print(second());
}

void pushDateTimeToWire()
{
  // digital clock display of the time
  Wire.write(year());
  Wire.write('-');
  Wire.write(month());
  Wire.write('-');
  Wire.write(day());
  Wire.write('T');
  Wire.write(hour());
  Wire.write(':');
  Wire.write(minute());
  Wire.write(':');
  Serial.print(second());
}

void setTempRanges()
{
  int profile = 2;
  if (forceProfile == 0)
  {
    if (hour() >= 16 && hour() < 20)
      profile = 0;
    else if (hour() >= 5 && hour() < 7)
      profile = 0;
    else if (hour() < 5 || hour() >= 20)
      profile = 1;
  }
  else {
    profile = forceProfile;
  }

  if (currentProfile != profile)
  {
    currentProfile = profile;
    for (int a=0; a<SWITCH_COUNT*2; a++)
    {
      tempRanges[a] = tempProfiles[currentProfile][a];
    }
  }
}


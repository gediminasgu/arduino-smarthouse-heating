#define TEMPERATURE_PRECISION 12

void InitDevices()
{
  // locate devices on the bus
  Serial.print("Locating devices...");
  Serial.print("Found ");
  thermometersCount = sensors.getDeviceCount();
  Serial.print(thermometersCount, DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (sensors.isParasitePowerMode()) {
    Serial.println("ON");
  }
  else {
    Serial.println("OFF");
  }

  // method 1: by index
  for (int a=0; a<thermometersCount; a++)
  {
    if (!sensors.getAddress(thermometers[a], a))
    {
      Serial.print("Unable to find address for Device "); 
      Serial.println(a);
    }
    Serial.print("Device ");
    Serial.print(a);
    Serial.print(" Address: ");
    Serial.print(GetAddress(thermometers[a]));
    //printAddress(thermometers[a]);
    Serial.println();
    
    if (thermometers[a][7]==0xB9)  // lauko termometras
    {
      sensors.setResolution(thermometers[a], 9);
    }
    else
    {
      sensors.setResolution(thermometers[a], TEMPERATURE_PRECISION);
    }

    Serial.print("Resolution: ");
    Serial.print(sensors.getResolution(thermometers[a]), DEC); 
    Serial.println();
  }
}

boolean areArrayEqual(byte arr1[], byte arr2[], int length) {
	for (int a=0; a<length; a++){
		if (arr1[a] != arr2[a])
			return false;
	}
	return true;
}

String GetAddress(DeviceAddress deviceAddress)
{
  String s = "";
  char hexval[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
  for(int j = 0; j < 8; j++){
    s += hexval[((deviceAddress[j] >> 4) & 0xF)];
    s += hexval[(deviceAddress[j]) & 0x0F];
  }

  return s;
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  Serial.print(tempC);
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(sensors.getResolution(deviceAddress));
  Serial.println();    
}

// main function to print information about a device
void printData(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  
  if (tempC > -70 && tempC < 70)
  {
    //printAddress(deviceAddress);
    Serial.print("#");
    Serial.print(GetAddress(deviceAddress));
    Serial.print('\t');
    Serial.print(tempC);
    Serial.println();
  }
}

void pushDataToWire(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);

  if (tempC > -70 && tempC < 70)
  {
    SmartWire.initEvent();
    SmartWire.writeToBuf((unsigned char)0x02);
    for( int x = 0; x < 8; x++) {
      SmartWire.writeToBuf(deviceAddress[x]);
    }
    SmartWire.writeToBuf(tempC);
    SmartWire.flush();
  }
}


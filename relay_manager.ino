#include <EEPROM.h>

#define DEBUG 0

#define ON "ON"
#define OFF "OFF"
#define MODE "MODE"

#define DEVICE_COUNT 6

#define LOOP_DELAY 1000

#define LOW_TRIGGERED 0
#define HIGH_TRIGGERED 1

#define DEFAULT_MODE HIGH_TRIGGERED

#define SERIAL_RATE 115200
/*
   MEMORY:
   -------------------------------------
   | Address           | Description   |
   -------------------------------------
   | sizeof(device)    | Device data   |
   | * deviceId        |               |
   -------------------------------------
*/

struct device
{
  byte inputPin;
  byte relayPin;
  bool pinsEqual;
  bool isOn;
  bool isHighState;
};

device data[DEVICE_COUNT];

void setup()
{
  Serial.begin(SERIAL_RATE);

  bool modified = false;
  for (int i = 0; i < DEVICE_COUNT; i++)
  {
    int address = getDeviceConfigAddress(i);
    EEPROM.get(address, data[i]);

    setPinModes(&data[i]);
    if (data[i].inputPin == 255 || data[i].inputPin == 0) {
      setPinNumbers(&data[i], i);
      modified = true;
    }

    if (modified) {
      EEPROM.put(address, data[i]);
      modified = false;
    }

#if DEBUG
    Serial.print("deviceId = ");
    Serial.print(i);
    Serial.print(" isHighState = ");
    Serial.println(data[i].isHighState);
#endif
  }

#if DEBUG
  Serial.println("data loaded");
#endif
}

void loop()
{
  if (Serial.available())
  {
    String data = Serial.readString();
    String cmd = data.substring(0, data.indexOf('|'));
    String params = data.substring(data.indexOf('|') + 1);

    handleCommand(cmd, params);
    return;
  }

  for (byte i = 0; i < DEVICE_COUNT; i++)
  {
    updateDeviceRelay(i);
  }

  delay(LOOP_DELAY);
}

void setPinNumbers(device *devicePtr, byte deviceNumber)
{
  devicePtr->inputPin = 2 + deviceNumber * 2;
  devicePtr->relayPin = 3 + deviceNumber * 2;
}

void setPinModes(device *devicePtr)
{
  pinMode(devicePtr->inputPin, INPUT_PULLUP);
  pinMode(devicePtr->relayPin, OUTPUT);

  devicePtr->pinsEqual = digitalRead(devicePtr->inputPin) == HIGH;
}

void handleCommand(String command, String params)
{
  if (command.equals(ON))
  {
    turnOn(params);
  }

  if (command.equals(OFF))
  {
    turnOff(params);
  }

  if (command.equals(MODE))
  {
    changeTriggering(params);
  }
}

void turnOn(String params)
{
  int deviceNumber = params.toInt();

  if (deviceNumber < DEVICE_COUNT) {
#if DEBUG
    Serial.print("turnOn");
    Serial.print(" deviceNumber = ");
    Serial.print(deviceNumber);
    Serial.print(" isHighState = ");
    Serial.println(data[deviceNumber].isHighState);
#endif

    if (data[deviceNumber].isOn)
      return;

    if (data[deviceNumber].isHighState)
    {
#if DEBUG
      Serial.println("state high - turning on");
#endif
      digitalWrite(data[deviceNumber].relayPin, HIGH);
    }
    else
    {
#if DEBUG
      Serial.println("state low - turning on");
#endif
      digitalWrite(data[deviceNumber].relayPin, LOW);
    }

    data[deviceNumber].isOn = true;
    data[deviceNumber].pinsEqual = digitalRead(data[deviceNumber].inputPin) == HIGH;
  }
}

void turnOff(String params)
{
  int deviceNumber = params.toInt();

  if (deviceNumber < DEVICE_COUNT) {

#if DEBUG
    Serial.print("turnOff");
    Serial.print(" deviceNumber = ");
    Serial.print(deviceNumber);
    Serial.print(" isHighState = ");
    Serial.println(data[deviceNumber].isHighState);
#endif

    if (!data[deviceNumber].isOn)
      return;

    if (data[deviceNumber].isHighState)
    {
#if DEBUG
      Serial.println("state high - turning off");
#endif
      digitalWrite(data[deviceNumber].relayPin, LOW);
    }
    else
    {
#if DEBUG
      Serial.println("state low - turning off");
#endif
      digitalWrite(data[deviceNumber].relayPin, HIGH);
    }

    data[deviceNumber].isOn = false;
    data[deviceNumber].pinsEqual = digitalRead(data[deviceNumber].inputPin) == HIGH;
  }
}

void changeTriggering(String params)
{
  int deviceNumber = params.substring(0, params.indexOf(';')).toInt();
  int mode = params.substring(params.indexOf(';') + 1).toInt();

#if DEBUG
  Serial.print("changeTriggering");
  Serial.print(" deviceNumber = ");
  Serial.print(deviceNumber);
  Serial.print(" mode = ");
  Serial.println(mode);
#endif

  if (data[deviceNumber].isHighState != (mode == HIGH_TRIGGERED)) {
    data[deviceNumber].isHighState = mode == HIGH_TRIGGERED;
    EEPROM.put(getDeviceConfigAddress(deviceNumber), data[deviceNumber]);
  }
}

void updateDeviceRelay(byte deviceId)
{
  bool pinsEqual = digitalRead(data[deviceId].inputPin) == HIGH;

#if DEBUG
  Serial.print("data[deviceId].inputPin = ");
  Serial.println(data[deviceId].inputPin);

  Serial.print("data[deviceId].outputPin = ");
  Serial.println(data[deviceId].outputPin);

  Serial.print("data[deviceId].pinsEqual = ");
  Serial.println(data[deviceId].pinsEqual);
  Serial.print("pinsEqual = ");
  Serial.println(pinsEqual);
#endif

  if (data[deviceId].pinsEqual != pinsEqual)
  {
    data[deviceId].pinsEqual = pinsEqual;

    if (data[deviceId].isOn)
    {
#if DEBUG
      Serial.println("turning off");
#endif
      turnOff(String(deviceId));
    }
    else
    {
#if DEBUG
      Serial.println("turning on");
#endif
      turnOn(String(deviceId));
    }
  }
}

int getDeviceConfigAddress(byte deviceId) {
  return sizeof(device) * deviceId;
}

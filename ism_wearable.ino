// Feather9x_RX
// -*- mode: C++ -*-

 /* RadioHead Driver */
#include <SPI.h>
#include <RH_RF95.h>

/* Haptic Motor Driver */
#include <Wire.h>
#include "Adafruit_DRV2605.h"
Adafruit_DRV2605 drv;
 
/* for feather m0 RFM9x */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3 
 
// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0
 
// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
 
// Blinky on receipt
#define LED 13

int buttonApin = 6;
bool vibrating = false;
int deviceId = 0;

void setup()
{
  pinMode(LED, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  pinMode(buttonApin, INPUT_PULLUP);
  digitalWrite(RFM95_RST, HIGH);
 
  Serial.begin(115200);

  // Uncomment if wanting device to be dependent on a serial connection. (i.e. debugging)
  /*while (!Serial) {
    delay(1);
  }*/
  delay(100);
 
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
 
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");
 
  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);
 
  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on
  // The default transmitter power is 13dBm, using PA_BOOST.
  rf95.setTxPower(23, false);

  /* Haptic Motor Setup */
  drv.begin();
  // I2C trigger by sending 'go' command 
  drv.setMode(DRV2605_MODE_INTTRIG); // default, internal trigger when sending GO command
  drv.selectLibrary(1);
  drv.setWaveform(0, 10);  // Vibration Pattern -> Double Tick - 100%
  drv.setWaveform(1, 44);  // Vibration Pattern -> Long Double Sharp Tick 1 - 100%
  drv.setWaveform(2, 0);  // end of waveforms

  /* Setup Random ID for device (unique per install) */
  randomSeed(analogRead(0));
  deviceId = random(1000, 10000);
  Serial.println("Generated ID: " + String(deviceId));
}


void loop()
{
  if (rf95.available())
  {
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
 
    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      RH_RF95::printBuffer("Received: ", buf, len);
      String _message = String((char*)buf);
      String _rssi = "RSSI: " + String(rf95.lastRssi());
      Serial.println("Got: " + _message + " (" + _rssi + ")");

      // Check the recieved message for a command from the homebase device
      CheckCommand(_message);

      // Send a response to the homebase
      SendReply();
      
      digitalWrite(LED, LOW);
    }
    else
    {
      Serial.println("Receive failed");
    }
  }

  if(vibrating)
  {
    // Play vibration pattern
    drv.go();
  }
  
}

void CheckCommand(String _command)
{
  if(_command.substring(0, 4) != "cmd:")
  {
    // Not a command. Exit.
    return;
  }
  
  String _idSubStr = _command.substring(4, 8);
  String _signalSubStr = _command.substring(8, 9);
  int _idExtract;
  bool _signalExtract;
  
  _idExtract = _idSubStr.toInt();
  _signalExtract = _signalSubStr.toInt(); 

  if(_idExtract == deviceId)
  {
    vibrating = _signalExtract;
    Serial.println("UPDATED VIBRATING STATUS BOOL ---> Status: " + String(vibrating));
  }
}

void SendReply()
{
  String _msgId = "'id':" + String(deviceId);
  String _msgVibrating = ",'vibrating':" + String((int)vibrating) + ",";
  String _msgSignalToHub = "'btn_press':";
  if(digitalRead(buttonApin) == LOW)
  {
    _msgSignalToHub = _msgSignalToHub + "1";
  }
  else
  {
    _msgSignalToHub = _msgSignalToHub + "0";
  }
  String _msgCombined = _msgId + _msgVibrating + _msgSignalToHub;
  char _radioPacket[38];
  
  for(int i = 0; i < (sizeof(_radioPacket) - 1); i++)
  {
    _radioPacket[i] = _msgCombined[i];
  }
  _radioPacket[37] = 0;
  
  rf95.send((uint8_t *)_radioPacket, sizeof(_radioPacket));
  rf95.waitPacketSent();
  Serial.println("Sent a reply");
}

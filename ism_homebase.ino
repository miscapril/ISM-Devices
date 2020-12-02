// Feather9x_TX
// -*- mode: C++ -*-

 /* RadioHead Driver */
#include <SPI.h>
#include <RH_RF95.h>

/* for feather m0  */
#define RFM95_CS 8
#define RFM95_RST 4
#define RFM95_INT 3

// Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

void setup() 
{
  pinMode(RFM95_RST, OUTPUT);
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
}

// packet counter, we increment per xmission
int16_t packetnum = 0;

void loop()
{
  delay(1000); // Wait 1 second between transmits

  // Check if we received a command from the serial connection (paired computer program)
  // then forward it to wearables
  CheckSendCommand();
  SendPing();
}

void CheckSendCommand()
{
  Serial.println("---> Checking Serial Input <---");
  String _serialBuffer = Serial.readString();
  
  if(sizeof(_serialBuffer) > 8)
  {
    String _prefix = _serialBuffer.substring(0, 4);
    if(_prefix == "cmd:")
    {
      char _command[9];
      for(int i = 0; i < 9; i++)
      {
        _command[i] = _serialBuffer[i];
      }
      rf95.send((uint8_t *)_command, sizeof(_command));
      rf95.waitPacketSent();
      Serial.println("Sending Command ---> " + _serialBuffer);
    }
  }
}

void SendPing()
{
  // Send a message to rf95_server  
  char radiopacket[20] = "Ping #      ";
  itoa(packetnum++, radiopacket+6, 10);
  Serial.println("Transmitting " + String(radiopacket));
  radiopacket[19] = 0;
  
  delay(10);
  rf95.send((uint8_t *)radiopacket, 20);

  Serial.println("Waiting for packet to complete..."); 
  delay(10);
  rf95.waitPacketSent();
  
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println("Waiting for reply...");
  if (rf95.waitAvailableTimeout(1000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
   {
      String _response = String((char*)buf);
      String _rssi = "'rssi':" + String(rf95.lastRssi());
      String _toSerial = "{" + _response + "," + _rssi + "}";
      Serial.print(_toSerial);
    }
    else
    {
      Serial.println("!!! Receive failed !!!");
    }
  }
  else
  {
    Serial.println("No response...");
  }
}

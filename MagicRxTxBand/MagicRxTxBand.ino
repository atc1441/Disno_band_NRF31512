#include "SPI.h"
// By @ATC1441 https://atcnetz.de
// This Scetch will show you the Disno MagicBands around and will try to wake them up periodically to enable their TX

// The nRF24L01 Pinout
const static uint8_t PIN_RADIO_CE = 12;
const static uint8_t PIN_RADIO_CSN = 16;
const static uint8_t PIN_RADIO_MOSI = 17;
const static uint8_t PIN_RADIO_MISO = 27;
const static uint8_t PIN_RADIO_SCK = 14;

void setup()
{
  Serial.begin(115200);
  delay(1000);
  pinMode(PIN_RADIO_CE, OUTPUT);
  digitalWrite(PIN_RADIO_CE, LOW);
  pinMode(PIN_RADIO_CSN, OUTPUT);
  digitalWrite(PIN_RADIO_CSN, HIGH);
  SPI.begin(PIN_RADIO_SCK, PIN_RADIO_MISO, PIN_RADIO_MOSI, PIN_RADIO_CSN);
}

uint8_t cha = 0;
uint32_t lastTime = 0;

uint8_t channels[] = {1, 24, 50, 76};
uint8_t TX_Channel = 82;


void RX_Round(uint8_t channel)
{
  writeRegister(0x01, 0x00);// Disable shockburst
  writeRegister(0x04, 0x00);// Setup retry 0
  writeRegister(0x05, channel);// Channel
  writeRegister(0x03, 0x02);//Address Width
  writeRegister(0x06, 0x06);
  uint8_t address0[4] = {0x5F, 0xCF, 0x04, 0x55};
  writeRegister(0x0a, &address0, 4);// Pipe 0
  writeRegister(0x11, 8);// 8-bytes RF Packets expected
  writeRegister(0x00, 0b01111111);// Enable RX with CRC 2 bytes and no IRQ

  digitalWrite(PIN_RADIO_CE, HIGH);
  delay(100);// Wait 100 ms for a radio Packet
  digitalWrite(PIN_RADIO_CE, LOW);
  writeRegister(0x00, 0x0e);
  uint8_t fifo_rx = (readRegister(0x17) & 1) == 0;

  if (fifo_rx) {// If a Packet was received print it out
    uint8_t len_in = readRegister(0x11);
    uint8_t in_data[32];
    spiTransfer(1, 0x61, &in_data, len_in);// Read Packet from radio

    Serial.printf("CH: %i MagicBand ID: %02X%02X%02X%02X%02X Counter: %i Extras: %02X %02X Raw: ", channel, in_data[0], in_data[1], in_data[2], in_data[3], in_data[4], in_data[5], in_data[6], in_data[7]);
    for (int i = 0; i < len_in; i++)
    {
      Serial.printf("0x%02X,", in_data[i]);
    }
    Serial.println("");
  }
}

uint8_t dataOut[6] = {3, 0, 0, 0, 0, 0};// This CMD array will enable the periodig sending of the Band ID
void TX_Round(uint8_t channel)
{
  writeRegister(0x01, 0x00);
  writeRegister(0x04, 0x00);
  writeRegister(0x05, channel);// Channel
  writeRegister(0x03, 0x03);//Address Width 5 bytes
  uint8_t addressRF[5] = {0xFF, 0x4B, 0xE4, 0x92, 0xE4};// Broadcast Address
  writeRegister(0x10, &addressRF, 5);
  writeRegister(0x06, 0x06);
  writeRegister(0x00, 0x0e);// Enable RX with 2 bytes CRC

  spiTransfer(0, 0xa0, dataOut, 6);
  digitalWrite(PIN_RADIO_CE, HIGH);
  delayMicroseconds(25);


  Serial.printf("Start sending Wakeup Packet for 10 Seconds: ");
  for (int a = 0; a < 6; a++)
    Serial.printf(" 0x%02X,", dataOut[a]);
  Serial.printf("\r\n");

  uint32_t wakeupTxTimeout = millis() + 10000;
  while (millis() < wakeupTxTimeout) {
    spiTransfer(0, 0xa0, dataOut, 6);// Resend TX Packet
    delayMicroseconds(100);
  }
  digitalWrite(PIN_RADIO_CE, LOW);
  writeRegister(0x07, 0x20);
}

uint32_t lastWakeupRound = 0;
void loop()
{
  RX_Round(channels[0]);
  if (millis() - lastWakeupRound >= 60000)
  {
    TX_Round(TX_Channel);// Blocks for around 10 Seconds
    Serial.println("Done sending back to RX");
    lastWakeupRound = millis();
  }
}

// SPI Functions are from this library: https://github.com/dparson55/NRFLite
void spiTransfer(bool transferType, uint8_t regName, void *data, uint8_t length)
{
  uint8_t* intData = reinterpret_cast<uint8_t*>(data);
  digitalWrite(PIN_RADIO_CSN, LOW);
  SPI.beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
  SPI.transfer(regName);
  for (uint8_t i = 0; i < length; ++i) {
    uint8_t newData = SPI.transfer(intData[i]);
    if (transferType == 1)
      intData[i] = newData;
  }
  SPI.endTransaction();
  digitalWrite(PIN_RADIO_CSN, HIGH);
}
uint8_t readRegister(uint8_t regName)
{
  uint8_t data;
  readRegister(regName, &data, 1);
  return data;
}
void readRegister(uint8_t regName, void *data, uint8_t length)
{
  spiTransfer(1, (0x1F & regName), data, length);
}
void writeRegister(uint8_t regName, uint8_t data)
{
  writeRegister(regName, &data, 1);
}
void writeRegister(uint8_t regName, void *data, uint8_t length)
{
  spiTransfer(0, (0x20 | (0x1F & regName)), data, length);
}
// END SPI Functions

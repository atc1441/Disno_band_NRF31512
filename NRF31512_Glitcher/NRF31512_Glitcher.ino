#include <SPI.h>
// By @ATC1441 https://atcnetz.de
// This Scetch will let you Fault inject an nRF31512 8051 and compatible SoC and dump its firmware content to the Serial Terminal

// The nRF31512 Pinout
#define NRF_PIN_FCSN    5
#define NRF_PIN_RESET   16
#define NRF_PIN_PROG    17
#define NRF_PIN_POWER   22
#define NRF_PIN_POWER1   25
#define NRF_PIN_POWER2   26
#define NRF_PIN_POWER3   27
#define NRF_PIN_GLITCH  4

SPIClass * vspi = NULL;
uint8_t spi_send(uint8_t data) {
  vspi->beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  uint8_t temp = vspi->transfer(data);
  vspi->endTransaction();
  return temp;
}

uint8_t temp_spi_data;
void setup() {
  Serial.begin(115200);
  delay(1500);

  vspi = new SPIClass(VSPI);
  vspi->begin(18, 19, 23, -1);

  pinMode(NRF_PIN_GLITCH, OUTPUT);
  digitalWrite(NRF_PIN_GLITCH, LOW);

  pinMode(NRF_PIN_POWER, OUTPUT);
  digitalWrite(NRF_PIN_POWER, LOW);
  pinMode(NRF_PIN_POWER1, OUTPUT);
  digitalWrite(NRF_PIN_POWER1, LOW);
  pinMode(NRF_PIN_POWER2, OUTPUT);
  digitalWrite(NRF_PIN_POWER2, LOW);
  pinMode(NRF_PIN_POWER3, OUTPUT);
  digitalWrite(NRF_PIN_POWER3, LOW);

  pinMode(NRF_PIN_PROG, OUTPUT);
  digitalWrite(NRF_PIN_PROG, HIGH);
  pinMode(NRF_PIN_RESET, OUTPUT);
  digitalWrite(NRF_PIN_RESET, HIGH);
  pinMode(NRF_PIN_FCSN, OUTPUT);
  digitalWrite(NRF_PIN_FCSN, HIGH);
  Serial.println("Lets start the glitch");
}

void loop() {
  for (int i = 0; i < 50000; i += 10)
  {
    digitalWrite(NRF_PIN_POWER, HIGH);
    digitalWrite(NRF_PIN_POWER1, HIGH);
    digitalWrite(NRF_PIN_POWER2, HIGH);
    digitalWrite(NRF_PIN_POWER3, HIGH);
    delayMicroseconds(i);
    digitalWrite(NRF_PIN_GLITCH, HIGH);
    delayMicroseconds(500);
    digitalWrite(NRF_PIN_GLITCH, LOW);
    delay(10);

    digitalWrite(NRF_PIN_FCSN, LOW);
    spi_send(0x05);// Read Status register
    temp_spi_data = spi_send(0x00);
    digitalWrite(NRF_PIN_FCSN, HIGH);
    Serial.printf("Delay: %i\r\n", i);
    if ((temp_spi_data & 0x04) == 0x00)
    {
      Serial.printf("Successfull after %i us, here is the Firmware dump:\r\n", i);
      digitalWrite(NRF_PIN_FCSN, LOW);
      spi_send(0x03);// Read CMD
      spi_send(0x00);// Address byte0
      spi_send(0x00);// Address byte1
      for (int a = 0; a < 18096; a++)
        Serial.printf("0x%02X,", spi_send(0x00));
      digitalWrite(NRF_PIN_FCSN, HIGH);
      // END
      Serial.printf("\r\nDONE\r\n");
      digitalWrite(NRF_PIN_PROG, LOW);
      digitalWrite(NRF_PIN_RESET, LOW);
      delay(10);
      digitalWrite(NRF_PIN_RESET, HIGH);
      SPI.end();
      while (1);
      //END
    }
    digitalWrite(NRF_PIN_POWER, LOW);
    digitalWrite(NRF_PIN_POWER1, LOW);
    digitalWrite(NRF_PIN_POWER2, LOW);
    digitalWrite(NRF_PIN_POWER3, LOW);
    delay(3);
  }
}

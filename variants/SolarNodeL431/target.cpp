#include <Arduino.h>
#include "target.h"
#include <helpers/ArduinoHelpers.h>

SolarNodeL431Board board;
static SPIClass spi = SPIClass(LORA_MOSI, LORA_MISO, LORA_SCLK);
CustomSX1262 radio1 = new Module(P_LORA_NSS, P_LORA_DIO_1, P_LORA_RESET, P_LORA_BUSY, spi);

CustomSX1262Wrapper radio_driver(radio1, board);

VolatileRTCClock rtc_clock;
SolarNodeL431Sensors sensors = SolarNodeL431Sensors();

#ifndef LORA_CR
  #define LORA_CR      5
#endif


bool radio_init() {  
#ifdef SX126X_TCXO_VOLTAGE
  float tcxo = SX126X_TCXO_VOLTAGE;
#else
  float tcxo = 1.6f;
#endif
  spi.begin();

  int status = radio1.begin(LORA_FREQ, LORA_BW, LORA_SF, LORA_CR, RADIOLIB_SX126X_SYNC_WORD_PRIVATE, LORA_TX_POWER, 8, tcxo);
  if (status != RADIOLIB_ERR_NONE) {
    Serial.print("ERROR: radio1 init failed: ");
    Serial.println(status);
    return false;  // fail
  }
  
  radio1.setCRC(1);
  
#if defined(SX126X_RXEN) && defined(SX126X_TXEN)
  radio.setRfSwitchPins(SX126X_RXEN, SX126X_TXEN);
#endif

#ifdef SX126X_CURRENT_LIMIT
  radio1.setCurrentLimit(SX126X_CURRENT_LIMIT);
#endif
#ifdef SX126X_DIO2_AS_RF_SWITCH
  radio1.setDio2AsRfSwitch(SX126X_DIO2_AS_RF_SWITCH);
#endif
#ifdef SX126X_RX_BOOSTED_GAIN
  radio1.setRxBoostedGainMode(SX126X_RX_BOOSTED_GAIN);
#endif

  return true;  // success
}

uint32_t radio_get_rng_seed() {
  return radio1.random(0x7FFFFFFF);
}

void radio_set_params(float freq, float bw, uint8_t sf, uint8_t cr) {
  radio1.setFrequency(freq);
  radio1.setSpreadingFactor(sf);
  radio1.setBandwidth(bw);
  radio1.setCodingRate(cr);
}

void radio_set_tx_power(uint8_t dbm) {
  radio1.setOutputPower(dbm);
}

mesh::LocalIdentity radio_new_identity() {
  RadioNoiseListener rng(radio1);
  return mesh::LocalIdentity(&rng);  // create new random identity
}

#pragma once

#include <variant_STM32L431CCT6.h>

// LoRa definitions
#define P_LORA_NSS            D12 //PA_12
#define P_LORA_DIO_1          D11 //PA_11
#define P_LORA_RESET          D15 //PA_15
#define P_LORA_BUSY           D31 //PB_15

#define P_LORA2_NSS            D30 //PB_14
#define P_LORA2_DIO_1          D29 //PB_13
#define P_LORA2_BUSY           D28 //PB_12
//custom name to avoid init in radiolib
#define LORA_SCLK             PIN_SPI_SCK
#define LORA_MISO             PIN_SPI_MISO
#define LORA_MOSI             PIN_SPI_MOSI
#ifndef LORA_TX_POWER
#define LORA_TX_POWER 22
#endif
#define SX126X_DIO2_AS_RF_SWITCH true
#define SX126X_TCXO_VOLTAGE 1.8
#define SX126X_CURRENT_LIMIT 140
#define SX126X_RX_BOOSTED_GAIN 1
//#define P_LORA_TX_LED PB_2

#define PIN_USER_BTN PH_3
#define FLASH_END_ADDR FLASH_END
/*
  -D P_LORA_DIO_1=3
  -D P_LORA_NSS=8
  -D P_LORA_RESET=5
  -D P_LORA_DIO_0=RADIOLIB_NC
  -D P_LORA_DIO_2=RADIOLIB_NC
  -D P_LORA_BUSY=4
  -D P_LORA_SCLK=10
  -D P_LORA_MISO=6
  -D P_LORA_MOSI=7
  -D SX126X_DIO2_AS_RF_SWITCH=true
  -D SX126X_DIO3_TCXO_VOLTAGE=1.8
  -D SX126X_CURRENT_LIMIT=140
  -D SX126X_RX_BOOSTED_GAIN=1*/
#undef RNG

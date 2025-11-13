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
#undef RNG

// HAL settings
#define HAL_QSPI_MODULE_ENABLED
#define HAL_LPTIM_MODULE_ENABLED
#define HAL_DAC_MODULE_ENABLED
#define PWM_RESOLUTION 12
// External memory IC SETTINGS
#define W25Q_FS
#define LFS_FLASH_TOTAL_SIZE ((1024 - 256) * 1024)
#define W25Q_DATA_LINES      2
/// Mem size in M-bit
#define W25Q_FLASH_SIZE      8U // 8 M-bit
/// Mem big block size in KB
#define W25Q_BLOCK_SIZE      64U // 64 KB: 256 pages
/// Mem small block size in KB
#define W25Q_SBLOCK_SIZE     32U // 32 KB: 128 pages
/// Mem sector size in KB
#define W25Q_SECTOR_SIZE     4U // 4 KB : 16 pages
/// Mem page size in bytes
#define W25Q_PAGE_SIZE       256U // 256 byte : 1 page
/// Blocks count
#define BLOCK_COUNT (W25Q_FLASH_SIZE * 2) 
/// Sector count
#define SECTOR_COUNT (BLOCK_COUNT * 16) 
/// Pages count
#define PAGE_COUNT (SECTOR_COUNT * 16)
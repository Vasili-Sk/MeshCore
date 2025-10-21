#include "target.h"

#include <Arduino.h>
#include <helpers/stm32/STM32Board.h>
#include <w25q_mem.h>

// Solar panel voltage, MCU current, Battery current, Solar current, temp sensor, v reference
uint16_t adc_values[6] = { 0 }; // IN5, IN10, IN11, IN12, TS, VREFINT
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;
QSPI_HandleTypeDef hqspi = {0};

float vSolar = 0;
float iSolar = 0;
float iMCU = 0;
float vBattery = 0;
float iBattery = 0;
float temperature = 0;
// VREFINT_CAL_ADDR
void SetupSTM_ADC();
void DMA_Init();
void setupClock26MHz();
void STM_QSPI_Init();
uint8_t buf[1024];

void SolarNodeL431Board::begin() {
  setupClock26MHz();
  Serial.end();
  Serial.begin(115200);

  Serial.print("MeshCore SolarNodeL431 starting..\n");
  STM32Board::begin();
  STM_QSPI_Init();
  W25Q_Init();
  //W25Q_EraseChip();
  int state = W25Q_ReadRaw(buf,1024, 0);

  Serial.printf("Core clock: %u\n", SystemCoreClock, state);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, 1);

  //RCC->BDCR &= ~RCC_BDCR_LSEON; // Clear LSEON bit
  pinMode(ENABLE_SENSORS, OUTPUT);
  pinMode(ENABLE_SHUNTS, OUTPUT);
  digitalWrite(ENABLE_SHUNTS, 0);

  pinMode(USER_BTN, INPUT);
  // ADC
  pinMode(ADC_VSOLAR, INPUT_ANALOG);
  pinMode(ADC_ISOLAR, INPUT_ANALOG);
  pinMode(ADC_IMCU, INPUT_ANALOG);
  pinMode(ADC_IBAT, INPUT_ANALOG);
  DMA_Init();
  SetupSTM_ADC();
}

const char *SolarNodeL431Board::getManufacturerName() const {
  return "VasiliSk";
}

uint16_t SolarNodeL431Board::getBattMilliVolts() {
  return vBattery * 1000;
}

void SolarNodeL431Board::setGpio(uint32_t values) {
  // set led values
  digitalWrite(LED_BUILTIN, values & 1);
}

uint32_t SolarNodeL431Board::getGpio() {
  // get led value
  return digitalRead(USER_BTN);
}

void SetupSTM_ADC() {
  __HAL_RCC_ADC_CLK_ENABLE();
  hadc1.Instance = ADC1;
  if (SystemCoreClock < 40000000)
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV64; // 26 MHz / 64 = 406kHz
  else
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV256; // 80 MHz / 256 = 312.5 kHz

  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 6; // 6 channels
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
  if (HAL_ADC_Init(&hadc1) != HAL_OK) {
    return;
  }

  // Configure channels
  ADC_ChannelConfTypeDef sConfig = { 0 };
  sConfig.SamplingTime = ADC_SAMPLETIME_640CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED; //for f**k sake STM... seriously!?

  sConfig.Channel = ADC_CHANNEL_5;
  sConfig.Rank = ADC_REGULAR_RANK_1; // IN5 (PA0)
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  sConfig.Channel = ADC_CHANNEL_10;
  sConfig.Rank = ADC_REGULAR_RANK_2; // IN10 (PA5)
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  sConfig.Channel = ADC_CHANNEL_11;
  sConfig.Rank = ADC_REGULAR_RANK_3; // IN11 (PA6)
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  sConfig.Channel = ADC_CHANNEL_12;
  sConfig.Rank = ADC_REGULAR_RANK_4; // IN12 (PA7)
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
  sConfig.Rank = ADC_REGULAR_RANK_5; // TS
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  sConfig.Channel = ADC_CHANNEL_VREFINT;
  sConfig.Rank = ADC_REGULAR_RANK_6; // VREFINT
  HAL_ADC_ConfigChannel(&hadc1, &sConfig);

  HAL_SYSCFG_EnableVREFBUF();
  HAL_SYSCFG_VREFBUF_VoltageScalingConfig(SYSCFG_VREFBUF_VOLTAGE_SCALE0);
  HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);

  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)&adc_values, 6);
  //HAL_ADC_Start_IT(&hadc1);
}

void DMA_Init() {
  __HAL_RCC_DMA1_CLK_ENABLE();
  hdma_adc1.Instance = DMA1_Channel1;
  hdma_adc1.Init.Request = DMA_REQUEST_0; // ADC
  hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
  hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
  hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
  hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
  hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
  hdma_adc1.Init.Mode = DMA_CIRCULAR;
  hdma_adc1.Init.Priority = DMA_PRIORITY_HIGH;
  if (HAL_DMA_Init(&hdma_adc1) != HAL_OK) {
    return;
  }
  __HAL_LINKDMA(&hadc1, DMA_Handle, hdma_adc1);
  // Enable DMA transfer complete interrupt
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
}

#define ADC_SCALE 4095.0f
extern "C" void DMA1_Channel1_IRQHandler(void) {
  if (__HAL_DMA_GET_FLAG(&hdma_adc1, DMA_FLAG_TC1)) {

    // 0- Solar panel voltage, 1-MCU current, 2-Battery current, 3-Solar current, 4-temp sensor, 5-v reference
    // DMA updates adc_values[0-5] (IN5, IN10, IN11, IN12, TS, VREFINT)
    int32_t vrefint_cal = *VREFINT_CAL_ADDR;
    float ts_cal1 = *TEMPSENSOR_CAL1_ADDR; // 30°C
    float ts_cal2 = *TEMPSENSOR_CAL2_ADDR; // 110°C

    // Calculate VDD (mV)
    vBattery = (TEMPSENSOR_CAL_VREFANALOG * vrefint_cal) / adc_values[5]  * 0.001f; // VREFINT
    iBattery = (vBattery * adc_values[2] / ADC_SCALE  - vBattery * (20.0f / (10.0f + 20.0f))) ; // 10mR, 100X multiplier, 1V = 1A, with offset

    // Calculate temperature (°C)
    temperature =
        ((float)((adc_values[4] * (int32_t)(vBattery * 1000) / TEMPSENSOR_CAL_VREFANALOG) - ts_cal1) *
         ((float)TEMPSENSOR_CAL2_TEMP - (float)TEMPSENSOR_CAL1_TEMP)) /
            (ts_cal2 - ts_cal1) +
        (float)TEMPSENSOR_CAL1_TEMP;

    vSolar = vBattery * adc_values[0] / ADC_SCALE * ((510.0f + 100.0f) / 100.0f) ;
    iSolar = vBattery * adc_values[3] / ADC_SCALE; // 10mR, 100X multiplier, 1V = 1A

    iMCU = vBattery * adc_values[1] / ADC_SCALE; // 10mR, 100X multiplier, 1V = 1A
    __HAL_DMA_CLEAR_FLAG(&hdma_adc1, DMA_FLAG_TC1);
  }
  HAL_DMA_IRQHandler(&hdma_adc1);
}

// Function to safely configure 26 MHz SYSCLK with Voltage Range 2
void setupClock26MHz() {
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  // Step 1: Switch to HSI (16 MHz) as temporary SYSCLK to ensure stability
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    Error_Handler();
  }

  // Step 2: Enable HSI and configure PLL for 26 MHz
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;  // HSI / 1 = 16 MHz
  RCC_OscInitStruct.PLL.PLLN = 13; // 16 * 13 = 208 MHz VCO
  RCC_OscInitStruct.PLL.PLLR = 8;  // 208 / 8 = 26 MHz
  RCC_OscInitStruct.PLL.PLLP = 7;  // For SAI (not used)
  RCC_OscInitStruct.PLL.PLLQ = 4;  // For USB/RNG (not used)
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
    Error_Handler();
  }

  // Step 3: Set voltage scaling to Range 2 (safe for 26 MHz)
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);
  HAL_Delay(1);  // Brief delay to stabilize voltage (1 ms)

  // Step 4: Switch SYSCLK to PLL (26 MHz) and update flash latency
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;  // HCLK = 26 MHz
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;   // APB1 = 26 MHz
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;   // APB2 = 26 MHz
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    Error_Handler();
  }

  // Step 5: Disable unused oscillators for power savings
  __HAL_RCC_MSI_DISABLE();
  __HAL_RCC_HSE_CONFIG(RCC_HSE_OFF);
  SystemCoreClockUpdate();
}


void STM_QSPI_Init(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable GPIO clocks
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_QSPI_CLK_ENABLE();

    // QSPI CLK (PB10), NCS (PB11), IO0 (PB1), IO1 (PB0)
    GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_10 | GPIO_PIN_11;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;  // AF10 for CLK/NCS
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    // Configure QSPI handle
    hqspi.Instance = QUADSPI;
    hqspi.Init.ClockPrescaler = 1;  // QSPI clock = SYSCLK / (1+1) = 40MHz (adjust for your flash)
    hqspi.Init.FifoThreshold = 1;   // FIFO threshold for interrupts/DMA
    hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE;  // Sample shift for stability
    hqspi.Init.FlashSize = 19;      // 1MB = 2^20 bytes, so FlashSize = log2(1MB) - 1 = 19
    hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE;  // CS high time
    hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;  // Clock polarity low, phase first edge
    hqspi.Init.FlashID = QSPI_FLASH_ID_1;      // Single flash chip
    hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;  // Single flash mode

    // Initialize QSPI
    if (HAL_QSPI_Init(&hqspi) != HAL_OK) {
        // Initialization error
        while (1) { /* Handle error */ }
    }
}


bool SolarNodeL431Sensors::begin() {
  return true;
}

bool SolarNodeL431Sensors::querySensors(uint8_t requester_permissions, CayenneLPP &telemetry) {
  if (requester_permissions & TELEM_PERM_BASE) {
    telemetry.addTemperature(TELEM_CHANNEL_SELF, temperature);
    //telemetry.addCurrent(TELEM_CHANNEL_SELF, iMCU);
    //telemetry.addPower(TELEM_CHANNEL_SELF, iMCU * vBattery);
  }
  next_available_channel = TELEM_CHANNEL_SELF + 1;

  if (requester_permissions & TELEM_PERM_ENVIRONMENT) {
    telemetry.addVoltage(next_available_channel, vSolar);
    telemetry.addCurrent(next_available_channel, iSolar);
    telemetry.addPower(next_available_channel, vSolar * iSolar * 1000);
    next_available_channel++;

    telemetry.addVoltage(next_available_channel, vBattery );
    //telemetry.addCurrent(next_available_channel, iBattery);
    //telemetry.addPower(next_available_channel, iBattery * vBattery);
    //telemetry.addFrequency(next_available_channel, SystemCoreClock);
    next_available_channel++;
  }
  
  return true;
}
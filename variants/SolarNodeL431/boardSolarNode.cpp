#include "target.h"

#include <Arduino.h>
#include <helpers/stm32/STM32Board.h>
#include <w25q_mem.h>
#include "adc_hal.h"
#include "mppt.h"

QSPI_HandleTypeDef hqspi = { 0 };

uint8_t buf[1024];
LPTIM_HandleTypeDef hlptim1;
// VREFINT_CAL_ADDR
void LPTIM1_Init();
void boardGPIOinit();
void setupClock26MHz();
void STM_QSPI_Init();

void SolarNodeL431Board::begin() {
  Serial.print("MeshCore SolarNodeL431 starting..\n");
  STM32Board::begin();
  STM_QSPI_Init();
  W25Q_Init();
  // W25Q_EraseChip();
  // int state = W25Q_ReadRaw(buf,1024, 0);
    HAL_DBGMCU_EnableDBGSleepMode();

  Serial.printf("Core clock: %u\n", SystemCoreClock);

  ADC_HAL_Init();
  MPPT_Init();
  LPTIM1_Init();
  //keep ultra low power while battery barely charged
  while (vBattery10hz < 3.1f) {
    __WFI(); 
  } 
  /*while (1) {
    __WFI(); 
  }*/
}

void initVariant(){
  boardGPIOinit();
  //setupClock26MHz();
}

void boardGPIOinit() {
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  // reset radio for low power
  digitalWrite(P_LORA_RESET, 0);
  pinMode(P_LORA_RESET, OUTPUT);

  digitalWrite(LED_BUILTIN, 0);
  pinMode(LED_BUILTIN, OUTPUT);

  // RCC->BDCR &= ~RCC_BDCR_LSEON; // Clear LSEON bit
  digitalWrite(ENABLE_SENSORS, 1);
  pinMode(ENABLE_SENSORS, OUTPUT);
  digitalWrite(ENABLE_SHUNTS, 0);
  pinMode(ENABLE_SHUNTS, OUTPUT);

  pinMode(USER_BTN, INPUT);
  // ADC
  pinMode(ADC_VSOLAR, INPUT_ANALOG);
  pinMode(ADC_ISOLAR, INPUT_ANALOG);
  pinMode(ADC_IMCU, INPUT_ANALOG);
  pinMode(ADC_IBAT, INPUT_ANALOG);
  pinMode(DAC_OUT, INPUT_ANALOG);
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



void LPTIM1_Init(void) {
  // 2. Enable LPTIM1 clock
  __HAL_RCC_LPTIM1_CLK_ENABLE();

  // 3. Select HSI16 as LPTIM1 clock source
  __HAL_RCC_LPTIM1_CONFIG(RCC_LPTIM1CLKSOURCE_HSI);

  // 4. LPTIM handle configuration
  hlptim1.Instance = LPTIM1;
  hlptim1.Init.Clock.Source = LPTIM_CLOCKSOURCE_APBCLOCK_LPOSC;
  hlptim1.Init.Clock.Prescaler = LPTIM_PRESCALER_DIV128; // No division → 16 MHz
  hlptim1.Init.Trigger.Source = LPTIM_TRIGSOURCE_SOFTWARE;
  hlptim1.Init.OutputPolarity = LPTIM_OUTPUTPOLARITY_HIGH;
  hlptim1.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
  hlptim1.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;

  if (HAL_LPTIM_Init(&hlptim1) != HAL_OK) {
    Error_Handler();
  }

  // 6. NVIC
  HAL_NVIC_SetPriority(LPTIM1_IRQn, 2, 0);
  HAL_NVIC_EnableIRQ(LPTIM1_IRQn);

  // Start 10 Hz timeout
  uint32_t arr = (HSI_VALUE / 128 / 100) - 1; 
  HAL_LPTIM_Counter_Start_IT(&hlptim1, arr);
  //use only LPTIM_IT_ARRM interrupt
  __HAL_LPTIM_DISABLE_IT(&hlptim1, LPTIM_IT_ARROK);
  __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_IT_ARROK);
}

// IRQ Handler
extern "C" void LPTIM1_IRQHandler(void) {
  ADC_HAL_Start();

  static int ledBlink = 0;
  ledBlink++;
  if (ledBlink == 10) {
    MPPT_Tick();
    digitalToggle(LED_BUILTIN);
    ledBlink = 0;
  }
  __HAL_LPTIM_CLEAR_FLAG(&hlptim1, LPTIM_FLAG_ARRM);
}





// Function to safely configure 26 MHz SYSCLK with Voltage Range 2
void setupClock26MHz() {
  RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };
  RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };

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
  HAL_Delay(1); // Brief delay to stabilize voltage (1 ms)

  // Step 4: Switch SYSCLK to PLL (26 MHz) and update flash latency
  RCC_ClkInitStruct.ClockType =
      RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1; // HCLK = 26 MHz
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;  // APB1 = 26 MHz
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;  // APB2 = 26 MHz
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK) {
    Error_Handler();
  }

  // Step 5: Disable unused oscillators for power savings
  __HAL_RCC_MSI_DISABLE();
  __HAL_RCC_HSE_CONFIG(RCC_HSE_OFF);
  SystemCoreClockUpdate();
}

void STM_QSPI_Init(void) {
  GPIO_InitTypeDef GPIO_InitStruct = { 0 };

  // Enable GPIO clocks
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_QSPI_CLK_ENABLE();

  // QSPI CLK (PB10), NCS (PB11), IO0 (PB1), IO1 (PB0)
  GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_10 | GPIO_PIN_11;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI; // AF10 for CLK/NCS
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  // Configure QSPI handle
  hqspi.Instance = QUADSPI;
  hqspi.Init.ClockPrescaler = 1; // QSPI clock = SYSCLK / (1+1) = 40MHz (adjust for your flash)
  hqspi.Init.FifoThreshold = 1;  // FIFO threshold for interrupts/DMA
  hqspi.Init.SampleShifting = QSPI_SAMPLE_SHIFTING_NONE; // Sample shift for stability
  hqspi.Init.FlashSize = 19; // 1MB = 2^20 bytes, so FlashSize = log2(1MB) - 1 = 19
  hqspi.Init.ChipSelectHighTime = QSPI_CS_HIGH_TIME_1_CYCLE; // CS high time
  hqspi.Init.ClockMode = QSPI_CLOCK_MODE_0;                  // Clock polarity low, phase first edge
  hqspi.Init.FlashID = QSPI_FLASH_ID_1;                      // Single flash chip
  hqspi.Init.DualFlash = QSPI_DUALFLASH_DISABLE;             // Single flash mode

  // Initialize QSPI
  if (HAL_QSPI_Init(&hqspi) != HAL_OK) {
    // Initialization error
    while (1) { /* Handle error */
    }
  }
}

bool SolarNodeL431Sensors::begin() {
  return true;
}

bool SolarNodeL431Sensors::querySensors(uint8_t requester_permissions, CayenneLPP &telemetry) {

  telemetry.addTemperature(TELEM_CHANNEL_SELF, temperature10hz);
  // telemetry.addCurrent(TELEM_CHANNEL_SELF, iMCU);
  // telemetry.addPower(TELEM_CHANNEL_SELF, iMCU * vBattery * 1000);

  next_available_channel = TELEM_CHANNEL_SELF + 1;
  telemetry.addVoltage(next_available_channel, vSolar10hz);
  telemetry.addCurrent(next_available_channel, iSolar10hz);
  telemetry.addPower(next_available_channel, pSolar10hz * 1000);

  next_available_channel++;
  // telemetry.addVoltage(next_available_channel, vBattery);
  // telemetry.addCurrent(next_available_channel, iBattery);
  // telemetry.addPower(next_available_channel, iBattery * vBattery);
  // telemetry.addFrequency(next_available_channel, SystemCoreClock);

  return true;
}
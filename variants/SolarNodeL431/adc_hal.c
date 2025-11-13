#include <Arduino.h>

#define ADC_SCALE 4095.0f
// Solar panel voltage, MCU current, Battery current, Solar current, temp sensor, v reference
uint16_t adc_values[6] = { 0 }; // IN5, IN10, IN11, IN12, TS, VREFINT
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

float vSolar = 0, vSolar10hz = 0;
float iSolar = 0, iSolar10hz = 0;
float pSolar = 0, pSolar10hz = 0;
float iMCU = 0;
float vBattery = 0, vBattery10hz = 0;
float iBattery = 0;
float temperature = 0, temperature10hz = 0;

void DMA_Init();

static float LPfilter(float old, float new, float filter_const) {
  old -= (filter_const) * (old - (new));
  return old;
}

void ADC_HAL_Init() {
  __HAL_RCC_ADC_CLK_ENABLE();
  DMA_Init();

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
  hadc1.Init.ContinuousConvMode = DISABLE;
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
  sConfig.SamplingTime = ADC_SAMPLETIME_92CYCLES_5;
  sConfig.SingleDiff = ADC_SINGLE_ENDED; // for f**k sake STM... seriously!?

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

  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)&adc_values, 6);
  // HAL_ADC_Start_IT(&hadc1);
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

void DMA1_Channel1_IRQHandler(void) {
  if (__HAL_DMA_GET_FLAG(&hdma_adc1, DMA_FLAG_TC1)) {

    // 0- Solar panel voltage, 1-MCU current, 2-Battery current, 3-Solar current, 4-temp sensor, 5-v reference
    // DMA updates adc_values[0-5] (IN5, IN10, IN11, IN12, TS, VREFINT)
    int32_t vrefint_cal = *VREFINT_CAL_ADDR;
    float ts_cal1 = *TEMPSENSOR_CAL1_ADDR; // 30°C
    float ts_cal2 = *TEMPSENSOR_CAL2_ADDR; // 110°C

    // Calculate VDD (mV)
    vBattery = (TEMPSENSOR_CAL_VREFANALOG * vrefint_cal) / adc_values[5] * 0.001f; // VREFINT
    iBattery = (vBattery * adc_values[2] / ADC_SCALE -
                vBattery * (20.0f / (10.0f + 20.0f))); // 10mR, 100X multiplier, 1V = 1A, with offset

    // Calculate temperature (°C)
    temperature =
        ((float)((adc_values[4] * (int32_t)(vBattery * 1000) / TEMPSENSOR_CAL_VREFANALOG) - ts_cal1) *
         ((float)TEMPSENSOR_CAL2_TEMP - (float)TEMPSENSOR_CAL1_TEMP)) /
            (ts_cal2 - ts_cal1) +
        (float)TEMPSENSOR_CAL1_TEMP;

    vSolar = vBattery * adc_values[0] / ADC_SCALE * ((510.0f + 100.0f) / 100.0f);
    iSolar = vBattery * adc_values[3] / ADC_SCALE; // 10mR, 100X multiplier, 1V = 1A

    iMCU = vBattery * adc_values[1] / ADC_SCALE; // 10mR, 100X multiplier, 1V = 1A
    __HAL_DMA_CLEAR_FLAG(&hdma_adc1, DMA_FLAG_TC1);

    vSolar10hz = LPfilter(vSolar10hz, vSolar, 0.05f);
    iSolar10hz = LPfilter(iSolar10hz, iSolar, 0.05f);
    vBattery10hz = LPfilter(vBattery10hz, vBattery, 0.05f);
    temperature10hz = LPfilter(temperature10hz, temperature, 0.05f);
    pSolar = vSolar * iSolar;
    pSolar10hz = LPfilter(pSolar10hz, pSolar, 0.05f);
  }
  HAL_DMA_IRQHandler(&hdma_adc1);
}

void ADC_HAL_Start() {
  HAL_ADC_Start_IT(&hadc1);
}
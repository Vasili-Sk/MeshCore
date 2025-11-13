#pragma once
extern float vSolar;
extern float iSolar;
extern float pSolar;
extern float iMCU;
extern float vBattery;
extern float iBattery;
extern float temperature;

extern float vSolar10hz;
extern float iSolar10hz;
extern float vBattery10hz;
extern float temperature10hz;
extern float pSolar10hz;

#ifdef __cplusplus
extern "C" {
#endif

void ADC_HAL_Init();
void ADC_HAL_Start();

#ifdef __cplusplus
}
#endif
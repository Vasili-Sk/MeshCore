#include "adc_hal.h"
#include <Arduino.h>

DAC_HandleTypeDef hdac1;
typedef struct {
  float ki;
  float kp;
  float integral;
  float min;
  float max;
  int16_t last_output;

  float Vpv_delta;
  float Ipv_delta;
  float Ppw_delta;
  float pSolar_last;
  float vSolar_last;
  float iSolar_last;
} MPPT_t;
MPPT_t mppt_state = { 0 };

void MPPT_Init() {
  mppt_state.ki = 20;
  mppt_state.kp = 50;
  mppt_state.max = 4095;

  analogWrite(A4, 0); // reset pin to ground
  // DAC1->MCR = DAC_MCR_MODE1_1; //disable buffer
}

int utils_threshold(float value, float tres) {
  if (value > tres)
    return 1;
  else if (value < -tres)
    return -1;
  return 0;
}

void MPPT_Tick() {
  mppt_state.Vpv_delta = vSolar10hz - mppt_state.vSolar_last;
  mppt_state.Ipv_delta = iSolar10hz - mppt_state.iSolar_last;
  mppt_state.Ppw_delta = pSolar10hz - mppt_state.pSolar_last;

  const uint16_t stepChange = 0.1f * 4095.0f / (vBattery10hz * MPPT_DIVIDER); // some value to see difference at i v
  bool changed = false;
  int dvChange = utils_threshold(mppt_state.Vpv_delta, 0.1f);
  if (dvChange == 0) {
    // small voltage change
    int diChange = utils_threshold(mppt_state.Ipv_delta, 0.002f);
    if (diChange > 0) {
      mppt_state.last_output += stepChange;
      mppt_state.iSolar_last = iSolar10hz;
    } else if (diChange < 0) {
      mppt_state.last_output -= stepChange * 2;
      mppt_state.iSolar_last = iSolar10hz;
    }
  } else {
    // big voltage change
    int dpvChange = utils_threshold(mppt_state.Ppw_delta / mppt_state.Vpv_delta, 0.005f);
    if (dpvChange > 0) {
      mppt_state.last_output += stepChange;
    } else if (dpvChange < 0) {
      mppt_state.last_output -= stepChange * 2;
    }
    mppt_state.vSolar_last = vSolar10hz;
    mppt_state.iSolar_last = iSolar10hz;
    mppt_state.pSolar_last = pSolar10hz;
  }

  if (vSolar10hz > 4.0f && iSolar10hz > 0.002f) {
    // try to limit dc-dc
    mppt_state.last_output++;
  } else {
    // stop mppt operation
    mppt_state.last_output--;
  }

  if (mppt_state.last_output > mppt_state.max)
    mppt_state.last_output = mppt_state.max;
  else if (mppt_state.last_output < mppt_state.min)
    mppt_state.last_output = mppt_state.min;

  analogWrite(A4, mppt_state.last_output); // reset pin to ground
}
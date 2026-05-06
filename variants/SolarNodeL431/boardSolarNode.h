#pragma once

#include <Arduino.h>
#include <helpers/stm32/STM32Board.h>

#define ADC_VSOLAR      A0
#define ADC_ISOLAR      A7
#define ADC_IMCU        A5
#define ADC_IBAT        A6
#define DAC_OUT         A4
#define ENABLE_SENSORS  D33 //PC_14
#define ENABLE_SHUNTS   D34 //PC_15


#ifdef __cplusplus

class SolarNodeL431Board : public STM32Board {
public:
    void begin() override;
    void sleep(int);
    const char* getManufacturerName() const override;
    uint16_t getBattMilliVolts() override;
    void setGpio(uint32_t values) override;
    uint32_t getGpio() override;
    bool startOTAUpdate(const char* id, char reply[]) override;
};

class SolarNodeL431Sensors : public SensorManager {
protected:
  int next_available_channel = TELEM_CHANNEL_SELF + 1;
public:
  SolarNodeL431Sensors(){};
  bool begin() override;
  bool querySensors(uint8_t requester_permissions, CayenneLPP& telemetry) override;
//  int getNumSettings() const override;
//  const char* getSettingName(int i) const override;
//  const char* getSettingValue(int i) const override;
//  bool setSettingValue(const char* name, const char* value) override;

};
#endif
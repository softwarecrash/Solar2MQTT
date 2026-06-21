# Datapoints and units

Available datapoints depend on the inverter type and on which values the inverter actually sends.

## Main groups

In MQTT and in the JSON output you will mainly see these groups:

- `RawData`: original inverter replies and command answers
- `EspData`: device information, Wi-Fi, MQTT status and ESP runtime sensors
- `DeviceData`: mostly fixed inverter information
- `LiveData`: current live inverter values and DS18B20 temperature sensors
- `Status`: connection and status information

## MQTT layout

With the normal MQTT topic layout:

- `<MQTT topic>/EspData/<name>`
- `<MQTT topic>/DeviceData/<name>`
- `<MQTT topic>/LiveData/<name>`
- `<MQTT topic>/RAW/<command>`
- `<MQTT topic>/Alive`
- `<MQTT topic>/DeviceControl/Set_Command_answer`

With JSON mode:

- `<MQTT topic>/Data`

If Home Assistant discovery is enabled together with JSON mode, the normal MQTT datapoint topics are still published as well.

## Common `EspData` points

| Datapoint | Unit | Notes |
|-----------|------|-------|
| `ESP_Internal_Temperature` | C | ESP internal temperature sensor, only on supported chips |

## Common `LiveData` points

| Datapoint | Unit | Notes |
|-----------|------|-------|
| `AC_In_Voltage` | V | Grid or input voltage |
| `AC_In_Frequenz` | Hz | Grid or input frequency |
| `AC_Out_Voltage` | V | Inverter output voltage |
| `AC_Out_Frequenz` | Hz | Inverter output frequency |
| `AC_Out_VA` | VA | Output apparent power |
| `AC_Out_Watt` | W | Output active power |
| `AC_Out_Percent` | % | Output load |
| `Battery_Voltage` | V | Battery voltage |
| `Battery_Charge_Current` | A | Charge current |
| `Battery_Discharge_Current` | A | Discharge current |
| `Battery_Percent` | % | Battery state of charge if available |
| `PV_Input_Voltage` | V | PV voltage |
| `PV_Input_Power` | W | PV power |
| `PV_Charging_Power` | W | Solar charging power |
| `PV1_Input_Voltage` | V | First PV tracker voltage |
| `PV1_Input_Current` | A | First PV tracker current |
| `PV1_Input_Power` | W | First PV tracker power |
| `PV2_Input_Voltage` | V | Second PV tracker voltage |
| `PV2_Input_Current` | A | Second PV tracker current |
| `PV2_Input_Power` | W | Second PV tracker power |
| `SCC_Charge_Power` | W | Charger power if available |
| `Inverter_Operation_Mode` | text | Standby / line / battery / fault etc. |
| `Device_Status` | text | Device status text |
| `Fault_Code` | text | Fault summary |
| `Warning_Code` | text | Warning summary |
| `DS18B20_<n>` | °C | External 1-Wire temperature sensors |

## SRNE phase `LiveData` points

These points are currently used by the Anenji/SRNE Modbus implementation when the inverter exposes the extended P02 inverter data block.

| Datapoint | Unit | Notes |
|-----------|------|-------|
| `AC_In_Voltage_L1` / `AC_In_Voltage_L2` / `AC_In_Voltage_L3` | V | Grid voltage per phase |
| `AC_In_Frequency_L1` / `AC_In_Frequency_L2` | Hz | Grid frequency where a protocol exposes per-phase frequency aliases |
| `AC_In_Current_L1` | A | Grid current for phase A where exposed by the base inverter block |
| `AC_Out_Voltage_L1` / `AC_Out_Voltage_L2` / `AC_Out_Voltage_L3` | V | Output voltage per phase |
| `AC_Out_Frequency_L1` / `AC_Out_Frequency_L2` | Hz | Output frequency where a protocol exposes per-phase frequency aliases |
| `AC_Out_Current_L1` / `AC_Out_Current_L2` / `AC_Out_Current_L3` | A | Load-side output current per phase |
| `AC_Out_Watt_L1` / `AC_Out_Watt_L2` / `AC_Out_Watt_L3` | W | Load active power per phase |
| `AC_Out_VA_L1` / `AC_Out_VA_L2` / `AC_Out_VA_L3` | VA | Load apparent power per phase |
| `AC_Out_Percent_L1` / `AC_Out_Percent_L2` / `AC_Out_Percent_L3` | % | Load ratio per phase |

## Common `DeviceData` points

| Datapoint | Unit | Notes |
|-----------|------|-------|
| `Protocol_ID` | text | Detected inverter protocol |
| `Device_Model` | text | Model string from inverter |
| `Machine_Type` | text | Inverter type information |
| `Battery_Rating_Voltage` | V | Nominal battery voltage |
| `Battery_Bulk_Voltage` | V | Bulk charging voltage |
| `Battery_Float_Voltage` | V | Float charging voltage |
| `Battery_Recharge_Voltage` | V | Recharge threshold |
| `Battery_Redischarge_Voltage` | V | Redischarge threshold |
| `Current_Max_AC_Charging_Current` | A | AC charging current limit |
| `Current_Max_Charging_Current` | A | Total charging current limit |
| `Output_Source_Priority` | text | Output source mode |
| `Charger_Source_Priority` | text | Charger source mode |
| `Solar_Power_Priority` | text | PV priority mode |

## RawData

Typical raw keys include:

- `QPI`
- `QMN`
- `QPIRI`
- `QPIGS`
- `QPIGS2`
- `QPIWS`
- `QALL`
- `Q1`
- `CommandAnswer`

This section is mainly useful when support or troubleshooting is needed.

## Notes

- Not every inverter exposes every field.
- Some values may appear under slightly different names depending on the inverter family.
- If a field is missing on your device, that does not automatically mean something is wrong.

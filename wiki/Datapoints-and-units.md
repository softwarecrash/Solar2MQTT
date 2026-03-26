# Datapoints and units

Available datapoints depend on the inverter type and on which values the inverter actually sends.

## Main groups

In MQTT and in the JSON output you will mainly see these groups:

- `RawData`: original inverter replies and command answers
- `EspData`: device information, Wi-Fi and MQTT status
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
| `PV_Input_Current` | A | PV current |
| `PV_Input_Power` | W | PV power |
| `PV_Charging_Power` | W | Solar charging power |
| `PV2_Input_Voltage` | V | Second PV tracker voltage |
| `PV2_Input_Current` | A | Second PV tracker current |
| `PV2_Input_Power` | W | Second PV tracker power |
| `SCC_Charge_Power` | W | Charger power if available |
| `Inverter_Operation_Mode` | text | Standby / line / battery / fault etc. |
| `Device_Status` | text | Device status text |
| `Fault_Code` | text | Fault summary |
| `Warning_Code` | text | Warning summary |
| `DS18B20_<n>` | °C | External 1-Wire temperature sensors |

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

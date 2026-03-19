# Solar2MQTT Protocol Reference

This file is the fixed anchor for protocols, device types, detection, and data fields in `Solar2MQTT`.
It should always reflect the current implementation state in the code, not just the PDFs stored in this folder.

Goals:
- document known protocol families and variants in one place
- document detection criteria
- collect visible data fields and raw commands as a reference
- make it easier to match new dumps against existing protocols
- make gaps for new protocols or variants obvious

## 1. Code Anchors

The actual implementation lives in these files:

- `src/solar/SolarTypes.h`
- `src/descriptors.h`
- `src/PI_Serial/PI_Serial.cpp`
- `src/PI_Serial/QPI.h`
- `src/PI_Serial/QMN.h`
- `src/PI_Serial/QPIRI.h`
- `src/PI_Serial/QPIGS.h`
- `src/PI_Serial/QPIGS2.h`
- `src/PI_Serial/QMOD.h`
- `src/PI_Serial/QFLAG.h`
- `src/PI_Serial/QPIWS.h`
- `src/PI_Serial/Q1.h`
- `src/PI_Serial/QEX.h`
- `src/modbus/device/must_pv_ph18/*`
- `src/modbus/device/anenji/*`
- `src/modbus/device/deye/*`

## 2. Protocol Sources In This Folder

Serial PI family:

- `PI16_VoltronicPowerSUNNYProtocol.md`
- `PI18_InfiniSolar-V-protocol-20170926.pdf`
- `PI30_Communication-Protocol-20150924-Customer.pdf`
- `PI30_HS_MS_MSX_RS232_Protocol_20140822_after_current_upgrade.pdf`
- `HS_MS_MSX RS232 Protocol 20140603B (1).pdf`
- `PI30MAX.Communication.Protocol20210217.pdf`
- `PI30REVO.20Protocol.20V03--20201112.pdf`
- `PI30_PIP-GK_MK-Protocol.pdf`
- `PI41_LV5048.5KW.protocol-20190222.for.customer.pdf`

Modbus / register tables:

- `ANENJI RS232.docx.pdf`
- `ToDo/PH1800 PV1800 EP1800 PV3500 EP3500 RS485 Modbud RTU communication Protocol 1.4.15 (1).xlsx`
- Deye Modbus V118 PDF in `ToDo/` (original file uses a non-ASCII filename)

Additional references / candidates:

- `ToDo/SMG-RS232+Communication+Protocol+V1.0.1-1.pdf`
- `raw data from devices/EASun sp-3200.txt`
- `raw data from devices/Phocos Anygrid PSW-H 5 kW.txt`

## 3. Currently Known Protocol Types In Code

| Code Type | Transport | Status | Parser Status | Notes |
| --- | --- | --- | --- | --- |
| `PI15` | UART ASCII | detected | raw-only | replies end up in `RawData`, no fixed field mapping |
| `PI16` | UART ASCII | detected | raw-only | replies end up in `RawData`, including `QPIBI` |
| `PI18` | UART `^P...` | detected | structured | dedicated `^P005PI`, `^P007PIRI`, `^P005GS`, `^P006MOD`, `^P007FLAG`, `^P005FWS`, energy commands |
| `PI30` | UART ASCII | detected | structured | generic PI30 base path |
| `PI30_MAX` | UART ASCII | detected | structured | MAX variant, extended `QFLAG` / `QPIWS` / `QPIRI` |
| `PI30_REVO` | UART ASCII | detected | structured | REVO variant with `QALL` layout |
| `PI30_PIP_GK` | UART ASCII | detected | structured | PIP-GK/MK variant with ECO mode / extended `QPIWS` |
| `PI41` | UART ASCII | detected | structured | PI41 / LV5048, including `QPIGS2` L2 layout |
| `PI30_UNKNOWN` | UART ASCII | detected | raw-first | replying PI-family device without a clean variant match |
| `MODBUS_MUST` | RS485 Modbus | detected | structured | MUST PV/PH18 |
| `MODBUS_ANENJI` | RS485 Modbus | detected | structured | Anenji |
| `MODBUS_DEYE` | RS485 Modbus | detected | partially structured | currently only core registers |

Not present as a dedicated code type:

- `SMG` from `ToDo/SMG-RS232+Communication+Protocol+V1.0.1-1.pdf`

## 4. Detection And Variant Refinement

### 4.1 Primary Detection

Serial PI devices:

- `QPI = 15` -> `PI15`
- `QPI = 16` -> `PI16`
- `QPI = 18` -> `PI18`
- `QPI = 30` -> `PI30`
- `QPI = 41` -> `PI41`
- any other numeric `QPI` -> `PI30_UNKNOWN`
- if `QPI` does not match, but `^P005PI = 18` -> `PI18`
- if `QPIRI`, `QPIGS`, or `QMOD` respond but no clean match exists -> `PI30_UNKNOWN`

Modbus:

- separate detection in the Modbus layer
- then mapped to `MODBUS_MUST`, `MODBUS_ANENJI`, or `MODBUS_DEYE`

### 4.2 Refinement Inside The PI30 Family

These signatures are evaluated after basic detection:

- `QALL` with at least 18 fields plus matching PI30 REVO layouts -> `PI30_REVO`
- `QPIGS2` with at least 11 fields -> `PI41`
- `QPIRI` with at least 28 fields -> `PI30_MAX`
- `QFLAG` contains `d` or `D` -> `PI30_MAX`
- `QMOD = E` -> `PI30_PIP_GK`
- `QPIWS` length >= 36 together with `QPIGS >= 21` and `QPIRI >= 25` -> `PI30_PIP_GK`
- replying PI device without a stronger signature -> `PI30`

### 4.3 Fallback For Unknown But Replying Devices

If a device replies but cannot be classified cleanly:

- the type remains `PI30_UNKNOWN` or `PI15` / `PI16`
- replies are still collected as raw data
- `RawData` and the debug report remain usable
- the goal is that users can send dumps even if no final parser exists yet

## 5. JSON Sections In The Project

These sections are relevant for protocol and dump analysis:

- `RawData`: raw command replies
- `DeviceData`: parsed static / configuration-related fields
- `LiveData`: parsed live values, counters, and status information
- `EspData`: ESP runtime data, not device-specific
- `Status`: detection, connection, and operating state

## 6. Raw Command Inventory

These raw replies can be stored in the project:

- `qpi`
- `qsvfw2`
- `qall`
- `qpiri`
- `qmd`
- `qpibi`
- `qmn`
- `qflag`
- `q1`
- `qpigs`
- `qpigs2`
- `qmod`
- `qt`
- `qet`
- `qey`
- `qem`
- `qed`
- `qlt`
- `qly`
- `qlm`
- `qld`
- `qpiws`
- `commandAnswer`

Minimum dump set for new unknown serial devices:

- `QPI`
- `QSVFW2`
- `QMD` or `QMN`
- `QPIRI`
- `QPIGS`
- `QPIGS2`
- `QMOD`
- `QFLAG`
- `QPIWS`
- `QALL`
- `Q1`
- `QPIBI` if available

## 7. Command Coverage By Family

| Family | Core Commands | Extra Detection / Variants |
| --- | --- | --- |
| `PI15` | `QPI`, `QSVFW2`, `QMD`, `QFLAG`, `QPIRI`, `QPIGS`, `QMOD`, `QPIWS` | raw-only |
| `PI16` | `QPI`, `QSVFW2`, `QMD`, `QFLAG`, `QPIRI`, `QPIGS`, `QMOD`, `QPIWS`, `QPIBI` | raw-only |
| `PI18` | `^P005PI`, `^P007PIRI`, `^P005GS`, `Q1`, `^P006MOD`, `^P007FLAG`, `^P005FWS` | `^P004T`, `^P013ED`, `^P011EM`, `^P009EY`, `^P005ET` |
| `PI30` | `QPI`, `QMN`, `QPIRI`, `QPIGS`, `QMOD`, `QFLAG`, `QPIWS`, `Q1` | `QET`, `QT`, `QEY`, `QEM`, `QED`, `QLT`, `QLY`, `QLM`, `QLD` |
| `PI30_MAX` | PI30 base | `QFLAG d`, `QPIRI >= 28`, extended `QPIWS` |
| `PI30_REVO` | PI30 base | `QALL`, REVO `QPIGS` tail |
| `PI30_PIP_GK` | PI30 base | `QMOD = E`, extended `QPIWS` |
| `PI41` | PI30 base | `QPIGS2` L2 layout |
| `PI30_UNKNOWN` | `QPI`, `QSVFW2`, `QMD`, `QFLAG`, `QPIRI`, `QPIGS`, `QMOD`, `QPIWS` | raw-first |
| `MODBUS_MUST` | holding registers from the MUST table | model / charger detection |
| `MODBUS_ANENJI` | holding registers from the Anenji table | serial number registers |
| `MODBUS_DEYE` | core holding registers | currently partial coverage |

## 8. Data Field Superset

Important:

- this is the superset of all JSON fields currently known by the code
- not every protocol provides every field
- unknown or incomplete devices can still be analyzed through `RawData`

### 8.1 Request And Error Status

- `NAK`
- `NOA`
- `ERCRC`

### 8.2 Identity And Base Status

- `Protocol_ID`
- `Device_Model`
- `Inverter_Operation_Mode`
- `Warning_Code`
- `Fault_Code`

### 8.3 QPIRI / ^P007PIRI: Static Ratings And Configuration Data

- `AC_In_Rating_Voltage`
- `AC_In_Rating_Current`
- `AC_Out_Rating_Voltage`
- `AC_Out_Rating_Frequency`
- `AC_Out_Rating_Current`
- `AC_Out_Rating_Apparent_Power`
- `AC_Out_Rating_Active_Power`
- `Battery_Rating_Voltage`
- `Battery_Recharge_Voltage`
- `Battery_Under_Voltage`
- `Battery_Bulk_Voltage`
- `Battery_Float_Voltage`
- `Battery_Type`
- `Current_Max_AC_Charging_Current`
- `Current_Max_Charging_Current`
- `Input_Voltage_Range`
- `Output_Source_Priority`
- `Charger_Source_Priority`
- `Parallel_Max_Num`
- `Machine_Type`
- `Topology`
- `Output_Mode`
- `Battery_Redischarge_Voltage`
- `PV_OK_Condition_For_Parallel`
- `PV_Power_Balance`
- `Max_Charging_Time_At_CV_Stage`
- `Operation_Logic`
- `Max_Discharging_Current`
- `Solar_Power_Priority`
- `MPPT_String`

Known layout variants:

- 21 fields: REVO base layout
- 25 fields: PI30 base layout
- 26 fields: PI30 with `Max_Charging_Time_At_CV_Stage`
- 28 fields: MAX layout with `Operation_Logic` and `Max_Discharging_Current`
- PI18 `^P007PIRI`: dedicated `P007` layout with `Solar_Power_Priority` and `MPPT_String`

### 8.4 QFLAG / ^P007FLAG: Static Flags

- `Buzzer_Enabled`
- `Overload_Bypass_Enabled`
- `Power_Saving_Enabled`
- `LCD_Reset_To_Default_Enabled`
- `Data_Log_Pop_Up`
- `Overload_Restart_Enabled`
- `Over_Temperature_Restart_Enabled`
- `LCD_Backlight_Enabled`
- `Primary_Source_Interrupt_Alarm_Enabled`
- `Record_Fault_Code_Enabled`
- `Solar_Feed_To_Grid_Enabled`

### 8.5 QPIGS / ^P005GS / QALL: Live Values

- `AC_In_Voltage`
- `AC_In_Frequency`
- `AC_Out_Voltage`
- `AC_Out_Frequency`
- `AC_Out_VA`
- `AC_Out_Watt`
- `AC_Out_Percent`
- `AC_In_Voltage_L2`
- `AC_In_Frequency_L2`
- `AC_Out_Voltage_L2`
- `AC_Out_Frequency_L2`
- `AC_Out_VA_L2`
- `AC_Out_Watt_L2`
- `AC_Out_Percent_L2`
- `Inverter_Bus_Voltage`
- `Battery_Voltage`
- `Battery_Voltage_L2`
- `Battery_Charge_Current`
- `Battery_Discharge_Current`
- `Battery_Percent`
- `Battery_Load`
- `Inverter_Bus_Temperature`
- `PV_Input_Current`
- `PV_Input_Voltage`
- `PV_Input_Power`
- `PV_Charging_Power`
- `Battery_SCC_Volt`
- `Battery_SCC2_Volt`
- `Status_Flag`
- `Battery_Voltage_Offset_Fans_On`
- `EEPROM_Version`
- `Device_Status`
- `Device_Status_L2`
- `Solar_Feed_To_Grid_Status`
- `Country`
- `Solar_Feed_To_Grid_Power`
- `MPPT1_Charger_Temperature`
- `MPPT2_Charger_Temperature`
- `PV_Generation_Day`
- `PV_Generation_Sum`
- `PV1_Input_Power`
- `PV2_Input_Power`
- `PV1_Input_Voltage`
- `PV2_Input_Voltage`
- `Configuration_State`
- `MPPT1_Charger_Status`
- `MPPT2_Charger_Status`
- `Load_Connection`
- `Battery_Power_Direction`
- `ACDC_Power_Direction`
- `Line_Power_Direction`
- `Local_Parallel_ID`

Derived fields:

- `Battery_Load = Battery_Charge_Current - Battery_Discharge_Current`
- `PV_Input_Power = PV_Input_Voltage * PV_Input_Current`
- `PV2_Input_Power = PV2_Input_Voltage * PV2_Input_Current`
- for `PI18`, `PV_Input_Voltage`, `PV_Charging_Power`, and `PV_Input_Current` are combined from `PV1_*` and `PV2_*`

### 8.6 QPIGS2: Additional Values

- `PV2_Input_Current`
- `PV2_Input_Voltage`
- `PV2_Charging_Power`

PI41 L2 layout:

- `AC_In_Voltage_L2`
- `AC_In_Frequency_L2`
- `AC_Out_Voltage_L2`
- `AC_Out_Frequency_L2`
- `AC_Out_VA_L2`
- `AC_Out_Watt_L2`
- `AC_Out_Percent_L2`
- `PV2_Input_Current`
- `PV2_Input_Voltage`
- `Battery_Voltage_L2`
- `Device_Status_L2`

### 8.7 Q1: Additional Sensors And Charge State

- `Time_Until_Absorb_Charge`
- `Time_Until_Float_Charge`
- `SCC_Flag`
- `Allow_SCC_On_Flag`
- `Charge_Average_Current`
- `Tracker_Temperature`
- `Inverter_Temperature`
- `Battery_Temperature`
- `Transformer_Temperature`
- `Fan_Lock_Status`
- `Fan_Speed`
- `SCC_Charge_Power`
- `Parallel_Warning`
- `Sync_Frequency`
- `Inverter_Charge_State`

Known `Inverter_Charge_State` text values:

- `No charging`
- `Bulk stage`
- `Absorb`
- `Float`

### 8.8 QET / QT / QEY / QEM / QED / QLT / QLY / QLM / QLD: Energy Counters

- `PV_Generation_Sum`
- `PV_Generation_Year`
- `PV_Generation_Month`
- `PV_Generation_Day`
- `AC_In_Generation_Sum`
- `AC_In_Generation_Year`
- `AC_In_Generation_Month`
- `AC_In_Generation_Day`

### 8.9 Modbus-Specific Additional Fields

- `Negative_Battery_Voltage`
- `Output_Current`
- `Output_Load_Percent`
- `Output_Power`
- `AC_Output_Current`
- `AC_Output_Frequency`
- `AC_Output_Power`
- `AC_Output_Voltage`
- `Positive_Battery_Voltage`
- `DCDC_Temperature`
- `AC_Radiator_Temperature`
- `Accumulated_Day`

## 9. Modbus Device Coverage

### 9.1 MUST PV/PH18

Core status:

- `Inverter_Operation_Mode`
- `PV_Charger_Workstate`
- `PV_Charger_MPPT_State`
- `PV_Charger_Charge_State`

Typical live values:

- `PV_Input_Voltage`
- `PV_Input_Current`
- `PV_Charging_Power`
- `MPPT1_Charger_Temperature`
- `Battery_Voltage`
- `AC_Out_Voltage`
- `AC_In_Voltage`
- `Inverter_Bus_Voltage`
- `Output_Current`
- `AC_Output_Current`
- `Output_Power`
- `AC_Out_Watt`
- `AC_Out_Percent`
- `AC_Out_Frequency`
- `AC_In_Frequency`
- `AC_Radiator_Temperature`
- `Transformer_Temperature`
- `Battery_Load`
- `PV_Generation_Sum`
- `Accumulated_Day`

Typical static values:

- `Battery_Type`
- `Battery_Float_Voltage`

### 9.2 ANENJI

Typical live values:

- `Inverter_Operation_Mode`
- `PV_Input_Power`
- `PV_Input_Voltage`
- `PV_Input_Current`
- `Battery_Percent`
- `Battery_Voltage`
- `Inverter_Temperature`
- `DCDC_Temperature`
- `Output_Load_Percent`

Typical static values:

- `Battery_Type`

Additional:

- serial number blocks `SN1` to `SN6`

### 9.3 DEYE

Currently only core coverage:

- `Inverter_Operation_Mode`
- `PV_Input_Voltage`
- `PV_Input_Current`
- `PV_Input_Power`
- `Output_Power`
- `AC_Output_Power`
- `Inverter_Temperature`
- `Battery_Temperature`
- `Battery_Voltage`
- `Battery_Percent`
- `AC_Out_Rating_Active_Power`

Notes:

- Deye is intentionally only partially covered right now
- full variant coverage needs more model-specific register tables

## 10. Known Dumps And Their Current Match

- `raw data from devices/EASun sp-3200.txt` -> generic `PI30` candidate
- `raw data from devices/Phocos Anygrid PSW-H 5 kW.txt` -> `PI30_MAX`-like variant with extended `QFLAG` / `QPIWS`

## 11. Decision Rules For New Dumps

A new dump should be checked in this order:

1. Is there a numeric `QPI`?
2. Is it `15`, `16`, `18`, `30`, or `41`?
3. Is there `QALL >= 18` together with REVO-like PI layouts -> REVO?
4. Is there `QPIGS2 >= 11` -> PI41?
5. Is there `QPIRI >= 28` or `QFLAG` with `d` -> MAX?
6. Is there `QMOD = E` or extended `QPIWS` -> PIP-GK/MK?
7. Does only the PI30 core set reply, but without a clean variant match -> `PI30` or `PI30_UNKNOWN`?
8. Does an ASCII PI command reply at all, but no field mapping fits -> keep the dump, preserve `RawData`, and evaluate whether a new type or new layout variant is needed

## 12. When A New Protocol Is Needed

A new dedicated protocol type makes sense if at least one of these applies:

- different transport format than an existing family
- different start characters / delimiters / checksum logic
- different core commands, not just different field counts
- different meaning of existing fields
- a stable dedicated variant signature that cannot be represented cleanly through existing arrays

No new family is needed when:

- only field count or command tail differs from a known command
- extra fields can be explained through `QALL`, `QPIGS2`, or `Q1`
- the device is just another PI30 sub-variant

## 13. Maintenance Notes

When code or protocol handling is extended, these items must be kept in sync:

- `src/solar/SolarTypes.h`
- parsers in `src/PI_Serial/*` or `src/modbus/device/*`
- `src/descriptors.h`
- this file `Protocol/PROTOCOL_REFERENCE.md`

When a new unknown device appears, at minimum archive:

- the full `RawData` block
- `Preview Report`
- the device model
- attached protocol / physical interface
- notes about which values look plausible or obviously wrong

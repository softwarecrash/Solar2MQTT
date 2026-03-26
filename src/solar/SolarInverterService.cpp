#include "solar/SolarInverterService.h"

#include <stdio.h>

#include "core/SettingsPrefs.h"
#include "main.h"
#include "solar/InverterHardware.h"

extern Settings _settings;

namespace
{
HardwareSerial g_inverterSerial(1);
constexpr uint32_t kSimulationIntervalMs = 1000;
constexpr char kSimulationProtocolId[] = "PI30";

uint32_t wavePosition(uint32_t tick, uint32_t period, uint32_t phase)
{
    if (period == 0)
    {
        return 0;
    }

    return (tick + phase) % period;
}

int triangleWaveInt(uint32_t tick, uint32_t period, int minValue, int maxValue, uint32_t phase = 0)
{
    if (period < 2 || maxValue <= minValue)
    {
        return minValue;
    }

    const uint32_t position = wavePosition(tick, period, phase);
    const uint32_t half = period / 2U;
    const int range = maxValue - minValue;

    if (position < half)
    {
        return minValue + static_cast<int>((static_cast<uint64_t>(range) * position) / half);
    }

    return maxValue - static_cast<int>((static_cast<uint64_t>(range) * (position - half)) / (period - half));
}

int centeredWaveInt(uint32_t tick, uint32_t period, int center, int amplitude, uint32_t phase = 0)
{
    return triangleWaveInt(tick, period, center - amplitude, center + amplitude, phase);
}

int mapIntClamped(int value, int inMin, int inMax, int outMin, int outMax)
{
    if (inMax <= inMin)
    {
        return outMin;
    }

    if (value < inMin)
    {
        value = inMin;
    }
    else if (value > inMax)
    {
        value = inMax;
    }

    const int numerator = (value - inMin) * (outMax - outMin);
    return outMin + (numerator / (inMax - inMin));
}

void formatFixed1(char *buffer, size_t bufferLen, int tenths)
{
    const long whole = tenths / 10;
    const long fraction = labs(static_cast<long>(tenths % 10));
    snprintf(buffer, bufferLen, "%ld.%01ld", whole, fraction);
}

void formatFixed2(char *buffer, size_t bufferLen, int hundredths)
{
    const long whole = hundredths / 100;
    const long fraction = labs(static_cast<long>(hundredths % 100));
    snprintf(buffer, bufferLen, "%ld.%02ld", whole, fraction);
}

const char *simulationModeText(char mode)
{
    switch (mode)
    {
    case 'B':
        return DESCR_Battery;
    case 'Y':
        return DESCR_Bypass;
    default:
        return "Line";
    }
}

const char *simulationChargeState(int chargeCurrent, int batteryPercent)
{
    if (chargeCurrent < 2)
    {
        return DESCR_No_Charging;
    }
    if (batteryPercent > 92)
    {
        return DESCR_Float;
    }
    if (batteryPercent > 80)
    {
        return DESCR_Absorb;
    }
    return DESCR_Bulk_Stage;
}
}

SolarInverterService::SolarInverterService(SolarState &state)
    : _state(state),
      _serial(g_inverterSerial),
      _client(nullptr),
      _simulationEnabled(false),
      _transportPaused(false),
      _simulationLastUpdateMs(0),
      _loopbackRequested(false),
      _loopbackInProgress(false),
      _loopbackDone(false),
      _loopbackOk(false),
      _loopbackWaitStart(0)
{
    _loopbackMessage.reserve(64);
}

SolarInverterService::~SolarInverterService()
{
    if (_client != nullptr)
    {
        delete _client;
        _client = nullptr;
    }
    _serial.end();
}

void SolarInverterService::begin()
{
    createClient();
}

void SolarInverterService::loop()
{
    handleSimulation();
    if (_simulationEnabled)
    {
        return;
    }

    handleLoopback();
    if (_client != nullptr && !_loopbackInProgress)
    {
        _client->loop();
    }
}

void SolarInverterService::reconfigure()
{
    createClient();
}

void SolarInverterService::setTransportPaused(bool paused)
{
    if (_transportPaused == paused)
    {
        return;
    }

    _transportPaused = paused;

    if (!_simulationEnabled)
    {
        _loopbackRequested = false;
        if (_loopbackInProgress && paused)
        {
            _loopbackInProgress = false;
            _loopbackDone = true;
            _loopbackOk = false;
            _loopbackMessage = "Loopback canceled while AP mode is active";
        }
    }

    if (_client != nullptr && !_simulationEnabled)
    {
        _client->setSuspend(paused);
        if (paused)
        {
            _client->connection = false;
        }
    }

    writeLog(paused ? "Inverter transport paused in AP mode"
                    : "Inverter transport resumed after AP mode");

    if (_callback)
    {
        _callback();
    }
}

void SolarInverterService::queueCommand(const String &command)
{
    String normalizedCommand = command;
    normalizedCommand.trim();

    if (normalizedCommand.isEmpty())
    {
        return;
    }

    if (_client != nullptr)
    {
        _client->get.raw.commandAnswer = "";
        _state.updateRaw("CommandAnswer", "");
    }

    if (handleSimulationCommand(normalizedCommand))
    {
        return;
    }

    if (_transportPaused && !_simulationEnabled)
    {
        if (_client == nullptr)
        {
            createClient();
        }
        setCommandAnswer("Inverter communication is paused while AP mode is active.");
        if (_callback)
        {
            _callback();
        }
        return;
    }

    if (_client == nullptr)
    {
        createClient();
        if (_client == nullptr)
        {
            return;
        }
    }

    _client->get.raw.commandAnswer = "";
    _state.updateRaw("CommandAnswer", "");

    if (_simulationEnabled)
    {
        if (normalizedCommand.equalsIgnoreCase("autodetect"))
        {
            setCommandAnswer("Simulation active. Use 'sim off' first.");
        }
        else if (normalizedCommand.startsWith("setp "))
        {
            setCommandAnswer("Simulation is fixed to PI30.");
        }
        else if (normalizedCommand.equalsIgnoreCase("QPI"))
        {
            setCommandAnswer(_client->get.raw.qpi);
        }
        else if (normalizedCommand.equalsIgnoreCase("QPIRI"))
        {
            setCommandAnswer(_client->get.raw.qpiri);
        }
        else if (normalizedCommand.equalsIgnoreCase("QMN"))
        {
            setCommandAnswer(_client->get.raw.qmn);
        }
        else if (normalizedCommand.equalsIgnoreCase("QFLAG"))
        {
            setCommandAnswer(_client->get.raw.qflag);
        }
        else if (normalizedCommand.equalsIgnoreCase("Q1"))
        {
            setCommandAnswer(_client->get.raw.q1);
        }
        else if (normalizedCommand.equalsIgnoreCase("QPIGS"))
        {
            setCommandAnswer(_client->get.raw.qpigs);
        }
        else if (normalizedCommand.equalsIgnoreCase("QPIGS2"))
        {
            setCommandAnswer(_client->get.raw.qpigs2);
        }
        else if (normalizedCommand.equalsIgnoreCase("QMOD"))
        {
            setCommandAnswer(_client->get.raw.qmod);
        }
        else if (normalizedCommand.equalsIgnoreCase("QT"))
        {
            setCommandAnswer(_client->get.raw.qt);
        }
        else if (normalizedCommand.equalsIgnoreCase("QET"))
        {
            setCommandAnswer(_client->get.raw.qet);
        }
        else if (normalizedCommand.startsWith("QEY"))
        {
            setCommandAnswer(_client->get.raw.qey);
        }
        else if (normalizedCommand.startsWith("QEM"))
        {
            setCommandAnswer(_client->get.raw.qem);
        }
        else if (normalizedCommand.startsWith("QED"))
        {
            setCommandAnswer(_client->get.raw.qed);
        }
        else if (normalizedCommand.equalsIgnoreCase("QLT"))
        {
            setCommandAnswer(_client->get.raw.qlt);
        }
        else if (normalizedCommand.startsWith("QLY"))
        {
            setCommandAnswer(_client->get.raw.qly);
        }
        else if (normalizedCommand.startsWith("QLM"))
        {
            setCommandAnswer(_client->get.raw.qlm);
        }
        else if (normalizedCommand.startsWith("QLD"))
        {
            setCommandAnswer(_client->get.raw.qld);
        }
        else if (normalizedCommand.equalsIgnoreCase("QALL"))
        {
            setCommandAnswer(_client->get.raw.qall);
        }
        else if (normalizedCommand.equalsIgnoreCase("QPIWS"))
        {
            setCommandAnswer(_client->get.raw.qpiws);
        }
        else
        {
            setCommandAnswer(DESCR_req_NAK);
        }

        if (_callback)
        {
            _callback();
        }
        return;
    }

    if (normalizedCommand.equalsIgnoreCase("autodetect"))
    {
        createClient();
        return;
    }

    if (normalizedCommand.startsWith("setp "))
    {
        const String parameterString = normalizedCommand.substring(5);
        const int parameter = parameterString.toInt();
        if (parameterString != "0" && parameter == 0)
        {
            writeLog("Invalid parameter for setp");
            return;
        }
        if (parameter >= NoD && parameter < PROTOCOL_TYPE_MAX)
        {
            _client->protocol = static_cast<protocol_type_t>(parameter);
            writeLog("Protocol forced to %s", protocolToString(_client->protocol));
        }
        return;
    }

    _client->sendCommand(normalizedCommand);
}

bool SolarInverterService::isConnected() const
{
    if (_simulationEnabled)
    {
        return true;
    }

    if (_transportPaused)
    {
        return false;
    }

    return _client != nullptr && _client->connection;
}

bool SolarInverterService::isBusy() const
{
    return _client != nullptr && _client->isBusy();
}

protocol_type_t SolarInverterService::protocol() const
{
    if (_simulationEnabled)
    {
        return PI30;
    }

    return _client != nullptr ? _client->protocol : NoD;
}

const char *SolarInverterService::protocolName() const
{
    return protocolToString(protocol());
}

bool SolarInverterService::requestLoopback()
{
    if (_simulationEnabled)
    {
        _loopbackDone = true;
        _loopbackOk = false;
        _loopbackMessage = "Loopback unavailable in simulation";
        return false;
    }

    if (_transportPaused)
    {
        _loopbackDone = true;
        _loopbackOk = false;
        _loopbackMessage = "Loopback unavailable while AP mode is active";
        return false;
    }

    if (_loopbackRequested || _loopbackInProgress || _client == nullptr)
    {
        return false;
    }

    _loopbackRequested = true;
    _loopbackDone = false;
    _loopbackOk = false;
    _loopbackMessage = "Loopback started";
    return true;
}

bool SolarInverterService::hasCommandAnswer() const
{
    return _client != nullptr && _client->get.raw.commandAnswer.length() > 0;
}

String SolarInverterService::consumeCommandAnswer()
{
    if (_client == nullptr)
    {
        return "";
    }

    const String answer = _client->get.raw.commandAnswer;
    _client->get.raw.commandAnswer = "";
    _state.updateRaw("CommandAnswer", "");
    return answer;
}

void SolarInverterService::createClient()
{
    _state.refreshBindings();

    if (_client != nullptr)
    {
        delete _client;
        _client = nullptr;
    }

    _serial.end();
    _state.doc()["DeviceData"].to<JsonObject>().clear();
    _state.doc()["LiveData"].to<JsonObject>().clear();
    _state.refreshBindings();

    g_inverterHardwareConfig.rxPin = _settings.get.inverterRxPin();
    g_inverterHardwareConfig.txPin = _settings.get.inverterTxPin();
    g_inverterHardwareConfig.dirPin = _settings.get.inverterDirPin();

    _client = new PI_Serial(_serial, g_inverterHardwareConfig.rxPin, g_inverterHardwareConfig.txPin);
    _client->setDelayTime(_settings.get.pollIntervalMs());
    _client->callback([this]()
                      {
        refreshRawState();
        if (_callback)
        {
            _callback();
        } });
    if (_simulationEnabled || _transportPaused)
    {
        _client->setSuspend(true);
    }
    _client->Init();
    if (_simulationEnabled)
    {
        updateSimulationState(false);
    }
    else
    {
        refreshRawState();
    }
    writeLog("Inverter transport ready on RX=%d TX=%d DIR=%d",
             g_inverterHardwareConfig.rxPin,
             g_inverterHardwareConfig.txPin,
             g_inverterHardwareConfig.dirPin);
}

void SolarInverterService::refreshRawState()
{
    if (_client == nullptr)
    {
        return;
    }

    _state.updateRaw("QPI", _client->get.raw.qpi);
    _state.updateRaw("QSVFW2", _client->get.raw.qsvfw2);
    _state.updateRaw("QPIRI", _client->get.raw.qpiri);
    _state.updateRaw("QMD", _client->get.raw.qmd);
    _state.updateRaw("QPIBI", _client->get.raw.qpibi);
    _state.updateRaw("QMN", _client->get.raw.qmn);
    _state.updateRaw("QFLAG", _client->get.raw.qflag);
    _state.updateRaw("Q1", _client->get.raw.q1);
    _state.updateRaw("QPIGS", _client->get.raw.qpigs);
    _state.updateRaw("QPIGS2", _client->get.raw.qpigs2);
    _state.updateRaw("QMOD", _client->get.raw.qmod);
    _state.updateRaw("QT", _client->get.raw.qt);
    _state.updateRaw("QET", _client->get.raw.qet);
    _state.updateRaw("QEY", _client->get.raw.qey);
    _state.updateRaw("QEM", _client->get.raw.qem);
    _state.updateRaw("QED", _client->get.raw.qed);
    _state.updateRaw("QLT", _client->get.raw.qlt);
    _state.updateRaw("QLY", _client->get.raw.qly);
    _state.updateRaw("QLM", _client->get.raw.qlm);
    _state.updateRaw("QLD", _client->get.raw.qld);
    _state.updateRaw("QALL", _client->get.raw.qall);
    _state.updateRaw("QPIWS", _client->get.raw.qpiws);
    _state.updateRaw("CommandAnswer", _client->get.raw.commandAnswer);
}

void SolarInverterService::handleLoopback()
{
    if (_simulationEnabled)
    {
        return;
    }

    if (_client == nullptr)
    {
        return;
    }

    if (_loopbackRequested && !_loopbackInProgress)
    {
        _loopbackRequested = false;
        _loopbackInProgress = true;
        _loopbackDone = false;
        _loopbackOk = false;
        _loopbackMessage = "Running...";
        _client->setSuspend(true);
        _loopbackWaitStart = millis();
        return;
    }

    if (!_loopbackInProgress)
    {
        return;
    }

    if (_client->isBusy())
    {
        if ((millis() - _loopbackWaitStart) > 5000)
        {
            _loopbackInProgress = false;
            _loopbackDone = true;
            _loopbackOk = false;
            _loopbackMessage = "Serial busy";
            _client->setSuspend(false);
        }
        return;
    }

    String details;
    const bool ok = _client->loopbackTest(details);
    _loopbackInProgress = false;
    _loopbackDone = true;
    _loopbackOk = ok;
    _loopbackMessage = details;
    _client->setSuspend(false);
}

void SolarInverterService::handleSimulation()
{
    if (!_simulationEnabled)
    {
        return;
    }

    const uint32_t now = millis();
    if (_simulationLastUpdateMs != 0 && (now - _simulationLastUpdateMs) < kSimulationIntervalMs)
    {
        return;
    }

    updateSimulationState(true);
}

void SolarInverterService::setCommandAnswer(const String &answer)
{
    if (_client == nullptr)
    {
        return;
    }

    _client->get.raw.commandAnswer = answer;
    _state.updateRaw("CommandAnswer", _client->get.raw.commandAnswer);
}

bool SolarInverterService::handleSimulationCommand(const String &command)
{
    if (!command.equalsIgnoreCase("sim on") &&
        !command.equalsIgnoreCase("sim off") &&
        !command.equalsIgnoreCase("sim status") &&
        !command.equalsIgnoreCase("sim"))
    {
        return false;
    }

    if (_client == nullptr)
    {
        createClient();
        if (_client == nullptr)
        {
            return true;
        }
    }

    if (command.equalsIgnoreCase("sim on"))
    {
        if (_simulationEnabled)
        {
            setCommandAnswer("Simulation PI30 already active.");
            if (_callback)
            {
                _callback();
            }
            return true;
        }

        enableSimulation();
        return true;
    }

    if (command.equalsIgnoreCase("sim off"))
    {
        if (!_simulationEnabled)
        {
            setCommandAnswer("Simulation is already disabled.");
            if (_callback)
            {
                _callback();
            }
            return true;
        }

        disableSimulation();
        return true;
    }

    setCommandAnswer(_simulationEnabled ? "Simulation PI30 active." : "Simulation disabled.");
    if (_callback)
    {
        _callback();
    }
    return true;
}

void SolarInverterService::enableSimulation()
{
    _state.refreshBindings();

    _simulationEnabled = true;
    _simulationLastUpdateMs = 0;
    _loopbackRequested = false;
    _loopbackInProgress = false;
    _loopbackDone = false;
    _loopbackOk = false;
    _loopbackMessage = "Simulation active";

    if (_client != nullptr)
    {
        _client->setSuspend(true);
    }

    _state.raw().clear();
    _state.doc()["DeviceData"].to<JsonObject>().clear();
    _state.doc()["LiveData"].to<JsonObject>().clear();
    _state.refreshBindings();
    updateSimulationState(false);
    setCommandAnswer("Simulation PI30 enabled.");

    writeLog("PI30 simulation enabled");
    if (_callback)
    {
        _callback();
    }
}

void SolarInverterService::disableSimulation()
{
    _state.refreshBindings();

    _simulationEnabled = false;
    _simulationLastUpdateMs = 0;

    writeLog("PI30 simulation disabled");
    createClient();
    _state.doc()["EspData"]["Simulation_Active"] = false;
    _state.doc()["EspData"]["Simulation_Mode"] = "";
    _state.doc()["Status"]["simulationEnabled"] = false;
    _state.doc()["Status"]["simulationProtocol"] = "";
    _state.refreshBindings();
    setCommandAnswer("Simulation disabled.");

    if (_callback)
    {
        _callback();
    }
}

void SolarInverterService::updateSimulationState(bool forceNotify)
{
    if (!_simulationEnabled || _client == nullptr)
    {
        return;
    }

    _state.refreshBindings();
    JsonDocument &doc = _state.doc();
    doc["DeviceData"].to<JsonObject>().clear();
    doc["LiveData"].to<JsonObject>().clear();

    auto setDevice = [&doc](const char *key, const auto &value)
    {
        doc["DeviceData"][key] = value;
    };

    auto setLive = [&doc](const char *key, const auto &value)
    {
        doc["LiveData"][key] = value;
    };

    const uint32_t now = millis();
    const uint32_t tick = now / 1000U;

    const int solarPercent = triangleWaveInt(tick, 46, 12, 94, 3);
    const int loadPercent = triangleWaveInt(tick, 34, 18, 66, 11);
    const int batteryPercent = triangleWaveInt(tick, 150, 44, 96, 27);
    const int chargeCurrent = mapIntClamped(solarPercent - loadPercent, 0, 80, 0, 48);
    const int dischargeCurrent = mapIntClamped(loadPercent - solarPercent, 0, 80, 0, 34);
    const int pvPower = mapIntClamped(solarPercent, 0, 100, 120, 3200);
    const int loadPower = mapIntClamped(loadPercent, 0, 100, 260, 2100);
    const int loadVa = loadPower + mapIntClamped(loadPercent, 0, 100, 40, 260);
    const int acInVoltage10 = centeredWaveInt(tick, 22, 2300, 18, 4);
    const int acOutVoltage10 = centeredWaveInt(tick, 18, 2298, 12, 7);
    const int acInFrequency10 = centeredWaveInt(tick, 28, 500, 2, 5);
    const int acOutFrequency10 = centeredWaveInt(tick, 32, 500, 1, 9);
    const int batteryVoltage100 = centeredWaveInt(tick, 90, 5180, 260, 14);
    const int pvVoltage10 = centeredWaveInt(tick, 40, 1560, 220, 1);
    const int pvCurrent10 = mapIntClamped(solarPercent, 0, 100, 8, 175);
    const int batterySccVolt100 = batteryVoltage100 + 36;
    const int busTemperature = centeredWaveInt(tick, 80, 36, 8, 2);
    const int trackerTemperature = centeredWaveInt(tick, 84, 31, 6, 8);
    const int inverterTemperature = centeredWaveInt(tick, 72, 34, 7, 15);
    const int batteryTemperature = centeredWaveInt(tick, 66, 29, 5, 20);
    const int transformerTemperature = centeredWaveInt(tick, 78, 33, 6, 25);
    const int chargeAverageCurrent = chargeCurrent > 0 ? chargeCurrent - 2 : 0;
    const int dailyPv = 6 + static_cast<int>((tick / 90U) % 7U);
    const int monthlyPv = 142 + static_cast<int>((tick / 50U) % 18U);
    const int yearlyPv = 1735 + static_cast<int>((tick / 25U) % 45U);
    const int totalPv = 12840 + static_cast<int>((tick / 8U) % 320U);
    const int dailyAcIn = 1 + static_cast<int>((tick / 160U) % 2U);
    const int monthlyAcIn = 18 + static_cast<int>((tick / 90U) % 5U);
    const int yearlyAcIn = 240 + static_cast<int>((tick / 40U) % 20U);
    const int totalAcIn = 1820 + static_cast<int>((tick / 18U) % 90U);
    const char modeChar = (batteryPercent < 54 || dischargeCurrent > (chargeCurrent + 6)) ? 'B' : 'L';

    char bufferA[32];
    char bufferB[32];
    char bufferC[160];

    setDevice(DESCR_Protocol_ID, kSimulationProtocolId);
    setDevice(DESCR_Device_Model, "Solar2MQTT Simulator");
    setDevice(DESCR_Buzzer_Enabled, true);
    setDevice(DESCR_Overload_Bypass_Enabled, true);
    setDevice(DESCR_Power_Saving_Enabled, false);
    setDevice(DESCR_LCD_Reset_To_Default_Enabled, false);
    setDevice(DESCR_Overload_Restart_Enabled, true);
    setDevice(DESCR_Over_Temperature_Restart_Enabled, true);
    setDevice(DESCR_LCD_Backlight_Enabled, true);
    setDevice(DESCR_Primary_Source_Interrupt_Alarm_Enabled, false);
    setDevice(DESCR_Record_Fault_Code_Enabled, true);
    setDevice(DESCR_AC_In_Rating_Voltage, 230.0);
    setDevice(DESCR_AC_In_Rating_Current, 21.7);
    setDevice(DESCR_AC_Out_Rating_Voltage, 230.0);
    setDevice(DESCR_AC_Out_Rating_Frequency, 50.0);
    setDevice(DESCR_AC_Out_Rating_Current, 21.7);
    setDevice(DESCR_AC_Out_Rating_Apparent_Power, 5000);
    setDevice(DESCR_AC_Out_Rating_Active_Power, 5000);
    setDevice(DESCR_Battery_Rating_Voltage, 48.0);
    setDevice(DESCR_Battery_Recharge_Voltage, 51.0);
    setDevice(DESCR_Battery_Under_Voltage, 46.0);
    setDevice(DESCR_Battery_Bulk_Voltage, 56.4);
    setDevice(DESCR_Battery_Float_Voltage, 54.0);
    setDevice(DESCR_Battery_Type, 1);
    setDevice(DESCR_Current_Max_AC_Charging_Current, 60);
    setDevice(DESCR_Current_Max_Charging_Current, 120);
    setDevice(DESCR_Input_Voltage_Range, 0);
    setDevice(DESCR_Output_Source_Priority, 2);
    setDevice(DESCR_Charger_Source_Priority, 1);
    setDevice(DESCR_Parallel_Max_Num, 1);
    setDevice(DESCR_Machine_Type, 2);
    setDevice(DESCR_Topology, 0);
    setDevice(DESCR_Output_Mode, 0);
    setDevice(DESCR_Battery_Redischarge_Voltage, 52.0);
    setDevice(DESCR_PV_OK_Condition_For_Parallel, 0);
    setDevice(DESCR_PV_Power_Balance, 1);
    setDevice(DESCR_Max_Charging_Time_At_CV_Stage, 120);
    setDevice(DESCR_Operation_Logic, 0);
    setDevice(DESCR_Max_Discharging_Current, 120);
    setDevice(DESCR_Solar_Power_Priority, 1);
    setDevice(DESCR_MPPT_String, 1);

    setLive(DESCR_AC_In_Voltage, acInVoltage10 / 10.0);
    setLive(DESCR_AC_In_Frequency, acInFrequency10 / 10.0);
    setLive(DESCR_AC_Out_Voltage, acOutVoltage10 / 10.0);
    setLive(DESCR_AC_Out_Frequency, acOutFrequency10 / 10.0);
    setLive(DESCR_AC_Out_VA, loadVa);
    setLive(DESCR_AC_Out_Watt, loadPower);
    setLive(DESCR_AC_Out_Percent, loadPercent);
    setLive(DESCR_Battery_Voltage, batteryVoltage100 / 100.0);
    setLive(DESCR_Battery_Charge_Current, chargeCurrent);
    setLive(DESCR_Battery_Percent, batteryPercent);
    setLive(DESCR_Inverter_Bus_Temperature, busTemperature);
    setLive(DESCR_PV_Input_Current, pvCurrent10 / 10.0);
    setLive(DESCR_PV_Input_Voltage, pvVoltage10 / 10.0);
    setLive(DESCR_Battery_SCC_Volt, batterySccVolt100 / 100.0);
    setLive(DESCR_Battery_Discharge_Current, dischargeCurrent);
    setLive(DESCR_Status_Flag, "00000000");
    setLive(DESCR_Battery_Voltage_Offset_Fans_On, 0.0);
    setLive(DESCR_EEPROM_Version, 22);
    setLive(DESCR_PV_Charging_Power, pvPower);
    setLive(DESCR_Device_Status, "000");
    setLive(DESCR_Solar_Feed_To_Grid_Status, 0);
    setLive(DESCR_Country, 0);
    setLive(DESCR_Solar_Feed_To_Grid_Power, 0);
    setLive(DESCR_Battery_Load, chargeCurrent - dischargeCurrent);
    setLive(DESCR_PV_Input_Power, pvPower);
    setLive(DESCR_Time_Until_Absorb_Charge, 18);
    setLive(DESCR_Time_Until_Float_Charge, 42);
    setLive(DESCR_SCC_Flag, "SIM");
    setLive(DESCR_Allow_SCC_On_Flag, true);
    setLive(DESCR_Charge_Average_Current, chargeAverageCurrent);
    setLive(DESCR_Tracker_Temperature, trackerTemperature);
    setLive(DESCR_Inverter_Temperature, inverterTemperature);
    setLive(DESCR_Battery_Temperature, batteryTemperature);
    setLive(DESCR_Transformer_Temperature, transformerTemperature);
    setLive(DESCR_Fan_Lock_Status, false);
    setLive(DESCR_Fan_Speed, 58 + static_cast<int>((tick + 7U) % 18U));
    setLive(DESCR_SCC_Charge_Power, pvPower);
    setLive(DESCR_Parallel_Warning, false);
    setLive(DESCR_Sync_Frequency, true);
    setLive(DESCR_Inverter_Charge_State, simulationChargeState(chargeCurrent, batteryPercent));
    setLive(DESCR_PV_Generation_Sum, totalPv);
    setLive(DESCR_PV_Generation_Year, yearlyPv);
    setLive(DESCR_PV_Generation_Month, monthlyPv);
    setLive(DESCR_PV_Generation_Day, dailyPv);
    setLive(DESCR_AC_In_Generation_Sum, totalAcIn);
    setLive(DESCR_AC_In_Generation_Year, yearlyAcIn);
    setLive(DESCR_AC_In_Generation_Month, monthlyAcIn);
    setLive(DESCR_AC_In_Generation_Day, dailyAcIn);
    setLive(DESCR_Inverter_Operation_Mode, simulationModeText(modeChar));
    setLive(DESCR_Fault_Code, "Ok");

    _client->protocol = PI30;
    _client->connection = true;
    _client->requestStaticData = false;
    _client->get.raw.qpi = kSimulationProtocolId;
    _client->get.raw.qmn = "SIM-PI30";
    _client->get.raw.qflag = "EabuvxyzDjk";
    _client->get.raw.qmod = String(modeChar);
    _client->get.raw.qpiws = "00000000000000000000000000000000";
    _client->get.raw.qt = "20260319";
    _client->get.raw.qet = String(totalPv);
    _client->get.raw.qey = String(yearlyPv);
    _client->get.raw.qem = String(monthlyPv);
    _client->get.raw.qed = String(dailyPv);
    _client->get.raw.qlt = String(totalAcIn);
    _client->get.raw.qly = String(yearlyAcIn);
    _client->get.raw.qlm = String(monthlyAcIn);
    _client->get.raw.qld = String(dailyAcIn);
    _client->get.raw.qall = _client->get.raw.qpiws;
    _client->get.raw.qpigs2 = DESCR_req_NOA;

    formatFixed1(bufferA, sizeof(bufferA), acInVoltage10);
    formatFixed1(bufferB, sizeof(bufferB), acInFrequency10);
    formatFixed1(bufferC, sizeof(bufferC), acOutVoltage10);
    String qpigs;
    qpigs.reserve(128);
    qpigs += bufferA;
    qpigs += ' ';
    qpigs += bufferB;
    qpigs += ' ';
    qpigs += bufferC;
    qpigs += ' ';
    formatFixed1(bufferA, sizeof(bufferA), acOutFrequency10);
    qpigs += bufferA;
    qpigs += ' ';
    qpigs += String(loadVa);
    qpigs += ' ';
    qpigs += String(loadPower);
    qpigs += ' ';
    qpigs += String(loadPercent);
    qpigs += ' ';
    formatFixed2(bufferA, sizeof(bufferA), batteryVoltage100);
    qpigs += bufferA;
    qpigs += ' ';
    qpigs += String(chargeCurrent);
    qpigs += ' ';
    qpigs += String(batteryPercent);
    qpigs += ' ';
    qpigs += String(busTemperature);
    qpigs += ' ';
    formatFixed1(bufferA, sizeof(bufferA), pvCurrent10);
    qpigs += bufferA;
    qpigs += ' ';
    formatFixed1(bufferA, sizeof(bufferA), pvVoltage10);
    qpigs += bufferA;
    qpigs += ' ';
    formatFixed2(bufferA, sizeof(bufferA), batterySccVolt100);
    qpigs += bufferA;
    qpigs += ' ';
    qpigs += String(dischargeCurrent);
    qpigs += " 00000000 ";
    qpigs += String(0);
    qpigs += ' ';
    qpigs += String(pvPower);
    qpigs += " 000";
    _client->get.raw.qpigs = qpigs;

    String qpiri;
    qpiri.reserve(160);
    qpiri += "230.0 21.7 230.0 50.0 21.7 5000 5000 48.0 51.0 46.0 56.4 54.0 1 60 120 0 2 1 1 2 52.0 0 1";
    _client->get.raw.qpiri = qpiri;

    String q1;
    q1.reserve(96);
    q1 += String(18);
    q1 += ' ';
    q1 += String(42);
    q1 += " SIM 1 ";
    q1 += String(chargeAverageCurrent);
    q1 += ' ';
    q1 += String(trackerTemperature);
    q1 += ' ';
    q1 += String(inverterTemperature);
    q1 += ' ';
    q1 += String(batteryTemperature);
    q1 += ' ';
    q1 += String(transformerTemperature);
    q1 += " 0 ";
    q1 += String(58 + static_cast<int>((tick + 7U) % 18U));
    q1 += ' ';
    q1 += String(pvPower);
    q1 += " 0 1 ";
    q1 += simulationChargeState(chargeCurrent, batteryPercent);
    _client->get.raw.q1 = q1;

    doc["EspData"]["Simulation_Active"] = true;
    doc["EspData"]["Simulation_Mode"] = kSimulationProtocolId;
    doc["Status"]["simulationEnabled"] = true;
    doc["Status"]["simulationProtocol"] = kSimulationProtocolId;
    _state.refreshBindings();

    refreshRawState();
    _simulationLastUpdateMs = now;

    if (forceNotify && _callback)
    {
        _callback();
    }
}

#include "PiReplyReader.h"
#include "SerialSettingsGuard.h"
#include "SerialSettingsGuard.h"
// #define isDEBUG
#include "ArduinoJson.h"
#include "PI_Serial.h"

#include <ctype.h>
// static
#include "QPI.h"
#include "QPIRI.h"
#include "QMN.h"
#include "QFLAG.h"
// variable
#include "Q1.h"
#include "QPIGS.h"
#include "QPIGS2.h"
#include "QMOD.h"
#include "QEX.h"
#include "QPIWS.h"
extern void writeLog(const char *format, ...);

namespace
{
constexpr unsigned long kPiConnectionHoldMs = 5000UL;

String sanitizeLogText(const String &value, size_t maxLen = 64)
{
    String sanitized;
    sanitized.reserve(maxLen);

    for (size_t i = 0; i < value.length() && sanitized.length() < maxLen; ++i)
    {
        const unsigned char c = static_cast<unsigned char>(value.charAt(i));
        if (c >= 32 && c <= 126)
        {
            sanitized += static_cast<char>(c);
        }
        else
        {
            sanitized += '.';
        }
    }

    if (value.length() > maxLen)
    {
        sanitized += "...";
    }

    return sanitized;
}

String sanitizePiReplyText(const String &value)
{
    if (value.isEmpty())
    {
        return value;
    }

    String sanitized;
    sanitized.reserve(value.length());

    bool lastWasSpace = true;
    for (size_t i = 0; i < value.length(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(value.charAt(i));
        if (c >= 33 && c <= 126)
        {
            sanitized += static_cast<char>(c);
            lastWasSpace = false;
        }
        else if (c == ' ' || c == '\t' || c == '\r' || c == '\n')
        {
            if (!lastWasSpace && !sanitized.isEmpty())
            {
                sanitized += ' ';
                lastWasSpace = true;
            }
        }
    }

    sanitized.trim();
    return sanitized;
}

bool hasOnlyPrintablePiReplyChars(const String &value)
{
    for (size_t i = 0; i < value.length(); ++i)
    {
        const unsigned char c = static_cast<unsigned char>(value.charAt(i));
        if (c < 32 || c > 126)
        {
            return false;
        }
    }
    return true;
}

int countDelimitedFields(const String &payload, char delimiter)
{
    if (payload.isEmpty() || payload == DESCR_req_NAK || payload == DESCR_req_NOA || payload == DESCR_req_ERCRC)
    {
        return 0;
    }

    char buffer[384];
    payload.toCharArray(buffer, sizeof(buffer));
    char *fields[40];
    return pi_split_fields(buffer, delimiter, fields, 40);
}

bool hasFlagSymbol(const String &flags, char symbol)
{
    for (size_t i = 0; i < flags.length(); ++i)
    {
        const char c = static_cast<char>(tolower(static_cast<unsigned char>(flags.charAt(i))));
        if (c == 'e' || c == 'd')
        {
            continue;
        }
        if (c == tolower(static_cast<unsigned char>(symbol)))
        {
            return true;
        }
    }
    return false;
}

bool isNumericProtocolId(const String &response, int &protocolId)
{
    if (!response.startsWith("PI"))
    {
        return false;
    }

    const String numericPart = response.substring(2);
    if (numericPart.isEmpty())
    {
        return false;
    }

    for (size_t i = 0; i < numericPart.length(); ++i)
    {
        if (!isDigit(static_cast<unsigned char>(numericPart.charAt(i))))
        {
            return false;
        }
    }

    protocolId = numericPart.toInt();
    return true;
}

bool hasRevoSignature(int qpiriFields, int qpigsFields, int qallFields)
{
    return qallFields >= 18 &&
           qpigsFields >= 21 &&
           qpiriFields > 0 &&
           qpiriFields <= 21;
}

void restoreJsonObject(JsonObject target, JsonObjectConst backup)
{
    target.clear();
    for (JsonPairConst entry : backup)
    {
        target[entry.key()] = entry.value();
    }
}

void copySelectedJsonKeys(JsonObject target,
                          JsonObjectConst source,
                          const char *const *keys,
                          size_t count)
{
    for (size_t i = 0; i < count; ++i)
    {
        const char *key = keys[i];
        JsonVariantConst value = source[key];
        if (!value.isNull())
        {
            target[key] = value;
        }
    }
}

bool hasExpectedResponsePrefix(const String &response, const char *startChar)
{
    if (startChar == nullptr || startChar[0] == '\0')
    {
        return true;
    }

    if (strcmp(startChar, "(") == 0)
    {
        return response.length() > 0 && response.charAt(0) == '(';
    }

    if (strcmp(startChar, "^Dxxx") == 0)
    {
        return response.length() > 1 && response.charAt(0) == '^' && response.charAt(1) == 'D';
    }

    return response.startsWith(startChar);
}

size_t matchedResponsePrefixLength(const String &response, const char *startChar, protocol_type_t protocol, const String &command)
{
    if (hasExpectedResponsePrefix(response, startChar))
    {
        return (startChar == nullptr) ? 0U : strlen(startChar);
    }

    // Some PI18 devices answer Q1 in the classic PI30-style frame although the
    // rest of the protocol uses ^Dxxx replies.
    if (protocol == PI18 && command == "Q1" && response.length() > 0 && response.charAt(0) == '(')
    {
        return 1U;
    }

    return 0U;
}

bool isEchoedCommand(const String &response, const String &command, const char *startChar)
{
    if (response.isEmpty() || command.isEmpty())
    {
        return false;
    }

    if (hasExpectedResponsePrefix(response, startChar))
    {
        return false;
    }

    return response.startsWith(command);
}

uint16_t calculatePiCrc(const uint8_t *data, size_t len)
{
    if (data == nullptr || len == 0)
    {
        return 0;
    }

    static constexpr uint16_t crcTable[16] = {
        0x0000, 0x1021, 0x2042, 0x3063,
        0x4084, 0x50A5, 0x60C6, 0x70E7,
        0x8108, 0x9129, 0xA14A, 0xB16B,
        0xC18C, 0xD1AD, 0xE1CE, 0xF1EF};

    uint16_t crc = 0;
    const uint8_t *ptr = data;
    while (len-- != 0)
    {
        uint8_t nibble = static_cast<uint8_t>((crc >> 8) >> 4);
        crc <<= 4;
        crc ^= crcTable[nibble ^ (*ptr >> 4)];

        nibble = static_cast<uint8_t>((crc >> 8) >> 4);
        crc <<= 4;
        crc ^= crcTable[nibble ^ (*ptr & 0x0F)];
        ++ptr;
    }

    uint8_t crcLow = lowByte(crc);
    uint8_t crcHigh = highByte(crc);
    if (crcLow == 0x28 || crcLow == 0x0D || crcLow == 0x0A)
    {
        ++crcLow;
    }
    if (crcHigh == 0x28 || crcHigh == 0x0D || crcHigh == 0x0A)
    {
        ++crcHigh;
    }

    return static_cast<uint16_t>(crcHigh << 8) | crcLow;
}

void copyJsonObject(JsonObject target, JsonObjectConst source)
{
    target.clear();
    if (source.isNull())
    {
        return;
    }

    for (JsonPairConst entry : source)
    {
        target[entry.key()] = entry.value();
    }
}

String variantToLogString(JsonVariantConst value)
{
    if (value.isNull())
    {
        return "";
    }

    String text;
    serializeJson(value, text);
    if (text.length() >= 2 && text.charAt(0) == '"' && text.charAt(text.length() - 1) == '"')
    {
        text = text.substring(1, text.length() - 1);
    }
    return sanitizeLogText(text, 48);
}

String lookupLogValue(JsonObjectConst object, const char *key)
{
    if (object.isNull() || key == nullptr || key[0] == '\0')
    {
        return "";
    }

    return variantToLogString(object[key]);
}

String firstLogValue(JsonObjectConst object, std::initializer_list<const char *> keys)
{
    for (const char *key : keys)
    {
        const String value = lookupLogValue(object, key);
        if (!value.isEmpty())
        {
            return value;
        }
    }
    return "";
}

String metricLogValue(JsonObjectConst object, std::initializer_list<const char *> keys, const char *suffix)
{
    String value = firstLogValue(object, keys);
    if (value.isEmpty())
    {
        return "-";
    }
    if (suffix != nullptr && suffix[0] != '\0')
    {
        value += suffix;
    }
    return value;
}
} // namespace
//---------------------------------------------------------------------- 
//  Public Functions
//----------------------------------------------------------------------

PI_Serial::PI_Serial(HardwareSerial &serialPort, int rx, int tx)
{
    this->my_serialIntf = &serialPort;
    _rxPin = rx;
    _txPin = tx;
    serialIntfBaud = 2400;
    get.raw.qpi.reserve(8);
    get.raw.qsvfw2.reserve(24);
    get.raw.qall.reserve(80);
    get.raw.qpiri.reserve(96);
    get.raw.qmd.reserve(64);
    get.raw.qpibi.reserve(80);
    get.raw.qmn.reserve(48);
    get.raw.qflag.reserve(24);
    get.raw.qbeqi.reserve(80);
    get.raw.q1.reserve(64);
    get.raw.qpigs.reserve(96);
    get.raw.qpigs2.reserve(24);
    get.raw.qmod.reserve(8);
    get.raw.qt.reserve(12);
    get.raw.qet.reserve(12);
    get.raw.qey.reserve(12);
    get.raw.qem.reserve(12);
    get.raw.qed.reserve(12);
    get.raw.qlt.reserve(12);
    get.raw.qly.reserve(12);
    get.raw.qlm.reserve(12);
    get.raw.qld.reserve(12);
    get.raw.commandAnswer.reserve(96);
    get.raw.qpiws.reserve(48);
    customCommandBuffer.reserve(32);
}

PI_Serial::~PI_Serial()
{
    if (modbus != nullptr)
    {
        delete modbus;
        modbus = nullptr;
    }
}

unsigned long PI_Serial::piReadTimeoutMs() const
{
    // Preserve the PR's baud-dependent first-byte allowance. The bounded
    // reader has a separate frame deadline and inter-byte stall budget.
    return PiReplyReader::firstByteTimeoutMs(serialIntfBaud);
}

void PI_Serial::beginSerial(unsigned int baud)
{
    // Autodetect and the loopback restore both re-open the port at a different
    // baud, so the timeout has to be recomputed every time rather than once in
    // Init(). Pairing the two here is what keeps them from drifting apart.
    serialIntfBaud = baud;
    this->my_serialIntf->setTimeout(piReadTimeoutMs());
    this->my_serialIntf->begin(baud, SERIAL_8N1, _rxPin, _txPin);
}

bool PI_Serial::Init()
{
    // Null check the serial interface
    if (this->my_serialIntf == NULL)
    {
        writeLog("No serial specificed!");
        return false;
    }
    if (suspendSerial.load(std::memory_order_relaxed))
    {
        this->beginSerial(serialIntfBaud);
        return true;
    }
    autoDetect();
    if (isModbus())
    {
        if (requestCallback)
        {
            modbus->callback(requestCallback);
        }
        return true;
    }
    // beginSerial() sets the timeout itself; this covers the branch where the
    // port is already open at the right baud and no begin() follows.
    this->my_serialIntf->setTimeout(piReadTimeoutMs());
    if (protocol == NoD)
    {
        this->beginSerial(serialIntfBaud);
    }
    return true;
}

bool PI_Serial::loop()
{
    if (suspendSerial.load(std::memory_order_relaxed))
    {
        return false;
    }

    const unsigned long now = millis();
    constexpr unsigned long kNoDeviceRetryMs = 30000UL;

    if (protocol == NoD)
    {
        if (now < nextDetectAt)
        {
            return false;
        }

        clearCycleBackup();
        autoDetect();
        previousTime = now;
        nextDetectAt = (protocol == NoD) ? (now + kNoDeviceRetryMs) : (now + delayTime);
        if (requestCallback)
        {
            requestCallback();
        }
        connection = false;
        return true;
    }

    if (now - previousTime > delayTime)
    {
        if (protocol != NoD)
        {
            if (sendCustomCommand())
            {
                clearCycleBackup();
                requestStaticData = true;
                requestCounter = 0;
                previousTime = millis();
                if (requestCallback)
                {
                    requestCallback();
                }
                return true;
            }
            if (isModbus())
            {
                modbus->loop();

                // The Modbus driver marks a completed PowMr live pass so the
                // outer PI layer can publish one coherent state update. Pure
                // MODBUS_POWMR stays Modbus-only. The explicit
                // MODBUS_POWMR_PI mode may add selected PI30-only fields.
                if (isPowMrProtocol(protocol) && modbus->consumePowMrLivePassCompleted())
                {
                    if (isPowMrPiHybridProtocol(protocol))
                    {
                        pollPowMrPiSupplement();
                    }

                    if (requestCallback)
                    {
                        requestCallback();
                    }
                }
            }
            else if (isRawOnlyPiProtocol(protocol))
            {
                bool requestOk = false;
                switch (requestStaticData)
                {
                case true:
                    requestOk = requestUnsupportedPiStatic();
                    if (requestOk)
                    {
                        requestCounter++;
                        if (requestCounter >= 4)
                        {
                            requestCounter = 0;
                            requestStaticData = false;
                            logStaticSummary();
                        }
                    }
                    else
                    {
                        requestCounter = 0;
                    }
                    break;

                case false:
                    requestOk = requestUnsupportedPiDynamic();
                    if (requestOk)
                    {
                        requestCounter++;
                        if (requestCounter >= 4)
                        {
                            requestCounter = 0;
                            refineProtocol();
                            logDynamicSummary();
                            markSuccessfulDynamicCycle();
                            if (requestCallback)
                            {
                                requestCallback();
                            }
                        }
                    }
                    else
                    {
                        requestCounter = 0;
                    }
                    break;
                }
            }
            else
            {
                switch (requestStaticData)
                {
                case true:
                    if (requestCounter == 0)
                    {
                        beginCycleBackup();
                    }
                    switch (requestCounter)
                    {
                    case 0:
                        if (PIXX_QPIRI())
                        {
                            requestCounter++;
                        }
                        else
                        {
                            restoreCycleBackup();
                            requestCounter = 0;
                        }
                        break;
                    case 1:
                        if (PIXX_QMN())
                        {
                            requestCounter++;
                        }
                        else
                        {
                            restoreCycleBackup();
                            requestCounter = 0;
                        }
                        break;
                    case 2:
                        if (PIXX_QPI())
                        {
                            requestCounter++;
                        }
                        else
                        {
                            restoreCycleBackup();
                            requestCounter = 0;
                        }
                        break;
                    case 3:
                        if (PIXX_QFLAG())
                        {
                            requestCounter++;
                            if (cycleHadSuccessfulReply)
                            {
                                clearCycleBackup();
                                logStaticSummary();
                            }
                            else
                            {
                                writeLog("[PI][WARN] proto=%s no valid static data", protocolToString(protocol));
                                restoreCycleBackup();
                                if (requestCallback)
                                {
                                    requestCallback();
                                }
                            }
                        }
                        else
                        {
                            restoreCycleBackup();
                            requestCounter = 0;
                        }
                        requestCounter = 0;
                        requestStaticData = false;
                        break;
                    }
                    break;

                case false:
                    if (requestCounter == 0)
                    {
                        beginCycleBackup();
                    }
                    switch (requestCounter)
                    {
                    case 0:
                        if (PIXX_QPIGS())
                        {
                            requestCounter++;
                        }
                        else
                        {
                            restoreCycleBackup();
                            requestCounter = 0;
                        }
                        break;
                    case 1:
                        if (PIXX_QPIGS2())
                        {
                            requestCounter++;
                        }
                        else
                        {
                            restoreCycleBackup();
                            requestCounter = 0;
                        }
                        break;
                    case 2:
                        if (PIXX_QMOD())
                        {
                            requestCounter++;
                        }
                        else
                        {
                            restoreCycleBackup();
                            requestCounter = 0;
                        }
                        break;
                    case 3:
                        if (PIXX_Q1())
                        {
                            requestCounter++;
                        }
                        else
                        {
                            restoreCycleBackup();
                            requestCounter = 0;
                        }
                        break;
                    case 4:
                        if (PIXX_QEX())
                        {
                            requestCounter++;
                        }
                        else
                        {
                            restoreCycleBackup();
                            requestCounter = 0;
                        }
                        break;
                    case 5:
                        if (PIXX_QPIWS())
                        {
                            requestCounter++;
                        }
                        else
                        {
                            restoreCycleBackup();
                            requestCounter = 0;
                        }
                        break;
                    case 6:
                        if (PIXX_QPIRI())
                        {
                            requestCounter++;
                        }
                        else
                        {
                            restoreCycleBackup();
                            requestCounter = 0;
                        }
                        break;
                    case 7:
                        refineProtocol();
                        if (cycleHadSuccessfulReply)
                        {
                            logDynamicSummary();
                            markSuccessfulDynamicCycle();
                            if (requestCallback)
                            {
                                requestCallback();
                            }
                            clearCycleBackup();
                        }
                        else
                        {
                            writeLog("[PI][WARN] proto=%s no valid dynamic data", protocolToString(protocol));
                            restoreCycleBackup();
                            if (requestCallback)
                            {
                                requestCallback();
                            }
                        }
                        requestCounter = 0;
                        break;
                    }
                    break;
                }
            }
            if (isModbus())
            {
                connection = modbus->connection;
            }
            else
            {
                connection = (lastSuccessfulDynamicCycleAt != 0) &&
                             ((millis() - lastSuccessfulDynamicCycleAt) <= kPiConnectionHoldMs);
            }
            previousTime = millis();
        }
    }
    return true;
}

void PI_Serial::beginCycleBackup()
{
    cycleLiveBackup.clear();
    cycleStaticBackup.clear();
    copyJsonObject(cycleLiveBackup.to<JsonObject>(), JsonObjectConst(liveData));
    copyJsonObject(cycleStaticBackup.to<JsonObject>(), JsonObjectConst(staticData));
    cycleBackupActive = true;
    cycleHadSuccessfulReply = false;
}

void PI_Serial::restoreCycleBackup()
{
    if (!cycleBackupActive)
    {
        return;
    }

    copyJsonObject(liveData, cycleLiveBackup.as<JsonObjectConst>());
    copyJsonObject(staticData, cycleStaticBackup.as<JsonObjectConst>());
    clearCycleBackup();
}

void PI_Serial::clearCycleBackup()
{
    cycleBackupActive = false;
    cycleHadSuccessfulReply = false;
    cycleLiveBackup.clear();
    cycleStaticBackup.clear();
}

void PI_Serial::markSuccessfulDynamicCycle()
{
    lastSuccessfulDynamicCycleAt = millis();
    connection = true;
    connectionCounter = 0;
}

void PI_Serial::logStaticSummary() const
{
    const String protocolName = sanitizeLogText(String(protocolToString(protocol)), 20);
    const String protocolId = firstLogValue(JsonObjectConst(staticData), {DESCR_Protocol_ID});
    const String model = firstLogValue(JsonObjectConst(staticData), {DESCR_Device_Model});

    if (isRawOnlyPiProtocol(protocol))
    {
        writeLog("[PI][STATIC] proto=%s id=%s model=%s raw(qpiri=%u qpigs=%u qmod=%u qpiws=%u)",
                 protocolName.c_str(),
                 protocolId.isEmpty() ? "-" : protocolId.c_str(),
                 model.isEmpty() ? "-" : model.c_str(),
                 static_cast<unsigned>(get.raw.qpiri.length()),
                 static_cast<unsigned>(get.raw.qpigs.length()),
                 static_cast<unsigned>(get.raw.qmod.length()),
                 static_cast<unsigned>(get.raw.qpiws.length()));
        return;
    }

    writeLog("[PI][STATIC] proto=%s id=%s model=%s",
             protocolName.c_str(),
             protocolId.isEmpty() ? "-" : protocolId.c_str(),
             model.isEmpty() ? "-" : model.c_str());
}

void PI_Serial::logDynamicSummary() const
{
    const JsonObjectConst live = JsonObjectConst(liveData);
    const String protocolName = sanitizeLogText(String(protocolToString(protocol)), 20);
    const String mode = firstLogValue(live, {DESCR_Inverter_Operation_Mode, DESCR_Inverter_Charge_State});
    const String pv = metricLogValue(live, {DESCR_PV_Input_Power, DESCR_PV_Charging_Power, DESCR_SCC_Charge_Power}, "W");
    const String load = metricLogValue(live, {DESCR_AC_Out_Watt}, "W");
    const String batteryVoltage = metricLogValue(live, {DESCR_Battery_Voltage, DESCR_Positive_Battery_Voltage}, "V");
    const String batteryPercent = metricLogValue(live, {DESCR_Battery_Percent}, "%");
    const String chargeCurrent = metricLogValue(live, {DESCR_Battery_Charge_Current}, "A");
    const String fault = firstLogValue(live, {DESCR_Fault_Code, DESCR_Warning_Code});

    if (isRawOnlyPiProtocol(protocol) &&
        pv == "-" &&
        load == "-" &&
        batteryVoltage == "-" &&
        batteryPercent == "-")
    {
        writeLog("[PI][OK] proto=%s raw(qpiri=%u qpigs=%u qmod=%u qpiws=%u)",
                 protocolName.c_str(),
                 static_cast<unsigned>(get.raw.qpiri.length()),
                 static_cast<unsigned>(get.raw.qpigs.length()),
                 static_cast<unsigned>(get.raw.qmod.length()),
                 static_cast<unsigned>(get.raw.qpiws.length()));
        return;
    }

    String battery = batteryVoltage;
    if (batteryPercent != "-")
    {
        battery += "/";
        battery += batteryPercent;
    }

    writeLog("[PI][OK] proto=%s mode=%s pv=%s load=%s batt=%s chg=%s fault=%s",
             protocolName.c_str(),
             mode.isEmpty() ? "-" : mode.c_str(),
             pv.c_str(),
             load.c_str(),
             battery.c_str(),
             chargeCurrent.c_str(),
             fault.isEmpty() ? "-" : fault.c_str());
}

void PI_Serial::callback(std::function<void()> func)
{
    requestCallback = func;
}

String PI_Serial::sendCommand(String command)
{
    if (command == "")
    {
        return command;
    }
    customCommandBuffer = command;
    return command;
}

bool PI_Serial::loopbackTest(String &details)
{
    details = "";
    if (this->my_serialIntf == nullptr)
    {
        details = "No serial interface";
        return false;
    }

    SerialSettingsGuard<HardwareSerial> restore(*my_serialIntf, SERIAL_8N1, _rxPin, _txPin, serialIntfBaud);
    const unsigned long baud = restore.baud();
    this->my_serialIntf->begin(baud, SERIAL_8N1, _rxPin, _txPin);
    this->my_serialIntf->setTimeout(200);

    while (this->my_serialIntf->available() > 0)
    {
        this->my_serialIntf->read();
    }

    const char *pattern = "S2MQT";
    const size_t patternLen = strlen(pattern);
    this->my_serialIntf->write(reinterpret_cast<const uint8_t *>(pattern), patternLen);
    this->my_serialIntf->flush();
    delay(20);

    char buffer[8] = {};
    size_t readLen = this->my_serialIntf->readBytes(buffer, patternLen);
    bool ok = (readLen == patternLen && memcmp(buffer, pattern, patternLen) == 0);
    // The guard restores the UART and timeout for either test result.
    details = ok ? "Loopback OK" : "Loopback failed";
    return ok;
}

void PI_Serial::setSuspend(bool enabled)
{
    suspendSerial.store(enabled, std::memory_order_relaxed);
    if (enabled)
    {
        abortAutoDetect.store(true, std::memory_order_relaxed);
    }
    else
    {
        abortAutoDetect.store(false, std::memory_order_relaxed);
    }
}

bool PI_Serial::isSuspended() const
{
    return suspendSerial.load(std::memory_order_relaxed);
}

bool PI_Serial::isBusy() const
{
    return busyCount.load(std::memory_order_relaxed) > 0;
}
//----------------------------------------------------------------------
// Private Functions
//----------------------------------------------------------------------
void PI_Serial::autoDetect() // function for autodetect the inverter type
{
    busyCount.fetch_add(1, std::memory_order_relaxed);
    protocol_type_t piFallbackProtocol = NoD;
    bool tryModbus = false;
    if (abortAutoDetect.load(std::memory_order_relaxed) || suspendSerial.load(std::memory_order_relaxed))
    {
        writeLog("[PI][DETECT] aborted");
        goto autodetect_done;
    }
    protocol = NoD;
    if (modbus != nullptr)
    {
        delete modbus;
        modbus = nullptr;
    }
    writeLog("[PI][DETECT] start");
    if (forcedProtocol != NoD)
    {
        protocol = forcedProtocol;
        if (isModbusProtocol(protocol))
        {
            this->my_serialIntf->end();
            modbus = new MODBUS(this->my_serialIntf, _rxPin, _txPin);
            if (modbus == nullptr || !modbus->Init() || !modbus->forceProtocol(protocol))
            {
                delete modbus;
                modbus = nullptr;
                protocol = NoD;
            }
        }
        else
        {
            serialIntfBaud = 2400;
            startChar = protocol == PI18 ? "^Dxxx" : "(";
            delimiter = protocol == PI18 ? "," : " ";
            this->beginSerial(serialIntfBaud);
        }
        writeLog("[PI][DETECT] forced proto=%s", protocolToString(protocol));
        goto autodetect_done;
    }
    for (size_t i = 0; i < 3; i++) // try 3 times to detect the inverter
    {
        if (abortAutoDetect.load(std::memory_order_relaxed) || suspendSerial.load(std::memory_order_relaxed))
        {
            writeLog("[PI][DETECT] aborted");
            goto autodetect_done;
        }

        startChar = "(";
        serialIntfBaud = 2400;
        this->beginSerial(serialIntfBaud);
        get.raw.qpi = this->requestData("QPI");
        if (abortAutoDetect.load(std::memory_order_relaxed) || suspendSerial.load(std::memory_order_relaxed))
        {
            writeLog("[PI][DETECT] aborted");
            goto autodetect_done;
        }
        int numericProtocolId = 0;
        if (isNumericProtocolId(get.raw.qpi, numericProtocolId))
        {
            delimiter = " ";
            switch (numericProtocolId)
            {
            case 15:
                protocol = PI15;
                writeLog("[PI][DETECT] match=PI15");
                break;
            case 16:
                protocol = PI16;
                writeLog("[PI][DETECT] match=PI16");
                break;
            case 18:
                protocol = PI18;
                delimiter = ",";
                startChar = "^Dxxx";
                writeLog("[PI][DETECT] match=PI18");
                break;
            case 41:
                protocol = PI41;
                writeLog("[PI][DETECT] match=PI41");
                break;
            case 30:
                protocol = PI30;
                writeLog("[PI][DETECT] match=PI30");
                break;
            default:
                protocol = PI30_UNKNOWN;
                writeLog("[PI][DETECT] match=PI%d (unsupported family)", numericProtocolId);
                break;
            }
            break;
        }
        startChar = "^Dxxx";
        this->beginSerial(serialIntfBaud);
        get.raw.qpi = this->requestData("^P005PI");
        if (abortAutoDetect.load(std::memory_order_relaxed) || suspendSerial.load(std::memory_order_relaxed))
        {
            writeLog("[PI][DETECT] aborted");
            goto autodetect_done;
        }
        if (get.raw.qpi != "" && get.raw.qpi == "18")
        {
            writeLog("[PI][DETECT] match=PI18");
            delimiter = ",";
            protocol = PI18;
            break;
        }

        startChar = "(";
        delimiter = " ";
        this->beginSerial(serialIntfBaud);
        get.raw.qpiri = this->requestData("QPIRI");
        if (isValidResponse(get.raw.qpiri))
        {
            protocol = PI30_UNKNOWN;
            writeLog("[PI][DETECT] unknown PI-family answered QPIRI");
            break;
        }

        get.raw.qpigs = this->requestData("QPIGS");
        if (isValidResponse(get.raw.qpigs))
        {
            protocol = PI30_UNKNOWN;
            writeLog("[PI][DETECT] unknown PI-family answered QPIGS");
            break;
        }

        get.raw.qmod = this->requestData("QMOD");
        if (isValidResponse(get.raw.qmod))
        {
            protocol = PI30_UNKNOWN;
            writeLog("[PI][DETECT] unknown PI-family answered QMOD");
            break;
        }
        this->my_serialIntf->end();
    }
    piFallbackProtocol = protocol;
    tryModbus = protocol == NoD || protocol == PI30 || protocol == PI30_UNKNOWN;
    if (tryModbus)
    {
        this->my_serialIntf->end();
    }
    if (tryModbus &&
        !abortAutoDetect.load(std::memory_order_relaxed) &&
        !suspendSerial.load(std::memory_order_relaxed))
    {
        modbus = new MODBUS(this->my_serialIntf, _rxPin, _txPin);
        if (modbus != nullptr)
        {
            modbus->Init();
            const protocol_type_t modbusProtocol = modbus->autoDetect(piFallbackProtocol == PI30);
            if (modbusProtocol != NoD)
            {
                protocol = modbusProtocol;
            }
            else
            {
                delete modbus;
                modbus = nullptr;
                protocol = piFallbackProtocol;
                if (protocol == PI30 || protocol == PI30_UNKNOWN)
                {
                    this->beginSerial(serialIntfBaud);
                }
            }
        }
    } 
    writeLog("[PI][DETECT] done proto=%s", protocolToString(protocol));
autodetect_done:
    if (busyCount.load(std::memory_order_relaxed) > 0)
    {
        busyCount.fetch_sub(1, std::memory_order_relaxed);
    }
}

void PI_Serial::refineProtocol()
{
    if (forcedProtocol != NoD)
    {
        return;
    }

    if (!isPi30LikeProtocol(protocol))
    {
        return;
    }

    const protocol_type_t previousProtocol = protocol;

    int numericProtocolId = 0;
    if (isNumericProtocolId(get.raw.qpi, numericProtocolId) && numericProtocolId == 41)
    {
        protocol = PI41;
        return;
    }

    const int qpiriFields = countDelimitedFields(get.raw.qpiri, delimiter[0]);
    const int qpigsFields = countDelimitedFields(get.raw.qpigs, delimiter[0]);
    const int qpigs2Fields = countDelimitedFields(get.raw.qpigs2, delimiter[0]);
    const int qallFields = countDelimitedFields(get.raw.qall, delimiter[0]);
    const bool hasQallLayout = qallFields >= 18;
    const bool hasModeEco = get.raw.qmod.equalsIgnoreCase("E");
    const bool hasFlagD = hasFlagSymbol(get.raw.qflag, 'd');
    const size_t qpiwsLength = isValidResponse(get.raw.qpiws) ? get.raw.qpiws.length() : 0;
    const bool revoSignature = hasRevoSignature(qpiriFields, qpigsFields, qallFields) ||
                               (previousProtocol == PI30_REVO && hasQallLayout && qpigsFields >= 21);

    if (revoSignature)
    {
        protocol = PI30_REVO;
        return;
    }

    if (qpiriFields >= 28 || hasFlagD)
    {
        protocol = PI30_MAX;
        return;
    }

    if (hasModeEco || (qpiwsLength >= 36 && qpigsFields >= 21 && qpiriFields >= 25))
    {
        protocol = PI30_PIP_GK;
        return;
    }

    if (protocol == PI30_UNKNOWN && (qpigsFields > 0 || qpiriFields > 0 || qpiwsLength > 0))
    {
        protocol = PI30;
    }

    if (protocol != previousProtocol)
    {
        writeLog("[PI][DETECT] refined=%s", protocolToString(protocol));
    }
}

bool PI_Serial::requestAndStoreRaw(const char *command, String &target, bool &hadSuccessfulReply)
{
    target = requestData(command);
    if (isValidResponse(target))
    {
        hadSuccessfulReply = true;
        return true;
    }
    return target == DESCR_req_NAK || target == DESCR_req_NOA || target == DESCR_req_ERCRC;
}

bool PI_Serial::requestUnsupportedPiStatic()
{
    bool hadSuccessfulReply = false;

    switch (requestCounter)
    {
    case 0:
        if (!requestAndStoreRaw("QPI", get.raw.qpi, hadSuccessfulReply))
        {
            return false;
        }
        if (isValidResponse(get.raw.qpi))
        {
            staticData[DESCR_Protocol_ID] = get.raw.qpi;
        }
        break;
    case 1:
        if (!requestAndStoreRaw("QSVFW2", get.raw.qsvfw2, hadSuccessfulReply))
        {
            return false;
        }
        break;
    case 2:
        if (!requestAndStoreRaw("QMD", get.raw.qmd, hadSuccessfulReply))
        {
            return false;
        }
        if (isValidResponse(get.raw.qmd))
        {
            staticData[DESCR_Device_Model] = get.raw.qmd;
        }
        break;
    case 3:
        if (!requestAndStoreRaw("QFLAG", get.raw.qflag, hadSuccessfulReply))
        {
            return false;
        }
        break;
    default:
        return true;
    }

    if (hadSuccessfulReply)
    {
        connectionCounter = 0;
    }
    return true;
}

bool PI_Serial::requestUnsupportedPiDynamic()
{
    bool hadSuccessfulReply = false;

    switch (requestCounter)
    {
    case 0:
        if (!requestAndStoreRaw("QPIRI", get.raw.qpiri, hadSuccessfulReply))
        {
            return false;
        }
        break;
    case 1:
        if (!requestAndStoreRaw("QPIGS", get.raw.qpigs, hadSuccessfulReply))
        {
            return false;
        }
        break;
    case 2:
        if (!requestAndStoreRaw("QMOD", get.raw.qmod, hadSuccessfulReply))
        {
            return false;
        }
        break;
    case 3:
        if (!requestAndStoreRaw("QPIWS", get.raw.qpiws, hadSuccessfulReply))
        {
            return false;
        }
        if (protocol == PI16)
        {
            requestAndStoreRaw("QPIBI", get.raw.qpibi, hadSuccessfulReply);
        }
        break;
    default:
        return true;
    }

    if (hadSuccessfulReply)
    {
        connectionCounter = 0;
    }
    return true;
}

void PI_Serial::backupPowMrNativeState(JsonDocument &staticBackup, JsonDocument &liveBackup) const
{
    staticBackup.set(staticData);
    liveBackup.set(liveData);
}

void PI_Serial::restorePowMrNativeState(JsonDocument &staticBackup, JsonDocument &liveBackup)
{
    restoreJsonObject(staticData, staticBackup.as<JsonObjectConst>());
    restoreJsonObject(liveData, liveBackup.as<JsonObjectConst>());
}

bool PI_Serial::runPowMrPiSupplementCommand(const char *command)
{
    if (!isPowMrPiHybridProtocol(protocol) || command == nullptr || command[0] == '\0')
    {
        return false;
    }

    JsonDocument staticBackup;
    JsonDocument liveBackup;
    backupPowMrNativeState(staticBackup, liveBackup);

    const protocol_type_t savedProtocol = protocol;
    const char *savedStartChar = startChar;
    const char *savedDelimiter = delimiter;
    const unsigned int savedBaud = serialIntfBaud;

    protocol = PI30_MAX;
    startChar = "(";
    delimiter = " ";
    serialIntfBaud = 2400;
    this->beginSerial(serialIntfBaud);

    bool ok = false;
    if (strcmp(command, "Q1") == 0)
    {
        ok = PIXX_Q1();
    }
    else if (strcmp(command, "QPIGS") == 0)
    {
        ok = PIXX_QPIGS();
    }
    else if (strcmp(command, "QPIRI") == 0)
    {
        ok = PIXX_QPIRI();
    }
    else if (strcmp(command, "QFLAG") == 0)
    {
        ok = PIXX_QFLAG();
    }
    else if (strcmp(command, "QBEQI") == 0)
    {
        get.raw.qbeqi = requestData("QBEQI");
        if (isValidResponse(get.raw.qbeqi))
        {
            char buffer[128];
            get.raw.qbeqi.toCharArray(buffer, sizeof(buffer));
            char *fields[12];
            const int fieldCount = pi_split_fields(buffer, ' ', fields, 12);
            if (fieldCount >= 1 &&
                (strcmp(fields[0], "0") == 0 || strcmp(fields[0], "1") == 0))
            {
                staticData[DESCR_Battery_Equalization_Enabled] = strcmp(fields[0], "1") == 0;
                if (fieldCount >= 9 &&
                    (strcmp(fields[8], "0") == 0 || strcmp(fields[8], "1") == 0))
                {
                    liveData[DESCR_Battery_Equalization_Active] = strcmp(fields[8], "1") == 0;
                }
                ok = true;
            }
        }
    }

    JsonDocument piStatic;
    JsonDocument piLive;
    piStatic.set(staticData);
    piLive.set(liveData);

    protocol = savedProtocol;
    startChar = savedStartChar;
    delimiter = savedDelimiter;
    serialIntfBaud = savedBaud;
    this->beginSerial(serialIntfBaud == 0 ? 2400 : serialIntfBaud);

    restorePowMrNativeState(staticBackup, liveBackup);

    if (!ok)
    {
        writeLog("[POWMR][PI+MODBUS] %s supplement failed", command);
        return false;
    }

    if (strcmp(command, "QPIRI") == 0)
    {
        static const char *const keys[] = {
            DESCR_AC_In_Rating_Voltage,
            DESCR_AC_In_Rating_Current,
            DESCR_AC_Out_Rating_Current,
            DESCR_AC_Out_Rating_Apparent_Power,
            DESCR_AC_Out_Rating_Active_Power,
            DESCR_Battery_Rating_Voltage,
            DESCR_Parallel_Max_Num,
            DESCR_Machine_Type,
            DESCR_Topology,
            DESCR_Output_Mode,
            DESCR_PV_OK_Condition_For_Parallel,
            DESCR_PV_Power_Balance,
            DESCR_Max_Charging_Time_At_CV_Stage,
            DESCR_Operation_Logic,
            DESCR_Max_Discharging_Current,
        };
        copySelectedJsonKeys(staticData,
                             piStatic.as<JsonObjectConst>(),
                             keys,
                             sizeof(keys) / sizeof(keys[0]));
    }
    else if (strcmp(command, "QPIGS") == 0)
    {
        static const char *const keys[] = {
            DESCR_Inverter_Bus_Voltage,
            DESCR_Inverter_Bus_Temperature,
            DESCR_PV1_Input_Current,
            DESCR_Battery_SCC_Volt,
            DESCR_EEPROM_Version,
            DESCR_PV_Charging_Power,
            DESCR_Device_Status,
            DESCR_Solar_Feed_To_Grid_Status,
            DESCR_Country,
            DESCR_Solar_Feed_To_Grid_Power,
        };
        copySelectedJsonKeys(liveData,
                             piLive.as<JsonObjectConst>(),
                             keys,
                             sizeof(keys) / sizeof(keys[0]));
    }
    else if (strcmp(command, "QBEQI") == 0)
    {
        static const char *const staticKeys[] = {
            DESCR_Battery_Equalization_Enabled,
        };
        copySelectedJsonKeys(staticData,
                             piStatic.as<JsonObjectConst>(),
                             staticKeys,
                             sizeof(staticKeys) / sizeof(staticKeys[0]));

        static const char *const liveKeys[] = {
            DESCR_Battery_Equalization_Active,
        };
        copySelectedJsonKeys(liveData,
                             piLive.as<JsonObjectConst>(),
                             liveKeys,
                             sizeof(liveKeys) / sizeof(liveKeys[0]));
    }
    else if (strcmp(command, "QFLAG") == 0)
    {
        static const char *const keys[] = {
            DESCR_Buzzer_Enabled,
            DESCR_Overload_Bypass_Enabled,
            DESCR_Power_Saving_Enabled,
            DESCR_LCD_Reset_To_Default_Enabled,
            DESCR_Data_Log_Pop_Up,
            DESCR_Overload_Restart_Enabled,
            DESCR_Over_Temperature_Restart_Enabled,
            DESCR_LCD_Backlight_Enabled,
            DESCR_Primary_Source_Interrupt_Alarm_Enabled,
            DESCR_Record_Fault_Code_Enabled,
            DESCR_Solar_Feed_To_Grid_Enabled,
        };
        copySelectedJsonKeys(staticData,
                             piStatic.as<JsonObjectConst>(),
                             keys,
                             sizeof(keys) / sizeof(keys[0]));
    }
    else if (strcmp(command, "Q1") == 0)
    {
        static const char *const keys[] = {
            DESCR_Time_Until_Absorb_Charge,
            DESCR_Time_Until_Float_Charge,
            DESCR_SCC_Flag,
            DESCR_Allow_SCC_On_Flag,
            DESCR_Charge_Average_Current,
            DESCR_Tracker_Temperature,
            DESCR_Battery_Temperature,
            DESCR_Transformer_Temperature,
            DESCR_Fan_Lock_Status,
            DESCR_Fan_Speed,
            DESCR_SCC_Charge_Power,
            DESCR_Parallel_Warning,
            DESCR_Sync_Frequency,
            DESCR_Inverter_Charge_State,
        };
        copySelectedJsonKeys(liveData,
                             piLive.as<JsonObjectConst>(),
                             keys,
                             sizeof(keys) / sizeof(keys[0]));
    }

    return true;
}

bool PI_Serial::pollPowMrPiSupplement()
{
    if (!isPowMrPiHybridProtocol(protocol))
    {
        return false;
    }

    const unsigned long now = millis();
    const PowMrPiSupplementQuery query =
        powMrPiSupplementScheduler.next(now,
                                        powMrPiFlagRefreshRequested,
                                        powMrPiEqualizationRefreshRequested,
                                        true);

    const char *command = nullptr;
    switch (query)
    {
    case PowMrPiSupplementQuery::QFLAG:
        command = "QFLAG";
        break;
    case PowMrPiSupplementQuery::QBEQI:
        command = "QBEQI";
        break;
    case PowMrPiSupplementQuery::QPIRI:
        command = "QPIRI";
        break;
    case PowMrPiSupplementQuery::QPIGS:
        command = "QPIGS";
        break;
    case PowMrPiSupplementQuery::Q1:
        command = "Q1";
        break;
    default:
        return false;
    }

    // Every attempt is tracked. Failed/unsupported/CRC-bad queries enter a
    // retry backoff, so they cannot consume every completed Modbus pass and
    // starve the remaining PI supplement queries.
    const bool ok = runPowMrPiSupplementCommand(command);
    powMrPiSupplementScheduler.record(query, now, ok);

    if (ok && query == PowMrPiSupplementQuery::QFLAG)
    {
        powMrPiFlagRefreshRequested = false;
    }
    else if (ok && query == PowMrPiSupplementQuery::QBEQI)
    {
        powMrPiEqualizationRefreshRequested = false;
    }

    return true;
}

bool PI_Serial::isValidResponse(const String &response) const
{
    return !response.isEmpty() &&
           response != DESCR_req_NAK &&
           response != DESCR_req_NOA &&
           response != DESCR_req_ERCRC;
}

bool PI_Serial::sendCustomCommand()
{
    if (customCommandBuffer == "")
        return false;

    if (isModbus() && isPowMrPiHybridProtocol(protocol) &&
        customCommandBuffer.startsWith("powmr pi "))
    {
        String piCommand = customCommandBuffer.substring(9);
        piCommand.trim();
        if (piCommand.isEmpty())
        {
            get.raw.commandAnswer = "ERROR: syntax powmr pi <PI30-command>";
        }
        else
        {
            // PowMr/Victor exposes both Modbus RTU and a Voltronic-like PI30
            // command interface on the same 2400-baud UART. Temporarily parse
            // the reply as PI30 without changing the active Modbus protocol.
            const protocol_type_t savedProtocol = protocol;
            const char *savedStartChar = startChar;
            const char *savedDelimiter = delimiter;
            const unsigned int savedBaud = serialIntfBaud;

            protocol = PI30_MAX;
            startChar = "(";
            delimiter = " ";
            serialIntfBaud = 2400;
            this->beginSerial(serialIntfBaud);
            get.raw.commandAnswer = requestData(piCommand);
            if ((piCommand.startsWith("PE") || piCommand.startsWith("PD")) &&
                piCommand.length() >= 3)
            {
                powMrPiFlagRefreshRequested = true;
            }
            if (piCommand.startsWith("PBEQE"))
            {
                powMrPiEqualizationRefreshRequested = true;
            }

            protocol = savedProtocol;
            startChar = savedStartChar;
            delimiter = savedDelimiter;
            serialIntfBaud = savedBaud;
            this->beginSerial(serialIntfBaud == 0 ? 2400 : serialIntfBaud);
        }
    }
    else if (isModbus())
    {
        get.raw.commandAnswer = modbus->requestData(customCommandBuffer);
    }
    else
    {
        get.raw.commandAnswer = requestData(customCommandBuffer);
    }
    customCommandBuffer = "";
    return true;
}

String PI_Serial::requestData(String command)
{

    String commandBuffer = "";
    commandBuffer.reserve(128);
    uint16_t crcCalc = 0;
    uint16_t crcRecive = 0;

    busyCount.fetch_add(1, std::memory_order_relaxed);
    while (this->my_serialIntf->available() > 0)
    {
        this->my_serialIntf->read();
    }
    this->my_serialIntf->write(command.c_str());
    this->my_serialIntf->write(highByte(getCRC(command)));
    this->my_serialIntf->write(lowByte(getCRC(command)));
    this->my_serialIntf->write(0x0D);
    this->my_serialIntf->flush();

    delay(20);
    struct Transport
    {
        HardwareSerial &serial;
        const std::atomic_bool &suspended;
        uint32_t now() const { return millis(); }
        int read() { return serial.read(); }
        void pause() { delay(1); }
        bool cancelled() const { return suspended.load(std::memory_order_relaxed); }
    } transport{*my_serialIntf, suspendSerial};
    char replyBuffer[PiReplyReader::kMaxReplyBytes];
    const auto reply = PiReplyReader::read(transport, replyBuffer, sizeof(replyBuffer),
                                           PiReplyReader::frameTimeoutMs(serialIntfBaud), piReadTimeoutMs());
    if (reply.status == PiReplyReader::Status::Complete)
    {
        commandBuffer.concat(replyBuffer, reply.length);
    }
    else if (reply.length > 0)
    {
        writeLog("[PI][WARN] cmd=%s incomplete reply status=%u bytes=%u",
                 command.c_str(), static_cast<unsigned>(reply.status), static_cast<unsigned>(reply.length));
    }

    const size_t cbLen = commandBuffer.length();
    const char *cbBuf = commandBuffer.c_str();
    const size_t prefixLen = matchedResponsePrefixLength(commandBuffer, startChar, protocol, command);

    if (cbLen >= 3 &&
        prefixLen > 0 &&
        getCRC(cbBuf, cbLen - 2) == 256U * (uint8_t)commandBuffer[cbLen - 2] + (uint8_t)commandBuffer[cbLen - 1] &&
        getCRC(cbBuf, cbLen - 2) != 0 && 256U * (uint8_t)commandBuffer[cbLen - 2] + (uint8_t)commandBuffer[cbLen - 1] != 0)
    {
        crcCalc = 256U * (uint8_t)commandBuffer[cbLen - 2] + (uint8_t)commandBuffer[cbLen - 1];
        crcRecive = getCRC(cbBuf, cbLen - 2);
        commandBuffer.remove(cbLen - 2); // remove the crc
        commandBuffer.remove(0, prefixLen); // remove the matched start frame

        if (!hasOnlyPrintablePiReplyChars(commandBuffer))
        {
            const String sanitizedReply = sanitizeLogText(commandBuffer, 72);
            writeLog("[PI][ERR] cmd=%s non-printable payload len=%u recv=\"%s\"",
                     command.c_str(),
                     static_cast<unsigned>(commandBuffer.length()),
                     sanitizedReply.c_str());
            connectionCounter++;
            commandBuffer = "ERCRC";
        }
        else
        {
            // requestOK++;
            connectionCounter = 0;
        }
    }
    else if (cbLen >= 2 &&
             prefixLen > 0 &&
             getCHK(cbBuf, cbLen - 1) + 1 == commandBuffer[cbLen - 1] &&
             getCHK(cbBuf, cbLen - 1) + 1 != 0 && commandBuffer[cbLen - 1] != 0 &&
             command == "QALL" // crude fix for the qall chk thing
             )                 // CHK for QALL
    {
        crcCalc = getCHK(cbBuf, cbLen - 1) + 1;
        crcRecive = commandBuffer[cbLen - 1];
        commandBuffer.remove(cbLen - 1); // remove the crc
        commandBuffer.remove(0, prefixLen); // remove the matched start frame

        // requestOK++;
        connectionCounter = 0;
    }
    else if (commandBuffer == "NAK" ||
             (cbLen >= (strlen(startChar) + 3) &&
              memcmp(cbBuf + strlen(startChar), "NAK", 3) == 0)) // catch NAK without crc
    {
        commandBuffer = "NAK";
    }
    else if (commandBuffer == "") // catch empty answer, its similar to NAK
    {
        writeLog("[PI][WARN] cmd=%s no answer", command.c_str());
        commandBuffer = "NOA";
    }
    else if (isEchoedCommand(commandBuffer, command, startChar))
    {
        writeLog("[PI][WARN] echo cmd=%s ignored", command.c_str());
        commandBuffer = "NOA";
    }
    else
    {
        const String sanitizedReply = sanitizeLogText(commandBuffer, 72);
        writeLog("[PI][ERR] cmd=%s crc_rx=%04X crc_calc=%04X len=%u recv=\"%s\"",
                 command.c_str(),
                 crcRecive,
                 crcCalc,
                 static_cast<unsigned>(cbLen),
                 sanitizedReply.c_str());
        connectionCounter++;
        commandBuffer = "ERCRC";
    }
    if (busyCount.load(std::memory_order_relaxed) > 0)
    {
        busyCount.fetch_sub(1, std::memory_order_relaxed);
    }

    if (!commandBuffer.isEmpty() &&
        commandBuffer != DESCR_req_NAK &&
        commandBuffer != DESCR_req_NOA &&
        commandBuffer != DESCR_req_ERCRC)
    {
        cycleHadSuccessfulReply = true;
        commandBuffer = sanitizePiReplyText(commandBuffer);
    }

    return commandBuffer;
}

uint16_t PI_Serial::getCRC(String data) // get a calculated crc from a string
{
    return getCRC(data.c_str(), data.length());
}

byte PI_Serial::getCHK(String data) // get a calculatedt CHK
{
    return getCHK(data.c_str(), data.length());
}

uint16_t PI_Serial::getCRC(const char *data, size_t len) // get a calculated crc from a buffer
{
    return calculatePiCrc(reinterpret_cast<const uint8_t *>(data), len);
}

byte PI_Serial::getCHK(const char *data, size_t len) // get a calculatedt CHK from a buffer
{
    byte chk = 0;
    if (!data || len == 0)
    {
        return chk;
    }
    for (size_t i = 0; i < len; i++)
    {
        chk += data[i];
    }
    return chk;
}

char *PI_Serial::getModeDesc(char mode) // get the char from QMOD and make readable things
{
    char *modeString;
    switch (mode)
    {
    default:
        modeString = (char *)("");
        break;
    case 'P':
        modeString = (char *)"Power On";
        break;
    case 'S':
        modeString = (char *)"Standby";
        break;
    case 'Y':
        modeString = (char *)"Bypass";
        break;
    case 'L':
        modeString = (char *)"Line";
        break;
    case 'B':
        modeString = (char *)"Battery";
        break;
    case 'T':
        modeString = (char *)"Battery Test";
        break;
    case 'F':
        modeString = (char *)"Fault";
        break;
    case 'D':
        modeString = (char *)"Shutdown";
        break;
    case 'H':
        modeString = (char *)(protocol == PI30_REVO ? "Sleep" : "Power saving");
        break;
    case 'G':
        modeString = (char *)"Grid";
        break;
    case 'C':
        modeString = (char *)"Charge";
        break;
    case 'E':
        modeString = (char *)"ECO";
        break;
    }
    return modeString;
}

bool PI_Serial::isModbus()
{
    return isModbusProtocol(protocol);
}

bool PI_Serial::checkQFLAG(const String& flags, char symbol) {
    bool enabled = false;
    for (unsigned int i = 0; i < flags.length(); i++) {
        const char c = static_cast<char>(tolower(static_cast<unsigned char>(flags.charAt(i))));
        if (c == 'e') enabled = true;
        else if (c == 'd') enabled = false;
        else if (c == static_cast<char>(tolower(static_cast<unsigned char>(symbol)))) return enabled;
    }
    return false;
}

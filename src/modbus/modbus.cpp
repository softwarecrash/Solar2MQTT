// #define isDEBUG
#include "modbus.h"
#include "device/must_pv_ph18.h"
#include "device/deye.h"

extern void writeLog(const char *format, ...);

unsigned int dir_pin;

//----------------------------------------------------------------------
//  Public Functions
//----------------------------------------------------------------------

MODBUS::MODBUS(SoftwareSerial *port)
{
    my_serialIntf = port;
    dir_pin = RS485_DIR_PIN;

    if (strcmp(HWBOARD, "esp01_1m") == 0)
    {
        dir_pin = RS485_ESP01_DIR_PIN;
    }
}

void MODBUS::preTransmission()
{
    if (strcmp(HWBOARD, "esp12e") == 0)
    {
        digitalWrite(MAX485_DONGLE_RE_NEG_PIN, 1);
        digitalWrite(MAX485_DONGLE_DE_PIN, 1);
    }
    else
    {
        digitalWrite(dir_pin, 1);
    }
}

void MODBUS::postTransmission()
{
    if (strcmp(HWBOARD, "esp12e") == 0)
    {
        digitalWrite(MAX485_DONGLE_RE_NEG_PIN, 0);
        digitalWrite(MAX485_DONGLE_DE_PIN, 0);
    }
    else
    {
        digitalWrite(dir_pin, 0);
    }
}

bool MODBUS::Init()
{
    // Null check the serial interface
    if (this->my_serialIntf == NULL)
    {
        writeLog("No serial specificed!");
        return false;
    }
    this->my_serialIntf->setTimeout(2000);

    // Init in receive mode
    if (strcmp(HWBOARD, "esp12e") == 0)
    {
        pinMode(MAX485_DONGLE_RE_NEG_PIN, OUTPUT);
        pinMode(MAX485_DONGLE_DE_PIN, OUTPUT);
    }
    else
    {
        pinMode(dir_pin, OUTPUT);
    }
    this->postTransmission();

    // Callbacks allow us to configure the RS485 transceiver correctly
    mb.preTransmission(preTransmission);
    mb.postTransmission(postTransmission);

    return true;
}

void MODBUS::prepareRegisters()
{
    const modbus_register_t *registers_live = device->getLiveRegisters();
    const modbus_register_t *registers_static = device->getStaticRegisters();

    live_info = {
        .variant = &liveData,
        .registers = registers_live,
        .array_size = sizeof(registers_live) / sizeof(modbus_register_t),
        .curr_register = 0};
    static_info = {
        .variant = &staticData,
        .registers = registers_static,
        .array_size = sizeof(registers_static) / sizeof(modbus_register_t),
        .curr_register = 0};
    previousTime = millis();
}

void MODBUS::loop()
{
    if (device == nullptr)
    {
        return;
    }

    if (millis() - previousTime < cmdDelayTime)
    {
        return;
    }

    modbus_register_info_t *cur_info_registers = &live_info;
    if (requestStaticData)
    {
        cur_info_registers = &static_info;
    }
    switch (parseModbusToJson(*cur_info_registers))
    {
    case READ_OK:
        connectionCounter = 0;
        break;
    case READ_FAIL:
        connectionCounter++;
        break;
    default:
        break;
    }

    connection = connectionCounter < MAX_CONNECTION_ATTEMPTS;
    if (isAllRegistersRead(*cur_info_registers))
    {
        requestStaticData = false;
        requestCallback();
    }

    previousTime = millis();
}

void MODBUS::callback(std::function<void()> func)
{
    requestCallback = func;
}

String MODBUS::requestData(String command)
{
    requestStaticData = true;
    return "";
}

bool MODBUS::getModbusResultMsg(uint8_t result)
{
    String tmpstr2 = "";
    switch (result)
    {
    case mb.ku8MBSuccess:
        return true;
        break;
    case mb.ku8MBIllegalFunction:
        tmpstr2 = "Illegal Function";
        break;
    case mb.ku8MBIllegalDataAddress:
        tmpstr2 = "Illegal Data Address";
        break;
    case mb.ku8MBIllegalDataValue:
        tmpstr2 = "Illegal Data Value";
        break;
    case mb.ku8MBSlaveDeviceFailure:
        tmpstr2 = "Slave Device Failure";
        break;
    case mb.ku8MBInvalidSlaveID:
        tmpstr2 = "Invalid Slave ID";
        break;
    case mb.ku8MBInvalidFunction:
        tmpstr2 = "Invalid Function";
        break;
    case mb.ku8MBResponseTimedOut:
        tmpstr2 = "Response Timed Out";
        break;
    case mb.ku8MBInvalidCRC:
        tmpstr2 = "Invalid CRC";
        break;
    default:
        tmpstr2 = "Unknown error: " + String(result);
        break;
    }
    writeLog("%s", tmpstr2.c_str());
    return false;
}

bool MODBUS::getModbusValue(uint16_t register_id, modbus_entity_t modbus_entity, uint16_t *value_ptr, uint16_t readBytes)
{
    // writeLog("Requesting data");
    for (uint8_t i = 0; i < MODBUS_RETRIES; i++)
    {
        if (MODBUS_RETRIES > 1)
        {
            // writeLog("Trial %d/%d", i + 1, MODBUS_RETRIES);
        }
        if (modbus_entity == MODBUS_TYPE_HOLDING)
        {
            uint8_t result = mb.readHoldingRegisters(register_id, readBytes);
            bool is_received = getModbusResultMsg(result);
            if (is_received)
            {
                *value_ptr = mb.getResponseBuffer(0);
                return true;
            }
        }
        else
        {
            writeLog("Unsupported Modbus entity type");
            value_ptr = nullptr;
            return false;
        }
    }
    // Time-out
    writeLog("Time-out");
    value_ptr = nullptr;
    return false;
}

String MODBUS::toBinary(uint16_t input)
{
    String output;
    while (input != 0)
    {
        output = (input % 2 == 0 ? "0" : "1") + output;
        input /= 2;
    }
    return output;
}

bool MODBUS::decodeDiematicDecimal(uint16_t int_input, int8_t decimals, float *value_ptr)
{
    if (int_input == 65535)
    {
        value_ptr = nullptr;
        return false;
    }
    else
    {
        uint16_t masked_input = int_input & 0x7FFF;
        float output = static_cast<float>(masked_input);
        if (int_input >> 15 == 1)
        {
            output = -output;
        }
        *value_ptr = output / pow(10, decimals);
        return true;
    }
}

bool MODBUS::readModbusRegisterToJson(const modbus_register_t *reg, JsonObject *variant)
{
    // register found
    if (reg == nullptr || variant == nullptr)
    {
        return false; // Return false if invalid input
    }
    // writeLog("Register id=%d type=0x%x name=%s", reg->id, reg->type, reg->name);
    uint16_t raw_value = 0;

    float final_value;

    uint16_t readBytes = 1;

    if (reg->type == REGISTER_TYPE_U32 || reg->type == REGISTER_TYPE_U32_ONE_DECIMAL)
    {
        readBytes = 2;
    }

    if (getModbusValue(reg->id, reg->modbus_entity, &raw_value, readBytes))
    {
        writeLog("Raw value: %s=%#06x\n", reg->name, raw_value);

        switch (reg->type)
        {
        case REGISTER_TYPE_U16:
            // writeLog("Value: %u", raw_value);
            (*variant)[reg->name] = raw_value + reg->offset;
            break;
        case REGISTER_TYPE_INT16:
            // writeLog("Value: %u", raw_value);
            (*variant)[reg->name] = static_cast<int16_t>(raw_value) + reg->offset;
            break;
        case REGISTER_TYPE_U32:
            // writeLog("Value: %u", raw_value);
            (*variant)[reg->name] = (raw_value + (mb.getResponseBuffer(1) << 16)) + reg->offset;
            break;
        case REGISTER_TYPE_U32_ONE_DECIMAL:
            // writeLog("Value: %u", raw_value);
            (*variant)[reg->name] = (raw_value + (mb.getResponseBuffer(1) << 16)) * 0.1 + reg->offset;
            break;
        case REGISTER_TYPE_DIEMATIC_ONE_DECIMAL:
            if (decodeDiematicDecimal(raw_value, 1, &final_value))
            {
                // writeLog("Raw value: %#06x, floatValue: %f",raw_value, final_value);
                (*variant)[reg->name] = ((int)(final_value * 100 + 0.5) / 100.0) + reg->offset;
            }
            else
            {
                writeLog("Invalid Diematic value");
            }
            break;

        case REGISTER_TYPE_DIEMATIC_TWO_DECIMAL:
            if (decodeDiematicDecimal(raw_value, 2, &final_value))
            {
                // writeLog("Value: %.1f", final_value);
                (*variant)[reg->name] = ((int)(final_value * 1000 + 0.5) / 1000.0) + reg->offset;
            }
            else
            {
                writeLog("Invalid Diematic value");
            }
            break;
        case REGISTER_TYPE_BITFIELD:

            for (uint8_t j = 0; j < 16; ++j)
            {
                const char *bit_varname = reg->optional_param.bitfield[j];
                if (bit_varname == nullptr)
                {
                    writeLog("[bit%02d] end of bitfield reached", j);
                    break;
                }
                const uint8_t bit_value = raw_value >> j & 1;
                writeLog(" [bit%02d] %s=%d", j, bit_varname, bit_value);
                (*variant)[bit_varname] = bit_value;
            }
            break;

        case REGISTER_TYPE_CUSTOM_VAL_NAME:
        {
            bool isfound = false;
            for (uint8_t j = 0; j < 16; ++j)
            {
                const char *bit_varname = reg->optional_param.bitfield[j];
                if (bit_varname == nullptr)
                {
                    writeLog("bitfield[%d] is null", j);
                    break;
                }
                if (j == raw_value)
                {
                    // writeLog("Match found, value: %s", bit_varname);
                    (*variant)[reg->name] = bit_varname;
                    isfound = true;
                    break;
                }
            }
            if (!isfound)
            {
                writeLog("CUSTOM_VAL_NAME not found for raw_value=%d", raw_value);
            }
            break;
        }
        case REGISTER_TYPE_ASCII:
        {
            String out = "";
            char high_byte = (raw_value >> 8) & 0xFF;
            char low_byte = raw_value & 0xFF;

            out += high_byte;
            out += low_byte;
            (*variant)[reg->name] = out;
            break;
        }
        case REGISTER_TYPE_DEBUG:

            writeLog("Raw DEBUG value: %s=%#06x %s", reg->name, raw_value, toBinary(raw_value).c_str());
            break;

        default:
            // Unsupported type

            writeLog("Unsupported register type");
            break;
        }
    }
    else
    {
        writeLog("Request failed!");
        return false;
    }
    // writeLog("Request OK!");
    return true;
}

response_type_t MODBUS::parseModbusToJson(modbus_register_info_t &register_info, bool skip_reg_on_error)
{

    if (register_info.curr_register >= register_info.array_size)
    {
        register_info.curr_register = 0;
    }
    while (register_info.curr_register < register_info.array_size)
    {
        bool ret_val = readModbusRegisterToJson(&register_info.registers[register_info.curr_register], register_info.variant);
        if (ret_val || skip_reg_on_error)
        {
            register_info.curr_register++;
        }

        return ret_val ? READ_OK : READ_FAIL;
    }
    return READ_FAIL;
}

bool MODBUS::isAllRegistersRead(modbus_register_info_t &register_info)
{
    if (register_info.curr_register >= register_info.array_size)
    {
        return true;
    }
    return false;
}

//----------------------------------------------------------------------
// Private Functions
//----------------------------------------------------------------------
protocol_type_t MODBUS::autoDetect() // function for autodetect the inverter type
{
    protocol_type_t protocol = NoD;
    char modelName[20];

    writeLog("Try Autodetect Modbus device");

    ModbusDevice *devices[] = {new MustPV_PH18(), new DEYE()};

    for (size_t i = 0; i < sizeof(devices) / sizeof(devices[0]); ++i)
    {
        devices[i]->init(*my_serialIntf, mb);

        writeLog("Try to use: %s protocol", devices[i]->getName());
        devices[i]->retrieveModel(*this, modelName, sizeof(modelName));
        if (strlen(modelName) != 0)
        {
            writeLog("<Autodetect> Found Modbus device: %s", modelName);
            staticData["Device_Model"] = modelName;
            prepareRegisters();
            protocol = devices[i]->getProtocol();
            device = devices[i];
            return protocol;
        }
        delete devices[i];
    }

    return protocol;
}

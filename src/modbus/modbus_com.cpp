
#include "modbus_com.h"

//----------------------------------------------------------------------
//  Public Functions
//----------------------------------------------------------------------

extern void writeLog(const char *format, ...);

int _dir_pin;

MODBUS_COM::MODBUS_COM()
{
    _dir_pin = RS485_DIR_PIN;

    if (strcmp(HWBOARD, "esp01_1m") == 0)
    {
        _dir_pin = RS485_ESP01_DIR_PIN;
    }

    // Init in receive mode
    if (strcmp(HWBOARD, "esp12e") == 0)
    {
        pinMode(MAX485_DONGLE_RE_NEG_PIN, OUTPUT);
        pinMode(MAX485_DONGLE_DE_PIN, OUTPUT);
    }
    else
    {
        pinMode(_dir_pin, OUTPUT);
    }
    this->postTransmission();
    //set the default timeout
    _mb.setResponseTimeout(MODBUS_TIMEOUT);
    // Callbacks allow us to configure the RS485 transceiver correctly
    _mb.preTransmission(preTransmission);
    _mb.postTransmission(postTransmission);
}

void MODBUS_COM::preTransmission()
{
    if (strcmp(HWBOARD, "esp12e") == 0)
    {
        digitalWrite(MAX485_DONGLE_RE_NEG_PIN, 1);
        digitalWrite(MAX485_DONGLE_DE_PIN, 1);
    }
    else
    {
        digitalWrite(_dir_pin, 1);
    }
}

void MODBUS_COM::postTransmission()
{
    if (strcmp(HWBOARD, "esp12e") == 0)
    {
        digitalWrite(MAX485_DONGLE_RE_NEG_PIN, 0);
        digitalWrite(MAX485_DONGLE_DE_PIN, 0);
    }
    else
    {
        digitalWrite(_dir_pin, 0);
    }
}

ModbusMaster *MODBUS_COM::getModbusMaster()
{
    return &_mb;
}

bool MODBUS_COM::getModbusResultMsg(uint8_t result)
{
    String tmpstr2 = "";
    switch (result)
    {
    case _mb.ku8MBSuccess:
        return true;
        break;
    case _mb.ku8MBIllegalFunction:
        tmpstr2 = "Illegal Function";
        break;
    case _mb.ku8MBIllegalDataAddress:
        tmpstr2 = "Illegal Data Address";
        break;
    case _mb.ku8MBIllegalDataValue:
        tmpstr2 = "Illegal Data Value";
        break;
    case _mb.ku8MBSlaveDeviceFailure:
        tmpstr2 = "Slave Device Failure";
        break;
    case _mb.ku8MBInvalidSlaveID:
        tmpstr2 = "Invalid Slave ID";
        break;
    case _mb.ku8MBInvalidFunction:
        tmpstr2 = "Invalid Function";
        break;
    case _mb.ku8MBResponseTimedOut:
        tmpstr2 = "Response Timed Out";
        break;
    case _mb.ku8MBInvalidCRC:
        tmpstr2 = "Invalid CRC";
        break;
    default:
        tmpstr2 = "Unknown error: " + String(result);
        break;
    }
    writeLog("%s", tmpstr2.c_str());
    return false;
}

bool MODBUS_COM::getModbusValue(uint16_t register_id, modbus_entity_t modbus_entity, uint16_t *value_ptr, uint16_t readBytes)
{
    // writeLog("Requesting data");
    uint16_t data;
    for (uint8_t i = 0; i < MODBUS_RETRIES; i++)
    {
        if (MODBUS_RETRIES > 1)
        {
            // writeLog("Trial %d/%d", i + 1, MODBUS_RETRIES);
        }
        if (modbus_entity == MODBUS_TYPE_HOLDING)
        {
            uint8_t result = _mb.readHoldingRegisters(register_id, readBytes);
            bool is_received = getModbusResultMsg(result);
            if (is_received)
            {
                data = _mb.getResponseBuffer(0);
                if (_dataFilter){
                    data = _dataFilter(data);
                }
                *value_ptr = data;
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

String MODBUS_COM::toBinary(uint16_t input)
{
    String output;
    while (input != 0)
    {
        output = (input % 2 == 0 ? "0" : "1") + output;
        input /= 2;
    }
    return output;
}

bool MODBUS_COM::decodeDiematicDecimal(uint16_t int_input, int8_t decimals, float *value_ptr)
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

bool MODBUS_COM::readModbusRegisterToJson(const modbus_register_t *reg, JsonObject *variant)
{
    // register found
    if (reg == nullptr || variant == nullptr)
    {
        return false; // Return false if invalid input
    }
    //writeLog("Register id=%d type=0x%x name=%s", reg->id, reg->type, reg->name);
    uint16_t raw_value = 0;

    if (reg->type == REGISTER_TYPE_VIRTUAL_CALLBACK)
    {
        if (reg->callback != nullptr)
        {
            reg->callback(variant, 0, reg, *(this));
        }
        else
        {
            writeLog("No callback specified for %s", reg->name);
        }
        return true;
    }

    float final_value;

    uint16_t readBytes = 1;

    if (reg->type == REGISTER_TYPE_U32 || reg->type == REGISTER_TYPE_U32_ONE_DECIMAL)
    {
        readBytes = 2;
    }

    if (getModbusValue(reg->id, reg->modbus_entity, &raw_value, readBytes))
    {
        //writeLog("Raw value: %s=%#06x\n", reg->name, raw_value);

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
        case REGISTER_TYPE_INT16_ONE_DECIMAL:
            // writeLog("Value: %u", raw_value); 
            final_value =( static_cast<int16_t>(raw_value) / 10.0 )+ reg->offset;
            (*variant)[reg->name] = final_value;
            break;
        case REGISTER_TYPE_INT16_TWO_DECIMAL:
            // writeLog("Value: %u", raw_value);
            final_value =( static_cast<int16_t>(raw_value) / 100.0 )+ reg->offset;
            (*variant)[reg->name] = final_value;
            break;

        case REGISTER_TYPE_U32:
            // writeLog("Value: %u", raw_value);
            (*variant)[reg->name] = (raw_value + (_mb.getResponseBuffer(1) << 16)) + reg->offset;
            break;
        case REGISTER_TYPE_U32_ONE_DECIMAL:
            // writeLog("Value: %u", raw_value);
            (*variant)[reg->name] = (raw_value + (_mb.getResponseBuffer(1) << 16)) * 0.1 + reg->offset;
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
                //writeLog(" [bit%02d] %s=%d", j, bit_varname, bit_value);
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
                writeLog("CUSTOM_VAL_NAME not found for raw_value=%d register id=%d type=0x%x name=%s",raw_value, reg->id, reg->type, reg->name);
            }
            break;
        }
        case REGISTER_TYPE_ASCII:
        { 
            (*variant)[reg->name] = convertRegistersToASCII(&raw_value,1);
            break;
        }
        case REGISTER_TYPE_CALLBACK:
            if (reg->callback != nullptr)
            {
                reg->callback(variant, 0, reg, *(this));
            }
            else
            {
                writeLog("No callback specified for %s", reg->name);
            }
            break;
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

response_type_t MODBUS_COM::parseModbusToJson(modbus_register_info_t &register_info, bool skip_reg_on_error)
{
    // writeLog("parseModbusToJson %d", register_info.array_size);
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

bool MODBUS_COM::isAllRegistersRead(modbus_register_info_t &register_info)
{
    if (register_info.curr_register >= register_info.array_size)
    {
        return true;
    }
    return false;
}


void MODBUS_COM::setDataFilter(std::function<uint16_t(uint16_t)> func)
{
    _dataFilter = func;
}

String MODBUS_COM::convertRegistersToASCII(uint16_t* registers, size_t count) {
    String result = "";

    for (size_t i = 0; i < count; ++i) {
        char highByte = (char)((registers[i] >> 8) & 0xFF);
        char lowByte = (char)(registers[i] & 0xFF);

        if (highByte != '\0') result += highByte;
        if (lowByte != '\0') result += lowByte;
    }

    return result;
}

bool MODBUS_COM::writeRegister(uint16_t registerAddress, uint16_t value) {

    // Log the value
    writeLog("Writing to register %d: Value %d", registerAddress, value);

    // Clear the transmit buffer
    _mb.clearTransmitBuffer();

    // Set the transmit buffer with the scaled value
    _mb.setTransmitBuffer(0, value);

    // Write the single register
    uint8_t result = _mb.writeMultipleRegisters(registerAddress, 1);

    // Check the result
    if (result == _mb.ku8MBSuccess) {
        writeLog("Modbus write successful: Register %d, Value %.2f", registerAddress, value);
        return true;
    } else {
        getModbusResultMsg(result);
        return false;
    }

}
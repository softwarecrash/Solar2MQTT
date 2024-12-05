// #define isDEBUG
#include "modbus.h"

//----------------------------------------------------------------------
//  Public Functions
//----------------------------------------------------------------------

MODBUS::MODBUS(SoftwareSerial *port)
{
    my_serialIntf = port;
}

bool MODBUS::Init()
{
    // Null check the serial interface
    if (this->my_serialIntf == NULL)
    {
        writeLog("No serial specificed!");
        return false;
    }
    // this->my_serialIntf->setTimeout(2000);

    return true;
}

bool MODBUS::setProtocol(protocol_type_t protocol)
{
    char modelName[30];
    if (device != nullptr)
    {
        delete device;
        device = nullptr;
    }
    switch (protocol)
    {
    case MODBUS_DEYE:
        device = new Deye();
        break;
    case MODBUS_ANENJI:
        device = new Anenji();
        break;
    case MODBUS_MUST:
        device = new MustPV_PH18();
        break;
    case MODBUS_POW_HVM:
        device = new Pow_Hvm();
        break;
    default:
        break;
    }

    if (device != nullptr)
    {
        device->init(*my_serialIntf, _mCom);
        prepareRegisters();
        requestStaticData = true;
        connectionCounter = 0; 
        previousTime = millis();
        bool ret = device->retrieveModel(_mCom, modelName, sizeof(modelName));
        if (ret && strlen(modelName) != 0)
        {
            staticData["Device_Model"] = modelName;
            return true;
        }
    }
    else
    {
        writeLog("Unsupported protocol or failed to initialize device.");
    }
    return false;
}

void MODBUS::prepareRegisters()
{ 
    live_info = {
        .variant = &liveData,
        .registers =  device->getLiveRegisters(),
        .array_size = device->getLiveRegistersCount(),
        .curr_register = 0};
    static_info = {
        .variant = &staticData,
        .registers = device->getStaticRegisters(),
        .array_size = device->getStaticRegistersCount(),
        .curr_register = 0};
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
    switch (_mCom.parseModbusToJson(*cur_info_registers))
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
    if (_mCom.isAllRegistersRead(*cur_info_registers))
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

//----------------------------------------------------------------------
// Private Functions
//----------------------------------------------------------------------
protocol_type_t MODBUS::autoDetect() // function for autodetect the inverter type
{
    writeLog("Try Autodetect Modbus device");
    const size_t deviceCount = sizeof(modbus_protocols) / sizeof(modbus_protocols[0]);

    for (size_t i = 0; i < deviceCount; ++i)
    {
        if (setProtocol(modbus_protocols[i]))
        { 
            writeLog("<Autodetect> Found Modbus device: %s", staticData["Device_Model"]);
            return modbus_protocols[i];
        }
    }

    return NoD;
}

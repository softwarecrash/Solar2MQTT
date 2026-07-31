// #define isDEBUG
#include "modbus.h"
 
//----------------------------------------------------------------------
//  Public Functions
//----------------------------------------------------------------------

MODBUS::MODBUS(HardwareSerial *port, int rxPin, int txPin)
{
    my_serialIntf = port;
    _rxPin = rxPin;
    _txPin = txPin;
}

MODBUS::~MODBUS()
{
    delete device;
    device = nullptr;
}
 
bool MODBUS::Init()
{
    // Null check the serial interface
    if (this->my_serialIntf == NULL)
    {
        writeLog("No serial specificed!");
        return false;
    }
    //this->my_serialIntf->setTimeout(2000);
 
    return true;
}

void MODBUS::prepareRegisters()
{
    const modbus_register_t *registers_live = device->getLiveRegisters();
    const modbus_register_t *registers_static = device->getStaticRegisters();

    live_info = {
        .variant = &liveData,
        .registers = registers_live,
        .array_size = device->getLiveRegistersCount(),
        .curr_register = 0};
    static_info = {
        .variant = &staticData,
        .registers = registers_static,
        .array_size = device->getStaticRegistersCount(),
        .curr_register = 0};
    previousTime = millis();
}

void MODBUS::loop()
{
    if (device == nullptr)
    {
        return;
    }

    if (millis() - previousTime < kCommandDelayMs)
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
        if (requestCallback)
        {
            requestCallback();
        }
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
    writeLog("Custom Modbus command unsupported: %s", command.c_str());
    return "UNSUPPORTED";
} 

//----------------------------------------------------------------------
// Private Functions
//----------------------------------------------------------------------
protocol_type_t MODBUS::autoDetect() // function for autodetect the inverter type
{
    protocol_type_t protocol = NoD;
    char modelName[48] = {};
    long activeBaudRate = 0;
    const uint16_t normalResponseTimeout = _mCom.getResponseTimeout();

    writeLog("Try Autodetect Modbus device");
    _mCom.setResponseTimeout(MODBUS_DETECTION_TIMEOUT_MS);

    ModbusDevice *devices[] = { new Deye(), new SMGII11KW(), new SMG(), new AnenjiSrne(), new Anenji(), new MustPV_PH18()};
    const size_t deviceCount = sizeof(devices) / sizeof(devices[0]);

    for (size_t i = 0; i < deviceCount; ++i)
    {
        modelName[0] = '\0';
        const bool configureSerial = activeBaudRate != devices[i]->getBaudRate();
        devices[i]->init(*my_serialIntf, _rxPin, _txPin, _mCom, configureSerial);
        if (configureSerial)
        {
            activeBaudRate = devices[i]->getBaudRate();
            stabilizeSerial();
        }

        const bool detected = devices[i]->retrieveModel(_mCom, modelName, sizeof(modelName));

        if (detected && modelName[0] != '\0')
        {
            writeLog("<Autodetect> Found Modbus device: %s", modelName);
            staticData["Device_Model"] = modelName;
            
            device = devices[i];
            prepareRegisters();
            protocol = device->getProtocol();
            staticData[DESCR_Protocol_ID] = protocolToString(protocol);

            // Clean up candidates that were not selected. Earlier failures are
            // already deleted and nulled below, so guard against double-free.
            for (size_t j = 0; j < deviceCount; ++j)
            {
                if (j != i && devices[j] != nullptr)
                {
                    delete devices[j];
                    devices[j] = nullptr;
                }
            }
            _mCom.setResponseTimeout(normalResponseTimeout);
            return protocol;
        }
        delete devices[i];
        devices[i] = nullptr;
    }

    _mCom.setResponseTimeout(normalResponseTimeout);
    return protocol;
}

void MODBUS::stabilizeSerial()
{
    while (my_serialIntf->available() > 0)
    {
        my_serialIntf->read();
    }

    delay(kSerialStabilizationDelayMs);

    while (my_serialIntf->available() > 0)
    {
        my_serialIntf->read();
    }
}

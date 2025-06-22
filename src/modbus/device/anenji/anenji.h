#ifndef MODBUS_ANENJI_H
#define MODBUS_ANENJI_H

#include <modbus/device/modbus_device.h>

class Anenji : public ModbusDevice {
public:
    Anenji() : ModbusDevice(_baudRate, _modbusAddr, _protocol) {}

    virtual const modbus_register_t *getLiveRegisters() const override;
    virtual const modbus_register_t *getStaticRegisters() const override;
    const char *getName() const override;
    bool retrieveModel(MODBUS_COM &mCom, char *modelBuffer, size_t bufferSize) override;
    size_t getLiveRegistersCount() const override;
    size_t getStaticRegistersCount() const override;

private:
    static const long _baudRate = 9600;
    static const uint32_t _modbusAddr = 1;
    static const protocol_type_t _protocol = MODBUS_ANENJI;
    inline static const char *const _name = "Anenji";

    // CRC lookup tables (fill in with values from the protocol document)
    static constexpr uint8_t auchCRCHi[256] = {
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, // and so on
    };
    static constexpr uint8_t auchCRCLo[256] = {
        0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, // and so on
    };

    // Live Registers (dynamic data from device)
    inline static const modbus_register_t registers_live[] = {
        {201, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, "Inverter Operation Mode", 0, {.bitfield = {
            "Power On", "Standby", "Mains", "Off-Grid", "Bypass", "Charging", "Fault"}}},
        {202, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Effective Mains Voltage"},
        {203, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, "Mains Frequency"},
        {204, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "Average Mains Power"},
        {205, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Inverter Voltage"},
        {206, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Inverter Current"},
        {207, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, "Inverter Frequency"},
        {208, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "Average Inverter Power"},
        {209, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "Inverter Charging Power"},
        {210, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Output Effective Voltage"},
        {211, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Output Effective Current"},
        {212, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, "Output Frequency"},
        {213, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "Output Active Power"},
        {214, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "Output Apparent Power"},
        {215, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Battery Voltage"},
        {216, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Battery Current"},
        {217, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "Battery Power"},
        {219, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "PV Voltage"},
        {220, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "PV Current"},
        {223, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "PV Power"},
        {224, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "PV Charging Power"},
        {225, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "Load Percentage"},
        {226, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "DCDC Temperature"},
        {227, MODBUS_TYPE_HOLDING, REGISTER_TYPE_INT16, "Inverter Temperature"},
        {229, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "Battery Percentage"},
        {232, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Battery Average Current"},
        {233, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Inverter Charging Current"},
        {234, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "PV Charging Current"},
        // Add additional live registers as needed
    };

    // Static Registers (configuration data)
    inline static const modbus_register_t registers_static[] = {
        {300, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "Output Mode", 0, {.bitfield = {
            "Single", "Parallel", "3 Phase-P1", "3 Phase-P2", "3 Phase-P3", "Split Phase-P1", "Split Phase-P2"}}},
        {301, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "Output Priority", 0, {.bitfield = {
            "UTI", "SOL", "SBU", "SUB"}}},
        {302, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "Input Voltage Range", 0, {.bitfield = {
            "Wide", "Narrow"}}},
        {303, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "Buzzer Mode", 0, {.bitfield = {
            "Mute", "Warning", "Fault", "Fault Only"}}},
        {305, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "LCD Backlight", 0, {.bitfield = {
            "Timed Off", "Always On"}}},
        {306, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "LCD Return to Homepage", 0, {.bitfield = {
            "Do Not Return", "Return After 1 Minute"}}},
        {307, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "Energy-Saving Mode", 0, {.bitfield = {
            "Off", "On"}}},
        {308, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "Overload Auto-Restart", 0, {.bitfield = {
            "No Restart", "Automatic Restart"}}},
        {310, MODBUS_TYPE_HOLDING, REGISTER_TYPE_U16, "Overload Bypass Enable", 0, {.bitfield = {
            "Disable", "Enable"}}},
        {322, MODBUS_TYPE_HOLDING, REGISTER_TYPE_CUSTOM_VAL_NAME, "Battery Type", 0, {.bitfield = {
            "AGM", "FLD", "USER", "SMK1", "PYL", "FOX"}}},
        {320, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Output Voltage"},
        {321, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_TWO_DECIMAL, "Output Frequency"},
        {332, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Max Charging Current"},
        {333, MODBUS_TYPE_HOLDING, REGISTER_TYPE_DIEMATIC_ONE_DECIMAL, "Max Mains Charging Current"},
        // Add additional static registers as needed
    };

    inline static const modbus_register_t registers_device_serial[] = {
        {186, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN1"},
        {188, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN2"},
        {190, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN3"},
        {192, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN4"},
        {194, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN5"},
        {196, MODBUS_TYPE_HOLDING, REGISTER_TYPE_ASCII, "SN6"},
    };

    uint16_t calculateCRC(const uint8_t *data, uint16_t length) const {
        uint8_t crcHigh = 0xFF;
        uint8_t crcLow = 0xFF;
        while (length--) {
            uint8_t index = crcHigh ^ *data++;
            crcHigh = crcLow ^ auchCRCHi[index];
            crcLow = auchCRCLo[index];
        }
        return (crcHigh << 8 | crcLow);
    }
};

#endif

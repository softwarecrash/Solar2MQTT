#ifndef MODBUS_H
#define MODBUS_H

#include <HardwareSerial.h>
#include <ArduinoJson.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "modbus_com.h"
#include "powmr_hunt.h"
#include "device/modbus_device.h"
#include "device/must_pv_ph18/must_pv_ph18.h"
#include "device/deye/deye.h"
#include "device/anenji/anenji.h"
#include "device/anenji_srne/anenji_srne.h"
#include "device/smg/smg.h"
#include "device/smg_ii_11kw/smg_ii_11kw.h"
#include "device/powmr/powmr.h"

extern JsonObject deviceJson;
extern JsonObject staticData;
extern JsonObject liveData;

class MODBUS
{
public:
    const uint8_t MAX_CONNECTION_ATTEMPTS = 10;
    bool requestStaticData = true;
    bool connection = false;
    modbus_register_info_t live_info;
    modbus_register_info_t static_info;
    MODBUS(HardwareSerial *port, int rxPin, int txPin);
    ~MODBUS();

    /**
     * @brief Initializes this driver
     * @details Configures the serial peripheral and pre-loads the transmit buffer with command-independent bytes
     */
    bool Init();

    /**
     * @brief Updating the Data from the inverter
     */
    void loop();

    /**
     * @brief callback function
     *
     */
    void callback(std::function<void()> func);
    std::function<void()> requestCallback; 
    protocol_type_t autoDetect(bool powMrOnly = false);
    bool forceProtocol(protocol_type_t protocol);
    /**
     * @brief Sends a complete packet with the specified command
     * @details sends the command over the specified serial connection
     */
    String requestData(String command);
    bool isDiagnosticBusy() const { return _powmrDumpRunning; }
    bool consumePowMrLivePassCompleted()
    {
        const bool ready = _powmrLivePassCompleted;
        _powmrLivePassCompleted = false;
        return ready;
    }

private:
    static constexpr unsigned long kCommandDelayMs = 200;
    static constexpr unsigned long kSerialStabilizationDelayMs = 250;
    unsigned long previousTime = 0;

    byte requestCounter = 0;

    long long int connectionCounter = 0;

    byte qexCounter = 0;

    void prepareRegisters();
    void stabilizeSerial();
    static void powmrDumpTask(void *param);
    void runPowmrDump();
    PowmrHunt _powmrHunt;
    void stepPowmrHunt();
    String powmrHuntCommand(String command);
    void capturePowmrSocSample();
    String buildPowmrSocDiag() const;

    /**
     * @brief Serial interface used for communication
     * @details This is set in the constructor
     */
    HardwareSerial *my_serialIntf;
    int _rxPin;
    int _txPin;
    ModbusDevice *device = nullptr; 
    MODBUS_COM _mCom;

    volatile bool _powmrLivePassCompleted = false;
    volatile bool _powmrDumpRunning = false;
    volatile bool _powmrDumpReady = false;
    volatile uint16_t _powmrDumpCurrentRegister = 0;
    volatile uint16_t _powmrDumpReadable = 0;
    volatile uint16_t _powmrDumpFailed = 0;
    volatile bool _powmrCustomScan = false;
    volatile uint16_t _powmrScanStart = 0;
    volatile uint16_t _powmrScanEnd = 0;
    String _powmrDumpResult;
    TaskHandle_t _powmrDumpTask = nullptr;

    struct PowMrSocSample
    {
        uint32_t uptimeSeconds = 0;
        uint16_t batteryDeciVolts = 0;
        uint8_t socPercent = 0;
        uint16_t chargeAmps = 0;
        uint16_t dischargeAmps = 0;
        bool valid = false;
    };

    static constexpr uint8_t kPowMrSocHistorySize = 72;
    static constexpr uint32_t kPowMrSocSampleIntervalMs = 5UL * 60UL * 1000UL;
    PowMrSocSample _powmrSocHistory[kPowMrSocHistorySize] = {};
    uint8_t _powmrSocHistoryHead = 0;
    uint8_t _powmrSocHistoryCount = 0;
    unsigned long _powmrSocLastSampleMs = 0;

    static constexpr uint8_t kPowMrWatchCount = 64;
    uint16_t _powmrWatchValues[kPowMrWatchCount] = {};
    bool _powmrWatchValid = false;
    uint32_t _powmrWatchCapturedAt = 0;

};

#endif

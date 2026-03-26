#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

#include "PI_Serial/PI_Serial.h"
#include "core/SolarState.h"

class SolarInverterService
{
public:
    explicit SolarInverterService(SolarState &state);
    ~SolarInverterService();

    void begin();
    void loop();
    void reconfigure();
    void setTransportPaused(bool paused);

    void setCallback(std::function<void()> callback) { _callback = callback; }
    void queueCommand(const String &command);

    bool isConnected() const;
    bool isBusy() const;
    protocol_type_t protocol() const;
    const char *protocolName() const;
    bool simulationEnabled() const { return _simulationEnabled; }
    bool transportPaused() const { return _transportPaused; }

    bool requestLoopback();
    bool loopbackRunning() const { return _loopbackInProgress; }
    bool loopbackDone() const { return _loopbackDone; }
    bool loopbackOk() const { return _loopbackOk; }
    const String &loopbackMessage() const { return _loopbackMessage; }

    bool hasCommandAnswer() const;
    String consumeCommandAnswer();

    PI_Serial *client() const { return _client; }

private:
    void createClient();
    void refreshRawState();
    void handleLoopback();
    void handleSimulation();
    void setCommandAnswer(const String &answer);
    bool handleSimulationCommand(const String &command);
    void enableSimulation();
    void disableSimulation();
    void updateSimulationState(bool forceNotify);

    SolarState &_state;
    HardwareSerial &_serial;
    PI_Serial *_client;
    std::function<void()> _callback;

    bool _simulationEnabled;
    bool _transportPaused;
    uint32_t _simulationLastUpdateMs;

    bool _loopbackRequested;
    bool _loopbackInProgress;
    bool _loopbackDone;
    bool _loopbackOk;
    unsigned long _loopbackWaitStart;
    String _loopbackMessage;
};

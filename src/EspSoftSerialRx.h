/* EspSoftSerial receiver
Scott Day 2015
github.com/scottwday/EspSoftSerial

Uses a pin change interrupt and the tick timer as a uart receiver
*/

#ifndef ESPSOFTSERIAL_H
#define ESPSOFTSERIAL_H

#define MAX_ESPSOFTSERIAL_INSTANCES 4
#define SOFTSERIAL_BUFFER_LEN 256

#define SOFTSERIAL_ERROR_LONGLOW 1
#define SOFTSERIAL_ERROR_STOPBIT 2

#include <Arduino.h>
#include "CircularBuffer.h"

class EspSoftSerialRx
{
  private:
  
    typedef void (EspSoftSerialRx::*InterruptHandler)();

    unsigned long _halfBitRate;
    byte _rxPin;
    unsigned long _lastChangeTicks = 0;
    byte _errorState = 0;
    byte _bitCounter = 0;
    unsigned short _bitBuffer = 0;
    byte _inInterrupt = 0;
	byte _instanceId = 0;
    CircularBuffer<byte, SOFTSERIAL_BUFFER_LEN> _buffer;
    
    static byte _numInstances;
    static EspSoftSerialRx* _instances[MAX_ESPSOFTSERIAL_INSTANCES];

  public:

    // Supply the baud rate and the pin you want to use
    void begin(const unsigned long baud, const byte rxPin);
	
	// Call this at least every 10 seconds to prevent bytes getting lost
	void service();
	
	// Read the next byte from the buffer
    bool read(byte& c);
	
	void setEnabled(bool enabled);

	void reset();

  private:
    
	static void onRxPinChange0();
    static void onRxPinChange1();
    static void onRxPinChange2();
    static void onRxPinChange3();
    byte getNumBitPeriodsSinceLastChange(unsigned long ticks);
    inline void addBits(byte numBits, byte value);
    void onRxPinChange();
    
};

#endif

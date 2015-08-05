#include <arduino.h>
#include "EspSoftSerialRx.h"

//Make sure static numInstances gets initialised to zero.
byte EspSoftSerialRx::_numInstances = 0;

//Static pointers to instances, allows interrupts to be forwarded
EspSoftSerialRx* EspSoftSerialRx::_instances[MAX_ESPSOFTSERIAL_INSTANCES];



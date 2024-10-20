/*
All known devices with PIxx protocol are react to the Q1 comand

examples:
27 fields, length 109: 00001 16971 01 00 00 026 033 022 029 02 00 000 0036 0000 0000 00.00 10 0 060 030 100 030 58.40 000 120 0 0000 - from https://github.com/jblance/mpp-solar/discussions/284
27 fields, length 105:     0 00027 00 00 00 000 033 027 035 02 00 000 0030 0000 0000 49.95 10 0 000 000 000 000 00.00 000 000 0 0000 - from https://github.com/softwarecrash/Solar2MQTT/issues/158
17 fields, length 70:  07792 00001 00 00 00 000 034 030 000 01 00 000 0030 0000 0000 00.00 13 from - -from https://github.com/softwarecrash/Solar2MQTT/discussions/144
13 fields, length 47:  00 00 00 000 026 021 026 00 00 000 0030 0000 13 -from https://github.com/softwarecrash/Solar2MQTT/discussions/144
13 fields, length 47:  00 00 00 000 039 030 031 00 00 000 0030 0000 11 - from manuA
22 fields, length 87   00000 00000 01 01 00 030 027 028 029 00 00 000 0031 0364 0000 00.00 11 07 00 3741 530 3 - from derLoosi
length helper: https://elmar-eigner.de/text-zeichen-laenge.html
*/
// Raw length 47

/* const char Time_until_absorb_charge[] PROGMEM = "Time_until_absorb_charge";
const char Time_until_float_charge[] PROGMEM = "Time_until_float_charge";
const char SCC_Flag[] PROGMEM= "SCC_Flag";                             // SCC Flag
const char AllowSccOnFlag[] PROGMEM= "AllowSccOnFlag";                 // AllowSccOnFlag
const char Charge_Average_Current[] PROGMEM= "Charge_Average_Current"; // ChargeAverageCurrent
const char Tracker_temperature[] PROGMEM= "Tracker_temperature";       // SCC PWM temperature
const char Inverter_temperature[] PROGMEM= "Inverter_temperature";
const char Battery_temperature[] PROGMEM= "Battery_temperature";
const char Transformer_temperature[] PROGMEM= "Transformer_temperature";
const char Fan_lock_status[] PROGMEM= "Fan_lock_status";
const char Fan_speed[] PROGMEM= "Fan_speed";               // Fan PWM speed
const char SCC_charge_power[] PROGMEM= "SCC_charge_power"; // SCC charge power
const char Parallel_Warning[] PROGMEM= "Parallel_Warning"; // Parallel Warning
const char Sync_frequency[] PROGMEM= "Sync_frequency";     // Sync frequency
const char Inverter_charge_state[] = "Inverter_charge_state";
const char unknown[] PROGMEM= "";


const char *const  Q1_47[] = {
    // [PI34 / MPPT-3000], [PI30 HS MS MSX], [PI30 Revo], [PI30 PIP], [PI41 / LV5048]
    Time_until_absorb_charge,
    Time_until_float_charge,
    unknown,
    Tracker_temperature,
    Inverter_temperature,
    Battery_temperature,
    Transformer_temperature,
    unknown,
    unknown,
    unknown,
    Fan_speed,
    unknown,
    Inverter_charge_state,
};

static const char *const q1List[] = {
    // [PI34 / MPPT-3000], [PI30 HS MS MSX], [PI30 Revo], [PI30 PIP], [PI41 / LV5048]
    Time_until_absorb_charge,
    Time_until_float_charge,
    unknown,
    Tracker_temperature,
    Inverter_temperature,
    Battery_temperature,
    Transformer_temperature,
    unknown,
    unknown,
    unknown,
    Fan_speed,
    unknown,
    Inverter_charge_state,
};
// Raw length 70
static const char *const q1List2[] = {
    // [PI30]
    Time_until_absorb_charge, // Time until the end of absorb charging
    Time_until_float_charge,  // Time until the end of float charging
    unknown,                         // SCC Flag
    unknown,                         // AllowSccOnFlag
    unknown,                         // ChargeAverageCurrent
    Tracker_temperature,      // SCC PWM temperature
    Inverter_temperature,
    Battery_temperature,
    Transformer_temperature,
    unknown, // GPIO13
    Fan_lock_status,
    unknown,
    Fan_speed, // Fan PWM speed
    unknown,          // SCC charge power
    unknown,          // Parallel Warning
    unknown,          // Sync frequency
    Inverter_charge_state,
};
// raw length 102 or 105??????
static const char *const q1ListP18[] = {
    // [PI18]
    Time_until_absorb_charge, // Time until the end of absorb charging
    Time_until_float_charge,  // Time until the end of float charging
    SCC_Flag,               // SCC Flag
    AllowSccOnFlag,         // AllowSccOnFlag
    Charge_Average_Current, // ChargeAverageCurrent
    Tracker_temperature,    // SCC PWM temperature
    Inverter_temperature,
    Battery_temperature,
    Transformer_temperature,
    unknown, // GPIO13
    Fan_lock_status,
    unknown,
    Fan_speed,        // Fan PWM speed
    SCC_charge_power, // SCC charge power
    Parallel_Warning, // Parallel Warning
    Sync_frequency,   // Sync frequency
    Inverter_charge_state,
    unknown,
    unknown,
    unknown,
    unknown,
    unknown,
    unknown,
    unknown,
    unknown,
    unknown,
    unknown,
}; */


static const char *const Q1_47[] = {
    // [PI34 / MPPT-3000], [PI30 HS MS MSX], [PI30 Revo], [PI30 PIP], [PI41 / LV5048]
    "Time_until_absorb_charge",
    "Time_until_float_charge",
    "",
    "Tracker_temperature",
    "Inverter_temperature",
    "Battery_temperature",
    "Transformer_temperature",
    "",
    "",
    "",
    "Fan_speed",
    "",
    "Inverter_charge_state",
};

static const char *const q1List[] = {
    // [PI34 / MPPT-3000], [PI30 HS MS MSX], [PI30 Revo], [PI30 PIP], [PI41 / LV5048]
    "Time_until_absorb_charge",
    "Time_until_float_charge",
    "",
    "Tracker_temperature",
    "Inverter_temperature",
    "Battery_temperature",
    "Transformer_temperature",
    "",
    "",
    "",
    "Fan_speed",
    "",
    "Inverter_charge_state",
};
// Raw length 70
static const char *const q1List2[] = {
    // [PI30]
    "Time_until_absorb_charge", // Time until the end of absorb charging
    "Time_until_float_charge",  // Time until the end of float charging
    "",                         // SCC Flag
    "",                         // AllowSccOnFlag
    "",                         // ChargeAverageCurrent
    "Tracker_temperature",      // SCC PWM temperature
    "Inverter_temperature",
    "Battery_temperature",
    "Transformer_temperature",
    "", // GPIO13
    "Fan_lock_status",
    "",
    "Fan_speed", // Fan PWM speed
    "",          // SCC charge power
    "",          // Parallel Warning
    "",          // Sync frequency
    "Inverter_charge_state",
};
// raw length 102 or 105??????
static const char *const q1ListP18[] = {
    // [PI18]
    "Time_until_absorb_charge", // Time until the end of absorb charging
    "Time_until_float_charge",  // Time until the end of float charging
    "SCC_Flag",               // SCC Flag
    "AllowSccOnFlag",         // AllowSccOnFlag
    "Charge_Average_Current", // ChargeAverageCurrent
    "Tracker_temperature",    // SCC PWM temperature
    "Inverter_temperature",
    "Battery_temperature",
    "Transformer_temperature",
    "", // GPIO13
    "Fan_lock_status",
    "",
    "Fan_speed",        // Fan PWM speed
    "SCC_charge_power", // SCC charge power
    "Parallel_Warning", // Parallel Warning
    "Sync_frequency",   // Sync frequency
    "Inverter_charge_state",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
};
bool PI_Serial::PIXX_Q1()
{
    if (protocol == PI30) // pi30 and pi18 devices react both on Q1
    {
        String commandAnswer = this->requestData("Q1");
        get.raw.q1 = commandAnswer;
        byte commandAnswerLength = commandAnswer.length();
        if (commandAnswer == "NAK")
        {
            return true;
        }
        if (commandAnswer == "ERCRC")
        {
            return false;
        }
        if (commandAnswerLength == 47 || commandAnswerLength == 105)
        {
            String strs[30];
            // Split the string into substrings
            int StringCount = 0;
            while (commandAnswer.length() > 0)
            {
                // int index = commandAnswer.indexOf(delimiter);
                int index = commandAnswer.indexOf(' ');
                if (index == -1) // No space found
                {
                    strs[StringCount++] = commandAnswer;
                    break;
                }
                else
                {
                    strs[StringCount++] = commandAnswer.substring(0, index);
                    commandAnswer = commandAnswer.substring(index + 1);
                }
            }

            for (unsigned int i = 0; i < sizeof q1List / sizeof q1List[0]; i++)
            {
                if (!strs[i].isEmpty() && strcmp(q1List[i], "") != 0)
                    liveData[q1List[i]] = (int)(strs[i].toFloat() * 100 + 0.5) / 100.0;
            }
        }
        if (commandAnswerLength == 70)
        {
            String strs[30];
            // Split the string into substrings
            int StringCount = 0;
            while (commandAnswer.length() > 0)
            {
                // int index = commandAnswer.indexOf(delimiter);
                int index = commandAnswer.indexOf(' ');
                if (index == -1) // No space found
                {
                    strs[StringCount++] = commandAnswer;
                    break;
                }
                else
                {
                    strs[StringCount++] = commandAnswer.substring(0, index);
                    commandAnswer = commandAnswer.substring(index + 1);
                }
            }

            for (unsigned int i = 0; i < sizeof q1List2 / sizeof q1List2[0]; i++)
            {
                if (!strs[i].isEmpty() && strcmp(q1List2[i], "") != 0)
                    liveData[q1List2[i]] = (int)(strs[i].toFloat() * 100 + 0.5) / 100.0;
            }
        }

        if (liveData["Inverter_charge_state"].is<JsonVariant>())
        {
            switch ((int)liveData["Inverter_charge_state"].as<unsigned int>())
            {
            default:
                // liveData["Inverter_charge_state"] = "no data";
                break;
            case 10:
                liveData["Inverter_charge_state"] = "No charging";
                break;
            case 11:
                liveData["Inverter_charge_state"] = "Bulk stage";
                break;
            case 12:
                liveData["Inverter_charge_state"] = "Absorb";
                break;
            case 13:
                liveData["Inverter_charge_state"] = "Float";
                break;
            }
        }

        return true;
    }
    else if (protocol == PI18)
    {
        String commandAnswer = this->requestData("Q1");
        get.raw.q1 = commandAnswer;
        byte commandAnswerLength = commandAnswer.length();
        if (commandAnswer == "NAK")
        {
            return true;
        }
        if (commandAnswer == "ERCRC")
        {
            return false;
        }
        if (commandAnswerLength >= 47 && commandAnswerLength <= 105)
        // if (commandAnswerLength == 47 || commandAnswerLength == 105)
        {
            String strs[30];
            // Split the string into substrings
            int StringCount = 0;
            while (commandAnswer.length() > 0)
            {
                // int index = commandAnswer.indexOf(delimiter);
                int index = commandAnswer.indexOf(' ');
                if (index == -1) // No space found
                {
                    strs[StringCount++] = commandAnswer;
                    break;
                }
                else
                {
                    strs[StringCount++] = commandAnswer.substring(0, index);
                    commandAnswer = commandAnswer.substring(index + 1);
                }
            }

            for (unsigned int i = 0; i < sizeof q1ListP18 / sizeof q1ListP18[0]; i++)
            {
                if (!strs[i].isEmpty() && strcmp(q1ListP18[i], "") != 0)
                    liveData[q1ListP18[i]] = (int)(strs[i].toFloat() * 100 + 0.5) / 100.0;
            }

            if (liveData["Inverter_charge_state"].is<JsonVariant>())
            {
                switch ((int)liveData["Inverter_charge_state"].as<unsigned int>())
                {
                default:
                    // liveData["Inverter_charge_state"] = "no data";
                    break;
                case 10:
                    liveData["Inverter_charge_state"] = "No charging";
                    break;
                case 11:
                    liveData["Inverter_charge_state"] = "Bulk stage";
                    break;
                case 12:
                    liveData["Inverter_charge_state"] = "Absorb";
                    break;
                case 13:
                    liveData["Inverter_charge_state"] = "Float";
                    break;
                }
            }
        }
        return true;
    }
    else if (protocol == NoD)
    {
        return false;
    }
    else
    {
        return false;
    }
}
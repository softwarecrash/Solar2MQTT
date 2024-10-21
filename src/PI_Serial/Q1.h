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

static const char *const DESCR_Time_until_absorb_charge = "Time_until_absorb_charge"; // time since absorb start
static const char *const DESCR_Time_until_float_charge = "Time_until_float_charge";   // tiem since float charge start
static const char *const DESCR_SCC_Flag = "SCC_Flag";                                 // SCC Flag
static const char *const DESCR_AllowSccOnFlag = "AllowSccOnFlag";                     // AllowSccOnFlag
static const char *const DESCR_Charge_Average_Current = "Charge_Average_Current";     // ChargeAverageCurrent
static const char *const DESCR_Tracker_temperature = "Tracker_temperature";           // Temp sensor 1 SCC PWM temperature
static const char *const DESCR_Inverter_temperature = "Inverter_temperature";         // temp sensor 2
static const char *const DESCR_Battery_temperature = "Battery_temperature";           // temp sensor 3
static const char *const DESCR_Transformer_temperature = "Transformer_temperature";   //
static const char *const DESCR_Fan_lock_status = "Fan_lock_status";                   // error flag for blocked fan
static const char *const DESCR_Fan_speed = "Fan_speed";                               // Fan PWM speed
static const char *const DESCR_SCC_charge_power = "SCC_charge_power";                 // SCC charge power
static const char *const DESCR_Parallel_Warning = "Parallel_Warning";                 // Parallel Warning
static const char *const DESCR_Sync_frequency = "Sync_frequency";                     // Sync frequency
static const char *const DESCR_Inverter_charge_state = "Inverter_charge_state";       // charge state
static const char *const DESCR_unknown = "";                                          // unknown state

const char *const Q1_47[] = {
    DESCR_Time_until_absorb_charge,
    DESCR_Time_until_float_charge,
    DESCR_unknown,
    DESCR_Tracker_temperature,
    DESCR_Inverter_temperature,
    DESCR_Battery_temperature,
    DESCR_Transformer_temperature,
    DESCR_unknown,
    DESCR_unknown,
    DESCR_unknown,
    DESCR_Fan_speed,
    DESCR_unknown,
    DESCR_Inverter_charge_state,
};
static const char *const Q1_70[] = {
    DESCR_Time_until_absorb_charge, // Time until the end of absorb charging
    DESCR_Time_until_float_charge,  // Time until the end of float charging
    DESCR_unknown,                  // SCC Flag
    DESCR_unknown,                  // AllowSccOnFlag
    DESCR_unknown,                  // ChargeAverageCurrent
    DESCR_Tracker_temperature,      // SCC PWM temperature
    DESCR_Inverter_temperature,
    DESCR_Battery_temperature,
    DESCR_Transformer_temperature,
    DESCR_unknown, // GPIO13
    DESCR_Fan_lock_status,
    DESCR_unknown,
    DESCR_Fan_speed, // Fan PWM speed
    DESCR_unknown,   // SCC charge power
    DESCR_unknown,   // Parallel Warning
    DESCR_unknown,   // Sync frequency
    DESCR_Inverter_charge_state,
};
static const char *const Q1_87[] = {
    DESCR_Time_until_absorb_charge, // Time until the end of absorb charging
    DESCR_Time_until_float_charge,  // Time until the end of float charging
    DESCR_unknown,                  // SCC Flag
    DESCR_unknown,                  // AllowSccOnFlag
    DESCR_unknown,                  // ChargeAverageCurrent
    DESCR_Tracker_temperature,      // SCC PWM temperature
    DESCR_Inverter_temperature,
    DESCR_Battery_temperature,
    DESCR_Transformer_temperature,
    DESCR_unknown, // GPIO13
    DESCR_Fan_lock_status,
    DESCR_unknown,
    DESCR_Fan_speed, // Fan PWM speed
    DESCR_unknown,   // SCC charge power
    DESCR_unknown,   // Parallel Warning
    DESCR_unknown,   // Sync frequency
    DESCR_Inverter_charge_state,
};
const char *const Q1_108[] = {
    DESCR_Time_until_absorb_charge, // Time until the end of absorb charging
    DESCR_Time_until_float_charge,  // Time until the end of float charging
    DESCR_SCC_Flag,                 // SCC Flag
    DESCR_AllowSccOnFlag,           // AllowSccOnFlag
    DESCR_Charge_Average_Current,   // ChargeAverageCurrent
    DESCR_Tracker_temperature,      // SCC PWM temperature
    DESCR_Inverter_temperature,
    DESCR_Battery_temperature,
    DESCR_Transformer_temperature,
    DESCR_unknown, // GPIO13
    DESCR_Fan_lock_status,
    DESCR_unknown,
    DESCR_Fan_speed,        // Fan PWM speed
    DESCR_SCC_charge_power, // SCC charge power
    DESCR_Parallel_Warning, // Parallel Warning
    DESCR_Sync_frequency,   // Sync frequency
    DESCR_Inverter_charge_state,
    DESCR_unknown,
    DESCR_unknown,
    DESCR_unknown,
    DESCR_unknown,
    DESCR_unknown,
    DESCR_unknown,
    DESCR_unknown,
    DESCR_unknown,
    DESCR_unknown,
    DESCR_unknown,
};

bool PI_Serial::PIXX_Q1()
{
    if (protocol == PI30 || protocol == PI18) // pi30 and pi18 devices react both on Q1
    {
        String strs[30];
        int StringCount = 0;
        String commandAnswer = this->requestData("Q1");
        get.raw.q1 = commandAnswer;

        if (commandAnswer == "NAK")
        {
            return true;
        }
        if (commandAnswer == "ERCRC")
        {
            return false;
        }
        switch (commandAnswer.length())
        {
        case 47:
            // Split the string into substrings
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
            for (unsigned int i = 0; i < sizeof Q1_47 / sizeof Q1_47[0]; i++)
            {
                if (!strs[i].isEmpty() && strcmp(Q1_47[i], "") != 0)
                    liveData[Q1_47[i]] = (int)(strs[i].toFloat() * 100 + 0.5) / 100.0;
            }
            break;
        case 70:
            // Split the string into substrings
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
            for (unsigned int i = 0; i < sizeof Q1_70 / sizeof Q1_70[0]; i++)
            {
                if (!strs[i].isEmpty() && strcmp(Q1_70[i], "") != 0)
                    liveData[Q1_70[i]] = (int)(strs[i].toFloat() * 100 + 0.5) / 100.0;
            }
            break;
        case 87:
            while (commandAnswer.length() > 0)
            {
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
            for (unsigned int i = 0; i < sizeof Q1_87 / sizeof Q1_87[0]; i++)
            {
                if (!strs[i].isEmpty() && strcmp(Q1_87[i], "") != 0)
                    liveData[Q1_87[i]] = (int)(strs[i].toFloat() * 100 + 0.5) / 100.0;
            }
            break;
        case 105:
        case 108:
            while (commandAnswer.length() > 0)
            {
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
            for (unsigned int i = 0; i < sizeof Q1_108 / sizeof Q1_108[0]; i++)
            {
                if (!strs[i].isEmpty() && strcmp(Q1_108[i], "") != 0)
                    liveData[Q1_108[i]] = (int)(strs[i].toFloat() * 100 + 0.5) / 100.0;
            }
            break;

        default:
            get.raw.q1 = "Wrong Length(" + (String)get.raw.q1.length() + "), Contact Dev:" + get.raw.q1;
            break;
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
    else if (protocol == NoD)
    {
        return false;
    }
    else
    {
        return false;
    }
}
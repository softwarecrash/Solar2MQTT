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
unsigned int Q1_47_length = 13;
const char *const Q1_47[] = {
    DESCR_Time_Until_Absorb_Charge,
    DESCR_Time_Until_Float_Charge,
    nullptr,
    DESCR_Tracker_Temperature,
    DESCR_Inverter_Temperature,
    DESCR_Battery_Temperature,
    DESCR_Transformer_Temperature,
    nullptr,
    nullptr,
    nullptr,
    DESCR_Fan_Speed,
    nullptr,
    DESCR_Inverter_Charge_State,
};
unsigned int Q1_70_length = 17;
const char *const Q1_70[] = {
    DESCR_Time_Until_Absorb_Charge, // Time until the end of absorb charging
    DESCR_Time_Until_Float_Charge,  // Time until the end of float charging
    nullptr,                  // SCC Flag
    nullptr,                  // AllowSccOnFlag
    nullptr,                  // ChargeAverageCurrent
    DESCR_Tracker_Temperature,      // SCC PWM temperature
    DESCR_Inverter_Temperature,
    DESCR_Battery_Temperature,
    DESCR_Transformer_Temperature,
    nullptr, // GPIO13
    DESCR_Fan_Lock_Status,
    nullptr,
    DESCR_Fan_Speed, // Fan PWM speed
    nullptr,   // SCC charge power
    nullptr,   // Parallel Warning
    nullptr,   // Sync frequency
    DESCR_Inverter_Charge_State,
};
unsigned int Q1_87_length = 17;
const char *const Q1_87[] = {
    DESCR_Time_Until_Absorb_Charge, // Time until the end of absorb charging
    DESCR_Time_Until_Float_Charge,  // Time until the end of float charging
    nullptr,                  // SCC Flag
    nullptr,                  // AllowSccOnFlag
    nullptr,                  // ChargeAverageCurrent
    DESCR_Tracker_Temperature,      // SCC PWM temperature
    DESCR_Inverter_Temperature,
    DESCR_Battery_Temperature,
    DESCR_Transformer_Temperature,
    nullptr, // GPIO13
    DESCR_Fan_Lock_Status,
    nullptr,
    DESCR_Fan_Speed, // Fan PWM speed
    nullptr,   // SCC charge power
    nullptr,   // Parallel Warning
    nullptr,   // Sync frequency
    DESCR_Inverter_Charge_State,
};
unsigned int Q1_22_length = 22;
const char *const Q1_22[] = {
    DESCR_Time_Until_Absorb_Charge, // Time until the end of absorb charging
    DESCR_Time_Until_Float_Charge,  // Time until the end of float charging
    nullptr,                  // SCC Flag
    nullptr,                  // AllowSccOnFlag
    nullptr,                  // ChargeAverageCurrent
    DESCR_Tracker_Temperature,      // SCC PWM temperature
    DESCR_Inverter_Temperature,
    DESCR_Battery_Temperature,
    DESCR_Transformer_Temperature,
    nullptr, // GPIO13
    DESCR_Fan_Lock_Status,
    nullptr,
    DESCR_Fan_Speed, // Fan PWM speed
    nullptr,   // SCC charge power
    nullptr,   // Parallel Warning
    nullptr,   // Sync frequency
    DESCR_Inverter_Charge_State,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};
unsigned int Q1_108_length = 27;
const char *const Q1_108[] = {
    DESCR_Time_Until_Absorb_Charge, // Time until the end of absorb charging
    DESCR_Time_Until_Float_Charge,  // Time until the end of float charging
    DESCR_SCC_Flag,                 // SCC Flag
    DESCR_Allow_SCC_On_Flag,           // AllowSccOnFlag
    DESCR_Charge_Average_Current,   // ChargeAverageCurrent
    DESCR_Tracker_Temperature,      // SCC PWM temperature
    DESCR_Inverter_Temperature,
    DESCR_Battery_Temperature,
    DESCR_Transformer_Temperature,
    nullptr, // GPIO13
    DESCR_Fan_Lock_Status,
    nullptr,
    DESCR_Fan_Speed,        // Fan PWM speed
    DESCR_SCC_Charge_Power, // SCC charge power
    DESCR_Parallel_Warning, // Parallel Warning
    DESCR_Sync_Frequency,   // Sync frequency
    DESCR_Inverter_Charge_State,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

bool PI_Serial::PIXX_Q1()
{
    if (isPi30LikeProtocol(protocol) || protocol == PI18) // pi30 and pi18 devices react both on Q1
    {
        pi_clear_json_fields(liveData, Q1_47);
        pi_clear_json_fields(liveData, Q1_70);
        pi_clear_json_fields(liveData, Q1_87);
        pi_clear_json_fields(liveData, Q1_22);
        pi_clear_json_fields(liveData, Q1_108);

        const char *const *q1List = nullptr;
        unsigned int q1LList_length = 0;
        String commandAnswer = this->requestData("Q1");
        get.raw.q1 = commandAnswer;

        if (commandAnswer == DESCR_req_NAK || commandAnswer == DESCR_req_NOA)
            return true;
        if (commandAnswer == DESCR_req_ERCRC)
            return false;
        char *fieldsQ1[30];
        char bufQ1Primary[256];
        char bufQ1Fallback[256];
        commandAnswer.toCharArray(bufQ1Primary, sizeof(bufQ1Primary));
        commandAnswer.toCharArray(bufQ1Fallback, sizeof(bufQ1Fallback));

        // Q1 is inconsistent across vendors. Most devices use spaces here, but some
        // protocol families use commas elsewhere. Try the configured delimiter first
        // and fall back to the alternative when it yields more fields.
        const char primaryDelimiter = delimiter[0];
        const char fallbackDelimiter = (primaryDelimiter == ' ') ? ',' : ' ';

        char *fieldsQ1Primary[30];
        char *fieldsQ1Fallback[30];
        int primaryCount = pi_split_fields(bufQ1Primary, primaryDelimiter, fieldsQ1Primary, 30);
        int fallbackCount = pi_split_fields(bufQ1Fallback, fallbackDelimiter, fieldsQ1Fallback, 30);

        int StringCount = primaryCount;
        if (fallbackCount > primaryCount)
        {
            for (int i = 0; i < 30; ++i)
                fieldsQ1[i] = fieldsQ1Fallback[i];
            StringCount = fallbackCount;
        }
        else
        {
            for (int i = 0; i < 30; ++i)
                fieldsQ1[i] = fieldsQ1Primary[i];
        }
        if (StringCount >= (int)Q1_108_length)
        {
            q1List = Q1_108;
            q1LList_length = Q1_108_length;
        }
        else if (StringCount >= (int)Q1_22_length)
        {
            q1List = Q1_22;
            q1LList_length = Q1_22_length;
        }
        else if (StringCount >= (int)Q1_70_length)
        {
            q1List = Q1_70;
            q1LList_length = Q1_70_length;
        }
        else if (StringCount >= (int)Q1_47_length)
        {
            q1List = Q1_47;
            q1LList_length = Q1_47_length;
        }
        else
        {
            return true;
        }
        for (unsigned int i = 0; i < q1LList_length && i < (unsigned int)StringCount; i++)
        {
            if (q1List[i] != nullptr && fieldsQ1[i] && fieldsQ1[i][0] != '\0')
                liveData[q1List[i]] = pi_parse_float2(fieldsQ1[i]);
        }

        if (liveData[DESCR_Inverter_Charge_State].is<JsonVariant>())
        {
            switch ((int)liveData[DESCR_Inverter_Charge_State].as<unsigned int>())
            {
            default:
                // liveData["Inverter_Charge_State"] = "no data";
                break;
            case 10:
                liveData[DESCR_Inverter_Charge_State] = DESCR_No_Charging;
                break;
            case 11:
                liveData[DESCR_Inverter_Charge_State] = DESCR_Bulk_Stage;
                break;
            case 12:
                liveData[DESCR_Inverter_Charge_State] = DESCR_Absorb;
                break;
            case 13:
                liveData[DESCR_Inverter_Charge_State] = DESCR_Float;
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

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
static const char *const q1List2[] = {
    // [PI30]
    "Time_until_absorb_charge", // Time until the end of absorb charging
    "Time_until_float_charge", // Time until the end of float charging
    "", // SCC Flag
    "", // AllowSccOnFlag
    "", // ChargeAverageCurrent
    "Tracker_temperature", // SCC PWM temperature
    "Inverter_temperature",
    "Battery_temperature",
    "Transformer_temperature",
    "", // GPIO13
    "Fan_lock_status",
    "",
    "Fan_speed", // Fan PWM speed
    "", // SCC charge power
    "", // Parallel Warning
    "", // Sync frequency
    "Inverter_charge_state",
};
bool PI_Serial::PIXX_Q1()
{
    if (protocol == PI30)
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
            String strs[16];
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
            String strs[16];
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

        if (liveData.containsKey("Inverter_charge_state"))
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
        if (commandAnswerLength == 47 || commandAnswerLength == 105)
        {
            String strs[16];
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

            if (liveData.containsKey("Inverter_charge_state"))
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
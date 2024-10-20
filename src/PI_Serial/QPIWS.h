bool PI_Serial::PIXX_QPIWS()
{
    if (protocol == PI30)
    {
        String commandAnswer = this->requestData("QPIWS");
        //String commandAnswer = "10000000001010000000000000000000";
        get.raw.qpiws = commandAnswer;
        if (commandAnswer == "NAK")
        {
            return true;
        }
        if (commandAnswer == "ERCRC")
        {
            return false;
        }
        if (commandAnswer.length() == 32)
        {
            std::vector<String> qpiwsStrings;
            if ((char)commandAnswer.charAt(1) == '1') qpiwsStrings.emplace_back("Inverter fault"); // 2
            if ((char)commandAnswer.charAt(2) == '1') qpiwsStrings.emplace_back("Bus over fault"); // 3
            if ((char)commandAnswer.charAt(3) == '1') qpiwsStrings.emplace_back("Bus under fault"); // 4
            if ((char)commandAnswer.charAt(4) == '1') qpiwsStrings.emplace_back("Bus soft fail fault"); // 5
            if ((char)commandAnswer.charAt(5) == '1') qpiwsStrings.emplace_back("Line fail warning"); // 6
            if ((char)commandAnswer.charAt(6) == '1') qpiwsStrings.emplace_back("OPV short warning"); // 7
            if ((char)commandAnswer.charAt(7) == '1') qpiwsStrings.emplace_back("Inverter voltage too low fault"); // 8
            if ((char)commandAnswer.charAt(8) == '1') qpiwsStrings.emplace_back("Inverter voltage too high fault"); // 9
            if ((char)commandAnswer.charAt(9) == '1') qpiwsStrings.emplace_back("Over temperature fault"); // 10
            if ((char)commandAnswer.charAt(10) == '1') qpiwsStrings.emplace_back("Fan locked fault"); // 11
            if ((char)commandAnswer.charAt(11) == '1') qpiwsStrings.emplace_back("Battery voltage too high fault"); // 12
            if ((char)commandAnswer.charAt(12) == '1') qpiwsStrings.emplace_back("Battery low alarm warning"); // 13
            if ((char)commandAnswer.charAt(14) == '1') qpiwsStrings.emplace_back("Battery under shutdown warning"); // 15
            if ((char)commandAnswer.charAt(16) == '1') qpiwsStrings.emplace_back("Overload fault"); // 17
            if ((char)commandAnswer.charAt(17) == '1') qpiwsStrings.emplace_back("EEPROM fault"); // 18
            if ((char)commandAnswer.charAt(18) == '1') qpiwsStrings.emplace_back("Inverter over current fault"); // 19
            if ((char)commandAnswer.charAt(19) == '1') qpiwsStrings.emplace_back("Inverter soft fail fault"); // 20
            if ((char)commandAnswer.charAt(20) == '1') qpiwsStrings.emplace_back("Self test fail fault"); // 21
            if ((char)commandAnswer.charAt(21) == '1') qpiwsStrings.emplace_back("OP DC voltage over fault"); // 22
            if ((char)commandAnswer.charAt(22) == '1') qpiwsStrings.emplace_back("Battery open fault"); // 23
            if ((char)commandAnswer.charAt(23) == '1') qpiwsStrings.emplace_back("Current sensor fail fault"); // 24
            if ((char)commandAnswer.charAt(24) == '1') qpiwsStrings.emplace_back("Battery short fault"); // 25
            if ((char)commandAnswer.charAt(25) == '1') qpiwsStrings.emplace_back("Power limit warning"); // 26
            if ((char)commandAnswer.charAt(26) == '1') qpiwsStrings.emplace_back("PV voltage high warning"); // 27
            if ((char)commandAnswer.charAt(27) == '1') qpiwsStrings.emplace_back("MPPT overload fault"); // 28
            if ((char)commandAnswer.charAt(28) == '1') qpiwsStrings.emplace_back("MPPT overload warning"); // 29
            if ((char)commandAnswer.charAt(29) == '1') qpiwsStrings.emplace_back("Battery too low to charge warning"); // 30
            if (!qpiwsStrings.empty())
            {
                String qpiwsStr = "";
                for (size_t i = 0; i < qpiwsStrings.size(); i++) {
                    qpiwsStr += qpiwsStrings[i];
                    if (i < qpiwsStrings.size() - 1) {
                        qpiwsStr += "; ";
                    }
                }
                liveData["Fault_code"] = qpiwsStr;
            }
            else
            {
                liveData["Fault_code"] = "Ok";
            }
        }
        return true;
    }
    else if(protocol == PI18){
        String commandAnswer = this->requestData("^P005FWS");
        get.raw.qpiws = commandAnswer;
        if (commandAnswer == "NAK")
        {
            return true;
        }
        if (commandAnswer == "ERCRC")
        {
            return false;
        }
        if (commandAnswer.length() == 37)
        {
/*             std::vector<String> qpiwsStrings;
            if ((char)commandAnswer.charAt(1) == '1') qpiwsStrings.emplace_back("Inverter fault"); // 2
            if ((char)commandAnswer.charAt(2) == '1') qpiwsStrings.emplace_back("Bus over fault"); // 3
            if ((char)commandAnswer.charAt(3) == '1') qpiwsStrings.emplace_back("Bus under fault"); // 4
            if ((char)commandAnswer.charAt(4) == '1') qpiwsStrings.emplace_back("Bus soft fail fault"); // 5
            if ((char)commandAnswer.charAt(5) == '1') qpiwsStrings.emplace_back("Line fail warning"); // 6
            if ((char)commandAnswer.charAt(6) == '1') qpiwsStrings.emplace_back("OPV short warning"); // 7
            if ((char)commandAnswer.charAt(7) == '1') qpiwsStrings.emplace_back("Inverter voltage too low fault"); // 8
            if ((char)commandAnswer.charAt(8) == '1') qpiwsStrings.emplace_back("Inverter voltage too high fault"); // 9
            if ((char)commandAnswer.charAt(9) == '1') qpiwsStrings.emplace_back("Over temperature fault"); // 10
            if ((char)commandAnswer.charAt(10) == '1') qpiwsStrings.emplace_back("Fan locked fault"); // 11
            if ((char)commandAnswer.charAt(11) == '1') qpiwsStrings.emplace_back("Battery voltage too high fault"); // 12
            if ((char)commandAnswer.charAt(12) == '1') qpiwsStrings.emplace_back("Battery low alarm warning"); // 13
            if ((char)commandAnswer.charAt(14) == '1') qpiwsStrings.emplace_back("Battery under shutdown warning"); // 15
            if ((char)commandAnswer.charAt(16) == '1') qpiwsStrings.emplace_back("Overload fault"); // 17
            if ((char)commandAnswer.charAt(17) == '1') qpiwsStrings.emplace_back("EEPROM fault"); // 18
            if ((char)commandAnswer.charAt(18) == '1') qpiwsStrings.emplace_back("Inverter over current fault"); // 19
            if ((char)commandAnswer.charAt(19) == '1') qpiwsStrings.emplace_back("Inverter soft fail fault"); // 20
            if ((char)commandAnswer.charAt(20) == '1') qpiwsStrings.emplace_back("Self test fail fault"); // 21
            if ((char)commandAnswer.charAt(21) == '1') qpiwsStrings.emplace_back("OP DC voltage over fault"); // 22
            if ((char)commandAnswer.charAt(22) == '1') qpiwsStrings.emplace_back("Battery open fault"); // 23
            if ((char)commandAnswer.charAt(23) == '1') qpiwsStrings.emplace_back("Current sensor fail fault"); // 24
            if ((char)commandAnswer.charAt(24) == '1') qpiwsStrings.emplace_back("Battery short fault"); // 25
            if ((char)commandAnswer.charAt(25) == '1') qpiwsStrings.emplace_back("Power limit warning"); // 26
            if ((char)commandAnswer.charAt(26) == '1') qpiwsStrings.emplace_back("PV voltage high warning"); // 27
            if ((char)commandAnswer.charAt(27) == '1') qpiwsStrings.emplace_back("MPPT overload fault"); // 28
            if ((char)commandAnswer.charAt(28) == '1') qpiwsStrings.emplace_back("MPPT overload warning"); // 29
            if ((char)commandAnswer.charAt(29) == '1') qpiwsStrings.emplace_back("Battery too low to charge warning"); // 30
            if (!qpiwsStrings.empty())
            {
                String qpiwsStr = "";
                for (size_t i = 0; i < qpiwsStrings.size(); i++) {
                    qpiwsStr += qpiwsStrings[i];
                    if (i < qpiwsStrings.size() - 1) {
                        qpiwsStr += "; ";
                    }
                }
                liveData["Fault_code"] = qpiwsStr;
            }
            else
            {
                liveData["Fault_code"] = "Ok";
            } */
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
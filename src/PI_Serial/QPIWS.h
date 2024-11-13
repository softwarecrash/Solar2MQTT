bool PI_Serial::PIXX_QPIWS()
{
    if (protocol == PI30)
    {
        String commandAnswer = this->requestData("QPIWS");
        //String commandAnswer = "10000000001010000000000000000000";
        get.raw.qpiws = commandAnswer;
        if (commandAnswer == DESCR_req_NAK || commandAnswer == DESCR_req_NOA)
        {
            return true;
        }
        if (commandAnswer == DESCR_req_ERCRC)
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
                liveData[DESCR_Fault_Code] = qpiwsStr;
            }
            else
            {
                liveData[DESCR_Fault_Code] = "Ok";
            }
        }else {
            get.raw.qpiws = "Wrong Length(" + (String)get.raw.qpiws.length() + "), Contact Dev:" +get.raw.qpiws;
        }
        return true;
    }
    else if(protocol == PI18){
        String commandAnswer = this->requestData("^P005FWS");
        get.raw.qpiws = commandAnswer;
        if (commandAnswer == DESCR_req_NAK || commandAnswer == DESCR_req_NOA)
        {
            return true;
        }
        if (commandAnswer == DESCR_req_ERCRC)
        {
            return false;
        }
        //[C: ^P005FWS][CR: B69E][CC: B69E][L:  36] from valqk
        //QPIWS 00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
        if (commandAnswer.length() == 36)
        {
            std::vector<String> qpiwsStrings;
            if (commandAnswer.substring(0,2) == "01") qpiwsStrings.emplace_back("Fan is locked"); // 2
            if (commandAnswer.substring(0,2) == "02") qpiwsStrings.emplace_back("Over temperature"); // 2
            if (commandAnswer.substring(0,2) == "03") qpiwsStrings.emplace_back("Battery voltage is too high"); // 2
            if (commandAnswer.substring(0,2) == "04") qpiwsStrings.emplace_back("Battery voltage is too low"); // 2
            if (commandAnswer.substring(0,2) == "05") qpiwsStrings.emplace_back("Output short circuited or Over temperature"); // 2
            if (commandAnswer.substring(0,2) == "06") qpiwsStrings.emplace_back("Output voltage is too high"); // 2
            if (commandAnswer.substring(0,2) == "07") qpiwsStrings.emplace_back("Over load time out"); // 2
            if (commandAnswer.substring(0,2) == "08") qpiwsStrings.emplace_back("Bus voltage is too high"); // 2
            if (commandAnswer.substring(0,2) == "09") qpiwsStrings.emplace_back("Bus soft start failed"); // 2
            if (commandAnswer.substring(0,2) == "11") qpiwsStrings.emplace_back("Main relay failed"); // 2
            if (commandAnswer.substring(0,2) == "51") qpiwsStrings.emplace_back("Over current inverter"); // 2
            if (commandAnswer.substring(0,2) == "52") qpiwsStrings.emplace_back("Bus soft start failed"); // 2
            if (commandAnswer.substring(0,2) == "53") qpiwsStrings.emplace_back("Inverter soft start failed"); // 2
            if (commandAnswer.substring(0,2) == "54") qpiwsStrings.emplace_back("Self-test failed"); // 2
            if (commandAnswer.substring(0,2) == "55") qpiwsStrings.emplace_back("Over DC voltage on output of inverter"); // 2
            if (commandAnswer.substring(0,2) == "56") qpiwsStrings.emplace_back("Battery connection is open"); // 2
            if (commandAnswer.substring(0,2) == "57") qpiwsStrings.emplace_back("Current sensor failed"); // 2
            if (commandAnswer.substring(0,2) == "58") qpiwsStrings.emplace_back("Output voltage is too low"); // 2
            if (commandAnswer.substring(0,2) == "60") qpiwsStrings.emplace_back("Inverter negative power"); // 2
            if (commandAnswer.substring(0,2) == "71") qpiwsStrings.emplace_back("Parallel version different"); // 2
            if (commandAnswer.substring(0,2) == "72") qpiwsStrings.emplace_back("Output circuit failed"); // 2
            if (commandAnswer.substring(0,2) == "80") qpiwsStrings.emplace_back("CAN communication failed"); // 2
            if (commandAnswer.substring(0,2) == "81") qpiwsStrings.emplace_back("Parallel host line lost"); // 2
            if (commandAnswer.substring(0,2) == "82") qpiwsStrings.emplace_back("Parallel synchronized signal lost"); // 2
            if (commandAnswer.substring(0,2) == "83") qpiwsStrings.emplace_back("Parallel battery voltage detect different"); // 2
            if (commandAnswer.substring(0,2) == "84") qpiwsStrings.emplace_back("Parallel Line voltage or frequency detect different"); // 2
            if (commandAnswer.substring(0,2) == "85") qpiwsStrings.emplace_back("Parallel Line input current unbalanced"); // 2
            if (commandAnswer.substring(0,2) == "86") qpiwsStrings.emplace_back("Parallel output setting different"); // 2

            if ((char)commandAnswer.charAt(3) == '1') qpiwsStrings.emplace_back("Line fail"); // 2
            if ((char)commandAnswer.charAt(5) == '1') qpiwsStrings.emplace_back("Over temperature"); // 20
            if ((char)commandAnswer.charAt(7) == '1') qpiwsStrings.emplace_back("Output circuit short"); // 3
            if ((char)commandAnswer.charAt(9) == '1') qpiwsStrings.emplace_back("Inverter over temperature"); // 4
            if ((char)commandAnswer.charAt(11) == '1') qpiwsStrings.emplace_back("Fan lock"); // 5
            if ((char)commandAnswer.charAt(13) == '1') qpiwsStrings.emplace_back("Battery voltage high"); // 6
            if ((char)commandAnswer.charAt(15) == '1') qpiwsStrings.emplace_back("Battery low"); // 7
            if ((char)commandAnswer.charAt(17) == '1') qpiwsStrings.emplace_back("Battery under"); // 8
            if ((char)commandAnswer.charAt(19) == '1') qpiwsStrings.emplace_back("Over load"); // 9
            if ((char)commandAnswer.charAt(21) == '1') qpiwsStrings.emplace_back("Eeprom fail"); // 10
            if ((char)commandAnswer.charAt(23) == '1') qpiwsStrings.emplace_back("Power limit"); // 11
            if ((char)commandAnswer.charAt(25) == '1') qpiwsStrings.emplace_back("PV1 voltage high"); // 12
            if ((char)commandAnswer.charAt(27) == '1') qpiwsStrings.emplace_back("PV2 voltage high"); // 13
            if ((char)commandAnswer.charAt(29) == '1') qpiwsStrings.emplace_back("MPPT1 overload warning"); // 15
            if ((char)commandAnswer.charAt(31) == '1') qpiwsStrings.emplace_back("MPPT2 overload warning"); // 17
            if ((char)commandAnswer.charAt(33) == '1') qpiwsStrings.emplace_back("Battery too low to charge for SCC1"); // 18
            if ((char)commandAnswer.charAt(35) == '1') qpiwsStrings.emplace_back("Battery too low to charge for SCC2"); // 19

            if (!qpiwsStrings.empty())
            {
                String qpiwsStr = "";
                for (size_t i = 0; i < qpiwsStrings.size(); i++) {
                    qpiwsStr += qpiwsStrings[i];
                    if (i < qpiwsStrings.size() - 1) {
                        qpiwsStr += "; ";
                    }
                }
                liveData[DESCR_Fault_Code] = qpiwsStr;
            } 
            else
            {
                liveData[DESCR_Fault_Code] = "Ok";
            }
        }else {
            get.raw.qpiws = "Wrong Length(" + (String)get.raw.qpiws.length() + "), Contact Dev:" +get.raw.qpiws;
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
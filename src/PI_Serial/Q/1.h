
bool PI_Serial::PIXX_Q1()
{
    if (!qAvaible.qall)
        return true;
    String commandAnswer = this->requestData("Q1");
    byte commandAnswerLength = commandAnswer.length();
    // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
    if (commandAnswer == "NAK")
    {
        qAvaible.q1 = false; // if recived NAK, set the command avaible to false and never aks again until reboot
        return true;
    }
    else if (commandAnswerLength == 47)
    {
        get.raw.q1 = commandAnswer;
        int index = 0;
        // 01 01 00 035 022 023 025 00 00 000 0100 9290 11
        get.variableData.timeUntilAbsorbCharge = getNextLong(commandAnswer, index);
        get.variableData.timeUntilfloatCharge = getNextLong(commandAnswer, index);
        get.variableData.dontKnow0 = getNextLong(commandAnswer, index);
        get.variableData.trackertemp = getNextLong(commandAnswer, index);
        get.variableData.InverterTemp = getNextLong(commandAnswer, index);
        get.variableData.batteryTemp = getNextLong(commandAnswer, index);
        get.variableData.transformerTemp = getNextLong(commandAnswer, index);
        get.variableData.dontKnow1 = getNextLong(commandAnswer, index);
        get.variableData.dontKnow2 = getNextLong(commandAnswer, index);
        get.variableData.dontKnow3 = getNextLong(commandAnswer, index);
        get.variableData.fanSpeed = getNextLong(commandAnswer, index);
        get.variableData.sccChargePower = getNextLong(commandAnswer, index) / 100; // divided by 100
                                              //"10": "nocharging", "11": "bulk stage", "12": "absorb", "13": "float"
        unsigned int mode = getNextLong(commandAnswer, index);
        switch (mode)
        {
        default:
            get.variableData.inverterChargeStatus = ("Undefined, Origin: " + mode);
            break;
        case 10:
            get.variableData.inverterChargeStatus = "no charging";
            break;
        case 11:
            get.variableData.inverterChargeStatus = "bulk stage";
            break;
        case 12:
            get.variableData.inverterChargeStatus = "absorb";
            break;
        case 13:
            get.variableData.inverterChargeStatus = "float";
            break;
        }

        return true;
    }
    else
    {
        return false;
    }
}

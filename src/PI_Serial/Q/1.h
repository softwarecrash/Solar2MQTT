
bool PI_Serial::PIXX_Q1()
{
    if (!qAvaible.q1)
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
        get.variableData.sccChargePower = (getNextLong(commandAnswer, index) / 10); // divided by 100 //not needed, we get in on qpigs allways
                                                             
        short mode = getNextInt(commandAnswer, index);
        switch (mode)
        {
        default:
            get.variableData.inverterChargeStatus = (char *)"no data";
            break;
        case 10:
            get.variableData.inverterChargeStatus = (char *)"no charging";
            break;
        case 11:
            get.variableData.inverterChargeStatus = (char *)"bulk stage";
            break;
        case 12:
            get.variableData.inverterChargeStatus = (char *)"absorb";
            break;
        case 13:
            get.variableData.inverterChargeStatus = (char *)"float";
            break;
        }

        return true;
    }
    else
    {
        return false;
    }
}

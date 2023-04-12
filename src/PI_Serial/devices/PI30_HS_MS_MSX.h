void PI_Serial::PI30_HS_MS_MSX_QPIGS()
{
    String commandAnswer = this->requestData("QPIGS");
    if (commandAnswer != "NAK" && commandAnswer.length() == 106) // make sure
    {
        int index = 0;
        get.variableData.gridV = getNextFloat(commandAnswer, index);
        get.variableData.gridHz = getNextFloat(commandAnswer, index);
        get.variableData.acOutV = getNextFloat(commandAnswer, index);
        get.variableData.acOutHz = getNextFloat(commandAnswer, index);
        get.variableData.acOutVa = (short)getNextLong(commandAnswer, index);
        get.variableData.acOutW = (short)getNextLong(commandAnswer, index);
        get.variableData.acOutPercent = (byte)getNextLong(commandAnswer, index);
        get.variableData.busV = (short)getNextLong(commandAnswer, index);
        get.variableData.battV = getNextFloat(commandAnswer, index);

        get.variableData.batteryLoad = (byte)getNextLong(commandAnswer, index);

        get.variableData.battPercent = (byte)getNextLong(commandAnswer, index);
        get.variableData.heatSinkDegC = getNextFloat(commandAnswer, index);
        get.variableData.solarA = (byte)getNextLong(commandAnswer, index);
        get.variableData.solarV = (byte)getNextLong(commandAnswer, index);
        get.variableData.sccBattV = getNextFloat(commandAnswer, index);

        get.variableData.batteryLoad = (get.variableData.batteryLoad - (byte)getNextLong(commandAnswer, index));

        get.variableData.addSbuPriorityVersion = getNextLong(commandAnswer, index);
        get.variableData.isConfigChanged = getNextLong(commandAnswer, index);
        get.variableData.isSccFirmwareUpdated = getNextFloat(commandAnswer, index);
        get.variableData.solarW = getNextFloat(commandAnswer, index);
        get.variableData.battVoltageToSteadyWhileCharging = getNextFloat(commandAnswer, index);
        get.variableData.chargingStatus = getNextLong(commandAnswer, index);
        get.variableData.reservedY = getNextLong(commandAnswer, index);
        get.variableData.reservedZ = getNextLong(commandAnswer, index);
        get.variableData.reservedAA = getNextLong(commandAnswer, index);
        get.variableData.reservedBB = getNextLong(commandAnswer, index);

        Serial.println(commandAnswer.length()); // debug
        Serial.println(commandAnswer);          // debug
    }
}
void PI_Serial::PI30_HS_MS_MSX_QMOD()
{
    String commandAnswer = this->requestData("QMOD");
    if (commandAnswer != "NAK" && commandAnswer.length() == 1) // make sure
    {
        get.variableData.operationMode = getModeDesc((char)commandAnswer.charAt(0));
    }
}
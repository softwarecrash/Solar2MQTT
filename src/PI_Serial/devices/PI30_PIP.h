/*








not usable, youst for testing


need resort to the PIP protocol








 */
void PI_Serial::PI30_PIP_QPIGS()
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
    }
}
void PI_Serial::PI30_PIP_QMOD()
{
    String commandAnswer = this->requestData("QMOD");
    if (commandAnswer != "NAK" && commandAnswer.length() == 1)
    {
        get.variableData.operationMode = getModeDesc((char)commandAnswer.charAt(0));
    }
}
void PI_Serial::PI30_PIP_QPIRI()
{
    String commandAnswer = this->requestData("QMOD");
    if (commandAnswer != "NAK" && commandAnswer.length() == 1)
    {
        int index = 0;                                                     // after the starting '('
    get.staticData.gridRatingV = getNextFloat(commandAnswer, index);   // BBB.B
    get.staticData.gridRatingA = getNextFloat(commandAnswer, index);   // CC.C
    get.staticData.acOutRatingV = getNextFloat(commandAnswer, index);  // DDD.D
    get.staticData.acOutRatingHz = getNextFloat(commandAnswer, index); // EE.E
    get.staticData.acOutRatingA = getNextFloat(commandAnswer, index);  // FF.F
    get.staticData.acOutRatungVA = getNextFloat(commandAnswer, index); // HHHH
    get.staticData.acOutRatingW = getNextFloat(commandAnswer, index);  // IIII
    get.staticData.battRatingV = getNextFloat(commandAnswer, index);   // JJ.J

    get.staticData.battreChargeV = getNextFloat(commandAnswer, index); // KK.K
    get.staticData.battUnderV = getNextFloat(commandAnswer, index);    // 1JJ.J
    get.staticData.battBulkV = getNextFloat(commandAnswer, index);     // 1KK.K
    get.staticData.battFloatV = getNextFloat(commandAnswer, index);    // LL.L

    switch ((byte)getNextLong(commandAnswer, index)) // O
    {
    case 0:
      get.staticData.battType = "AGM";
      break;
    case 1:
      get.staticData.battType = "Flooded";
      break;
    case 2:
      get.staticData.battType = "User";
      break;
    }
    }
}
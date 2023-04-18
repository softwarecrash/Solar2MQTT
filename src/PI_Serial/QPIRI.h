void PI_Serial::PI30_QPIRI()
{
  String commandAnswer = this->requestData("QPIRI");
  // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
  if (commandAnswer != "NAK" && commandAnswer.length() == 94)
  {
    int index = 0;
    get.staticData.gridRatingVoltage = getNextFloat(commandAnswer, index);          // BBB.B
    get.staticData.gridRatingCurrent = getNextFloat(commandAnswer, index);          // CC.C
    get.staticData.acOutputRatingVoltage = getNextFloat(commandAnswer, index);      // DDD.D
    get.staticData.acOutputRatingFrquency = getNextFloat(commandAnswer, index);     // EE.E
    get.staticData.acoutputRatingCurrent = getNextFloat(commandAnswer, index);      // FF.F
    get.staticData.acOutputRatingApparentPower = getNextLong(commandAnswer, index); // HHHH
    get.staticData.acOutputRatingActivePower = getNextLong(commandAnswer, index);   // IIII
    get.staticData.batteryRatingVoltage = getNextFloat(commandAnswer, index);       // JJ.J
    get.staticData.batteryReChargeVoltage = getNextFloat(commandAnswer, index);     // KK.K
    get.staticData.batteryUnderVoltage = getNextFloat(commandAnswer, index);        // JJ.J
    get.staticData.batteryBulkVoltage = getNextFloat(commandAnswer, index);         // KK.K
    get.staticData.batteryFloatVoltage = getNextFloat(commandAnswer, index);        // LL.L

    switch ((byte)getNextLong(commandAnswer, index)) // O
    {
    case 0:
      get.staticData.batterytype = "AGM";
      break;
    case 1:
      get.staticData.batterytype = "Flooded";
      break;
    case 2:
      get.staticData.batterytype = "User";
      break;
    }
    get.staticData.currentMaxAcChargingCurrent = getNextLong(commandAnswer, index); // PP
    get.staticData.currentMaxChargingCurrent = getNextLong(commandAnswer, index);   // QQ0

    switch ((byte)getNextLong(commandAnswer, index)) // o
    {
    case 0:
      get.staticData.inputVoltageRange = "Appliance";
      break;
    case 1:
      get.staticData.inputVoltageRange = "UPS";
      break;
    }
    switch ((byte)getNextLong(commandAnswer, index)) // P
    {
    case 0:
      get.staticData.outputSourcePriority = "Utility first";
      break;
    case 1:
      get.staticData.outputSourcePriority = "Solar first";
      break;
    case 2:
      get.staticData.outputSourcePriority = "SBU first";
      break;
    }
    switch ((byte)getNextLong(commandAnswer, index)) // Q
    {
    case 0:
      get.staticData.chargerSourcePriority = "Utility first";
      break;
    case 1:
      get.staticData.chargerSourcePriority = "Solar first";
      break;
    case 2:
      get.staticData.chargerSourcePriority = "Solar + Utility";
      break;
    case 3:
      get.staticData.chargerSourcePriority = "Only solar charging permitted";
      break;
    }
    get.staticData.parallelMaxNumber = getNextLong(commandAnswer, index); // R
    switch ((byte)getNextLong(commandAnswer, index))                      // SS
    {
    case 00:
      get.staticData.machineType = "Grid tie";
      break;
    case 01:
      get.staticData.machineType = "Off Grid";
      break;
    case 10:
      get.staticData.machineType = "Hybrid";
      break;
    }
    switch ((byte)getNextLong(commandAnswer, index)) // T
    {
    case 0:
      get.staticData.topolgy = "transformerless";
      break;
    case 1:
      get.staticData.topolgy = "Otransformer";
      break;
    }
  }
}
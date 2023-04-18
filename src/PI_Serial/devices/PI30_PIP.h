// QPIGS: BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EE.E UUU.U WW.WW PPPPP 00000000 QQ VV MMMMM 000
// QPIRI: BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O PP QQ0 O P Q R SS T U VV.V W X

void PI_Serial::PI30_PIP_QPIGS()
{
  String commandAnswer = this->requestData("QPIGS");
  // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
  if (commandAnswer != "NAK" && commandAnswer.length() == 106) // make sure
  {
    String tmpBit;
    int index = 0;
    int i = 0;
    get.variableData.gridVoltage = getNextFloat(commandAnswer, index);                // BBB.B
    get.variableData.gridFrequency = getNextFloat(commandAnswer, index);              // CC.C
    get.variableData.acOutputVoltage = getNextFloat(commandAnswer, index);            // DDD.D
    get.variableData.acOutputFrequency = getNextFloat(commandAnswer, index);          // EE.E
    get.variableData.acOutputApparentPower = getNextFloat(commandAnswer, index);      // FFFF
    get.variableData.acOutputActivePower = getNextLong(commandAnswer, index);         // GGGG
    get.variableData.outputLoadPercent = getNextLong(commandAnswer, index);           // HHH
    get.variableData.busVoltage = getNextLong(commandAnswer, index);                  // III
    get.variableData.batteryVoltage = getNextFloat(commandAnswer, index);             // JJ.JJ
    get.variableData.batteryChargingCurrent = getNextLong(commandAnswer, index);      // KKK
    get.variableData.batteryCapacity = getNextLong(commandAnswer, index);             // OOO
    get.variableData.inverterHeatSinkTemperature = getNextLong(commandAnswer, index); // TTTT
    get.variableData.pvInputCurrent[0] = getNextFloat(commandAnswer, index);   // EE.E
    get.variableData.pvInputVoltage[0] = getNextFloat(commandAnswer, index);            // UUU.U
    get.variableData.batteryVoltageFromScc = getNextFloat(commandAnswer, index);      // WW.WW
    get.variableData.batteryDischargeCurrent = getNextLong(commandAnswer, index);     // PPPP

    get.deviceStatus.pvOrAcFeedTheLoad = getNextBit(commandAnswer, index);        // b7
    get.deviceStatus.configurationStatus = getNextBit(commandAnswer, index);      // b6
    get.deviceStatus.sccFirmwareVersionChange = getNextBit(commandAnswer, index); // b5
    get.deviceStatus.loadStatus = getNextBit(commandAnswer, index);               // b4
    get.deviceStatus.chargingStatus = getNextBit(commandAnswer, index);           // b3
    get.deviceStatus.sccChargingStatus = getNextBit(commandAnswer, index);        // b2
    get.deviceStatus.acChargingStatus = getNextBit(commandAnswer, index);         // b1

    get.variableData.batteryVoltageOffsetForFansOn = getNextLong(commandAnswer, index); // QQ
    get.variableData.eepromVersion = getNextLong(commandAnswer, index);                 // VV
    get.variableData.pvChargingPower = getNextLong(commandAnswer, index);               // MMMMM

    get.deviceStatus.chargingToFloatingMode = getNextBit(commandAnswer, index); // b10
    get.deviceStatus.switchOn = getNextBit(commandAnswer, index);               // b9
    get.deviceStatus.dustproofInstalled = getNextBit(commandAnswer, index);     // b8

    get.variableData.batteryLoad = (get.variableData.batteryChargingCurrent - get.variableData.batteryDischargeCurrent);
  }
}
void PI_Serial::PI30_PIP_QMOD()
{
  String commandAnswer = this->requestData("QMOD");
  // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
  if (commandAnswer != "NAK" && commandAnswer.length() == 1)
  {
    get.variableData.operationMode = getModeDesc((char)commandAnswer.charAt(0));
  }
}
void PI_Serial::PI30_PIP_QPIRI()
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
// QPIGS: BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EE.E UUU.U WW.WW PPPPP 00000000 QQ VV MMMMM 000
// QPIRI: BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O PP QQ0 O P Q R SS T U VV.V W X

void PI_Serial::PI30_QPIGS()
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
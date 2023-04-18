//QPIGS: 239.4 49.9 230.0 50.0 0230 0187 004 374 53.00 000 068 0027 0002 088.7 52.96 00001 00110110 00 00 00121 010                             //loosi
//QPIGS: 000.0 00.0 229.5 50.0 0137 0062 004 365 26.55 007 100 0014 0007 075.0 26.57 00000 00110110 00 00 00184 010                             //crash seiner

//QPIGS: BBB.B CC.CC DD.DD EE.EE FF.FF GGGG ±HHH II.II ±JJJ KKKK 00000000                                                                       PI34 / MPPT-3000
//QPIGS: BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EEEE UUU.U WW.WW PPPPP 00000000                                             PI30 HS MS MSX
//QPIGS: BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EEEE UUU.U WW.WW PPPPP 00000000                                             PI30 Revo
//QPIGS: BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EEEE UUU.U WW.WW PPPPP 00000000 QQ VV MMMMM 000                             PI41 / LV5048
//QPIGS: BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EE.E UUU.U WW.WW PPPPP 00000000 QQ VV MMMMM 000                             PI30 PIP
//QPIGS: BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EE.E UUU.U WW.WW PPPPP 00000000 QQ VV MMMMM 000 Y ZZ AAAA                   PI30 Max

//QPIGS: MMM.M CBBBBB HH.H CZZZ.Z LLL.L MMMMM NN.N QQQ.Q DDD KKK.K VVV.V SSS.S RRR.R XXX PPPPP EEEEE OOOOO UUU.U WWW.W YYY.Y TTT.T 00000000     PI00
//QPIGS: MMM.M CBBBBB HH.H CZZZ.Z LLL.L MMMMM NN.N QQQ.Q DDD KKK.K VVV.V SSS.S RRR.R XXX PPPPP EEEEE OOOOO UUU.U WWW.W YYY.Y TTT.T 00000000     PI30 Infinisolar
//QPIGS: AAA.A BBBBBB CC.C DDDD.D EEE.E FFFFF GG.G HHH.H III JJJ.J KKK.K LLL.L MMM.M NNN OOOOO PPPPP QQQQQ RRR.R SSS.S TTT.T UUU.U V WWWWWWWWW  PI16

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
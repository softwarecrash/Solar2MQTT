//QPIGS: BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EEEE UUU.U WW.WW PPPPP 00000000
//QPIRI: BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O PP QQ0 O P Q R SS T U VV.V W X

//QPIRI: 230.0 21.7 230.0 50.0 21.7 5000 5000 48.0 48.0 46.0 56.4 56.4 2 02 060 0 2 1 9 01 0 0 52.0 0 1 300                                     loosi (PI41 / LV5048)
//QPIRI: 230.0 13.0 230.0 50.0 13.0 3000 3000 24.0 22.0 21.9 29.2 29.2 2 02 040 1 1 3 1 01 0 0 25.0 0 1                                         crash seiner

//QPIRI: AAA.A BB.B CCC.C DDD.D EEE.E FF.F GGG.G H II J        not correct letters equal to other documents                                     PI16
//QPIRI: BBB.B FF.F III.I EEE.E DDD.D AA.A GGG.G R MM T                                                                                         PI00
//QPIRI: BBB.B FF.F III.I EEE.E DDD.D AA.A GGG.G R MM T                                                                                         PI30 Infinisolar

//QPIRI: BBBB CC DD.D EE.EE FF.FF GG HH II.I JJ KKKK L MM.MM N                                                                                  PI34 / MPPT-3000
//QPIRI: BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O P0 QQ0 O P Q R SS T                                                    PI30 Revo
//QPIRI: BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O PP QQ0 O P Q R SS T U VV.V W X                                         PI30 HS MS MSX
//QPIRI: BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O PP QQ0 O P Q R SS T U VV.V W X                                         PI30 PIP
//QPIRI: BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O PP QQ0 O P Q R SS T U VV.V W X YYY                                     PI41 / LV5048
//QPIRI: BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O PP QQ0 O P Q R SS T U VV.V W X YYY Z CCC                               PI30 Max


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




//                                                                                                                                              all-solutions ->def. pip / prod. hs ms msx
//                                                                                                                                              dbl]rock -> pi30revo
/*
not sorted, need work
*/
void PI_Serial::PI30_HS_MS_MSX_QPIGS()
{
    String commandAnswer = this->requestData("QPIGS");
    if (commandAnswer != "NAK" && commandAnswer.length() == 90) // make sure
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
void PI_Serial::PI30_HS_MS_MSX_QMOD()
{
    String commandAnswer = this->requestData("QMOD");
    if (commandAnswer != "NAK" && commandAnswer.length() == 1)
    {
        get.variableData.operationMode = getModeDesc((char)commandAnswer.charAt(0));
    }
}
void PI_Serial::PI30_HS_MS_MSX_QPIRI()
{
    String commandAnswer = this->requestData("QPIRI");
    if (commandAnswer != "NAK" && commandAnswer.length() == 94)
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
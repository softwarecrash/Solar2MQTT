// QPIRI: 230.0 21.7 230.0 50.0 21.7 5000 5000 48.0 48.0 46.0 56.4 56.4 2 02 060 0 2 1 9 01 0 0 52.0 0 1 300                                     loosi (PI41 / LV5048)
// QPIRI: 230.0 13.0 230.0 50.0 13.0 3000 3000 24.0 22.0 21.9 29.2 29.2 2 02 040 1 1 3 1 01 0 0 25.0 0 1                                         crash seiner

// QPIRI: AAA.A BB.B CCC.C DDD.D EEE.E FF.F GGG.G H II J        not correct letters equal to other documents                                     PI16
// QPIRI: BBB.B FF.F III.I EEE.E DDD.D AA.A GGG.G R MM T                                                                                         PI00
// QPIRI: BBB.B FF.F III.I EEE.E DDD.D AA.A GGG.G R MM T                                                                                         PI30 Infinisolar

// QPIRI: BBBB CC DD.D EE.EE FF.FF GG HH II.I JJ KKKK L MM.MM N                                                                                  PI34 / MPPT-3000
// QPIRI: BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O P0 QQ0 O P Q R SS T                                                    PI30 Revo
// QPIRI: BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O PP QQ0 O P Q R SS T U VV.V W X                                         PI30 HS MS MSX
// QPIRI: BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O PP QQ0 O P Q R SS T U VV.V W X                                         PI30 PIP
// QPIRI: BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O PP QQ0 O P Q R SS T U VV.V W X YYY                                     PI41 / LV5048
// QPIRI: BBB.B CC.C DDD.D EE.E FF.F HHHH IIII JJ.J KK.K JJ.J KK.K LL.L O PP QQ0 O P Q R SS T U VV.V W X YYY Z CCC                               PI30 Max

void PI_Serial::PIXX_QPIRI()
{
  String commandAnswer = this->requestData("QPIRI");
  // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
  if (commandAnswer == "NAK")
  {
    qAvaible.qpiri = false; // if recived NAK, set the command avaible to false and never aks again until reboot
  }
  else if (commandAnswer.length() == 83 || // Revo
           commandAnswer.length() == 94 || // PIP MSX
           commandAnswer.length() == 98 || // LV5048
           commandAnswer.length() == 104   // PI30 MAX
  )
  {
    get.raw.qpiri = commandAnswer;
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
      get.staticData.batterytype = (char *)"AGM";
      break;
    case 1:
      get.staticData.batterytype = (char *)"Flooded";
      break;
    case 2:
      get.staticData.batterytype = (char *)"User";
      break;
    }
    get.staticData.currentMaxAcChargingCurrent = getNextLong(commandAnswer, index); // PP
    get.staticData.currentMaxChargingCurrent = getNextLong(commandAnswer, index);   // QQ0

    switch ((byte)getNextLong(commandAnswer, index)) // o
    {
    case 0:
      get.staticData.inputVoltageRange = (char *)"Appliance";
      break;
    case 1:
      get.staticData.inputVoltageRange = (char *)"UPS";
      break;
    }
    switch ((byte)getNextLong(commandAnswer, index)) // P
    {
    case 0:
      get.staticData.outputSourcePriority = (char *)"Utility first";
      break;
    case 1:
      get.staticData.outputSourcePriority = (char *)"Solar first";
      break;
    case 2:
      get.staticData.outputSourcePriority = (char *)"SBU first";
      break;
    }
    switch ((byte)getNextLong(commandAnswer, index)) // Q
    {
    case 0:
      get.staticData.chargerSourcePriority = (char *)"Utility first";
      break;
    case 1:
      get.staticData.chargerSourcePriority = (char *)"Solar first";
      break;
    case 2:
      get.staticData.chargerSourcePriority = (char *)"Solar + Utility";
      break;
    case 3:
      get.staticData.chargerSourcePriority = (char *)"Only solar charging permitted";
      break;
    }
    get.staticData.parallelMaxNumber = getNextLong(commandAnswer, index); // R
    switch ((byte)getNextLong(commandAnswer, index))                      // SS
    {
    case 00:
      get.staticData.machineType = (char *)"Grid tie";
      break;
    case 01:
      get.staticData.machineType = (char *)"Off Grid";
      break;
    case 10:
      get.staticData.machineType = (char *)"Hybrid";
      break;
    }
    switch ((byte)getNextLong(commandAnswer, index)) // T
    {
    case 0:
      get.staticData.topolgy = (char *)"transformerless";
      break;
    case 1:
      get.staticData.topolgy = (char *)"Otransformer";
      break;
    }
  }
}
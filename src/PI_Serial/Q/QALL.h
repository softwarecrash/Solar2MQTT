
// QALL: BBB CC.C DDD EE.E FFFF GGG HH.H III JJJ KKK LLL MM.M NNNN OOOOOO PPPPPP Q KK SS - PI30 Revo
void PI_Serial::PIXX_QALL()
{
  String commandAnswer = this->requestData("QALL");
  // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
  if (commandAnswer == "NAK")
  {
    qAvaible.qall = false; // if recived NAK, set the command avaible to false and never aks again until reboot
  }
  else if (commandAnswer.length() == 79 // Revo Qall
  )
  {
    get.raw.qall = commandAnswer;
    int index = 0;
    get.variableData.gridVoltage = getNextFloat(commandAnswer, index);              // BBB.B
    get.variableData.gridFrequency = getNextFloat(commandAnswer, index);            // CC.C
    get.variableData.acOutputVoltage = getNextFloat(commandAnswer, index);          // DDD.D
    get.variableData.acOutputFrequency = getNextFloat(commandAnswer, index);        // EE.E
    get.variableData.acOutputActivePower = getNextLong(commandAnswer, index);       // FFFF
    get.variableData.outputLoadPercent = getNextLong(commandAnswer, index);         // GGG
    get.variableData.batteryVoltage = getNextFloat(commandAnswer, index);           // HH.H
    get.variableData.batteryCapacity = getNextLong(commandAnswer, index);           // III
    get.variableData.batteryChargingCurrent = getNextLong(commandAnswer, index);    // JJJ
    get.variableData.batteryDischargeCurrent = getNextLong(commandAnswer, index);   // KKK
    get.variableData.pvInputVoltage[0] = getNextFloat(commandAnswer, index);        // LLL
    get.variableData.pvInputCurrent[0] = getNextFloat(commandAnswer, index);        // MM.M
    get.variableData.pvChargingPower = getNextLong(commandAnswer, index);           // NNNN
    get.variableData.pvGenerationDay = getNextLong(commandAnswer, index);           // OOOOOO
    get.variableData.pvGenerationSum = getNextLong(commandAnswer, index);           // PPPPPP
    get.variableData.operationMode = getModeDesc(getNextBit(commandAnswer, index)); // Q
    get.variableData.batteryLoad = (get.variableData.batteryChargingCurrent - get.variableData.batteryDischargeCurrent);
  }
}
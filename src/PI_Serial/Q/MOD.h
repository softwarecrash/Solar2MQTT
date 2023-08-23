bool PI_Serial::PIXX_QMOD()
{
  String commandAnswer = this->requestData("QMOD");
  if (commandAnswer.length() == 1)
  {
    rawData["QMOD"] = commandAnswer;
    liveData["Inverter_Operation_Mode"] = getModeDesc((char)commandAnswer.charAt(0));
  }
  return true;
}
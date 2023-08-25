bool PI_Serial::PIXX_QMOD()
{
  String commandAnswer = this->requestData("QMOD");
    if (commandAnswer == "NAK")
  {
    return true;
  }
  if (commandAnswer.length() == 1)
  {
    get.raw.qmod = commandAnswer;
    //rawData["QMOD"] = commandAnswer;
    liveData["Inverter_Operation_Mode"] = getModeDesc((char)commandAnswer.charAt(0));
  }
  return true;
}
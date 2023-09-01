bool PI_Serial::PIXX_QMOD()
{
  String commandAnswer = this->requestData("QMOD");
  get.raw.qmod = commandAnswer;
    if (commandAnswer == "NAK")
  {
    return true;
  }
    if(commandAnswer == "ERCRC")
  {
    return false;
  }
  if (commandAnswer.length() == 1)
  {
    liveData["Inverter_Operation_Mode"] = getModeDesc((char)commandAnswer.charAt(0));
  }
  return true;
}
bool PI_Serial::PIXX_QMOD()
{
  if (protocol == PI30)
  {
    String commandAnswer = this->requestData("QMOD");
    get.raw.qmod = commandAnswer;
    if (commandAnswer == "NAK")
    {
      return true;
    }
    if (commandAnswer == "ERCRC")
    {
      return false;
    }
    if (commandAnswer.length() == 1)
    {
      liveData["Inverter_Operation_Mode"] = getModeDesc((char)commandAnswer.charAt(0));
    }
    return true;
  }
  else if (protocol == PI18)
  {
    String commandAnswer = this->requestData("^P006MOD");
    get.raw.qmod = commandAnswer;
    if (commandAnswer == "NAK")
    {
      return true;
    }
    if (commandAnswer == "ERCRC")
    {
      return false;
    }
    if (commandAnswer.length() == 1)
    {
      // liveData["Inverter_Operation_Mode"] = getModeDesc((char)commandAnswer.charAt(0));
    }
    return true;
  }
  else if (protocol == NoD)
  {
    return false;
  }
  else
  {
    return false;
  }
}
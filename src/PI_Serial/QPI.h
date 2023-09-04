bool PI_Serial::PIXX_QPI()
{
  if (protocol == PI30)
  {
    String commandAnswer = this->requestData("QPI");
    get.raw.qpi = commandAnswer;
    if (commandAnswer == "NAK")
    {
      return true;
    }
    if (commandAnswer == "ERCRC")
    {
      return false;
    }
    staticData["Protocol_ID"] = commandAnswer;
    return true;
  }
  else if (protocol == PI18)
  {
    String commandAnswer = this->requestData("^P005PI");
    get.raw.qpi = commandAnswer;
    if (commandAnswer == "NAK")
    {
      return true;
    }
    if (commandAnswer == "ERCRC")
    {
      return false;
    }
    staticData["Protocol_ID"] = commandAnswer;
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
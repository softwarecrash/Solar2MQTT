bool PI_Serial::PIXX_QPI()
{
  String commandAnswer = this->requestData("QPI");
  if (commandAnswer == "NAK")
  {
    return true;
  }
    if(commandAnswer == "ERCRC")
  {
    return false;
  }
    get.raw.qpi = commandAnswer;
    staticData["Protocol_ID"] = commandAnswer;
    return true;
}
bool PI_Serial::PIXX_QPI()
{
  String commandAnswer = this->requestData("QPI");
  get.raw.qpi = commandAnswer;
  if (commandAnswer == "NAK")
  {
    return true;
  }
    if(commandAnswer == "ERCRC")
  {
    return false;
  }
    staticData["Protocol_ID"] = commandAnswer;
    return true;
}
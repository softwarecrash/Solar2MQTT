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
  qAvaible.qpi = true;
    get.raw.qpi = commandAnswer;
    //get.staticData.deviceProtocol = commandAnswer;
    staticData["Protocol_ID"] = commandAnswer;
    return true;
}
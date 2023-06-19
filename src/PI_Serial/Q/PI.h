bool PI_Serial::PIXX_QPI()
{
  String commandAnswer = this->requestData("QPI");
  if (commandAnswer == "NAK")
  {
    qAvaible.qpi = false; // if recived NAK, set the command avaible to false and never aks again until reboot
    return true;
  }
  qAvaible.qpi = true;
    get.raw.qpi = commandAnswer;
    get.staticData.deviceProtocol = commandAnswer;
    return true;
}
bool PI_Serial::PIXX_QPI()
{
  String commandAnswer = this->requestData("QPI");
  if (commandAnswer == "NAK")
  {
    qAvaible.qpi = false; // if recived NAK, set the command avaible to false and never aks again until reboot
    return true;
  }
    get.raw.qpi = commandAnswer;
    return true;
}
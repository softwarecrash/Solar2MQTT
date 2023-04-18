void PI_Serial::PIXX_QMOD()
{
  String commandAnswer = this->requestData("QMOD");
  // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
  if(commandAnswer == "NAK")
  {
    qAvaible.qmod = false; //iv recived NAK, set the command avaible to false and never aks again until reboot
  } else
  if (commandAnswer.length() == 1)
  {
    get.variableData.operationMode = getModeDesc((char)commandAnswer.charAt(0));
  }
}
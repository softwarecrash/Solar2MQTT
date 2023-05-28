bool PI_Serial::PIXX_QMOD()
{
  String commandAnswer = this->requestData("QMOD");
  // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
  if(commandAnswer == "NAK")
  {
    qAvaible.qmod = false; //if recived NAK, set the command avaible to false and never aks again until reboot
    return false;
  } else
  if (commandAnswer.length() == 1)
  {
    get.raw.qmod = commandAnswer;
    get.variableData.operationMode = getModeDesc((char)commandAnswer.charAt(0));
    return true;
  } else
  {
    return false;
  }
}
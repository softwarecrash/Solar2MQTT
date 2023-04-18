void PI_Serial::PI30_QMOD()
{
  String commandAnswer = this->requestData("QMOD");
  // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
  if (commandAnswer != "NAK" && commandAnswer.length() == 1)
  {
    get.variableData.operationMode = getModeDesc((char)commandAnswer.charAt(0));
  }
}
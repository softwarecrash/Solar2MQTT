bool PI_Serial::PIXX_QMN()
{
  String commandAnswer = this->requestData("QMN");
  // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
  if(commandAnswer == "NAK")
  {
    return true;
  }
    if(commandAnswer = "ERCRC")
  {
    return false;
  }
  if (commandAnswer.length() > 3 && commandAnswer.length() < 50)
  {
    get.raw.qmn = commandAnswer;
    //get.staticData.modelName = commandAnswer;
    staticData["Device_Model"] = commandAnswer;
    return true;
  }
  return true;
}
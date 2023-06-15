bool PI_Serial::PIXX_QMN()
{
  String commandAnswer = this->requestData("QMN");
  // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
  if(commandAnswer == "NAK")
  {
    qAvaible.qmn = false; //if recived NAK, set the command avaible to false and never aks again until reboot
    return true;
  } else
  {
    get.raw.qmn = commandAnswer;
    get.staticData.modelName = commandAnswer.c_str();
    return true;
  }
}
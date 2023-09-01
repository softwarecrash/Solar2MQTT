bool PI_Serial::PIXX_QET()
{
  String commandAnswer = this->requestData("QET");
  if (commandAnswer == "NAK")
  {
    return true;
  }
    if(commandAnswer == "ERCRC")
  {
    return false;
  }
    get.raw.qet = commandAnswer;
    liveData["PV_total_Generated_energy"] = commandAnswer;
    return true;
}
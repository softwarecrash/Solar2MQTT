bool PI_Serial::PIXX_QMOD()
{
  if (protocol == PI30)
  {
    String commandAnswer = this->requestData("QMOD");
    get.raw.qmod = commandAnswer;
    if (commandAnswer == "NAK")
    {
      return true;
    }
    if (commandAnswer == "ERCRC")
    {
      return false;
    }
    if (commandAnswer.length() == 1)
    {
      liveData["Inverter_Operation_Mode"] = getModeDesc((char)commandAnswer.charAt(0));
    }
    return true;
  }
  else if (protocol == PI18)
  {
    String commandAnswer = this->requestData("^P006MOD");
    get.raw.qmod = commandAnswer;
    if (commandAnswer == "NAK")
    {
      return true;
    }
    if (commandAnswer == "ERCRC")
    {
      return false;
    }
    if (commandAnswer.length() == 2)
    {
      switch (commandAnswer.toInt())
      {
      case 0:
        liveData["Inverter_Operation_Mode"] = "Power on";
        break;
      case 1:
        liveData["Inverter_Operation_Mode"] = "Standby";
        break;
      case 2:
        liveData["Inverter_Operation_Mode"] = "BYpass";
        break;
      case 3:
        liveData["Inverter_Operation_Mode"] = "Battery";
        break;
      case 4:
        liveData["Inverter_Operation_Mode"] = "Fault";
        break;
      case 5:
        liveData["Inverter_Operation_Mode"] = "Hybrid";
        break;
      default:
        liveData["Inverter_Operation_Mode"] = "No data";
        break;
      }

    }
    return true;
  }
  else if (protocol == NoD)
  {
    return false;
  }
  else
  {
    return false;
  }
}
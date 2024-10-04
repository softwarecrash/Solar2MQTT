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
      liveData[DESCR_LIVE_INVERTER_OPERATION_MODE] = getModeDesc((char)commandAnswer.charAt(0));
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
        liveData[DESCR_LIVE_INVERTER_OPERATION_MODE] = "Power on";
        break;
      case 1:
        liveData[DESCR_LIVE_INVERTER_OPERATION_MODE] = "Standby";
        break;
      case 2:
        liveData[DESCR_LIVE_INVERTER_OPERATION_MODE] = "Bypass";
        break;
      case 3:
        liveData[DESCR_LIVE_INVERTER_OPERATION_MODE] = "Battery";
        break;
      case 4:
        liveData[DESCR_LIVE_INVERTER_OPERATION_MODE] = "Fault";
        break;
      case 5:
        liveData[DESCR_LIVE_INVERTER_OPERATION_MODE] = "Hybrid";
        break;
      default:
        liveData[DESCR_LIVE_INVERTER_OPERATION_MODE] = "No data";
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
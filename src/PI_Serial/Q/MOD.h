bool PI_Serial::PIXX_QMOD()
{
  String commandAnswer = this->requestData("QMOD");
  if (commandAnswer.length() == 1)
  {
    rawData["QMOD"] = commandAnswer;

    char *modeString;
    switch ((char)commandAnswer.charAt(0))
    {
    default:
      modeString = (char *)("Undefined, Origin: " + (char)commandAnswer.charAt(0));
      break;
    case 'P':
      modeString = (char *)"Power On";
      break;
    case 'S':
      modeString = (char *)"Standby";
      break;
    case 'Y':
      modeString = (char *)"Bypass";
      break;
    case 'L':
      modeString = (char *)"Line";
      break;
    case 'B':
      modeString = (char *)"Battery";
      break;
    case 'T':
      modeString = (char *)"Battery Test";
      break;
    case 'F':
      modeString = (char *)"Fault";
      break;
    case 'D':
      modeString = (char *)"Shutdown";
      break;
    case 'G':
      modeString = (char *)"Grid";
      break;
    case 'C':
      modeString = (char *)"Charge";
      break;
    }
    liveData["Inverter_Operation_Mode"] = modeString;
  }
  return true;
}
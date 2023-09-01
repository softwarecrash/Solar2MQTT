static const char *const qpigs2List[] = {
    // [PI34 / MPPT-3000], [PI30 HS MS MSX], [PI30 Revo], [PI30 PIP], [PI41 / LV5048]
    "PV2_Input_Current",  // BBB.B
    "PV2_Input_Voltage",  // CCC.C
    "PV2_Charging_Power", // DDDD
};

bool PI_Serial::PIXX_QPIGS2()
{
  String commandAnswer = this->requestData("QPIGS2");
  get.raw.qpigs2 = commandAnswer;
  byte commandAnswerLength = commandAnswer.length();
  String strs[30]; // buffer for string splitting
  if (commandAnswer == "NAK")
  {
    return true;
  }
  if (commandAnswer == "ERCRC")
  {
    return false;
  }
  
  // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
  if (commandAnswerLength >= 10 && commandAnswerLength <= 20)
  {

    // Split the string into substrings
    int StringCount = 0;
    while (commandAnswer.length() > 0)
    {
      int index = commandAnswer.indexOf(' ');
      if (index == -1) // No space found
      {
        strs[StringCount++] = commandAnswer;
        break;
      }
      else
      {
        strs[StringCount++] = commandAnswer.substring(0, index);
        commandAnswer = commandAnswer.substring(index + 1);
      }
    }

    for (unsigned int i = 0; i < sizeof qpigs2List / sizeof qpigs2List[0]; i++)
    {
      if (!strs[i].isEmpty() && strcmp(qpigs2List[i], "") != 0)
        liveData[qpigs2List[i]] = (int)(strs[i].toFloat() * 100 + 0.5) / 100.0;
    }
    // make some things pretty
    liveData["PV2_Input_Power"] = (liveData["PV2_Input_Voltage"].as<unsigned short>() * liveData["PV2_Input_Current"].as<unsigned short>());
  }
  return true;
}
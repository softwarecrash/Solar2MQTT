
// QALL: BBB CC.C DDD EE.E FFFF GGG HH.H III JJJ KKK LLL MM.M NNNN OOOOOO PPPPPP Q KK SS - PI30 Revo
static const char *const qallList[] =
    // [PI30 Revo]
    {
        "AC_in_Voltage",             // BBB.B
        "AC_in_Frequenz",            // CC.C
        "AC_out_Voltage",            // DDD.D
        "AC_out_Frequenz",           // EE.E
        "AC_out_VA",                 // FFFF
        "AC_out_percent",            // GGG
        "Battery_Voltage",           // HH.H
        "Battery_Percent",           // III
        "Battery_Charge_Current",    // JJJ
        "Battery_Discharge_Current", // KKK
        "PV_Input_Voltage",          // LLL
        "PV_Input_Current",          // MM.M
        "PV_Charging_Power",         // NNNN
        "PV_generation_day",         // OOOOOO
        "PV_generation_sum",         // PPPPPP
        "Inverter_Operation_Mode",   // Q
        "Warning_Code",              // KK
        "Fault_code",                // SS
};
bool PI_Serial::PIXX_QALL()
{
  String commandAnswer = this->requestData("QALL");
  get.raw.qall = commandAnswer;
  //byte commandAnswerLength = commandAnswer.length();
  // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
  if (commandAnswer == "NAK")
  {
    return true;
  }
    if(commandAnswer == "ERCRC")
  {
    return false;
  }

  String strs[30];
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

  for (unsigned int i = 0; i < sizeof qpigsList / sizeof qpigsList[0]; i++)
  {
    if (!strs[i].isEmpty() && strcmp(qallList[i], "") != 0)
      liveData[qallList[i]] = (int)(strs[i].toFloat() * 100 + 0.5) / 100.0;
  }
  liveData["Inverter_Operation_Mode"] = getModeDesc((char)liveData["Inverter_Operation_Mode"].as<String>().charAt(0));
  liveData["Battery_Load"] = (liveData["Battery_Charge_Current"].as<unsigned short>() - liveData["Battery_Discharge_Current"].as<unsigned short>());

  return true;
}

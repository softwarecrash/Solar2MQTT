static const char *const qpigsList[][24] = {
    // [PI34 / MPPT-3000], [PI30 HS MS MSX], [PI30 Revo], [PI30 PIP], [PI41 / LV5048]
    {
        "AC_in_Voltage",                  // BBB.B
        "AC_in_Frequenz",                 // CC.C
        "AC_out_Voltage",                 // DDD.D
        "AC_out_Frequenz",                // EE.E
        "AC_out_VA",                      // FFFF
        "AC_out_Watt",                    // GGGG
        "AC_out_percent",                 // HHH
        "Inverter_Bus_Voltage",           // III
        "Battery_Voltage",                // JJ.JJ
        "Battery_Charge_Current",         // KKK
        "Battery_Percent",                // OOO
        "Inverter_Bus_Temperature",       // TTTT
        "PV_Input_Current",               // EE.E
        "PV_Input_Voltage",               // UUU.U
        "Battery_SCC_Volt",               // WW.WW
        "Battery_Discharge_Current",      // PPPP
        "Status_Flag",                    // b0-b7
        "Battery_voltage_offset_fans_on", // QQ
        "EEPROM_Version",                 // VV
        "PV_Charging_Power",              // MMMM
        "Device_Status",                  // b8-b10
        "Solar_feed_to_Grid_status",      // Y
        "Country",                        // ZZ
        "Solar_feed_to_grid_power",       // AAAA
    },
    // [PI16]
    {
        "Grid_voltage",             // AAA.A
        "Output power",             // BBBBBB
        "Grid frequency",           // CC.C
        "Output current",           // DDDD.D
        "C output voltage R",       // EEE.E
        "AC output power R",        // FFFFF
        "AC output frequency",      // GG.G
        "AC output current R",      // HHH.H
        "Output load percent",      // III
        "PBUS voltage",             // JJJ.J
        "SBUS voltage",             // KKK.K
        "Positive battery voltage", // LLL.L
        "Negative battery voltage", // MMM.M
        "Battery capacity",         // NNN
        "PV1 input power",          // OOOOO
        "PV2 input power",          // PPPPP
        "PV3 input power",          // QQQQQ
        "PV1 input voltage",        // RRR.R
        "PV2 input voltage",        // SSS.S
        "PV3 input voltage",        // TTT.T
        "Max temperature",          // UUU.U
    },
};
static const char *const qallList[] = {
    // [PI30 Revo]
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

static const char *const P005GS[][28] = {
    {"AC_in_Voltage", "10"},            // AAAA
    {"AC_in_Frequenz", "10"},           // BBB
    {"AC_out_Voltage", "10"},           // CCCC
    {"AC_out_Frequenz", "10"},          // DDD
    {"AC_out_VA", "0"},                 // EEEE
    {"AC_out_Watt", "0"},               // FFFF
    {"AC_out_percent", "0"},            // GGGG
    {"Battery_Voltage", "10"},          // HHHH
    {"Battery_SCC_Volt", "10"},         // III
    {"Battery_SCC2_Volt", "10"},        // JJJ
    {"Battery_Discharge_Current", "0"}, // KKK
    {"Battery_Charge_Current", "0"},    // LLL
    {"Battery_Percent", "0"},           // MMM
    {"Inverter_Bus_Temperature", "0"},  // NNN
    {"MPPT1_Charger_Temperature", "0"}, // OOO
    {"MPPT2_Charger_Temperature", "0"}, // PPP
    {"PV1_Input_Power", "0"},           // QQQQ
    {"PV2_Input_Power", "0"},           // RRRR
    {"PV1_Input_Voltage", "10"},        // SSSS
    {"PV2_Input_Voltage", "10"},        // TTTT
    {"Configuration_State", "0"},       // U
    {"MPPT1_Charger_Status", "0"},       // V
    {"MPPT2_CHarger_Status", "0"},      // W
    {"Load_Connection", "0"},           // X
    {"Battery_Power_Direction", "0"},   // Y
    {"ACDC_Power_Direction", "0"},      // Z
    {"Line_Power_Direction", "0"},      // a
    {"Local_Parallel_ID", "0"},         // b

};

bool PI_Serial::PIXX_QPIGS()
{
  if (protocol == PI30)
  {
    byte protocolNum = 0; // for future use
    String commandAnswerQALL = this->requestData("QALL");
    get.raw.qall = commandAnswerQALL;
    if (commandAnswerQALL == "ERCRC")
    {
      return false;
    }
    //
    String commandAnswerQPIGS = this->requestData("QPIGS");
    get.raw.qpigs = commandAnswerQPIGS;
    if (commandAnswerQPIGS == "NAK")
      return true;
    if (commandAnswerQPIGS == "ERCRC")
      return false;
    byte commandAnswerLength = commandAnswerQPIGS.length();

    // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
    if (commandAnswerLength >= 60 && commandAnswerLength <= 140)
    {
      if (commandAnswerLength <= 116)
      {
        protocolNum = 0;
      }
      else if (commandAnswerLength > 131)
      {
        protocolNum = 1;
      }

      // Split the string into substrings
      String strs[30]; // buffer for string splitting
      int StringCount = 0;
      while (commandAnswerQPIGS.length() > 0)
      {
        int index = commandAnswerQPIGS.indexOf(delimiter);
        if (index == -1) // No space found
        {
          strs[StringCount++] = commandAnswerQPIGS;
          break;
        }
        else
        {
          strs[StringCount++] = commandAnswerQPIGS.substring(0, index);
          commandAnswerQPIGS = commandAnswerQPIGS.substring(index + 1);
        }
      }

      for (unsigned int i = 0; i < sizeof qpigsList[protocolNum] / sizeof qpigsList[protocolNum][0]; i++)
      {
        if (!strs[i].isEmpty() && strcmp(qpigsList[protocolNum][i], "") != 0)
          liveData[qpigsList[protocolNum][i]] = (int)(strs[i].toFloat() * 100 + 0.5) / 100.0;
      }
      // make some things pretty
      liveData["Battery_Load"] = (liveData["Battery_Charge_Current"].as<unsigned short>() - liveData["Battery_Discharge_Current"].as<unsigned short>());
      liveData["PV_Input_Power"] = (liveData["PV_Input_Voltage"].as<unsigned short>() * liveData["PV_Input_Current"].as<unsigned short>());
    }

    if (get.raw.qall != "NAK" || get.raw.qall != "ERCRC")
    {
      String strsQALL[30];
      //  Split the string into substrings
      int StringCountQALL = 0;
      while (commandAnswerQALL.length() > 0)
      {
        int index = commandAnswerQALL.indexOf(delimiter);
        if (index == -1) // No space found
        {
          strsQALL[StringCountQALL++] = commandAnswerQALL;
          break;
        }
        else
        {
          strsQALL[StringCountQALL++] = commandAnswerQALL.substring(0, index);
          commandAnswerQALL = commandAnswerQALL.substring(index + 1);
        }
      }

      for (unsigned int i = 0; i < sizeof qallList / sizeof qallList[0]; i++)
      {
        if (!strsQALL[i].isEmpty() && strcmp(qallList[i], "") != 0)
          liveData[qallList[i]] = (int)(strsQALL[i].toFloat() * 100 + 0.5) / 100.0;
      }
      liveData["Inverter_Operation_Mode"] = getModeDesc((char)liveData["Inverter_Operation_Mode"].as<String>().charAt(0));
      liveData["Battery_Load"] = (liveData["Battery_Charge_Current"].as<unsigned short>() - liveData["Battery_Discharge_Current"].as<unsigned short>());
    }
    return true;
  }
  else if (protocol == PI18)
  {
    String commandAnswer = this->requestData("^P005GS");
    get.raw.qpigs = commandAnswer;
    if (commandAnswer == "NAK")
      return true;
    if (commandAnswer == "ERCRC")
      return false;
    byte commandAnswerLength = commandAnswer.length();

    // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
    if (commandAnswerLength >= 60 && commandAnswerLength <= 140)
    {
      // Split the string into substrings
      String strs[30]; // buffer for string splitting
      int StringCount = 0;
      while (commandAnswer.length() > 0)
      {
        int index = commandAnswer.indexOf(delimiter);
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

      for (unsigned int i = 0; i < sizeof P005GS[0] / sizeof P005GS[0][0]; i++)
      {
        if (!strs[i].isEmpty() && strcmp(P005GS[i][0], "") != 0)
        {

          if (atoi(P005GS[i][1]) > 0)
          {
            liveData[P005GS[i][0]] = (int)((strs[i].toFloat() / atoi(P005GS[i][1])) * 100 + 0.5) / 100.0;
          }
          else if (atoi(P005GS[i][1]) == 0)
          {
            liveData[P005GS[i][0]] = strs[i].toInt();
          }
          else
          {
            liveData[P005GS[i][0]] = strs[i];
          }
        }
      }
      // make some things pretty

      liveData["PV_Input_Voltage"] = (liveData["PV1_Input_Voltage"].as<unsigned short>() + liveData["PV2_Input_Voltage"].as<unsigned short>());
      liveData["PV_Charging_Power"] = (liveData["PV1_Input_Power"].as<unsigned short>() + liveData["PV2_Input_Power"].as<unsigned short>());

      liveData["PV_Input_Current"] = (liveData["PV_Charging_Power"].as<unsigned short>() / liveData["PV_Input_Voltage"].as<unsigned short>());

      liveData["Battery_Load"] = (liveData["Battery_Charge_Current"].as<unsigned short>() - liveData["Battery_Discharge_Current"].as<unsigned short>());
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
// QPIGS: 239.4 49.9 230.0 50.0 0230 0187 004 374 53.00 000 068 0027 0002 088.7 52.96 00001 00110110 00 00 00121 010                             //loosi
// QPIGS: 000.0 00.0 229.5 50.0 0137 0062 004 365 26.55 007 100 0014 0007 075.0 26.57 00000 00110110 00 00 00184 010                             //crash seiner

// GPIGS: 232.0 50.0 232.0 50.0 0263 0274 009 402 25.31 002 057 0688 0002 032.4 25.29 00000 10010110 00 04 00069 000                             //issues/27

// QPIGS: 231.1 49.9 000.0 00.0 0000 0000 000 402 26.76 000 096 075 00.1 120.0 00.00 00000 00000010 00 00 00016 000                              //samson 2400W Solar Hybrid Inverter 60A/80A 24V PV max 450V
// QPIGS: 230.6 49.9 000.0 00.0 0000 0000 000 421 26.70 000 100 0052 00.0 119.8 00.00 00000 00000110 00 00 00000 100                             //samson VM-3500W 80A/100A 24V PV max 500V

// QPIGS: BBB.B CC.CC DD.DD EE.EE FF.FF GGGG ±HHH II.II ±JJJ KKKK 00000000                                                                       PI34 / MPPT-3000
// QPIGS: BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EEEE UUU.U WW.WW PPPPP 00000000                                             PI30 HS MS MSX
// QPIGS: BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EEEE UUU.U WW.WW PPPPP 00000000                                             PI30 Revo
// QPIGS: BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EEEE UUU.U WW.WW PPPPP 00000000 QQ VV MMMMM 000                             PI41 / LV5048
// QPIGS: BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EE.E UUU.U WW.WW PPPPP 00000000 QQ VV MMMMM 000                             PI30 PIP
// QPIGS: BBB.B CC.C DDD.D EE.E FFFF GGGG HHH III JJ.JJ KKK OOO TTTT EE.E UUU.U WW.WW PPPPP 00000000 QQ VV MMMMM 000 Y ZZ AAAA                   PI30 Max

// QPIGS: AAA.A BBBBBB CC.C DDDD.D EEE.E FFFFF GG.G HHH.H III JJJ.J KKK.K LLL.L MMM.M NNN OOOOO PPPPP QQQQQ RRR.R SSS.S TTT.T UUU.U V WWWWWWWWW  PI16

// 000.0 00.0 229.9 50.0 0137 0082 004 403 29.23 001 100 0026 0004 081.5 29.17 00000 00110110 00 00 00131 110 neu ausgelesen von crash. watt werden nicht angezeigt

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

static const char *const P005GS[][8] = {
    {"AC_in_Voltage", "10"},   // AAAA
    {"AC_in_Frequenz", "10"},  // BBB
    {"AC_out_Voltage", "10"},  // CCCC
    {"AC_out_Frequenz", "10"}, // DDD
    {"AC_out_VA", "0"},        // EEEE
    {"AC_out_Watt", "0"},      // FFFF
    {"AC_out_percent", "0"},   // GGGG
    {"Battery_Voltage", "10"} //HHHH
/*
    "Inverter_Bus_Voltage",           // III
    
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
    */
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
      // liveData["Battery_Load"] = (liveData["Battery_Charge_Current"].as<unsigned short>() - liveData["Battery_Discharge_Current"].as<unsigned short>());
      // liveData["PV_Input_Power"] = (liveData["PV_Input_Voltage"].as<unsigned short>() * liveData["PV_Input_Current"].as<unsigned short>());
    }

    /*
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
        */
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
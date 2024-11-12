unsigned int qpigs_106_length = 24;
const char *const qpigs_106[] = {
    // 106 long have 24 fields
    // [PI34 / MPPT-3000], [PI30 HS MS MSX], [PI30 Revo], [PI30 PIP], [PI41 / LV5048]
    DESCR_AC_In_Voltage,                  // BBB.B
    DESCR_AC_In_Frequenz,                 // CC.C
    DESCR_AC_Out_Voltage,                 // DDD.D
    DESCR_AC_Out_Frequenz,                // EE.E
    DESCR_AC_Out_VA,                      // FFFF
    DESCR_AC_Out_Watt,                    // GGGG
    DESCR_AC_Out_Percent,                 // HHH
    DESCR_Inverter_Bus_Voltage,           // III
    DESCR_Battery_Voltage,                // JJ.JJ
    DESCR_Battery_Charge_Current,         // KKK
    DESCR_Battery_Percent,                // OOO
    DESCR_Inverter_Bus_Temperature,       // TTTT
    DESCR_PV_Input_Current,               // EE.E
    DESCR_PV_Input_Voltage,               // UUU.U
    DESCR_Battery_SCC_Volt,               // WW.WW
    DESCR_Battery_Discharge_Current,      // PPPP
    DESCR_Status_Flag,                    // b0-b7
    DESCR_Battery_Voltage_Offset_Fans_On, // QQ
    DESCR_EEPROM_Version,                 // VV
    DESCR_PV_Charging_Power,              // MMMM
    DESCR_Device_Status,                  // b8-b10
    DESCR_Solar_Feed_To_Grid_Status,      // Y
    DESCR_Country,                        // ZZ
    DESCR_Solar_Feed_To_Grid_Power,       // AAAA
};
unsigned int qpigs_90_length = 24;
const char *const qpigs_90[] = {
    // 90 long have 17 fields
    // [PI34 / MPPT-3000], [PI30 HS MS MSX], [PI30 Revo], [PI30 PIP], [PI41 / LV5048]
    DESCR_AC_In_Voltage,                  // BBB.B
    DESCR_AC_In_Frequenz,                 // CC.C
    DESCR_AC_Out_Voltage,                 // DDD.D
    DESCR_AC_Out_Frequenz,                // EE.E
    DESCR_AC_Out_VA,                      // FFFF
    DESCR_AC_Out_Watt,                    // GGGG
    DESCR_AC_Out_Percent,                 // HHH
    DESCR_Inverter_Bus_Voltage,           // III
    DESCR_Battery_Voltage,                // JJ.JJ
    DESCR_Battery_Charge_Current,         // KKK
    DESCR_Battery_Percent,                // OOO
    DESCR_Inverter_Bus_Temperature,       // TTTT
    DESCR_PV_Input_Current,               // EE.E
    DESCR_PV_Input_Voltage,               // UUU.U
    DESCR_Battery_SCC_Volt,               // WW.WW
    DESCR_Battery_Discharge_Current,      // PPPP
    DESCR_Status_Flag,                    // b0-b7
    DESCR_Battery_Voltage_Offset_Fans_On, // QQ
    DESCR_EEPROM_Version,                 // VV
    DESCR_PV_Charging_Power,              // MMMM
    DESCR_Device_Status,                  // b8-b10
    DESCR_Solar_Feed_To_Grid_Status,      // Y
    DESCR_Country,                        // ZZ
    DESCR_Solar_Feed_To_Grid_Power,       // AAAA
};
/* static const char *const qpigsList[][24] = {
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
        "Output_power",             // BBBBBB
        "Grid_frequency",           // CC.C
        "Output_current",           // DDDD.D
        "AC_output_voltage",        // EEE.E
        "AC_output_power",          // FFFFF
        "AC_output_frequency",      // GG.G
        "AC_output_current",        // HHH.H
        "Output_load_percent",      // III
        "PBUS_voltage",             // JJJ.J
        "SBUS_voltage",             // KKK.K
        "Positive_battery_voltage", // LLL.L
        "Negative_battery_voltage", // MMM.M
        "Battery_capacity",         // NNN
        "PV1_Input_Power",          // OOOOO
        "PV2_Input_Power",          // PPPPP
        "PV3_Input_Power",          // QQQQQ
        "PV1_Input_Voltage",        // RRR.R
        "PV2_Input_Voltage",        // SSS.S
        "PV3_Input_Voltage",        // TTT.T
        "Max_temperature",          // UUU.U
    },
}; */
unsigned int qallList_length = 18;
const char *const qallList[] = {
    // [PI30 Revo]
    DESCR_AC_In_Voltage,             // BBB.B
    DESCR_AC_In_Frequenz,            // CC.C
    DESCR_AC_Out_Voltage,            // DDD.D
    DESCR_AC_Out_Frequenz,           // EE.E
    DESCR_AC_Out_VA,                 // FFFF
    DESCR_AC_Out_Percent,            // GGG
    DESCR_Battery_Voltage,           // HH.H
    DESCR_Battery_Percent,           // III
    DESCR_Battery_Charge_Current,    // JJJ
    DESCR_Battery_Discharge_Current, // KKK
    DESCR_PV_Input_Voltage,          // LLL
    DESCR_PV_Input_Current,          // MM.M
    DESCR_PV_Charging_Power,         // NNNN
    DESCR_PV_Generation_Day,         // OOOOOO
    DESCR_PV_Generation_Sum,         // PPPPPP
    DESCR_Inverter_Operation_Mode,   // Q
    DESCR_Warning_Code,              // KK
    DESCR_Fault_Code,                // SS

};
const char *const P005GS[][28] = {
    {DESCR_AC_In_Voltage, "10"},            // AAAA
    {DESCR_AC_In_Frequenz, "10"},           // BBB
    {DESCR_AC_Out_Voltage, "10"},           // CCCC
    {DESCR_AC_Out_Frequenz, "10"},          // DDD
    {DESCR_AC_Out_VA, "0"},                 // EEEE
    {DESCR_AC_Out_Watt, "0"},                   // FFFF
    {DESCR_AC_Out_Percent, "0"},            // GGGG
    {DESCR_Battery_Voltage, "10"},          // HHHH
    {DESCR_Battery_SCC_Volt, "10"},         // III
    {DESCR_Battery_SCC2_Volt, "10"},            // JJJ
    {DESCR_Battery_Discharge_Current, "0"}, // KKK
    {DESCR_Battery_Charge_Current, "0"},    // LLL
    {DESCR_Battery_Percent, "0"},           // MMM
    {DESCR_Inverter_Bus_Temperature, "0"},  // NNN
    {DESCR_MPPT1_Charger_Temperature, "0"},     // OOO
    {DESCR_MPPT2_Charger_Temperature, "0"},     // PPP
    {DESCR_PV1_Input_Power, "0"},               // QQQQ
    {DESCR_PV2_Input_Power, "0"},               // RRRR
    {DESCR_PV1_Input_Voltage, "10"},            // SSSS
    {DESCR_PV2_Input_Voltage, "10"},            // TTTT
    {DESCR_Configuration_State, "0"},           // U
    {DESCR_MPPT1_Charger_Status, "0"},          // V
    {DESCR_MPPT2_CHarger_Status, "0"},          // W
    {DESCR_Load_Connection, "0"},               // X
    {DESCR_Battery_Power_Direction, "0"},       // Y
    {DESCR_ACDC_Power_Direction, "0"},          // Z
    {DESCR_Line_Power_Direction, "0"},          // a
    {DESCR_Local_Parallel_ID, "0"},             // b
};

bool PI_Serial::PIXX_QPIGS()
{
  const char *const *qpigsList = nullptr;
  unsigned int qpigsList_length;
  if (protocol == PI30)
  {
    // byte protocolNum = 0; // for future use
    get.raw.qall = "";
    String commandAnswerQALL = this->requestData("QALL");
    get.raw.qall = commandAnswerQALL;
    String commandAnswerQPIGS = this->requestData("QPIGS");
    get.raw.qpigs = commandAnswerQPIGS;
    if (commandAnswerQPIGS == DESCR_req_NAK || commandAnswerQPIGS == DESCR_req_NOA)
      return true;
    if (commandAnswerQPIGS == DESCR_req_ERCRC)
      return false;

    byte commandAnswerLength = commandAnswerQPIGS.length();
    // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
    if (commandAnswerLength >= 60 && commandAnswerLength <= 140)
    {
      if (commandAnswerLength <= 116)
      {
        qpigsList = qpigs_106;
        qpigsList_length = qpigs_106_length;
      }
      else if (commandAnswerLength > 131)
      {
        qpigsList = qpigs_90;
        qpigsList_length = qpigs_90_length;
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

      for (unsigned int i = 0; i < qpigsList_length; i++)
      {
        if (!strs[i].isEmpty() && sizeof *qpigsList[i] != 0)
        {
          liveData[qpigsList[i]] = (int)(strs[i].toFloat() * 100 + 0.5) / 100.0;
        }
      }
      // make some things pretty
      liveData[DESCR_Battery_Load] = (liveData[DESCR_Battery_Charge_Current].as<unsigned short>() - liveData[DESCR_Battery_Discharge_Current].as<unsigned short>());
      liveData[DESCR_PV_Input_Power] = (liveData[DESCR_PV_Input_Voltage].as<unsigned short>() * liveData[DESCR_PV_Input_Current].as<unsigned short>());
    }

    /*       bool handleAnswer = false;

          // or count how many fields it have and select the answer deepens on field count
          int count = 1;
          for (uint8_t i = 0; i < commandAnswerQPIGS.length(); i++)
            if (commandAnswerQPIGS[i] == ' ')
              count++;

          switch (commandAnswerQPIGS.length())
          {
          case 106: // 000.0 00.0 230.6 50.0 0000 0000 000 356 25.89 001 092 0032 0001 052.2 25.87 00000 00110110 00 00 00045 010 - 21 fields
            qlist = qpigs_106;
            handleAnswer = true;
            get.raw.qpigs = "Wrong Length(" + (String)count + "), Contact Dev:" + get.raw.qpigs;
            break;
          case 90: // 231.1 49.9 231.0 49.9 0000 0048 000 401 01.30 000 000 0026 0000 206.0 01.30 00000 00010110 - 21 fields
            qlist = qpigs_90;
            handleAnswer = true;
            get.raw.qpigs = "Wrong Length(" + (String)count + "), Contact Dev:" + get.raw.qpigs;
            break;
          default:
            // get.raw.qpigs = "Wrong Length(" + (String)get.raw.qpigs.length() + "), Contact Dev:" + get.raw.qpigs;
            break;
          } */

    /*       byte commandAnswerLength = commandAnswerQPIGS.length();
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
          } */

    if (get.raw.qall.length() > 10 /*get.raw.qall != "NAK" || get.raw.qall != "ERCRC" || get.raw.qall != ""*/)
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

      for (unsigned int i = 0; i < qallList_length; i++)
      {
         if (!strsQALL[i].isEmpty() && sizeof *qpigsList[i] != 0)
           liveData[qallList[i]] = (int)(strsQALL[i].toFloat() * 100 + 0.5) / 100.0;
      }
       liveData[DESCR_Inverter_Operation_Mode] = getModeDesc((char)liveData[DESCR_Inverter_Operation_Mode].as<String>().charAt(0));
       liveData[DESCR_Battery_Load] = (liveData[DESCR_Battery_Charge_Current].as<unsigned short>() - liveData[DESCR_Battery_Discharge_Current].as<unsigned short>());
    }

    return true;
  }
  else if (protocol == PI18)
  {
    String commandAnswer = this->requestData("^P005GS");
    get.raw.qpigs = commandAnswer;
    if (commandAnswer == DESCR_req_NAK || commandAnswer == DESCR_req_NOA)
      return true;
    if (commandAnswer == DESCR_req_ERCRC)
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

       liveData[DESCR_PV_Input_Voltage] = (liveData[DESCR_PV1_Input_Voltage].as<unsigned short>() + liveData[DESCR_PV2_Input_Voltage].as<unsigned short>());
       liveData[DESCR_PV_Charging_Power] = (liveData[DESCR_PV1_Input_Power].as<unsigned short>() + liveData[DESCR_PV2_Input_Power].as<unsigned short>());
       liveData[DESCR_PV_Input_Current] = (int)((liveData[DESCR_PV_Charging_Power].as<unsigned short>() / (liveData[DESCR_PV_Input_Voltage].as<unsigned short>() + 0.5)) * 100) / 100.0;
       liveData[DESCR_Battery_Load] = (liveData[DESCR_Battery_Charge_Current].as<unsigned short>() - liveData[DESCR_Battery_Discharge_Current].as<unsigned short>());
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

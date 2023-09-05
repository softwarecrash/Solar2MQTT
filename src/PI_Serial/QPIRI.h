static const char *const qpiriList[][30] = {
    // [PI34 / MPPT-3000], [PI30 HS MS MSX], [PI30 Revo], [PI30 PIP], [PI41 / LV5048]
    {
        "AC_in_rating_voltage",            // BBB.B
        "AC_in_rating_current",            // CC.C
        "AC_out_rating_voltage",           // DDD.D
        "AC_out_rating_frequency",         // EE.E
        "AC_out_rating_current",           // FF.F
        "AC_out_rating_apparent_power",    // HHHH
        "AC_out_rating_active_power",      // IIII
        "Battery_rating_voltage",          // JJ.J
        "Battery_re-charge_voltage",       // KK.K
        "Battery_under_voltage",           // JJ.J
        "Battery_bulk_voltage",            // KK.K
        "Battery_float_voltage",           // LL.L
        "Battery_type",                    // O
        "Current_max_AC_charging_current", // PP
        "Current_max_charging_current",    // QQ0
        "Input_voltage_range",             // O
        "Output_source_priority",          // P
        "Charger_source_priority",         // Q
        "Parallel_max_num",                // R
        "Machine_type",                    // SS
        "Topology",                        // T
        "Output_mode",                     // U
        "Battery_re-discharge_voltage",    // VV.V
        "PV_OK_condition_for_parallel",    // W
        "PV_power_balance",                // X
        "Max_charging_time_at_CV_stage",   // YYY
        "Operation_Logic",                 // Z
        "Max_discharging_current",         // CCC
    },
    // [PI16]
    {
        "AC_in_rating_voltage",            // BBB.B
        "AC_in_rating_current",            // CC.C
        "AC_out_rating_voltage",           // DDD.D
        "AC_out_rating_frequency",         // EE.E
        "AC_out_rating_current",           // FF.F
        "AC_out_rating_apparent_power",    // HHHH
        "AC_out_rating_active_power",      // IIII
        "Battery_rating_voltage",          // JJ.J
        "Battery_re-charge_voltage",       // KK.K
        "Battery_under_voltage",           // JJ.J
        "Battery_bulk_voltage",            // KK.K
        "Battery_float_voltage",           // LL.L
        "Battery_type",                    // O
        "Current_max_AC_charging_current", // PP
        "Current_max_charging_current",    // QQ0
        "Input_voltage_range",             // O
        "Output_source_priority",          // P
        "Charger_source_priority",         // Q
        "Parallel_max_num",                // R
        "Machine_type",                    // SS
        "Topology",                        // T
        "Output_mode",                     // U
        "Battery_re-discharge_voltage",    // VV.V
        "PV_OK_condition_for_parallel",    // W
        "PV_power_balance",                // X
        "Max_charging_time_at_CV_stage",   // YYY
        "Operation_Logic",                 // Z
        "Max_discharging_current",         // CCC
    },
};
static const char *const P007PIRI[][25] = {
    // [PI34 / MPPT-3000], [PI30 HS MS MSX], [PI30 Revo], [PI30 PIP], [PI41 / LV5048]

    {"AC_in_rating_voltage", "10"},           // AAAA
    {"AC_in_rating_current", "10"},           // BBB
    {"AC_out_rating_voltage", "10"},          // CCCC
    {"AC_out_rating_frequency", "10"},        // DDD
    {"AC_out_rating_current", "10"},          // EEE
    {"AC_out_rating_apparent_power", "0"},    // FFFF
    {"AC_out_rating_active_power", "0"},      // IIII
    {"Battery_rating_voltage", "10"},         // HHH
    {"Battery_re-charge_voltage", "10"},      // III
    {"Battery_re-discharge_voltage", "10"},   // JJJ
    {"Battery_under_voltage", "10"},          // KKK
    {"Battery_bulk_voltage", "10"},           // LLL
    {"Battery_float_voltage", "10"},          // MMM
    {"Battery_type", "0"},                    // N
    {"Current_max_AC_charging_current", "0"}, // OO
    {"Current_max_charging_current", "0"},    // PPP
    {"Input_voltage_range", "0"},             // Q
    {"Output_source_priority", "0"},          // R
    {"Charger_source_priority", "0"},         // S
    {"Parallel_max_num", "0"},                // T
    {"Machine_type", "0"},                    // U
    {"Topology", "0"},                        // V
    {"Output_mode", "0"},                     // W
    {"Solar_power_priority", "0"},            // Z
    {"MPPT_string", "0"},                     // a
};
bool PI_Serial::PIXX_QPIRI()
{
  if (protocol == PI30)
  {
    String commandAnswer = this->requestData("QPIRI");
    get.raw.qpiri = commandAnswer;
    // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
    if (commandAnswer == "NAK")
    {
      return true;
    }
    if (commandAnswer == "ERCRC")
    {
      return false;
    }
    if (commandAnswer.length() > 80 &&
        commandAnswer.length() < 105)
    {

      byte protocolNum = 0;
      String strs[30];
      // Split the string into substrings
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

      for (unsigned int i = 0; i < sizeof qpiriList[protocolNum] / sizeof qpiriList[protocolNum][0]; i++)
      {
        if (!strs[i].isEmpty() && strcmp(qpiriList[protocolNum][i], "") != 0)
          staticData[qpiriList[protocolNum][i]] = (int)(strs[i].toFloat() * 100 + 0.5) / 100.0;
      }

      switch ((byte)staticData["Battery_type"].as<unsigned int>())
      {
      case 0:
        staticData["Battery_type"] = "AGM";
        break;
      case 1:
        staticData["Battery_type"] = "Flooded";
        break;
      case 2:
        staticData["Battery_type"] = "User";
        break;
      default:
        break;
      }
      switch ((byte)staticData["Input_voltage_range"].as<unsigned int>())
      {
      case 0:
        staticData["Input_voltage_range"] = "Appliance";
        break;
      case 1:
        staticData["Input_voltage_range"] = "UPS";
        break;
      default:
        break;
      }
      switch ((byte)staticData["Output_source_priority"].as<unsigned int>())
      {
      case 0:
        staticData["Output_source_priority"] = "Utility first";
        break;
      case 1:
        staticData["Output_source_priority"] = "Solar first";
        break;
      case 2:
        staticData["Output_source_priority"] = "SBU first";
        break;
      default:
        break;
      }
      switch ((byte)staticData["Charger_source_priority"].as<unsigned int>())
      {
      case 0:
        staticData["Charger_source_priority"] = "Utility first";
        break;
      case 1:
        staticData["Charger_source_priority"] = "Solar first";
        break;
      case 2:
        staticData["Charger_source_priority"] = "Solar + Utility";
        break;
      case 3:
        staticData["Charger_source_priority"] = "Only solar charging permitted";
        break;
      default:
        break;
      }
      switch ((byte)staticData["Machine_type"].as<unsigned int>())
      {
      case 00:
        staticData["Machine_type"] = "Grid tie";
        break;
      case 01:
        staticData["Machine_type"] = "Off Grid";
        break;
      case 10:
        staticData["Machine_type"] = "Hybrid";
        break;
      default:
        break;
      }
      switch ((byte)staticData["Topology"])
      {
      case 0:
        staticData["Topology"] = "Transformerless";
        break;
      case 1:
        staticData["Topology"] = "Transformer";
        break;
      default:
        break;
        return true;
      }
      return true;
    }
    return true;
  }
  else if (protocol == PI18)
  {
        String commandAnswer = this->requestData("QPIRI");
    get.raw.qpiri = commandAnswer;
    // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
    if (commandAnswer == "NAK")
    {
      return true;
    }
    if (commandAnswer == "ERCRC")
    {
      return false;
    }
    if (commandAnswer.length() > 80 &&
        commandAnswer.length() < 105)
    {
      String strs[30];
      // Split the string into substrings
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

      for (unsigned int i = 0; i < sizeof qpiriList[0] / sizeof qpiriList[0][0]; i++)
      {
        if (!strs[i].isEmpty() && strcmp(qpiriList[0][i], "") != 0)

{

          //staticData[qpiriList[0][i]] = (int)(strs[i].toFloat() * 100 + 0.5) / 100.0;
}


      }
/*
      switch ((byte)staticData["Battery_type"].as<unsigned int>())
      {
      case 0:
        staticData["Battery_type"] = "AGM";
        break;
      case 1:
        staticData["Battery_type"] = "Flooded";
        break;
      case 2:
        staticData["Battery_type"] = "User";
        break;
      default:
        break;
      }
      switch ((byte)staticData["Input_voltage_range"].as<unsigned int>())
      {
      case 0:
        staticData["Input_voltage_range"] = "Appliance";
        break;
      case 1:
        staticData["Input_voltage_range"] = "UPS";
        break;
      default:
        break;
      }
      switch ((byte)staticData["Output_source_priority"].as<unsigned int>())
      {
      case 0:
        staticData["Output_source_priority"] = "Utility first";
        break;
      case 1:
        staticData["Output_source_priority"] = "Solar first";
        break;
      case 2:
        staticData["Output_source_priority"] = "SBU first";
        break;
      default:
        break;
      }
      switch ((byte)staticData["Charger_source_priority"].as<unsigned int>())
      {
      case 0:
        staticData["Charger_source_priority"] = "Utility first";
        break;
      case 1:
        staticData["Charger_source_priority"] = "Solar first";
        break;
      case 2:
        staticData["Charger_source_priority"] = "Solar + Utility";
        break;
      case 3:
        staticData["Charger_source_priority"] = "Only solar charging permitted";
        break;
      default:
        break;
      }
      switch ((byte)staticData["Machine_type"].as<unsigned int>())
      {
      case 00:
        staticData["Machine_type"] = "Grid tie";
        break;
      case 01:
        staticData["Machine_type"] = "Off Grid";
        break;
      case 10:
        staticData["Machine_type"] = "Hybrid";
        break;
      default:
        break;
      }
      switch ((byte)staticData["Topology"])
      {
      case 0:
        staticData["Topology"] = "Transformerless";
        break;
      case 1:
        staticData["Topology"] = "Transformer";
        break;
      default:
        break;
        return true;
      }
      */
      return true;
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
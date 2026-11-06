static const char *const qpigs2List[] = {
    // [PI34 / MPPT-3000], [PI30 HS MS MSX], [PI30 Revo], [PI30 PIP], [PI41 / LV5048]
    DESCR_PV2_Input_Current,  // BBB.B
    DESCR_PV2_Input_Voltage,  // CCC.C
    DESCR_PV2_Charging_Power, // DDDD
};
static const char *const qpigs2ListL2[] = {
    // [PI41 / LV5048]
    DESCR_AC_In_Voltage_L2,  // AAA.A
    DESCR_AC_In_Frequency_L2, // BB.B
    DESCR_AC_Out_Voltage_L2, // CCC.C
    DESCR_AC_Out_Frequency_L2, // DD.D
    DESCR_AC_Out_VA_L2,       // EEEE
    DESCR_AC_Out_Watt_L2,     // FFFF
    DESCR_AC_Out_Percent_L2,  // GGG
    DESCR_PV2_Input_Current,  // HHHH
    DESCR_PV2_Input_Voltage,  // III.I
    DESCR_Battery_Voltage_L2, // JJ.JJ
    DESCR_Device_Status_L2,   // b7b6b5b4b3b2b1b0
};

// PI30 2015-09-24, section 2.10: twelve-field SCC2/SCC3 status reply.
// Field positions are zero-based; the other fields are validated but not published here.
static constexpr int qpigs2LegacyPv2VoltageIndex = 1;
static constexpr int qpigs2LegacyScc2BatteryVoltageIndex = 2;
static constexpr int qpigs2LegacyPv2PowerIndex = 3;
static constexpr int qpigs2LegacyStatusIndex = 4;
static constexpr int qpigs2LegacyTotalPvPowerIndex = 11;
static constexpr int qpigs2LegacyFieldCount = 12;

static bool pi_is_legacy_qpigs2(char *const fields[], int fieldCount)
{
  // Validate the complete reply before publishing any of its values.
  if (fieldCount != qpigs2LegacyFieldCount)
    return false;
  for (int i = 0; i < fieldCount; ++i)
  {
    const size_t length = strlen(fields[i]);
    if (i == qpigs2LegacyStatusIndex)
    {
      if (length != 8 || strspn(fields[i], "01") != 8)
        return false;
      continue;
    }
    if (length == 0)
      return false;
    bool decimalPoint = false;
    unsigned long whole = 0;
    for (size_t j = 0; j < length; ++j)
    {
      const char value = fields[i][j];
      if (value == '.' && !decimalPoint && j > 0 && j + 1 < length)
      {
        decimalPoint = true;
        continue;
      }
      if (value < '0' || value > '9')
        return false;
      if (!decimalPoint)
      {
        // Keep the documented five-digit magnitude; allow leading zeros.
        if (whole > 9999UL)
          return false;
        whole = whole * 10UL + static_cast<unsigned long>(value - '0');
      }
    }
  }
  return true;
}

bool PI_Serial::PIXX_QPIGS2()
{
  if (protocol == PI16)
  {
    get.raw.qpigs2 = DESCR_req_NOA;
    return true;
  }
  else if (isPi30LikeProtocol(protocol))
  {
    String commandAnswer = this->requestData("QPIGS2");
    get.raw.qpigs2 = commandAnswer;
    // A CRC failure is rolled back by the polling cycle; do not expose a gap
    // while the request is still in progress.
    if (commandAnswer == DESCR_req_ERCRC)
      return false;

    pi_clear_json_fields(liveData, qpigs2List);
    pi_clear_json_fields(liveData, qpigs2ListL2);
    liveData.remove(DESCR_PV2_Input_Power);
    liveData.remove(DESCR_Battery_SCC2_Volt);
    liveData.remove(DESCR_PV_Total_Input_Power);

    if (commandAnswer == DESCR_req_NAK || commandAnswer == DESCR_req_NOA)
    {
      return true;
    }

    char bufQPIGS2[128];
    if (commandAnswer.length() >= sizeof(bufQPIGS2))
      return true;
    commandAnswer.toCharArray(bufQPIGS2, sizeof(bufQPIGS2));
    char *fieldsQPIGS2[30];
    int StringCount = pi_split_fields(bufQPIGS2, delimiter[0], fieldsQPIGS2, 30);

    const int pi41FieldCount = static_cast<int>(sizeof qpigs2ListL2 / sizeof qpigs2ListL2[0]);
    const int pv2FieldCount = static_cast<int>(sizeof qpigs2List / sizeof qpigs2List[0]);

    // Supported layouts: short PV2 (3 fields), PI41 L2 (11 fields),
    // and the PI30 SCC2/SCC3 status reply (12 fields).
    // Full QPIGS-style replies from single-tracker devices must not affect LiveData.
    if (protocol == PI41 && StringCount == pi41FieldCount)
    {
      for (unsigned int i = 0; i < sizeof qpigs2ListL2 / sizeof qpigs2ListL2[0]; i++)
      {
        if (fieldsQPIGS2[i][0] != '\0' && strcmp(qpigs2ListL2[i], "") != 0)
        {
          liveData[qpigs2ListL2[i]] = pi_parse_float2(fieldsQPIGS2[i]);
        }
      }
      liveData[DESCR_PV2_Input_Power] = pi_compute_power(
        liveData[DESCR_PV2_Input_Voltage].as<double>(),
        liveData[DESCR_PV2_Input_Current].as<double>());
      refineProtocol();
    }
    else if (StringCount == pv2FieldCount)
    {
      for (unsigned int i = 0; i < sizeof qpigs2List / sizeof qpigs2List[0]; i++)
      {
        if (fieldsQPIGS2[i][0] != '\0' && strcmp(qpigs2List[i], "") != 0)
          liveData[qpigs2List[i]] = pi_parse_float2(fieldsQPIGS2[i]);
      }
      // Derive PV2 input power from panel voltage and current.
      liveData[DESCR_PV2_Input_Power] = pi_compute_power(
        liveData[DESCR_PV2_Input_Voltage].as<double>(),
        liveData[DESCR_PV2_Input_Current].as<double>());
      refineProtocol();
    }
    else if (pi_is_legacy_qpigs2(fieldsQPIGS2, StringCount))
    {
      const double voltage = pi_parse_float2(fieldsQPIGS2[qpigs2LegacyPv2VoltageIndex]);
      const double power = pi_parse_float2(fieldsQPIGS2[qpigs2LegacyPv2PowerIndex]);
      liveData[DESCR_PV2_Input_Voltage] = voltage;
      liveData[DESCR_Battery_SCC2_Volt] = pi_parse_float2(fieldsQPIGS2[qpigs2LegacyScc2BatteryVoltageIndex]);
      liveData[DESCR_PV2_Charging_Power] = power;
      liveData[DESCR_PV2_Input_Power] = power;
      // Derive this current from reported PV2 charging power and PV voltage.
      // The first raw current field is not used; its physical interpretation
      // needs model-specific confirmation rather than a battery-side assumption.
      liveData[DESCR_PV2_Input_Current] = pi_compute_current(power, voltage);
      liveData[DESCR_PV_Total_Input_Power] = pi_parse_float2(fieldsQPIGS2[qpigs2LegacyTotalPvPowerIndex]);
      refineProtocol();
    }
    return true;
  }
  else if (protocol == PI18)
  {
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

static String pi_qmd_model(const String &response)
{
  // Wire format: 15-character model, then power, power factor, phases,
  // input/output voltage, battery blocks and block voltage. Transport text
  // normalization removes space padding, so locate the model from that suffix.
  constexpr size_t modelFieldLength = 15;
  constexpr size_t maxReplyLength = 46;
  constexpr size_t ratingWidths[] = {7, 2, 3, 3, 3, 2, 4};
  constexpr size_t ratingFieldCount = sizeof ratingWidths / sizeof ratingWidths[0];
  enum { RatedPower, PowerFactor, Phases, InputVoltage, OutputVoltage, BatteryBlocks, BlockVoltage };
  if (response.length() > maxReplyLength)
    return String();

  char buffer[maxReplyLength + 1];
  response.toCharArray(buffer, sizeof buffer);
  // At most eight single-character model words fit the 15-character field.
  char *fields[(modelFieldLength + 1) / 2 + ratingFieldCount + 1];
  const int count = pi_split_fields(buffer, ' ', fields, sizeof fields / sizeof fields[0]);
  if (count <= static_cast<int>(ratingFieldCount))
    return String();
  const size_t ratingStart = count - ratingFieldCount;
  for (size_t i = 0; i < ratingFieldCount; ++i)
  {
    const char *value = fields[ratingStart + i];
    const size_t length = strlen(value);
    if (length == 0 || length > ratingWidths[i] || (i != RatedPower && length != ratingWidths[i]))
      return String();
    size_t firstDigit = 0;
    if (i == RatedPower)
      while (firstDigit < length && value[firstDigit] == '#')
        ++firstDigit;
    if (firstDigit == length)
      return String();
    for (size_t j = firstDigit; j < length; ++j)
    {
      const char separator = (i == Phases && j == 1) ? '/' :
                             ((i == BlockVoltage && j == 2) ? '.' : '\0');
      if (separator ? value[j] != separator : (value[j] < '0' || value[j] > '9'))
        return String();
    }
  }

  // Validate the suffix to identify the boundary, but never apply its ratings:
  // QPIRI remains authoritative, even if QMD contains generic model defaults.
  String model = response.substring(0, fields[ratingStart] - buffer);
  model.trim();

  size_t first = 0, end = model.length();
  while (first < end && (model[first] == '#' || model[first] == ' '))
    ++first;
  while (end > first && (model[end - 1] == '#' || model[end - 1] == ' '))
    --end;
  if (first == end || end - first > modelFieldLength)
    return String();
  for (size_t i = first; i < end; ++i)
  {
    const char c = model[i];
    if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
          (c >= '0' && c <= '9') || c == ' ' || c == '-' || c == '_' || c == '.' || c == '/'))
      return String();
  }
  return model.substring(first, end);
}

bool PI_Serial::PIXX_QMN()
{
  if (protocol == PI16)
  {
    return true;
  }
  else if (isPi30LikeProtocol(protocol))
  {
    const String previousModel = staticData[DESCR_Device_Model] | "";
    staticData.remove(DESCR_Device_Model);
    get.raw.qmd = "";

    String commandAnswer = this->requestData("QMN");
    get.raw.qmn = commandAnswer;
    // Only an explicit QMN rejection triggers the legacy model inquiry.
    // QMN timeout/CRC handling remains unchanged.
    if (commandAnswer == DESCR_req_NAK)
    {
      get.raw.qmd = this->requestData("QMD");
      // QMD supplies optional metadata. Preserve its raw error, but do not
      // restart initialization or discard a known model if that inquiry fails.
      const String model = pi_qmd_model(get.raw.qmd);
      if (!model.isEmpty())
        staticData[DESCR_Device_Model] = model;
      else if (!previousModel.isEmpty())
        staticData[DESCR_Device_Model] = previousModel;
      return true;
    }
    // calculate the length with https://elmar-eigner.de/text-zeichen-laenge.html
    if (commandAnswer == DESCR_req_NAK || commandAnswer == DESCR_req_NOA)
    {
      return true;
    }
    if (commandAnswer == DESCR_req_ERCRC)
    {
      return false;
    }
    if (commandAnswer.length() > 3 && commandAnswer.length() < 50)
    {
      staticData[DESCR_Device_Model] = commandAnswer;
      return true;
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

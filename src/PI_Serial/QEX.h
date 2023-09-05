bool PI_Serial::PIXX_QEX()
{
  if (protocol == PI30)
  {
    String commandAnswer = this->requestData("QET");
    get.raw.qet = commandAnswer;
    if (commandAnswer == "NAK")
    {
      return true;
    }
    if (commandAnswer == "ERCRC")
    {
      return false;
    }
    liveData["PV_generation_sum"] = commandAnswer;
    return true;

    commandAnswer = this->requestData("QT");
    get.raw.qt = commandAnswer;
    if (commandAnswer == "ERCRC")
    {
      return false;
    }
    if (commandAnswer == "NAK")
    {
      return true;
    }
    else
    {
      commandAnswer = this->requestData("QEY" + get.raw.qt.substring(0, 4));
      get.raw.qey = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["PV_generation_year"] = commandAnswer;

      commandAnswer = this->requestData("QEM" + get.raw.qt.substring(0, 6));
      get.raw.qem = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["PV_generation_month"] = commandAnswer;

      commandAnswer = this->requestData("QED" + get.raw.qt.substring(0, 8));
      get.raw.qed = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["PV_generation_day"] = commandAnswer;

      commandAnswer = this->requestData("QLT");
      get.raw.qlt = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["AC_in_generation_sum "] = commandAnswer;

      commandAnswer = this->requestData("QLY" + get.raw.qt.substring(0, 4));
      get.raw.qly = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["AC_in_generation_year"] = commandAnswer;

      commandAnswer = this->requestData("QLM" + get.raw.qt.substring(0, 6));
      get.raw.qlm = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["AC_in_generation_month"] = commandAnswer;

      commandAnswer = this->requestData("QLD" + get.raw.qt.substring(0, 8));
      get.raw.qld = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["AC_in_generation_day"] = commandAnswer;
    }
  }
  else if (protocol == PI18)
  {
    String commandAnswer = this->requestData("^P005ET");
    get.raw.qet = commandAnswer;
    if (commandAnswer == "NAK")
    {
      return true;
    }
    if (commandAnswer == "ERCRC")
    {
      return false;
    }
    liveData["PV_generation_sum"] = commandAnswer;
    return true;

    commandAnswer = this->requestData("^P004T");
    get.raw.qt = commandAnswer;
    if (commandAnswer == "ERCRC")
    {
      return false;
    }
    if (commandAnswer == "NAK")
    {
      return true;
    }
    else
    {
      commandAnswer = this->requestData("QEY" + get.raw.qt.substring(0, 4));
      get.raw.qey = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["PV_generation_year"] = commandAnswer;

      commandAnswer = this->requestData("QEM" + get.raw.qt.substring(0, 6));
      get.raw.qem = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["PV_generation_month"] = commandAnswer;

      commandAnswer = this->requestData("QED" + get.raw.qt.substring(0, 8));
      get.raw.qed = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["PV_generation_day"] = commandAnswer;

      commandAnswer = this->requestData("QLT");
      get.raw.qlt = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["AC_in_generation_sum "] = commandAnswer;

      commandAnswer = this->requestData("QLY" + get.raw.qt.substring(0, 4));
      get.raw.qly = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["AC_in_generation_year"] = commandAnswer;

      commandAnswer = this->requestData("QLM" + get.raw.qt.substring(0, 6));
      get.raw.qlm = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["AC_in_generation_month"] = commandAnswer;

      commandAnswer = this->requestData("QLD" + get.raw.qt.substring(0, 8));
      get.raw.qld = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["AC_in_generation_day"] = commandAnswer;
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
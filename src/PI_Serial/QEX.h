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
    liveData["PV_generation_sum"] = commandAnswer.toInt();
    commandAnswer = this->requestData("QT");
    get.raw.qt = commandAnswer.substring(0, 8);
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
      liveData["PV_generation_year"] = commandAnswer.toInt();

      commandAnswer = this->requestData("QEM" + get.raw.qt.substring(0, 6));
      get.raw.qem = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["PV_generation_month"] = commandAnswer.toInt();

      commandAnswer = this->requestData("QED" + get.raw.qt.substring(0, 8));
      get.raw.qed = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["PV_generation_day"] = commandAnswer.toInt();

      commandAnswer = this->requestData("QLT");
      get.raw.qlt = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["AC_in_generation_sum "] = commandAnswer.toInt();

      commandAnswer = this->requestData("QLY" + get.raw.qt.substring(0, 4));
      get.raw.qly = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["AC_in_generation_year"] = commandAnswer.toInt();

      commandAnswer = this->requestData("QLM" + get.raw.qt.substring(0, 6));
      get.raw.qlm = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["AC_in_generation_month"] = commandAnswer.toInt();

      commandAnswer = this->requestData("QLD" + get.raw.qt.substring(0, 8));
      get.raw.qld = commandAnswer;
      if (commandAnswer == "ERCRC")
        return false;
      if (commandAnswer == "NAK")
        return true;
      liveData["AC_in_generation_day"] = commandAnswer.toInt();

      return true;
    }
  }
  else if (protocol == PI18)
  {
    String commandAnswer;
    commandAnswer = this->requestData("^P005ET");
    if (commandAnswer == "ERCRC" || commandAnswer == "NAK" || commandAnswer == "")
      return true;
    get.raw.qet = commandAnswer;
    liveData["PV_generation_sum"] = commandAnswer.toInt();

    commandAnswer = this->requestData("^P004T");
    if (commandAnswer == "ERCRC" || commandAnswer == "NAK" || commandAnswer == "" || commandAnswer.toInt() == 0)
    {
      return true;
    }
    else
    {
      get.raw.qt = commandAnswer;

      commandAnswer = this->requestData("^P013ED" + get.raw.qt.substring(0, 8));
      if (commandAnswer == "ERCRC" || commandAnswer == "NAK" || commandAnswer == "" || commandAnswer.toInt() == 0 || commandAnswer == get.raw.qem) // last short fix for strange data
        return true;
      get.raw.qed = commandAnswer;
      liveData["PV_generation_day"] = commandAnswer.toInt();

      commandAnswer = this->requestData("^P011EM" + get.raw.qt.substring(0, 6));
      if (commandAnswer == "ERCRC" || commandAnswer == "NAK" || commandAnswer == "" || commandAnswer.toInt() == 0)
        return true;
      get.raw.qem = commandAnswer;
      liveData["PV_generation_month"] = commandAnswer.toInt();

      get.raw.qt = commandAnswer;
      commandAnswer = this->requestData("^P009EY" + get.raw.qt.substring(0, 4));
      if (commandAnswer == "ERCRC" || commandAnswer == "NAK" || commandAnswer == "" || commandAnswer.toInt() == 0)
        return true;
      get.raw.qey = commandAnswer;
      liveData["PV_generation_year"] = commandAnswer.toInt();

      // return true;
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
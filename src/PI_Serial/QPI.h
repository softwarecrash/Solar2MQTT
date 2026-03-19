bool PI_Serial::PIXX_QPI()
{
  if (isPi30LikeProtocol(protocol) || isRawOnlyPiProtocol(protocol))
  {
    staticData.remove(DESCR_Protocol_ID);

    String commandAnswer = this->requestData("QPI");
    get.raw.qpi = commandAnswer;
    if (commandAnswer == DESCR_req_NAK || commandAnswer == DESCR_req_NOA)
    {
      return true;
    }
    if (commandAnswer == DESCR_req_ERCRC)
    {
      return false;
    }
    staticData[DESCR_Protocol_ID] = commandAnswer;
    refineProtocol();
    return true;
  }
  else if (protocol == PI18)
  {
    staticData.remove(DESCR_Protocol_ID);

    String commandAnswer = this->requestData("^P005PI");
    get.raw.qpi = commandAnswer;
    if (commandAnswer == DESCR_req_NAK || commandAnswer == DESCR_req_NOA)
    {
      return true;
    }
    if (commandAnswer == DESCR_req_ERCRC)
    {
      return false;
    }
    staticData[DESCR_Protocol_ID] = commandAnswer.toInt();
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

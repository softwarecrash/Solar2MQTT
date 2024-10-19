bool PI_Serial::PIXX_QFLAG()
{
    if (protocol == PI30)
    {
        String commandAnswer = this->requestData("QFLAG");
        get.raw.qflag = commandAnswer;
        byte commandAnswerLength = commandAnswer.length();
        if (commandAnswer == "NAK")
        {
            return true;
        }
        if (commandAnswer == "ERCRC")
        {
            return false;
        }
        if (commandAnswerLength == 11)
        {
            staticData["Buzzer_Enabled"] = checkQFLAG(commandAnswer, 'a');

            staticData["Buzzer_Enabled"] = checkQFLAG(commandAnswer, 'a');
            staticData["Overload_bypass_Enabled"] = checkQFLAG(commandAnswer, 'b');
            staticData["Power_saving_Enabled"] = checkQFLAG(commandAnswer, 'j');
            staticData["LCD_reset_to_default_Enabled"] = checkQFLAG(commandAnswer, 'k');
            staticData["Overload_restart_Enabled"] = checkQFLAG(commandAnswer, 'u');
            staticData["Over_temperature_restart_Enabled"] = checkQFLAG(commandAnswer, 'v');
            staticData["LCD_backlight_Enabled"] = checkQFLAG(commandAnswer, 'x');
            staticData["Primary_source_interrupt_alarm_Enabled"] = checkQFLAG(commandAnswer, 'y');
            staticData["Record_fault_code_Enabled"] = checkQFLAG(commandAnswer, 'z');
        }
        return true;
    }
    else if (protocol == PI18)
    {
        String commandAnswer = this->requestData("^P007FLAG");
        get.raw.qflag = commandAnswer;
        byte commandAnswerLength = commandAnswer.length();
        if (commandAnswer == "NAK")
        {
            return true;
        }
        if (commandAnswer == "ERCRC")
        {
            return false;
        }
        if (commandAnswerLength == 10)
        {
            staticData["Buzzer_Enabled"] = ((String)commandAnswer.charAt(0) == "1") ? true : false;
            staticData["Overload_bypass_Enabled"] = ((String)(commandAnswer.charAt(1)) == "1") ? true : false;
            staticData["LCD_reset_to_default_Enabled"] = ((String)(commandAnswer.charAt(2)) == "1") ? true : false;
            staticData["Overload_restart_Enabled"] = ((String)(commandAnswer.charAt(3)) == "1") ? true : false;
            staticData["Over_temperature_restart_Enabled"] = ((String)(commandAnswer.charAt(4)) == "1") ? true : false;
            staticData["LCD_backlight_Enabled"] = ((String)(commandAnswer.charAt(5)) == "1") ? true : false;
            staticData["Primary_source_interrupt_alarm_Enabled"] = ((String)(commandAnswer.charAt(6)) == "1") ? true : false;
            staticData["Record_fault_code_Enabled"] = ((String)(commandAnswer.charAt(7)) == "1") ? true : false;
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

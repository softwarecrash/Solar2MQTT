bool PI_Serial::PIXX_QFLAG()
{
    if (isPi30LikeProtocol(protocol))
    {
        staticData.remove(DESCR_Buzzer_Enabled);
        staticData.remove(DESCR_Overload_Bypass_Enabled);
        staticData.remove(DESCR_Power_Saving_Enabled);
        staticData.remove(DESCR_LCD_Reset_To_Default_Enabled);
        staticData.remove(DESCR_Data_Log_Pop_Up);
        staticData.remove(DESCR_Overload_Restart_Enabled);
        staticData.remove(DESCR_Over_Temperature_Restart_Enabled);
        staticData.remove(DESCR_LCD_Backlight_Enabled);
        staticData.remove(DESCR_Primary_Source_Interrupt_Alarm_Enabled);
        staticData.remove(DESCR_Record_Fault_Code_Enabled);
        staticData.remove(DESCR_Solar_Feed_To_Grid_Enabled);

        String commandAnswer = this->requestData("QFLAG");
        get.raw.qflag = commandAnswer;
        if (commandAnswer == DESCR_req_NAK || commandAnswer == DESCR_req_NOA)
        {
            return true;
        }
        if (commandAnswer == DESCR_req_ERCRC)
        {
            return false;
        }
        staticData[DESCR_Buzzer_Enabled] = checkQFLAG(commandAnswer, 'a');
        staticData[DESCR_Overload_Bypass_Enabled] = checkQFLAG(commandAnswer, 'b');
        if (commandAnswer.indexOf('j') >= 0 || commandAnswer.indexOf('J') >= 0)
        {
            staticData[DESCR_Power_Saving_Enabled] = checkQFLAG(commandAnswer, 'j');
        }
        staticData[DESCR_LCD_Reset_To_Default_Enabled] = checkQFLAG(commandAnswer, 'k');
        if (commandAnswer.indexOf('l') >= 0 || commandAnswer.indexOf('L') >= 0)
        {
            staticData[DESCR_Data_Log_Pop_Up] = checkQFLAG(commandAnswer, 'l');
        }
        if (commandAnswer.indexOf('d') >= 0 || commandAnswer.indexOf('D') >= 0)
        {
            staticData[DESCR_Solar_Feed_To_Grid_Enabled] = checkQFLAG(commandAnswer, 'd');
        }
        staticData[DESCR_Overload_Restart_Enabled] = checkQFLAG(commandAnswer, 'u');
        staticData[DESCR_Over_Temperature_Restart_Enabled] = checkQFLAG(commandAnswer, 'v');
        staticData[DESCR_LCD_Backlight_Enabled] = checkQFLAG(commandAnswer, 'x');
        staticData[DESCR_Primary_Source_Interrupt_Alarm_Enabled] = checkQFLAG(commandAnswer, 'y');
        staticData[DESCR_Record_Fault_Code_Enabled] = checkQFLAG(commandAnswer, 'z');
        refineProtocol();
        return true;
    }
    else if (protocol == PI18)
    {
        staticData.remove(DESCR_Buzzer_Enabled);
        staticData.remove(DESCR_Overload_Bypass_Enabled);
        staticData.remove(DESCR_Power_Saving_Enabled);
        staticData.remove(DESCR_LCD_Reset_To_Default_Enabled);
        staticData.remove(DESCR_Data_Log_Pop_Up);
        staticData.remove(DESCR_Overload_Restart_Enabled);
        staticData.remove(DESCR_Over_Temperature_Restart_Enabled);
        staticData.remove(DESCR_LCD_Backlight_Enabled);
        staticData.remove(DESCR_Primary_Source_Interrupt_Alarm_Enabled);
        staticData.remove(DESCR_Record_Fault_Code_Enabled);

        String commandAnswer = this->requestData("^P007FLAG");
        get.raw.qflag = commandAnswer;
        if (commandAnswer == DESCR_req_NAK || commandAnswer == DESCR_req_NOA)
        {
            return true;
        }
        if (commandAnswer == DESCR_req_ERCRC)
        {
            return false;
        }
        //[C: ^P007FLAG][CR: 190F][CC: 190F][L:  17]
        //QFLAG 1,1,0,0,0,1,1,1,0
        if (commandAnswer.length() >= 15)
        {
            staticData[DESCR_Buzzer_Enabled] = ((String)commandAnswer.charAt(0) == "1") ? true : false;
            staticData[DESCR_Overload_Bypass_Enabled] = ((String)(commandAnswer.charAt(2)) == "1") ? true : false;
            staticData[DESCR_LCD_Reset_To_Default_Enabled] = ((String)(commandAnswer.charAt(4)) == "1") ? true : false;
            staticData[DESCR_Overload_Restart_Enabled] = ((String)(commandAnswer.charAt(6)) == "1") ? true : false;
            staticData[DESCR_Over_Temperature_Restart_Enabled] = ((String)(commandAnswer.charAt(8)) == "1") ? true : false;
            staticData[DESCR_LCD_Backlight_Enabled] = ((String)(commandAnswer.charAt(10)) == "1") ? true : false;
            staticData[DESCR_Primary_Source_Interrupt_Alarm_Enabled] = ((String)(commandAnswer.charAt(12)) == "1") ? true : false;
            staticData[DESCR_Record_Fault_Code_Enabled] = ((String)(commandAnswer.charAt(14)) == "1") ? true : false;
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

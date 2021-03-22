String sendHTMLmqttsettings()
{
String ptr = HTMLhead();
    ptr += "<h1>Firmware Update</h1>\n";

    ptr += "<form method=\"POST\" action=\"/mqttsettingssave\" enctype=\"multipart/form-data\">\n";
    ptr += "<input class=\"btn btn-primary btn-block\" type=\"text\" name=\"mqtt_server\" value=\"_settings._mqttUser\">\n";//müsste noch als input statt btn 

    ptr += "<input class=\"btn btn-primary btn-block\" type=\"submit\" value=\"Save\">\n";
    ptr += "</form>\n";
    ptr += "<br/>\n";
    ptr += "<a class=\"btn btn-primary btn-block\" href=\"/\" role=\"button\">Main Page</a>\n";

    ptr +=HTMLfoot();
    return ptr;
}
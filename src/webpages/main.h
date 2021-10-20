String sendHTMLmain()
{
String ptr = HTMLhead();
    ptr += "<h1>Solar Inverter</h1>\n";

    ptr += "<p>\n";
    ptr += "<a class=\"btn btn-primary btn-block\" href=\"/livedata\" role=\"button\">Live Inverter Data</a>\n";
    ptr += "<br/>";
    ptr += "<a class=\"btn btn-primary btn-block\" href=\"/settings\" role=\"button\">Settings</a>\n";
    ptr += "</p>\n";

    ptr +=HTMLfoot();
    return ptr;
}
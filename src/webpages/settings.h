String sendHTMLsettings()
{
String ptr = HTMLhead();
    ptr += "<h1>Firmware Update</h1>\n";

    ptr += "<form method=\"POST\" action=\"/update\" enctype=\"multipart/form-data\">\n";
    ptr += "<input class=\"btn btn-primary\" type=\"file\" name=\"update\">\n";
    ptr += "<br/>\n";
    ptr += "<input class=\"btn btn-primary\" type=\"submit\" value=\"Update\">\n";
    ptr += "</form>\n";
    ptr += "<br/>\n";
    ptr += "<a class=\"btn btn-primary\" href=\"/reboot\" role=\"button\">Reboot</a>\n";
    ptr += "<br/>\n";
    ptr += "<a class=\"btn btn-primary\" href=\"/\" role=\"button\">Main Page</a>\n";

    ptr +=HTMLfoot();
    return ptr;
}
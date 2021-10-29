const char HTML_SETTINGS[] PROGMEM ="<figure class=\"text-center\"><h1>Settings</h1></figure>\n"
                                    "<div class=\"d-grid gap-2\">\n"
                                    "<form method=\"POST\" action=\"/update\" enctype=\"multipart/form-data\">\n"
                                    "<div class=\"input-group\">\n"
                                    "<input class=\"form-control\" id=\"inputGroupFile04\" aria-describedby=\"inputGroupFileAddon04\" aria-label=\"Upload\" type=\"file\" name=\"update\">"
                                    "<input class=\"btn btn-outline-secondary\" id=\"inputGroupFileAddon04\" type=\"submit\" value=\"Update\">\n"
                                    "</div>\n"
                                    "</form>\n"
                                    "<a class=\"btn btn-primary\" href=\"/settingsedit\" role=\"button\">Cofigure</a>\n"
                                    "<a class=\"btn btn-warning\" href=\"/reboot\" role=\"button\">Reboot</a>\n"
                                    "<a class=\"btn btn-primary\" href=\"/confirmreset\" role=\"button\">Reset ESP</a>\n"
                                    "<a class=\"btn btn-primary\" href=\"/\" role=\"button\">Back</a>\n"
                                    "</div>\n";
String sendHTMLsettings()
{
String ptr = HTMLhead();
    ptr += FPSTR(HTML_SETTINGS);
    ptr +=HTMLfoot();
    return ptr;
}

const char HTML_CONFIRMRESET[] PROGMEM ="<figure class=\"text-center\"><h1>Erease all Data?</h1></figure>\n"
                                    "<div class=\"d-grid gap-2\">\n"                                    
                                    "<a class=\"btn btn-danger\" href=\"/reset\" role=\"button\">Yes</a>\n"
                                    "<a class=\"btn btn-primary\" href=\"/settings\" role=\"button\">No</a>\n"
                                    "</div>\n";
String sendHTMLconfirmReset()
{
    String ptr = HTMLhead();
    ptr += FPSTR(HTML_CONFIRMRESET);
    ptr +=HTMLfoot();
    return ptr;
}
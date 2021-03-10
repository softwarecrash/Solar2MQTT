String HTMLhead(){

    String ptrHead = "<!DOCTYPE html> <html>\n";
    ptrHead += "<head>\n";
    ptrHead += "<meta http-equiv=\"content-type\" content=\"text/html;charset=UTF-8\">\n";
    ptrHead += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\n";
    ptrHead += "<link rel=\"icon\" type=\"image/png\" href=\"data:image/png;base64,iVBORw0KGgo=\">";
    ptrHead += "<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.4.1/css/bootstrap.min.css\">\n";
    ptrHead += "<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js\"></script>\n";
    ptrHead += "<script src=\"https://maxcdn.bootstrapcdn.com/bootstrap/3.4.1/js/bootstrap.min.js\"></script>\n";
    ptrHead += "<title>Solar 2 MQTT</title>\n";
    ptrHead += "</head>\n";
    ptrHead += "<body>\n";
    ptrHead += "<div class=\"container col-md-2 col-md-offset-5\">\n";
    return ptrHead;
}

String HTMLfoot(){

    String ptrFood = "";
    ptrFood += "<br/><br/>\n";
    ptrFood += "By <a href=\"https://github.com/softwarecrash/\" target=\"_blank\">Softwarecrash</a>\n";
    ptrFood += "</div>\n";
    ptrFood += "</body>\n";
    ptrFood += "</html>\n";
    return ptrFood;
}


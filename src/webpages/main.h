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

/*
<html>

<head>
    <meta http-equiv="content-type" content="text/html;charset=UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <link rel="stylesheet" href="https://maxcdn.bootstrapcdn.com/bootstrap/3.4.1/css/bootstrap.min.css">
    <script src="https://ajax.googleapis.com/ajax/libs/jquery/3.3.1/jquery.min.js"></script>
    <script src="https://maxcdn.bootstrapcdn.com/bootstrap/3.4.1/js/bootstrap.min.js"></script>
    <title>Hello from {{ hostname}} HTTP Server ESP8266 {"test"} - Page1</title>
</head>

<body>
    <div class="container">
        <h1>ESP8266 HTTP Server</h1>
        <p>Hello from ESP8266 HTTP Server. This is Page1.</p>
        <p>
            <a class="btn btn-primary disabled" href="/" role="button">Page1</a>
            <a class="btn btn-primary" href="/page2" role="button">Page2</a>
            <a class="btn btn-primary" href="/page3" role="button">Page3</a>
        </p>

        <p>Copyright (C) 2019 <a href="https://www.arduinoslovakia.eu">Arduino Slovakia</a>.</p>
    </div>
</body>

</html>
*/
String sendHTMLlive()
{
  String ptr = HTMLhead();

  ptr += "<h1>Solar Inverter Live Data</h1>\n";

  ptr += "<p>Solar In: ";
  ptr += "<span id=\"solarV\">";
  ptr += "</span>";
  ptr += "V | ";
  ptr += "<span id=\"solarA\">";
  ptr += "</span>";
  ptr += "A</p>\n";

  ptr += "<p>Grid In: ";
  ptr += "<span id=\"gridV\">";
  ptr += "</span>";
  ptr += "V | ";
  ptr += "<span id=\"gridHz\">";
  ptr += "</span>";
  ptr += "Hz</p>\n";

  ptr += "<p>AC Out: ";
  ptr += "<span id=\"acOutV\">";
  ptr += "</span>";
  ptr += "V | ";
  ptr += "<span id=\"acOutHz\">";
  ptr += "</span>";
  ptr += "Hz</p>\n";

  ptr += "<p>AC Load: ";
  ptr += "<span id=\"acOutVa\">";
  ptr += "</span>";
  ptr += "Va | ";
  ptr += "<span id=\"acOutW\">";
  ptr += "</span>";
  ptr += "W | ";
  ptr += "<span id=\"acOutPercent\">";
  ptr += "</span>";
  ptr += "%</p>\n";

  ptr += "<p>Bus : ";
  ptr += "<span id=\"busV\">";
  ptr += "</span>";
  ptr += "V</p>\n";

  ptr += "<p>Temperature: ";
  ptr += "<span id=\"heatSinkDegC\">";
  ptr += "</span>";
  ptr += "°C</p>\n";

  ptr += "<p>Battery: ";
  ptr += "<span id=\"battV\">";
  ptr += "</span>";
  ptr += "V | ";
  ptr += "<span id=\"battPercent\">";
  ptr += "</span>";
  ptr += "%</p>\n";

  ptr += "<p>SCC | In | Out: ";
  ptr += "<span id=\"sccBattV\">";
  ptr += "</span>";
  ptr += "V | ";
  ptr += "<span id=\"battChargeA\">";
  ptr += "</span>";
  ptr += "A | ";
  ptr += "<span id=\"battDischargeA\">";
  ptr += "</span>";
  ptr += "A</p>\n";

  ptr += "<p>";
  ptr += "<a class=\"btn btn-primary\" href=\"/\" role=\"button\">Main Page</a>\n";
  ptr += "</p>";

  ptr += "<script>\n";
  ptr += "        $(document).ready(function(load) {\n";
  ptr += "         function fetch() {\n";
  ptr += "        $.ajax({\n";
  ptr += "            url: \"livedataAjax\",\n";
  ptr += "            data: {},\n";
  ptr += "            type: \"get\",\n";
  ptr += "            dataType: \"json\",\n";
  ptr += "               cache: false,\n";
  ptr += "                success: function (data) {\n";
  ptr += "               document.getElementById(\"solarV\").innerHTML = data.solarV;\n";
  ptr += "               document.getElementById(\"solarA\").innerHTML = data.solarA;\n";
  ptr += "               document.getElementById(\"gridV\").innerHTML = data.gridV;\n";
  ptr += "               document.getElementById(\"gridHz\").innerHTML = data.gridHz;\n";
  ptr += "               document.getElementById(\"acOutV\").innerHTML = data.acOutV;\n";
  ptr += "               document.getElementById(\"acOutHz\").innerHTML = data.acOutHz;\n";
  ptr += "               document.getElementById(\"acOutVa\").innerHTML = data.acOutVa;\n";
  ptr += "               document.getElementById(\"acOutW\").innerHTML = data.acOutW;\n";
  ptr += "               document.getElementById(\"acOutPercent\").innerHTML = data.acOutPercent;\n";
  ptr += "               document.getElementById(\"busV\").innerHTML = data.busV;\n";
  ptr += "               document.getElementById(\"heatSinkDegC\").innerHTML = data.heatSinkDegC;\n";
  ptr += "               document.getElementById(\"battV\").innerHTML = data.battV;\n";
  ptr += "               document.getElementById(\"battPercent\").innerHTML = data.battPercent;\n";
  ptr += "               document.getElementById(\"battChargeA\").innerHTML = data.battChargeA;\n";
  ptr += "               document.getElementById(\"battDischargeA\").innerHTML = data.battDischargeA;\n";
  ptr += "               document.getElementById(\"sccBattV\").innerHTML = data.sccBattV\n";
  ptr += "            }\n";
  ptr += "        });\n";
  ptr += "        }\n";
  ptr += "        setInterval(fetch, 1000);\n";
  ptr += "        fetch();\n";
  ptr += "        });\n";
  ptr += "</script>\n";

  ptr += HTMLfoot();
  return ptr;
}
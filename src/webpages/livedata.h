String sendHTMLlive()
{
  String ptr = HTMLhead();

  ptr += "<h1>Solar Live Data</h1>\n";

  ptr += "<p>Solar In: ";
  ptr += "<span id=\"solarV\">N/A</span>";
  ptr += "V | ";
  ptr += "<span id=\"solarA\">N/A</span>";
  ptr += "A | ";
  ptr += "<span id=\"solarW\">N/A</span>";
  ptr += "W</p>\n";

  ptr += "<p>Grid In: ";
  ptr += "<span id=\"gridV\">N/A</span>";
  ptr += "V | ";
  ptr += "<span id=\"gridHz\">N/A</span>";
  ptr += "Hz</p>\n";

  ptr += "<p>AC Out: ";
  ptr += "<span id=\"acOutV\">N/A</span>";
  ptr += "V | ";
  ptr += "<span id=\"acOutHz\">N/A</span>";
  ptr += "Hz</p>\n";

  ptr += "<p>AC Load: ";
  ptr += "<span id=\"acOutVa\">N/A</span>";
  ptr += "Va | ";
  ptr += "<span id=\"acOutW\">N/A</span>";
  ptr += "W | ";
  ptr += "<span id=\"acOutPercent\">N/A</span>";
  ptr += "%</p>\n";

  ptr += "<p>Bus : ";
  ptr += "<span id=\"busV\">N/A</span>";
  ptr += "V</p>\n";

  ptr += "<p>Temperature: ";
  ptr += "<span id=\"heatSinkDegC\">N/A</span>";
  ptr += "°C</p>\n";

  ptr += "<p>Battery: ";
  ptr += "<span id=\"battV\">N/A</span>";
  ptr += "V | ";
  ptr += "<span id=\"battPercent\">N/A</span>";
  ptr += "% | cSOC: ";
  ptr += "<span id=\"cSOC\">N/A</span>";
  ptr += "%</p>\n";

  ptr += "<p>SCC: <span id=\"sccBattV\">N/A</span>V | In: <span id=\"battChargeA\">N/A</span>A | Out: <span id=\"battDischargeA\">N/A</span>A</p>";
  ptr += "\n";

  ptr += "<p>Inverter Mode: <span id=\"ivmode\">N/A</span></p>";
  ptr += "\n";

  ptr += "<p>";
  ptr += "<a class=\"btn btn-primary btn-block\" href=\"/\" role=\"button\">Main Page</a>\n";
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
  ptr += "               document.getElementById(\"solarW\").innerHTML = data.solarW;\n";
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
  ptr += "               document.getElementById(\"cSOC\").innerHTML = data.cSOC\n";
  ptr += "               document.getElementById(\"ivmode\").innerHTML = data.iv_mode\n";
  ptr += "            }\n";
  ptr += "        });\n";
  ptr += "        }\n";
  ptr += "        setInterval(fetch, 5000);\n";
  ptr += "        fetch();\n";
  ptr += "        });\n";
  ptr += "</script>\n";

  ptr += HTMLfoot();
  return ptr;
}
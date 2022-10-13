const char HTML_MAIN[] PROGMEM = R"rawliteral(
<figure class="text-center"><h2 id="devicename"></h2></figure>
<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Solar In: </div>
</div>
<div class="col">
<div class="bg-light"><span id="solarV" >N/A</span><span id="solarA" >N/A</span><span id="solarW" >N/A</span></div>
</div></div><div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Grid In: </div>
</div>
<div class="col">
<div class="bg-light"><span id="gridV">N/A</span><span id="gridHz">N/A</span></div>
</div>
</div>
<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">AC Out: </div>
</div>
<div class="col">
<div class="bg-light"><span id="acOutV">N/A</span><span id="acOutHz">N/A</span></div>
</div>
</div>
<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">AC Load: </div>
</div>
<div class="col">
<div class="bg-light"><span id="acOutVa">N/A</span><span id="acOutW">N/A</span><span id="acOutPercent">N/A</span></div>
</div>
</div>
<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Bus: </div>
</div>
<div class="col">
<div class="bg-light"><span id="busV">N/A</span></div>
</div>
</div>
<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Temperature: </div>
</div>
<div class="col">
<div class="bg-light"><span id="heatSinkDegC">N/A</span></div>
</div>
</div>
<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Battery: </div>
</div>
<div class="col">
<div class="bg-light"><span id="battV">N/A</span><span id="battPercent">N/A</span><span id="cSOC">N/A</span></div>
</div>
</div>
<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">SCC | IN | Out: </div>
</div>
<div class="col">
<div class="bg-light"><span id="sccBattV">N/A</span><span id="battChargeA">N/A</span><span id="battDischargeA">N/A</span></div>
</div>
</div>
<div class="row gx-0 mb-2">
<div class="col">
<div class="bg-light">Inverter Mode: </div>
</div>
<div class="col">
<div class="bg-light"><span id="ivmode">N/A</span></div>
</div>
</div>
<div class="d-grid gap-2">
<a class="btn btn-primary btn-block" href="/settings" role="button">Settings</a>
</div>
<script>
        $(document).ready(function(load) {
         function fetch() {
        $.ajax({
            url: "livejson",
            data: {},
            type: "get",
            dataType: "json",
               cache: false,
                success: function (data) {
                $("#solarV").html(data.solarV+'V ');
                $("#solarA").html(data.solarA+'A  ');
                $("#solarW").html(data.solarW+'W');
                $("#gridV").html(data.gridV+'V ');
                $("#gridHz").html(data.gridHz+'Hz ');
                $("#acOutV").html(data.acOutV+'V ');
                $("#acOutHz").html(data.acOutHz+'Hz ');
                $("#acOutVa").html(data.acOutVa+'Va ');
                $("#acOutW").html(data.acOutW+'W ');
                $("#acOutPercent").html(data.acOutPercent+'%');
                $("#busV").html(data.busV+'V ');
                $("#heatSinkDegC").html(data.heatSinkDegC+'°C ');
                $("#battV").html(data.battV+'V ');
                $("#battPercent").html(data.battPercent+'% ');
                $("#battChargeA").html(data.battChargeA+'A ');
                $("#battDischargeA").html(data.battDischargeA+'A ');
                $("#sccBattV").html(data.sccBattV+'V ');
                $("#cSOC").html(data.cSOC+'% ');
                $("#ivmode").html(data.iv_mode);
                $("#devicename").html(data.device_name);
            }
        });
        }
        setInterval(fetch, 5000);
        fetch();
        });
</script>
)rawliteral";

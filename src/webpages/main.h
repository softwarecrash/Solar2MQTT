const char HTML_MAIN[] PROGMEM = R"rawliteral(
    %HEAD_TEMPLATE%
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
<div class="bg-light"><span id="battV">N/A</span><span id="battPercent">N/A</span></div>
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
               document.getElementById("solarV").innerHTML = data.solarV+'V ';
               document.getElementById("solarA").innerHTML = data.solarA+'A  ';
               document.getElementById("solarW").innerHTML = data.solarW+'W';
               document.getElementById("gridV").innerHTML = data.gridV+'V ';
               document.getElementById("gridHz").innerHTML = data.gridHz+'Hz ';
               document.getElementById("acOutV").innerHTML = data.acOutV+'V ';
               document.getElementById("acOutHz").innerHTML = data.acOutHz+'Hz ';
               document.getElementById("acOutVa").innerHTML = data.acOutVa+'Va ';
               document.getElementById("acOutW").innerHTML = data.acOutW+'W ';
               document.getElementById("acOutPercent").innerHTML = data.acOutPercent+'%%';
               document.getElementById("busV").innerHTML = data.busV+'V ';
               document.getElementById("heatSinkDegC").innerHTML = data.heatSinkDegC+'°C ';
               document.getElementById("battV").innerHTML = data.battV+'V ';
               document.getElementById("battPercent").innerHTML = data.battPercent+'%% ';
               document.getElementById("battChargeA").innerHTML = data.battChargeA+'A ';
               document.getElementById("battDischargeA").innerHTML = data.battDischargeA+'A ';
               document.getElementById("sccBattV").innerHTML = data.sccBattV+'V ';
               document.getElementById("ivmode").innerHTML = data.iv_mode;
               document.getElementById("devicename").innerHTML = 'Device: '+data.device_name;
            }
        });
        }
        setInterval(fetch, 5000);
        fetch();
        });
</script>
%FOOT_TEMPLATE%
)rawliteral";
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
$(document).ready(function () {
        initWebSocket();
        });
    var gateway = `ws://${window.location.host}/ws`;
    var websocket;
    function initWebSocket() {
        console.log('Trying to open a WebSocket connection...');
        websocket = new WebSocket(gateway);
        websocket.onopen = onOpen;
        websocket.onclose = onClose;
        websocket.onerror = onError;
        websocket.onmessage = onMessage;
    }
    function onOpen(event) {
        console.log('Connection opened');
    }
    function onClose(event) {
        console.log('Connection closed');
        setTimeout(initWebSocket, 3000);
    }
    function onError(event) {
        console.log('Connection lost');
    }
    function onMessage(event) {
        var data = JSON.parse(event.data);
        document.getElementById("solarV").innerHTML = data.DeviceData.solarV+'V ';
        document.getElementById("solarA").innerHTML = data.DeviceData.solarA+'A  ';
        document.getElementById("solarW").innerHTML = data.DeviceData.solarW+'W';
        document.getElementById("gridV").innerHTML = data.DeviceData.gridV+'V ';
        document.getElementById("gridHz").innerHTML = data.DeviceData.gridHz+'Hz ';
        document.getElementById("acOutV").innerHTML = data.DeviceData.acOutV+'V ';
        document.getElementById("acOutHz").innerHTML = data.DeviceData.acOutHz+'Hz ';
        document.getElementById("acOutVa").innerHTML = data.DeviceData.acOutVa+'Va ';
        document.getElementById("acOutW").innerHTML = data.DeviceData.acOutW+'W ';
        document.getElementById("acOutPercent").innerHTML = data.DeviceData.acOutPercent+'%%';
        document.getElementById("busV").innerHTML = data.DeviceData.busV+'V ';
        document.getElementById("heatSinkDegC").innerHTML = data.DeviceData.heatSinkDegC+'°C ';
        document.getElementById("battV").innerHTML = data.DeviceData.battV+'V ';
        document.getElementById("battPercent").innerHTML = data.DeviceData.battPercent+'%% ';
        document.getElementById("battChargeA").innerHTML = data.DeviceData.battChargeA+'A ';
        document.getElementById("battDischargeA").innerHTML = data.DeviceData.battDischargeA+'A ';
        document.getElementById("sccBattV").innerHTML = data.DeviceData.sccBattV+'V ';
        document.getElementById("ivmode").innerHTML = data.DeviceData.iv_mode;
        document.getElementById("devicename").innerHTML = data.DeviceData.device_name;
    }

</script>
%FOOT_TEMPLATE%
)rawliteral";
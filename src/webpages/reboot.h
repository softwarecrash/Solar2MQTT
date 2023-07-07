/*
Solar2MQTT Project
https://github.com/softwarecrash/Solar2MQTT
*/
const char HTML_REBOOT[] PROGMEM = R"rawliteral(
%HEAD_TEMPLATE%
<figure class="text-center"><h1>Rebooting </h1><h2 id="wait">.</h2></figure>
<div class="d-grid gap-2">
<a class="btn btn-primary" href="/" role="button">Main</a>
</div>

<script>
$(document).ready(function () {
    window.dotsGoingUp = true;
    var dots = window.setInterval( function() {
        var wait = document.getElementById("wait");
        if ( window.dotsGoingUp ) 
            wait.innerHTML += ".";
        else {
            wait.innerHTML = wait.innerHTML.substring(1, wait.innerHTML.length);
            if ( wait.innerHTML === ".")
                window.dotsGoingUp = true;
        }
        if ( wait.innerHTML.length > 9 )
            window.dotsGoingUp = false;
        }, 100);

setTimeout(startCheck, 2000);

function startCheck(){
  setInterval(serverReachable, 1000);
}

function serverReachable() {
$.get("/").done(function () {
  window.location = "/";
  console.log("success");
}).fail(function () {
   console.log("failed.");
});
}

});
</script>

%FOOT_TEMPLATE%
)rawliteral";

const char HTML_REBOOT[] PROGMEM = R"rawliteral(
    %HEAD_TEMPLATE%
<figure class="text-center"><h1>Rebooting ...</h1></figure>
<div class="d-grid gap-2">
<a class="btn btn-primary" href="/" role="button">Main</a>
</div>

<script>
$(document).ready(function () {

setTimeout(serverReachable, 1000);

//event naviagor.onLine isn't fiable.
function serverReachable() {
  // IE vs. standard XHR creation
  var x = new ( window.ActiveXObject || XMLHttpRequest )( "Microsoft.XMLHTTP" ),
      s;
  x.open(
    // requesting the headers is faster, and just enough
    "HEAD",
    // append a random string to the current hostname,
    // to make sure we're not hitting the cache
    "//" + window.location.hostname + "/?rand=" + Math.random(),
    // make a synchronous request
    false
  );
  try {
    x.send();
    s = x.status;
    // Make sure the server is reachable
    if ( s >= 200 && s < 300 || s === 304 )
    {
        console.log('site reachable')
    } else 
    {
        console.log('site unreachable')
        console.log(s)
    }
    return ( s >= 200 && s < 300 || s === 304 );
  // catch network & other problems
  } catch (e) {
    console.log(e)
    return false;
  }
}
}
</script>

%FOOT_TEMPLATE%
)rawliteral";

const hostname = 'http://10.128.64.117:3031';

setTimeout(() => {
    var url = hostname + "/mediaManobrista1"; // Endpoint
    var xhttp = new XMLHttpRequest(); // Create instance
    xhttp.open("GET", url, false); // Make a get request
    xhttp.send();//The script's execution stops here until the server returns the requirements 
    response = JSON.parse(xhttp.responseText); // Saves what comes back from the request
    document.getElementById("averageTime1").innerHTML = response[0].tempoIda;
    console.log(response);

}, 1000)
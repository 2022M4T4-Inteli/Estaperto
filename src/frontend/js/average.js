const hostname = 'http://10.128.65.112:3031';

function getDriver(listNumber){
    if (listNumber == 2) {
        var img = document.querySelector("#img");
        img.setAttribute('src', '../assets/iconDriver.png');

        // Change attributes to change the driver in the side panel and show driver 2
        document.querySelector("#name").textContent = "Carlos Eduardo";
        document.querySelector("#estacio").textContent = "Arena Allianz Park";
        document.querySelector("#averageTime1").setAttribute('id', 'averageTime2');

        //function to get the time from driver 2 (back time)
        getTimeDriver2();
    }
    else if (listNumber == 1) {
        var img = document.querySelector("#img");
        img.setAttribute('src', '../assets/iconDriver2.png');
        
        // Change attributes to change the driver in the side panel and show driver 1
        document.querySelector("#name").textContent = "Luiz Felipe Kama";
        document.querySelector("#estacio").textContent = "Aero Congonhas";
        document.querySelector("#averageTime2").setAttribute('id', 'averageTime1');

        //function to get the time from driver 1 (go time)
        getTimeDriver1();
    }
};

// Insert into the report in the average time of the driver 2
function getTimeDriver2(){
    setTimeout(() => {
        var url = hostname + "/mediaManobrista2"; // Endpoint
        var xhttp = new XMLHttpRequest(); // Create instance
        xhttp.open("GET", url, false); // Make a get request
        xhttp.send();//The script's execution stops here until the server returns the requirements 
        response = JSON.parse(xhttp.responseText); // Saves what comes back from the request
        document.getElementById("averageTime2").innerHTML = response[0].tempoVolta + " min";
    }, 1000)
}

// Insert into the report in the average time of the driver 1
function getTimeDriver1() {
    url = hostname + "/mediaManobrista1";
    fetch(url)
    .then(response => response.json())
    .then(data => averageTime1.textContent=data[0].tempoIda + " min")
  }

getTimeDriver1();


var response;
var plateList= [];
var timeList = [];
var responseList = [];
var hostname = "http://10.128.65.55:3031"

// Every second this will run
setInterval(function(){
  var url = hostname + "/retornoTotem"; // Endpoint
  var xhttp = new XMLHttpRequest(); // Create instance
  xhttp.open("GET", url, false); // Make a get request
  xhttp.send();//The script's execution stops here until the server returns the requirements 
  response = JSON.parse(xhttp.responseText); // Saves what comes back from the request

  // Creating head of table
  document.getElementById('table').innerHTML = `<tr>
  <th>Posição</th>
  <th>Placa</th>
  <th>Tempo</th>
  </tr>`;

  // For each plate, we create a row
  for(var i = 0; i<Object.keys(response).length; i++){
    document.getElementById('table').innerHTML += '<tr><td><p>' + (i + 1) + '</p></td><td><p>'+ response[i].placa + '</p></td><td><p>' + response[i].tempoEstimado + ' min</p></td></tr>'
  }

  // Subtract a minute from each plate
  subtractMinute();
},  1000);

function subtractMinute(){

  // For each plate
  for(var i = 0; i<Object.keys(response).length; i++){

    // Check if estimated time is less than zero, and if it is, remove from the totem
    if(response[i].tempoEstimado < 0){
      var url = hostname + "/deleteZero";
      var xhttp = new XMLHttpRequest();
      xhttp.open("POST", url, true);
      xhttp.setRequestHeader('Content-type', 'application/json');
      xhttp.send(JSON.stringify([plateList[i]]));
    }
    
    // Else create a time for this plate if it doesn't already have one and includes in the list with all the current plates on display
    else{
      if(!responseList.includes(response[i].placa)){
        responseList.push(response[i].placa);    // Pushes to our list with the response plates
      }
      
      if(!plateList.includes(response[i].placa)){
        plateList.push(response[i].placa);  // Pushes to our list with all the current plates on display
        timeList.push(0); // Creates a time for this plate
      }
    }
  }
  
  var counter = 0;

  // For each plate in plate List
  for(var i = 0; i<plateList.length; i++){
    counter++;

    // If response List doesn't have this plate, remove the plate from our list with all the current plates on display and from the list with times
    if(!responseList.includes(plateList[i])){
      plateList.splice(counter, 1);
      timeList.splice(counter, 1);
    }
  }

  var url = hostname + "/subtractMinute"; // Endpoint URL for subtracting a minute from database

  // Every second execute what is inside
  setTimeout(() => {
    // For each time in the list
    for(var i = 0; i<timeList.length; i++){
      // If the time is 59 or bigger, we take a minute out from that plate on the database
      if(timeList[i] >= 59){
        var xhttp = new XMLHttpRequest();
        xhttp.open("POST", url, true);
        xhttp.setRequestHeader('Content-type', 'application/json');
        xhttp.send(JSON.stringify([plateList[i]])); // Sends the plate in a list format
        timeList[i] = 0; // Set that time to zero so the counting can restart
      }else{
        timeList[i]++; // Else, add one to the time
      }
    }
  }, 1000)
}

// Insert into the report in the average time of the driver
function getTimeDriver1() {
  url = hostname + "/mediaManobrista1";
  fetch(url)
  .then(response => response.json())
  .then(data => averageTime1.textContent=data[0].tempoIda)
}

getTimeDriver1();

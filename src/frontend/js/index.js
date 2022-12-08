var response;
var plateList= [];
var timeList = [];
var responseList = [];

// Every second this will run
setInterval(function(){
  var url = "http://10.128.65.251:3031/retornoTotem"; // Endpoint
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
  for(var i = 0; i<Object.keys(response).length; i++){

    if(!responseList.includes(response[i].placa)){
      responseList.push(response[i].placa);    
    }
    
    if(!plateList.includes(response[i].placa)){
      plateList.push(response[i].placa);
      timeList.push(0);
    }
  }
  
  var counter = 0;
  for(var i = 0; i<plateList.length; i++){
    counter++;

    if(!responseList.includes(plateList[i])){
      plateList.splice(counter, 1);
      timeList.splice(counter, 1);
    }
  }

  var url = "http://10.128.65.251:3031/subtractMinute";
  setTimeout(() => {
    for(var i = 0; i<timeList.length; i++){
      if(timeList[i] >= 59){
        var xhttp = new XMLHttpRequest();
        xhttp.open("POST", url, true);
        xhttp.setRequestHeader('Content-type', 'application/json');
        console.log(JSON.stringify(plateList[i]));
        xhttp.send(JSON.stringify([plateList[i]]));
        timeList[i] = 0;
      }else{
        timeList[i]++;
      }
    }
  }, 1000)
  console.log(timeList);
}

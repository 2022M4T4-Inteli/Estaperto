var response, plateList, timeList, responseList;

setInterval(function(){
  var url = "http://10.128.65.251:3031/retornoTotem";
  var xhttp = new XMLHttpRequest();
  xhttp.open("GET", url, false);
  xhttp.send();//The script's execution stops here until the server returns the requirements 
  response = JSON.parse(xhttp.responseText);

  // console.log(xhttp.responseText)

  document.getElementById('table').innerHTML = `<tr>
  <th>Posição</th>
  <th>Placa</th>
  <th>Tempo</th>
  </tr>`;

  for(var i = 0; i<Object.keys(response).length; i++){
    document.getElementById('table').innerHTML += '<tr><td><p>' + (i + 1) + '</p></td><td><p>'+ response[i].placa + '</p></td><td><p>' + response[i].tempoEstimado + ' min</p></td></tr>'
  }
  subtractMinute();
},  1000);

function subtractMinute(){
  for(var i = 0; i<Object.keys(response).length; i++){
    responseList.push(response.placa[i]);
    if(!plateList.includes(response.placa[i])){
      plateList.push(response.placa[i]);
      timeList.push(0);
    }
  }

  var counter = 0;
  for(plate in plateList){
    counter++;
    if(!responseList.includes(plate)){
      plateList.splice(counter, 1);
      timeList.splice(counter, 1);
    }
  }

  var url = "http://10.128.65.251:3031/subtractMinute";
  for(var i = 0; i<timeList.length; i++){
    if(timeList[i] >= 59){
      var xhttp = new XMLHttpRequest();
      xhttp.open("POST", url, false);
      xhttp.setRequestHeader('Content-type', 'application/json');
      xhttp.send(JSON.stringify(plateList[i]));//The script's execution stops here until the server returns the requirements 
      timeList[i] = 0
    }else{
      timeList[i]++;
    }
  }
}
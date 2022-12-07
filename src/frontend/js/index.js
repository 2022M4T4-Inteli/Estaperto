var response, plateList, counter;

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
},  10000);

function subtractMinute(){
  // for(var i = 0; i<Object.keys(response).length; i++){
  //   if(!plateList.includes(response.placa[i])){
  //     plateList.push(response.placa[i]);
  //   }
  // }
  if(counter == 60){
    var url = "http://10.128.65.251:3031/subtractMinute";
    var xhttp = new XMLHttpRequest();
    xhttp.open("POST", url, false);
    xhttp.send();//The script's execution stops here until the server returns the requirements 
  }else{
    counter++;
  }
}
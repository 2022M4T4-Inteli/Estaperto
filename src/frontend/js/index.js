setInterval(function(){
    var url = "http://10.128.65.251:3031/retornoTotem";
    var xhttp = new XMLHttpRequest();
    xhttp.open("GET", url, false);
    xhttp.send();//The script's execution stops here until the server returns the requirements 
    var response = JSON.parse(xhttp.responseText);

    console.log(xhttp.responseText)

    document.getElementById('table').innerHTML = `<tr>
    <th>Posição</th>
    <th>Placa</th>
    <th>Tempo</th>
  </tr>`

    for(var i = 0; i<Object.keys(response).length; i++){
        document.getElementById('table').innerHTML += '<tr><td><p>' + (i + 1) + '</p></td><td><p>'+ response[i].placa + '</p></td><td><p>' + response[i].tempoEstimado + ' min</p></td></tr>'
    }

},  1000);
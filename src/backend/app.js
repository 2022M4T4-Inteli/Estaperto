var WebSocketServer = require('websocket').server;
const express = require("express");
const sqlite3 = require("sqlite3");
const app = express();
const port = 3031;
const db = new sqlite3.Database("database.db")

app.use(express.static("../frontend"));

app.listen(port, () => {
  console.log(`Example app listening on port ${port}`)
})


//Cria o WebSocket server
wsServer = new WebSocketServer({
httpServer: app
});

//Chamado quando um client deseja conectar
wsServer.on('request', (request) => {
  console.log(`entrou`);
  //Aceita a conexão do client
  let client = request.accept(null, request.origin);
  console.log(`aceitou`);

  //Chamado quando o client envia uma mensagem
  client.on('message', (message) => {
      console.log(message);
      app.get("/att", (req, res) => {
        res.json(message);
      });
  });

//Chamado quando a conexão com o client é fechada
client.on('close', () => {
    console.log("Conexão fechada");
});
});

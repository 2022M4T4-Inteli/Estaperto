const express = require("express");
const sqlite3 = require('sqlite3').verbose();
const bodyParser = require('body-parser');
const urlencodedParser = bodyParser.urlencoded({ extended: false })
const app = express();

const hostname = '10.128.65.251';
const port = 3031;
const db = new sqlite3.Database("database.db")

app.use(express.static("../frontend/pages"));
app.use(express.static("../frontend/"));

app.listen(port, hostname, () => {
  console.log(`Page server running at http://${hostname}:${port}`);
});


app.use(express.json());

// The second time that the worker uses his card

app.post("/insertRecebimento", (req, res) => {
  const infos = req.body;
  console.log(req.body);
  db.get(
    `INSERT INTO carros (placa, manobristaIda, horarioRecebimento, horarioEstacionamento, tempoEstimado) VALUES ('${infos.placa}', '${infos.manobristaIda}', '${infos.horarioRecebimento}', '${infos.horarioEstacionamento}', '${infos.tempoEstimado}')`,
    (error, response) => {
        if (error) {
          console.log(error)
        }
    }
  );
});

// Get the id for the car
app.post("/idQuery", (req, res) => {
  db.get(
    'SELECT id FROM carros WHERE placa = "' + req.body.placa + '" ORDER BY id DESC LIMIT 1',
    (error, data) => {
      res.json(data)
    }
  );
});

// Fourth time that the worker uses his card
app.post("/insertRetirada", (req, res) => {
  const infos = req.body;
  console.log(req.body);
  db.get(
    `UPDATE carros SET manobristaVolta = "${infos.manobristaVolta}", horarioRetirada = "${infos.horarioRetirada}", horarioDevolucao = "${infos.horarioDevolucao}" WHERE id = '${infos.idQuery}'`,
    (error, response) => {
        if (error) {
          console.log(error)
        }
    }
  );
  // Takes the car out of queue
  db.all(
    `DELETE FROM totem WHERE id == '${infos.idQuery}'`,
    (error, response) => {
        if (error) {
          console.log(error)
        }
    }
  );
});

// Third time the worker uses his card, this endpoint includes the car on our frontEnd
app.post("/retornoCarro", (req, res) => {
  const infos = req.body;
  db.all(
    `INSERT INTO totem (placa, tempoEstimado, id) VALUES ('${infos.placa}', '${infos.tempoEstimado}', '${infos.idQuery}')`,
    (error, data) => {
      res.json(data)
    }
  );
});

// Endpoint for our frontEnd to get what cars are already being returned
app.get("/retornoTotem", (req, res) => {
  db.all(
    'SELECT placa, tempoEstimado FROM totem ORDER BY id ASC',
    (error, data) => {
      res.json(data)
    }
  );
});

// Subtract a minute from the car
app.post("/subtractMinute", (req, res) => {
  const infos = req.body;
  db.all(
    `UPDATE totem SET tempoEstimado = tempoEstimado - 1 WHERE placa = '${infos[0]}'`,
    (error, data) => {
      if (error) {
        console.log(error)
      }
    }
  );
});

const express = require("express");
const sqlite3 = require('sqlite3').verbose();
const bodyParser = require('body-parser');
const urlencodedParser = bodyParser.urlencoded({ extended: false })
const app = express();

const hostname = '10.128.64.56';
const port = 3031;
const db = new sqlite3.Database("database.db")

app.use(express.static("../frontend"));

app.listen(port, hostname, () => {
  console.log(`Page server running at http://${hostname}:${port}`);
});


app.use(express.json());

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

app.get("/idQuery", (req, res) => {
  db.all(
    'SELECT id FROM carros WHERE placa = ' + req.body.placa + ' ORDER BY id DESC LIMIT 1',
    (error, data) => {
      res.json(data)
    }
  );
});

app.post("/insertRetirada", (req, res) => {
  const infos = req.body;
  console.log(req.body);
  db.get(
    `INSERT INTO carros (manobristaVolta, horarioRetirada, horarioDevolucao) VALUES ('${infos.manobristaVolta}', '${infos.horarioRetirada}', '${infos.horarioDevolucao}' WHERE id == '${infos.idQuery}')`,
    (error, response) => {
        if (error) {
          console.log(error)
        }
    }
  );
});

app.get("/estimatedTime", (req, res) => {
  db.all(
    'SELECT tempoEstimado FROM carros WHERE placa = ' + req.body.placa + ' ORDER BY id DESC LIMIT 1',
    (error, data) => {
      res.json(data)
    }
  );
});

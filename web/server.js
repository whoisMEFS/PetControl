const express = require('express');
const path = require('path');
const sqlite3 = require('sqlite3').verbose();
const bodyParser = require('body-parser');
const cors = require('cors');


const app = express();
const DB_PATH = path.join(__dirname, '..', 'database', 'agendpet.db');
const db = new sqlite3.Database(DB_PATH, sqlite3.OPEN_READWRITE, (err) => {
if (err) console.error('Erro abrindo DB:', err.message);
else console.log('Conectado ao banco SQLite em', DB_PATH);
});


app.use(cors());
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, 'public')));


// Carrega rotas da pasta API
require('./api/clientes')(app, db);
require('./api/planos')(app, db);
require('./api/vencimentos')(app, db);
require('./api/email')(app);


const PORT = process.env.PORT || 3000;
app.listen(PORT, () => console.log(`Servidor rodando em http://localhost:${PORT}`));


process.on('SIGINT', () => {
console.log('\nFechando DB e saindo...');
db.close();
process.exit(0);
});
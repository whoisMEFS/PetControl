module.exports = (app, db) => {
app.get('/api/clientes', (req, res) => {
db.all('SELECT * FROM clientes', [], (err, rows) => {
if (err) return res.status(500).json({ error: err.message });
res.json(rows);
});
});


app.get('/api/clientes/:id', (req, res) => {
const id = req.params.id;
db.get('SELECT * FROM clientes WHERE id = ?', [id], (err, row) => {
if (err) return res.status(500).json({ error: err.message });
if (!row) return res.status(404).json({ error: 'Cliente não encontrado' });
res.json(row);
});
});
};
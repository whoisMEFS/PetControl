// /api/vencimentos?days=7
// Lista clientes com vencimento nos próximos X dias

module.exports = (app, db) => {
    app.get('/api/vencimentos', (req, res) => {
        const days = parseInt(req.query.days || "7");

        const sql = `
            SELECT id, nome, email, telefone, cpf_cnpj, plano, vencimento
            FROM clientes
            WHERE date(vencimento) <= date('now', '+' || ? || ' days')
            ORDER BY vencimento ASC;
        `;

        db.all(sql, [days], (err, rows) => {
            if (err) return res.status(500).json({ error: err.message });
            res.json(rows);
        });
    });
};

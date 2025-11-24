// /api/planos — lista todos os planos cadastrados nos clientes

module.exports = (app, db) => {
    app.get('/api/planos', (req, res) => {
        const sql = `
            SELECT id, nome, email, telefone, cpf_cnpj, plano, vencimento
            FROM clientes
            ORDER BY id;
        `;

        db.all(sql, [], (err, rows) => {
            if (err) return res.status(500).json({ error: err.message });
            res.json(rows);
        });
    });
};

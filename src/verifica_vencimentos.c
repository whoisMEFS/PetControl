#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


// Exemplo simples: abrir sqlite via system call para rodar uma query e decidir enviar email
// Para integração completa recomendo linkar a lib sqlite3 e compilar com -lsqlite3


int main(void) {
// Query: selecionar planos com vencimento <= date('now','+3 days')
// Aqui demonstramos via system, mas ideal é usar API sqlite3.
printf("Executando verificador de vencimentos...\n");
int r = system("sqlite3 ../src/agendpet.db \".headers on \" \"SELECT id,cliente_id,vencimento FROM planos WHERE date(vencimento) <= date('now','+3 days');\"");
if (r != 0) {
printf("Erro ao rodar sqlite3 (codigo %d)\n", r);
return 1;
}


// Se quiser chamar o PowerShell para enviar email (Windows):
// system("powershell -ExecutionPolicy Bypass -File ../config/send_email.ps1 -to 'cliente@example.com' -subject 'Seu plano vence' -body '...' ");


return 0;
}
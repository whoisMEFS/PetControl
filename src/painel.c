// painel.c
// Versão com ENVIO AUTOMÁTICO DE E-MAIL INTEGRADO
// - Envia avisos de plano vencido ou a vencer
// - Usa PowerShell: config/send_email.ps1
// - Evita duplicados via email_enviados.log
// - UI moderna (Raylib 5.0)
// - Scroll + tabela + cartões + exportação CSV
//
// Compile (exemplo):
// gcc painel.c sqlite3.c -I"path\to\raylib\include" -L"path\to\raylib\lib" -lraylib -lopengl32 -lgdi32 -lwinmm -static -o PetControl.exe

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "raylib.h"
#include "sqlite3.h"

#define MAX_REG 1024

// Layout constants
#define WINDOW_W 1100
#define WINDOW_H 700
#define TOP_BAR_H 70
#define SIDE_MENU_W 220
#define ROW_HEIGHT 45
#define TABLE_X 240
#define TABLE_W 840
#define TABLE_TOP 265
#define TABLE_VISIBLE_H 360   // height area for visible rows (adjust as needed)
#define SCROLLBAR_W 8

// ------------------------------------------------------------
// Estrutura Cliente
// ------------------------------------------------------------
typedef struct {
    int id;
    char nome[128];
    char email[128];
    char telefone[64];
    char cpf_cnpj[64];
    char plano[128];
    char vencimento[16]; // YYYY-MM-DD
} Cliente;

static Cliente clientes[MAX_REG];
static int totalClientes = 0;

// Scroll state
static float scrollY = 0.0f;        // pixels scrolled from top of table
static float scrollSpeed = 30.0f;   // pixels per wheel tick

// ------------------------------------------------------------
// Helpers: clamp, safe string copy
// ------------------------------------------------------------
static float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

static void safe_strcpy(char *dst, const char *src, size_t dstsz) {
    if (!dst || dstsz == 0) return;
    if (!src) { dst[0] = '\0'; return; }
    strncpy(dst, src, dstsz - 1);
    dst[dstsz - 1] = '\0';
}

// ------------------------------------------------------------
// Calcular dias restantes
// ------------------------------------------------------------
int diasRestantes(const char *dataVenc) {
    if (!dataVenc || strlen(dataVenc) < 10) return 99999;
    int ano, mes, dia;
    if (sscanf(dataVenc, "%d-%d-%d", &ano, &mes, &dia) != 3) return 99999;

    struct tm tmv = {0};
    tmv.tm_year = ano - 1900;
    tmv.tm_mon = mes - 1;
    tmv.tm_mday = dia;
    tmv.tm_hour = 12;

    time_t t_venc = mktime(&tmv);
    if (t_venc == (time_t)-1) return 99999;
    time_t now = time(NULL);

    double diff = difftime(t_venc, now);
    int dias = (int)(diff / 86400.0);
    return dias;
}

// ------------------------------------------------------------
// CSV escape (duplica aspas)
// ------------------------------------------------------------
static void csv_escape_and_write(FILE *f, const char *s) {
    if (!f) return;
    if (!s) { fputs("\"\"", f); return; }
    fputc('"', f);
    for (const char *p = s; *p; ++p) {
        if (*p == '"') fputc('"', f); // duplica aspas
        fputc(*p, f);
    }
    fputc('"', f);
}

// ------------------------------------------------------------
// Validação simples de e-mail
// ------------------------------------------------------------
static int email_valido(const char *e) {
    if (!e) return 0;
    const char *at = strchr(e, '@');
    if (!at) return 0;
    const char *dot = strchr(at, '.');
    if (!dot) return 0;
    return 1;
}

// ------------------------------------------------------------
// LOG: verificar se já foi enviado
// ------------------------------------------------------------
int emailJaEnviado(int id) {
    FILE *f = fopen("email_enviados.log", "r");
    if (!f) return 0; // arquivo inexistente => não enviado

    int idl;
    char data[64];
    while (fscanf(f, "%d;%63s", &idl, data) == 2) {
        if (idl == id) {
            fclose(f);
            return 1; // já enviado
        }
    }
    fclose(f);
    return 0;
}

// ------------------------------------------------------------
// LOG: registrar envio
// ------------------------------------------------------------
void registrarEnvioEmail(int id) {
    FILE *f = fopen("email_enviados.log", "a");
    if (!f) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    char data[32];
    sprintf(data, "%04d-%02d-%02d",
        t->tm_year + 1900,
        t->tm_mon + 1,
        t->tm_mday
    );

    fprintf(f, "%d;%s\n", id, data);
    fclose(f);
}

// ------------------------------------------------------------
// Sanitize argumento para PowerShell (remove aspas duplas)
// substitui " por ' para evitar quebrar a linha de comando
// ------------------------------------------------------------
static void sanitize_for_powershell(const char *in, char *out, size_t out_sz) {
    if (!out || out_sz == 0) return;
    if (!in) { out[0] = '\0'; return; }
    size_t j = 0;
    for (size_t i = 0; in[i] != '\0' && j + 1 < out_sz; ++i) {
        char ch = in[i];
        if (ch == '"') {
            if (j + 2 < out_sz) { out[j++] = '\''; } // convert quote to single quote
        } else {
            out[j++] = ch;
        }
    }
    out[j] = '\0';
}

// ------------------------------------------------------------
// Enviar e-mail chamando PowerShell
// ------------------------------------------------------------
void enviarEmailCliente(Cliente *c, int dias) {
    if (!c) return;
    if (!email_valido(c->email)) return;
    if (emailJaEnviado(c->id)) return;

    // montar comando com argumentos sanitizados
    char email_s[256], nome_s[256], venc_s[64];
    sanitize_for_powershell(c->email, email_s, sizeof(email_s));
    sanitize_for_powershell(c->nome, nome_s, sizeof(nome_s));
    sanitize_for_powershell(c->vencimento, venc_s, sizeof(venc_s));

    char cmd[1024];
    // certifique-se do caminho correto do script: config/send_email.ps1
    snprintf(cmd, sizeof(cmd),
        "powershell -ExecutionPolicy Bypass -File \"config/send_email.ps1\""
        " -email \"%s\" -nome \"%s\" -vencimento \"%s\" -dias \"%d\"",
        email_s, nome_s, venc_s, dias
    );

    // Run (blocking): se preferir rodar async, trocar por criação de thread
    int rc = system(cmd);
    // opcional: poderia analisar rc / saída do script
    (void)rc;

    registrarEnvioEmail(c->id);
}

// ------------------------------------------------------------
// Exportar CSV (UTF-8 BOM para compatibilidade web)
// ------------------------------------------------------------
void exportarRelatorioCSV(const char *fname) {
    FILE *f = fopen(fname, "w");
    if (!f) {
        TraceLog(LOG_WARNING, "Não foi possível criar CSV: %s", fname);
        return;
    }

    // BOM
    unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    fwrite(bom, 1, sizeof(bom), f);

    fprintf(f, "ID,Nome,Email,Telefone,CPF_CNPJ,Plano,Vencimento,Situacao,Dias\n");

    for (int i = 0; i < totalClientes; i++) {
        Cliente *c = &clientes[i];
        int dias = diasRestantes(c->vencimento);

        const char *sit =
            (dias < 0) ? "Expirado" :
            (dias <= 3 ? "A vencer" : "Ativo");

        // Escapar corretamente
        fprintf(f, "%d,", c->id);
        csv_escape_and_write(f, c->nome); fprintf(f, ",");
        csv_escape_and_write(f, c->email); fprintf(f, ",");
        csv_escape_and_write(f, c->telefone); fprintf(f, ",");
        csv_escape_and_write(f, c->cpf_cnpj); fprintf(f, ",");
        csv_escape_and_write(f, c->plano); fprintf(f, ",");
        csv_escape_and_write(f, c->vencimento); fprintf(f, ",");
        csv_escape_and_write(f, sit);
        fprintf(f, ",%d\n", dias);
    }

    fclose(f);
    TraceLog(LOG_INFO, "Relatorio gerado: %s", fname);
}

// ------------------------------------------------------------
// Botão estilizado
// ------------------------------------------------------------
bool Botao(Rectangle r, const char *label) {
    Vector2 m = GetMousePosition();
    bool hover = CheckCollisionPointRec(m, r);

    Color cor = hover ? (Color){220,220,220,255} : (Color){245,245,245,255};

    DrawRectangleRounded(r, 0.15f, 8, cor);
    DrawRectangleRoundedLines(r, 0.15f, 8, 1.0f, (Color){40,40,40,60});

    int tw = MeasureText(label, 18);
    DrawText(label, r.x + (r.width - tw)/2, r.y + 10, 18, BLACK);

    return hover && IsMouseButtonReleased(MOUSE_LEFT_BUTTON);
}

// ------------------------------------------------------------
// Card Bonito
// ------------------------------------------------------------
void Card(Rectangle r, const char *titulo, int valor, Color cor) {
    DrawRectangleRounded(r, 0.20f, 10, cor);
    DrawText(titulo, r.x + 20, r.y + 18, 20, WHITE);
    DrawText(TextFormat("%d", valor), r.x + 20, r.y + 55, 34, WHITE);
}

// ------------------------------------------------------------
// Carregar DB (com envio automático)
// ------------------------------------------------------------
int carregarClientesDB(const char *dbfile) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    const char *sql =
        "SELECT id, nome, email, telefone, cpf_cnpj, plano, vencimento "
        "FROM clientes ORDER BY id;";

    if (sqlite3_open(dbfile, &db) != SQLITE_OK) {
        TraceLog(LOG_WARNING, "Falha ao abrir DB: %s", dbfile);
        return 0;
    }

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        TraceLog(LOG_WARNING, "Falha prepare: %s", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 0;
    }

    totalClientes = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (totalClientes >= MAX_REG) break;

        Cliente *c = &clientes[totalClientes];
        c->id = sqlite3_column_int(stmt, 0);

        const unsigned char *t;
        t = sqlite3_column_text(stmt, 1); safe_strcpy(c->nome, t ? (const char*)t : "", sizeof(c->nome));
        t = sqlite3_column_text(stmt, 2); safe_strcpy(c->email, t ? (const char*)t : "", sizeof(c->email));
        t = sqlite3_column_text(stmt, 3); safe_strcpy(c->telefone, t ? (const char*)t : "", sizeof(c->telefone));
        t = sqlite3_column_text(stmt, 4); safe_strcpy(c->cpf_cnpj, t ? (const char*)t : "", sizeof(c->cpf_cnpj));
        t = sqlite3_column_text(stmt, 5); safe_strcpy(c->plano, t ? (const char*)t : "", sizeof(c->plano));
        t = sqlite3_column_text(stmt, 6); safe_strcpy(c->vencimento, t ? (const char*)t : "", sizeof(c->vencimento));

        // Envio automático: se vencido ou a vencer (<= 3 dias)
        int d = diasRestantes(c->vencimento);
        if ((d < 0 || d <= 3) && email_valido(c->email) && !emailJaEnviado(c->id)) {
            enviarEmailCliente(c, d);
            // registrarEnvioEmail é chamado dentro de enviarEmailCliente
        }

        totalClientes++;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return totalClientes;
}

// ------------------------------------------------------------
// Programa principal (PetControl - painel.c)
// ------------------------------------------------------------
int main(void) {
    // Inicializa janela
    InitWindow(WINDOW_W, WINDOW_H, "PetControl - Painel");
    SetTargetFPS(60);

    // Carrega recursos
    Texture2D logo = {0};
    if (FileExists("logo.png")) {
        logo = LoadTexture("logo.png");
    }

    // Carrega DB (arquivo agendpet.db) - note: esto tambem dispara envios automáticos
    int loaded = carregarClientesDB("database/agendpet.db");
    if (!loaded) {
        TraceLog(LOG_WARNING, "Nenhum cliente carregado. Verifique agendpet.db e tabela clientes.");
    }

    int page = 0;
    const int perPage = 14;

    // Pre-calc visible rows
    int visibleRows = (int)(TABLE_VISIBLE_H / ROW_HEIGHT);
    if (visibleRows < 1) visibleRows = 1;

    while (!WindowShouldClose()) {
        // Handle input for scroll (mouse wheel)
        float wheel = GetMouseWheelMove();
        if (wheel != 0.0f) {
            scrollY -= wheel * scrollSpeed; // wheel positive when scroll up
        }

        // total content height
        float contentH = (float)totalClientes * (float)ROW_HEIGHT;
        float maxScroll = contentH - (float)visibleRows * (float)ROW_HEIGHT;
        if (maxScroll < 0) maxScroll = 0;

        // Clamp scroll
        scrollY = clampf(scrollY, 0.0f, maxScroll);

        // Begin drawing
        BeginDrawing();
        ClearBackground((Color){240,240,240,255});

        // ------------------------------------------------------------
        // Barra superior moderna
        // ------------------------------------------------------------
        DrawRectangle(0, 0, WINDOW_W, TOP_BAR_H, (Color){35,90,180,255});

        // Logo: calcula escala para caber na barra
        if (logo.id != 0) {
            float maxLogoHeight = 60.0f; // altura maxima
            float scale = maxLogoHeight / (float)logo.height;
            if (scale <= 0) scale = 1.0f;
            float logoY = (TOP_BAR_H - (logo.height * scale)) / 2.0f;
            DrawTextureEx(logo, (Vector2){20, logoY}, 0.0f, scale, WHITE);
        }

        DrawText("PetControl - Painel", 100, 20, 28, WHITE);

        // ------------------------------------------------------------
        // MENU LATERAL
        // ------------------------------------------------------------
        DrawRectangle(0, TOP_BAR_H, SIDE_MENU_W, WINDOW_H - TOP_BAR_H, (Color){250,250,250,255});

        if (Botao((Rectangle){30, 100, 160, 45}, "Exportar CSV")) {
            exportarRelatorioCSV("relatorio_planos.csv");
        }

        if (Botao((Rectangle){30, 155, 160, 45}, "Recarregar DB")) {
            // limpar flag de clients e recarregar (não remove log de envios)
            carregarClientesDB("database/agendpet.db");

        }

        // botão manual para reenviar avisos (apenas para clientes não marcados como enviados)
        if (Botao((Rectangle){30, 210, 160, 45}, "Enviar Avisos")) {
            for (int i = 0; i < totalClientes; i++) {
                Cliente *c = &clientes[i];
                int d = diasRestantes(c->vencimento);
                if ((d < 0 || d <= 3) && email_valido(c->email) && !emailJaEnviado(c->id)) {
                    enviarEmailCliente(c, d);
                }
            }
        }

        if (Botao((Rectangle){30, 265, 160, 45}, "Sair")) {
            break;
        }

        // ------------------------------------------------------------
        // CARDS
        // ------------------------------------------------------------
        int ativos = 0, vencer = 0, expirados = 0;
        for (int i=0; i < totalClientes; i++) {
            int d = diasRestantes(clientes[i].vencimento);
            if (d < 0) expirados++;
            else if (d <= 3) vencer++;
            else ativos++;
        }

        Card((Rectangle){250, 90, 220, 110}, "Planos Ativos", ativos, (Color){76,175,80,255});
        Card((Rectangle){500, 90, 220, 110}, "A Vencer", vencer, (Color){255,152,0,255});
        Card((Rectangle){750, 90, 220, 110}, "Expirados", expirados, (Color){244,67,54,255});

        // ------------------------------------------------------------
        // CABEÇALHO TABELA
        // ------------------------------------------------------------
        DrawText("ID", TABLE_X + 10, 230, 20, BLACK);
        DrawText("Cliente", TABLE_X + 60, 230, 20, BLACK);
        DrawText("Plano", TABLE_X + 360, 230, 20, BLACK);
        DrawText("Vencimento", TABLE_X + 540, 230, 20, BLACK);

        // ------------------------------------------------------------
        // TABELA COM ROLAGEM
        // ------------------------------------------------------------
        // Compute first visible row from scrollY
        int firstRow = (int)(scrollY / ROW_HEIGHT);
        float offsetY = fmodf(scrollY, ROW_HEIGHT);

        // number of rows we will draw (visible plus one extra for partial)
        int rowsToDraw = visibleRows + 1;
        if (firstRow + rowsToDraw > totalClientes) rowsToDraw = totalClientes - firstRow;

        // Background for table area (nice subtle)
        DrawRectangle(TABLE_X, TABLE_TOP - 20, TABLE_W, TABLE_VISIBLE_H + 40, (Color){245,245,245,255});

        float y = TABLE_TOP - offsetY;
        for (int i = 0; i < rowsToDraw; i++) {
            int idx = firstRow + i;
            if (idx < 0 || idx >= totalClientes) continue;
            Cliente *c = &clientes[idx];
            int d = diasRestantes(c->vencimento);

            Color cor = (d < 0) ? RED :
                        (d <= 3) ? ORANGE :
                                  DARKGREEN;

            Color rowBg = (idx % 2 == 0) ? WHITE : (Color){235,235,235,255};
            DrawRectangle(TABLE_X, (int)(y - 5), TABLE_W, ROW_HEIGHT - 5, rowBg);

            DrawText(TextFormat("%d", c->id), TABLE_X + 10, (int)y, 18, BLACK);
            DrawText(c->nome, TABLE_X + 60, (int)y, 18, cor);
            DrawText(c->plano, TABLE_X + 360, (int)y, 18, BLACK);
            DrawText(TextFormat("%s (%d dias)", c->vencimento, d), TABLE_X + 540, (int)y, 18, BLACK);

            y += ROW_HEIGHT;
        }

        // If no clients, show placeholder
        if (totalClientes == 0) {
            DrawText("Nenhum cliente encontrado.", TABLE_X + 20, TABLE_TOP + 20, 20, GRAY);
        }

        // ------------------------------------------------------------
        // Scrollbar visual (direita da tabela)
        // ------------------------------------------------------------
        // Draw track
        float trackX = TABLE_X + TABLE_W + 8;
        float trackY = TABLE_TOP;
        float trackH = TABLE_VISIBLE_H;
        DrawRectangle((int)trackX, (int)trackY, SCROLLBAR_W, (int)trackH, (Color){220,220,220,255});

        // Thumb size proportional to visible area
        float thumbH;
        if (contentH <= 0.0f) thumbH = trackH;
        else thumbH = ( (float)visibleRows * ROW_HEIGHT / contentH ) * trackH;
        if (thumbH < 20.0f) thumbH = 20.0f; // min thumb size

        // Thumb position
        float thumbY;
        if (maxScroll <= 0.0f) thumbY = trackY;
        else thumbY = trackY + (scrollY / maxScroll) * (trackH - thumbH);
        DrawRectangle((int)trackX, (int)thumbY, SCROLLBAR_W, (int)thumbH, (Color){130,130,130,255});

        // ------------------------------------------------------------
        // Paginação (botões ainda funcionam, atualizam scroll)
        // ------------------------------------------------------------
        if (Botao((Rectangle){250, 650, 110, 35}, "Anterior")) {
            // move up one page worth
            scrollY -= (float)visibleRows * ROW_HEIGHT;
            scrollY = clampf(scrollY, 0.0f, maxScroll);
        }

        if (Botao((Rectangle){370, 650, 110, 35}, "Próxima")) {
            scrollY += (float)visibleRows * ROW_HEIGHT;
            scrollY = clampf(scrollY, 0.0f, maxScroll);
        }

        int currentPage = (int)(scrollY / ((float)visibleRows * ROW_HEIGHT)) + 1;
        int pageCount = (int)( (contentH + (visibleRows*ROW_HEIGHT - 1)) / (visibleRows*ROW_HEIGHT) );
        if (pageCount < 1) pageCount = 1;
        DrawText(TextFormat("Página %d / %d", currentPage, pageCount), 520, 655, 20, GRAY);

        EndDrawing();
    }

    // Cleanup
    if (logo.id != 0) UnloadTexture(logo);

    CloseWindow();
    return 0;
}

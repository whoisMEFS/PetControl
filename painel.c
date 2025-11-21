// painel.c
// Versão completa: PetControl UI
// - Logo redimensionado automaticamente e centralizado
// - Export CSV com BOM UTF-8
// - Rolagem com mouse (scroll) + barra de scroll visual
// - Layout moderno (cards, botões estilizados)
// - Pronto para compilar com Raylib 5.0 + sqlite3.c
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
    char vencimento[16];
} Cliente;

static Cliente clientes[MAX_REG];
static int totalClientes = 0;

// Scroll state
static float scrollY = 0.0f;        // pixels scrolled from top of table
static float scrollSpeed = 30.0f;   // pixels per wheel tick

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
    time_t now = time(NULL);

    return (int)((t_venc - now) / 86400.0);
}

// ------------------------------------------------------------
// Carregar DB
// ------------------------------------------------------------
int carregarClientesDB(const char *dbfile) {
    sqlite3 *db;
    sqlite3_stmt *stmt;

    const char *sql =
        "SELECT id, nome, email, telefone, cpf_cnpj, plano, vencimento "
        "FROM clientes ORDER BY id;";

    if (sqlite3_open(dbfile, &db) != SQLITE_OK)
        return 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        sqlite3_close(db);
        return 0;
    }

    totalClientes = 0;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        if (totalClientes >= MAX_REG) break;

        Cliente *c = &clientes[totalClientes];
        c->id = sqlite3_column_int(stmt, 0);

        const unsigned char *t;
        t = sqlite3_column_text(stmt, 1); strcpy(c->nome, t ? (char*)t : "");
        t = sqlite3_column_text(stmt, 2); strcpy(c->email, t ? (char*)t : "");
        t = sqlite3_column_text(stmt, 3); strcpy(c->telefone, t ? (char*)t : "");
        t = sqlite3_column_text(stmt, 4); strcpy(c->cpf_cnpj, t ? (char*)t : "");
        t = sqlite3_column_text(stmt, 5); strcpy(c->plano, t ? (char*)t : "");
        t = sqlite3_column_text(stmt, 6); strcpy(c->vencimento, t ? (char*)t : "");

        totalClientes++;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return totalClientes;
}

// ------------------------------------------------------------
// Exportar CSV (UTF-8 BOM para compatibilidade web)
// ------------------------------------------------------------
void exportarRelatorioCSV(const char *fname) {
    FILE *f = fopen(fname, "w");
    if (!f) {
        // fallback: show message in console (useful for debug if running from IDE)
        TraceLog(LOG_WARNING, "Não foi possível criar CSV: %s", fname);
        return;
    }

    // Escrever BOM UTF-8 para evitar problemas com acentuação em navegadores/Excel
    unsigned char bom[] = {0xEF, 0xBB, 0xBF};
    fwrite(bom, 1, sizeof(bom), f);

    fprintf(f, "ID,Nome,Email,Telefone,CPF_CNPJ,Plano,Vencimento,Situacao,Dias\n");

    for (int i = 0; i < totalClientes; i++) {
        Cliente *c = &clientes[i];
        int dias = diasRestantes(c->vencimento);

        const char *sit =
            (dias < 0) ? "Expirado" :
            (dias <= 3 ? "A vencer" : "Ativo");

        // Escape minimal: wrap textual fields in quotes
        fprintf(f,
                "%d,\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",\"%s\",%d\n",
                c->id, c->nome, c->email, c->telefone,
                c->cpf_cnpj, c->plano, c->vencimento, sit, dias);
    }

    fclose(f);
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

// Small helper: clamp
static float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
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
    // Tente carregar logo.png; se falhar, o programa continua sem imagem
    if (FileExists("logo.png")) {
        logo = LoadTexture("logo.png");
    }

    // Carrega DB (arquivo agendpet.db)
    carregarClientesDB("agendpet.db");

    // Variables for layout/interaction
    int porPagina = 12; // ainda mantemos paginacao via botoes, mas prioridade é scroll
    bool running = true;

    // Pre-calc visible rows
    int visibleRows = (int)(TABLE_VISIBLE_H / ROW_HEIGHT);
    if (visibleRows < 1) visibleRows = 1;

    // Main loop
    while (!WindowShouldClose() && running) {
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
            carregarClientesDB("agendpet.db");
        }

        if (Botao((Rectangle){30, 210, 160, 45}, "Sair")) {
            running = false;
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

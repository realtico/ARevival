#include "ui.h"

#include <langinfo.h>
#include <locale.h>
#include <ncurses.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
 * Altura fixa do HUD (linhas do topo). O log ocupa o resto da tela, numa
 * janela ncurses separada com scrollok ligado - assim redesenhar o HUD
 * nunca atropela a rolagem do log, e vice-versa.
 */
#define ALTURA_HUD 4

static WINDOW *janela_hud = NULL;
static WINDOW *janela_log = NULL;

/*
 * Pacote 16: setlocale(LC_ALL, "") so' resolve pra UTF-8 se o AMBIENTE ja'
 * tiver uma locale .UTF-8 exportada (LANG/LC_ALL/LC_CTYPE) - ela nao forca
 * UTF-8, so' pergunta ao SO qual locale usar. Se o ambiente falhar, tenta
 * "C.UTF-8" explicitamente - locale UTF-8 minima que a glibc traz pronta em
 * praticamente toda distro Linux moderna, sem precisar de locale-gen nem de
 * nada exportado pelo usuario.
 *
 * Isso resolve so' metade do bug, e por si so' NAO era suficiente: em
 * Linux/Ubuntu (nativo ou WSL), mesmo com LANG=en_US.UTF-8 corretamente
 * setado no ambiente, o texto acentuado ainda saia como "DepM-CM-3sito" em
 * vez de "Depósito". Causa real, confirmada reproduzindo o bug neste
 * repositorio: o Makefile linkava contra a libncurses "narrow" (8-bit, via
 * `pkg-config ncurses`), que trata cada BYTE de uma sequencia UTF-8
 * multi-byte como uma celula separada, independente da locale do processo -
 * so' a variante "wide" (`libncursesw`, `pkg-config ncursesw`) sabe
 * combinar bytes multi-byte num unico caractere. No macOS a libncurses do
 * sistema e' sempre wide por baixo dos panos, entao o bug nunca apareceu
 * la' mesmo com o link "errado". A correcao ficou no Makefile (usar
 * `ncursesw`); garantir_locale_utf8() continua necessaria como a outra
 * metade (garantir que a locale seja UTF-8 pra libncursesw ter o que
 * decodificar).
 */
static void garantir_locale_utf8(void) {
    setlocale(LC_ALL, "");
    const char *codeset = nl_langinfo(CODESET);
    if (codeset != NULL && strcmp(codeset, "UTF-8") == 0) {
        return;
    }
    setlocale(LC_ALL, "C.UTF-8");
}

void ui_iniciar(void) {
    /*
     * garantir_locale_utf8() cobre a metade "locale" do bug do Pacote 16
     * (ver comentario acima da funcao). A outra metade - libncurses
     * "narrow" vs "wide" - e' resolvida no Makefile (link contra
     * ncursesw), nao aqui.
     */
    garantir_locale_utf8();
    initscr();
    cbreak();     /* le tecla sem esperar Enter, mas deixa Ctrl-C funcionando (raw() desligaria isso) */
    noecho();     /* nao ecoa a tecla digitada - o jogo controla o que aparece na tela */
    keypad(stdscr, TRUE);
    curs_set(0);  /* sem cursor piscando - nao ha campo de texto livre, so leitura de digito */

    janela_hud = newwin(ALTURA_HUD, COLS, 0, 0);
    janela_log = newwin(LINES - ALTURA_HUD, COLS, ALTURA_HUD, 0);
    scrollok(janela_log, TRUE);
    keypad(janela_log, TRUE);
}

void ui_encerrar(void) {
    if (janela_hud != NULL) {
        delwin(janela_hud);
        janela_hud = NULL;
    }
    if (janela_log != NULL) {
        delwin(janela_log);
        janela_log = NULL;
    }
    endwin();
}

void ui_log(const char *fmt, ...) {
    char linha[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(linha, sizeof(linha), fmt, args);
    va_end(args);

    waddstr(janela_log, linha);
    waddch(janela_log, '\n');
    wrefresh(janela_log);
}

void ui_limpar_log(void) {
    werase(janela_log);
    wrefresh(janela_log);
}

void ui_desenhar_hud(const Jogador *jogador, const BaseDeDados *bd) {
    werase(janela_hud);
    box(janela_hud, 0, 0);

    const char *nome_arma = "-";
    if (jogador->arma_atual >= 0 && jogador->arma_atual < jogador->num_armas_obtidas) {
        int id_arma = jogador->armas_obtidas[jogador->arma_atual];
        if (id_arma >= 0 && id_arma < bd->num_armas) {
            nome_arma = bd->armas[id_arma].nome;
        }
    }

    mvwprintw(janela_hud, 1, 2, "Vida: %-4d  Energia: %-4d  Dinheiro: %-6d",
              jogador->vida, jogador->energia, jogador->dinheiro);
    mvwprintw(janela_hud, 2, 2, "Arma: %-24s  Escudo: %s  Medicamentos: %d",
              nome_arma,
              jogador->escudo_ligado ? "ligado" : "desligado",
              jogador->num_medicamentos);

    wrefresh(janela_hud);
}

int ui_ler_comando(void) {
    int tecla;
    do {
        tecla = wgetch(janela_log);
        if (tecla == 'h' || tecla == 'H') {
            return -1; /* pseudo-comando de ajuda, Pacote 11 */
        }
        if (tecla == 'm' || tecla == 'M') {
            return -2; /* pseudo-comando de mapa, Pacote 14 */
        }
    } while (tecla < '0' || tecla > '9');
    return tecla - '0';
}

int ui_aguardar_tecla(void) {
    return wgetch(janela_log);
}

int ui_ler_numero(void) {
    char buf[16] = {0};
    echo();
    curs_set(1);
    wgetnstr(janela_log, buf, (int)sizeof(buf) - 1);
    noecho();
    curs_set(0);
    return atoi(buf);
}

void ui_desenhar_mapa(const Mapa *mapa, const Jogador *jogador) {
    ui_log("Mapa conhecido (salas visitadas):");
    ui_log(" ");

    for (int linha = 0; linha < mapa->tamanho; linha++) {
        char salas[MAX_SALAS * 2 + 1];
        int pos = 0;
        for (int coluna = 0; coluna < mapa->tamanho; coluna++) {
            const Celula *celula = &mapa->celulas[linha][coluna];
            bool eh_teleporte = (linha == mapa->teleporte_linha && coluna == mapa->teleporte_coluna);
            if (linha == jogador->linha && coluna == jogador->coluna) {
                salas[pos++] = '@';
            } else if (eh_teleporte) {
                salas[pos++] = 'o'; /* pad do teleporte - sempre visitada, e' o inicio da partida */
            } else if (celula->visitada) {
                salas[pos++] = '.';
            } else {
                salas[pos++] = ' ';
            }

            if (coluna < mapa->tamanho - 1) {
                /* Porta Leste/Oeste so aparece se pelo menos um dos dois
                 * lados ja foi visitado - conectada[] e' simetrico entre
                 * vizinhos (map.c), entao tanto faz qual lado sabe dela. */
                bool porta_conhecida = celula->visitada || mapa->celulas[linha][coluna + 1].visitada;
                salas[pos++] = (porta_conhecida && celula->conectada[LESTE]) ? '-' : ' ';
            }
        }
        salas[pos] = '\0';
        ui_log("%s", salas);

        if (linha < mapa->tamanho - 1) {
            char portas[MAX_SALAS * 2 + 1];
            pos = 0;
            for (int coluna = 0; coluna < mapa->tamanho; coluna++) {
                const Celula *celula = &mapa->celulas[linha][coluna];
                bool porta_conhecida = celula->visitada || mapa->celulas[linha + 1][coluna].visitada;
                portas[pos++] = (porta_conhecida && celula->conectada[SUL]) ? '|' : ' ';
                if (coluna < mapa->tamanho - 1) {
                    portas[pos++] = ' ';
                }
            }
            portas[pos] = '\0';
            ui_log("%s", portas);
        }
    }

    ui_log(" ");
    ui_log("@ = voce   o = Sala de Teleporte   . = sala visitada   (espaco) = desconhecida");
}

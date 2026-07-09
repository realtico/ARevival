# Pacote 17 — mapa contínuo no canto da tela

**Tamanho:** M · **Depende de:** [Pacote 14](14-mapa-ascii.md)

## Objetivo

Hoje (Pacote 14) o mapa é um overlay: apertar `M` limpa o log e desenha o grid por cima, esperando
uma tecla pra voltar ao jogo (`mostrar_mapa()` em [game.c:171](../../aventureiro/src/game.c#L171),
`ui_limpar_log()` + `ui_desenhar_mapa()` + `ui_aguardar_tecla()`). Sugestão do usuário: manter o mapa
sempre visível, sendo atualizado ao vivo num canto da tela, sem precisar pedir com `M`.

## Entregáveis

- **Layout**: reservar uma região fixa da tela (ex. canto direito) pra uma nova `WINDOW*` de mapa em
  `ui.c`, ao lado das janelas de HUD/log já existentes (`janela_hud`, `janela_log` em
  [ui.c:16-17](../../aventureiro/src/ui.c#L16-L17)). Definir tamanho mínimo de terminal necessário
  (o grid pode não caber em terminais estreitos — decidir o que fazer: esconder o painel, avisar, ou
  exigir tamanho mínimo como já deve existir pro HUD/log).
- **Redesenho incremental**: chamar o desenho do mapa (reaproveitar a lógica de
  `ui_desenhar_mapa`/Pacote 14) toda vez que `visitada` mudar — ou seja, nos mesmos pontos que hoje
  marcam `Celula.visitada = true` (`entrar_em_sala` em `combat.c`, nascimento na Sala de Teleporte em
  `game.c:182`) — sem esperar o jogador apertar nada.
- **Decidir o destino do comando `M`**: remover (já que o mapa está sempre visível) ou manter como
  atalho pra dar zoom/full-screen no mapa. Definir com o usuário se ficar em dúvida; default sugerido:
  remover `M` e simplificar `ui_ler_comando()`/a ajuda (`mostrar_ajuda()` em
  [game.c:150-164](../../aventureiro/src/game.c#L150-L164)) de volta pra só `H`.
- Continua sem revelar tipo/conteúdo de salas não visitadas — mesma regra de fidelidade do Pacote 14.

## Critério de aceite

Jogando manualmente: o painel de mapa aparece sempre, atualiza sozinho ao entrar numa sala nova
(sem apertar `M`), e HUD/log continuam funcionando normalmente ao lado. Terminal pequeno demais tem
comportamento definido (não corrompe a tela). `tests/smoke_test.py` (pexpect+pyte) atualizado pra
cobrir o painel sempre visível.

**Resolvido e confirmado.** Implementado:

- `ui.c`: nova `WINDOW *janela_mapa`, criada em `ui_iniciar(int tamanho_mapa)` (assinatura mudou -
  agora recebe `Mapa::tamanho`, já conhecido em `main.c` antes de `ui_iniciar()`, pra calcular se o
  painel cabe). Painel só é criado se `COLS - largura_painel >= 40` (largura mínima decente pro
  log) e `LINES - ALTURA_HUD >= altura_painel` — caso contrário `janela_mapa` fica `NULL` e o jogo
  volta a se comportar como antes do Pacote 17 (log ocupa a tela toda), sem corromper nada.
  `ui_desenhar_mapa()` virou no-op silencioso quando `janela_mapa == NULL`.
- `ui_desenhar_mapa()` agora desenha direto na janela do painel (`mvwprintw`/`werase`/`wrefresh`),
  não mais via `ui_log()` — reaproveita a mesma lógica de símbolos do Pacote 14 (`@`, `o`, `.`,
  portas `-`/`|`, mesma regra de não revelar salas não visitadas).
- Trocado `box()` do ncurses por uma moldura desenhada manualmente (`desenhar_moldura`), tanto no
  painel de mapa quanto no HUD: `box()` usa o alternate character set do terminfo pros cantos, que
  nem todo terminal/emulador traduz direito (inclusive o `pyte` usado na verificação automatizada
  mostrava lixo tipo `l`/`q`/`k` em vez de linhas). A pedido do usuário, a moldura final usa
  semigráficos Unicode simples (`┌┐└┘─│`, estilo TUI clássico tipo Clipper/dBase dos anos 90, não a
  variante dupla `╔╗╚╝═║`) escritos como literais UTF-8 via `mvwprintw` (não `mvwaddch`, que é
  byte-a-byte e corromperia um caractere multi-byte) — funciona pelo mesmo mecanismo que já garante
  os acentos (Pacote 16: locale UTF-8 + libncursesw), sem depender de terminfo/ACS nenhum.
- `game.c`: `ui_desenhar_mapa(mapa, jogador)` chamado nos mesmos pontos onde `ui_desenhar_hud` já
  era chamado (início do loop, após narrar um resultado, após perseguição) - redesenha sempre,
  sem tentar rastrear se `visitada` mudou de fato (redesenhar é barato). Comando `M`/pseudo-comando
  `-2` removido (`ui_ler_comando`, `mostrar_mapa()`); ajuda e tela de título atualizadas pra não
  mencionar mais `M`.
- `tests/smoke_test.py`: nova `verificar_painel_mapa_visivel()` usa `pyte` pra confirmar que o
  painel aparece sem apertar nada e muda de conteúdo ao entrar numa sala nova - roda antes do fuzz
  loop. `M`/`m` removidos do alfabeto de fuzzing. `pyte` adicionado a `tests/requirements.txt`.
  Achado durante a implementação: o teste precisa fazer polling ativo (esperar um texto conhecido
  aparecer em tela) em vez de `sleep` fixo + leitura única - a primeira tecla enviada logo após o
  spawn pode se perder se o processo ainda não tiver chamado `cbreak()`. Achado 2: `add_test()` no
  `CMakeLists.txt` precisa de `WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}` explícito - o CTest roda com
  cwd em `CMAKE_BINARY_DIR` por padrão, e o jogo espera `data/` relativo ao cwd (default de
  `--data-dir`), então sem isso o binário morria na largada e o teste só via um EOF confuso. Achado
  3 (flakiness pega em ~2 de 3 execuções seguidas): esperar só pelo título "Mapa" aparecer não é
  suficiente - um redesenho grande (HUD + painel) pode chegar em vários pedaços pelo pty, e "Mapa"
  é a primeira coisa escrita no painel, então a condição batia antes do resto do frame (grid,
  posição do jogador, legenda) ter terminado de chegar. Corrigido esperando por um marcador que só
  aparece por último (a legenda) e drenando qualquer resto do frame ainda em trânsito antes de
  considerar a tela "pronta".

Verificação: build limpo via CMake, `ctest --test-dir build` passa (fuzz + verificação do painel),
repetido 8x seguidas sem falha após o fix de flakiness. Testado manualmente com pexpect+pyte em
terminais 100x30 (painel aparece com moldura `┌┐└┘─│`, grid mostra `@`/`o`/portas corretamente,
atualiza ao mover) e em 50x24/100x12/30x24 (painel corretamente omitido, log ocupa a tela toda,
processo não crasha).

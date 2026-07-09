# Pacote 27 — redesenhar o mapa antes de narrar a sala, não depois

**Tamanho:** S · **Depende de:** [Pacote 17](17-mapa-continuo.md), [Pacote 20](20-pausas-dramaticas.md)

## Objetivo

Hoje, ao mover (`comando_mover_interativo`, atalho de seta, `comando_fugir`, ou a perseguição de
`combate_seguir_tripulante_fugido`), a ordem no `game_loop` ([game.c](../../aventureiro/src/game.c))
é: executa o comando (que já atualiza `jogador->linha`/`coluna` internamente) → `narrar(&res)`
(imprime "Você entrou numa nova sala." + descrição, com as pausas dramáticas do Pacote 20) → só
**depois** `ui_desenhar_hud`/`ui_desenhar_mapa`. Ou seja, o painel de mapa continua mostrando a
posição **antiga** do jogador durante toda a narração (inclusive as pausas de ~1s da apresentação de
tripulante) e só pula pra posição nova quando a narração termina. Usuário pediu: atualizar o mapa
**antes** de começar a descrição da sala, não depois — assim o jogador já vê a posição nova no
painel enquanto lê a narração, em vez de um salto tardio.

## Entregáveis

- `game.c` (`game_loop`): mover a chamada `ui_desenhar_mapa(mapa, jogador)` pra **antes** de
  `narrar(&res)` (a posição do jogador já está atualizada nesse ponto, já que os comandos de
  movimento atualizam `jogador->linha`/`coluna` internamente antes de retornar `Resultado`). Manter
  `ui_desenhar_hud` **depois** de `narrar` como está hoje — o pedido foi especificamente sobre o
  mapa, não o HUD (vida/energia mudam por outros motivos que não só posição, ex. dano recebido, e
  faz sentido continuar refletindo o HUD só depois da narração explicar a mudança).
- Mesmo ajuste no trecho de perseguição (`combate_seguir_tripulante_fugido`): redesenhar o mapa
  antes de `narrar(&perseguicao)`, não depois.
- Como isso não depende de qual comando rodou (`ui_desenhar_mapa` já é barato/idempotente, ver
  Pacote 17), não precisa de lógica condicional por tipo de comando — mover a chamada uma vez no
  fluxo compartilhado já cobre mover, fugir e perseguição sem tocar em cada comando individualmente.

## Critério de aceite

Jogando manualmente: ao mover pra uma sala nova, o painel de mapa já mostra o `@` na posição nova
**antes** da primeira linha de narração da sala aparecer (não só depois que a narração termina).
Comandos que não mudam de sala (Atacar, Situação, etc.) continuam com a mesma aparência de hoje
(mapa não muda, então a ordem é imperceptível). `tests/smoke_test.py` continua passando.

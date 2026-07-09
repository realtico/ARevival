# Pacote 19 — remover "pressione uma tecla" de ajuda (H) e mapa (M)

**Tamanho:** S · **Depende de:** [Pacote 11](11-melhorias-jogabilidade.md), [Pacote 14](14-mapa-ascii.md)

## Objetivo

`mostrar_ajuda()` e `mostrar_mapa()` ([game.c:150-177](../../aventureiro/src/game.c#L150-L177))
terminam com `ui_log("(pressione uma tecla para continuar)")` + `ui_aguardar_tecla()`. É
redundante: nenhum dos dois é um comando real (não passam por `ui_limpar_log()`, que só roda pra
comandos numéricos 0-9 dentro do `switch` em `game_loop`), então o texto de ajuda/mapa continua na
tela até o jogador apertar a próxima tecla de qualquer forma — o próprio `ui_ler_comando()` do
próximo giro do loop já bloqueia esperando uma tecla. O aviso e a espera extra não fazem nada além
de exigir um toque a mais.

Nota: se o [Pacote 17](17-mapa-continuo.md) (mapa contínuo, sempre visível) for implementado antes
deste, o comando `M`/`mostrar_mapa()` pode deixar de existir de vez — nesse caso este pacote se
aplica só ao `H`.

## Entregáveis

- Em `mostrar_ajuda()`: remover as duas últimas linhas (`ui_log(" ")` +
  `ui_log("(pressione uma tecla para continuar)")`) e o `ui_aguardar_tecla()` final.
- Em `mostrar_mapa()` (se ainda existir nesse ponto — ver nota acima): mesma remoção.
- Conferir que nada mais dependia do valor de retorno de `ui_aguardar_tecla()` nesses dois pontos
  (hoje é descartado, então não deve haver efeito colateral).

## Critério de aceite

Jogando manualmente: apertar `H` mostra a ajuda e volta a aceitar qualquer tecla imediatamente,
sem o texto extra "(pressione uma tecla para continuar)" nem uma espera perceptível a mais além da
tecla que o jogador já ia apertar pra dar o próximo comando. Mesmo teste pra `M`, se ainda existir.
`tests/smoke_test.py` continua passando (o fuzzer já manda teclas aleatórias em sequência, incluindo
H/M, então cobre esse fluxo).

**Resolvido e confirmado.** O Pacote 17 (implementado antes deste) já tinha removido `M`/
`mostrar_mapa()` inteiro, então este pacote se aplicou só a `mostrar_ajuda()` (`game.c`): removidas
as duas linhas finais (`ui_log(" ")` + `ui_log("(pressione uma tecla para continuar)")`) e o
`ui_aguardar_tecla()`. Nada mais dependia do valor de retorno descartado.

Verificado com pexpect+pyte (seed 1): apertar `H` mostra a ajuda sem o texto extra, e a tecla
seguinte (`7`, Situação) já é processada na hora, sem precisar de um toque a mais antes. `ctest`
rodado 3x seguidas sem falha.

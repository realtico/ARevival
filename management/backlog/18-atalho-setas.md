# Pacote 18 — setas do teclado como atalho de movimento

**Tamanho:** S · **Depende de:** [Pacote 6a](06a-combat-basico.md), [Pacote 7](07-ui.md)

## Objetivo

Hoje mover exige dois passos: apertar `0`, depois a letra da direção (`N/S/L/O`) entre as saídas
disponíveis (`comando_mover_interativo` em
[game.c:72-91](../../aventureiro/src/game.c#L72-L91), via `ler_opcao(teclas)`). Sugestão do usuário:
as setas do teclado funcionarem como atalho direto — seta ativa o comando `0` + a direção
correspondente num único toque, sem passar pelo prompt.

## Entregáveis

- `keypad()` já está ligado em `janela_log` e `stdscr` ([ui.c:36](../../aventureiro/src/ui.c#L36) e
  [ui.c:42](../../aventureiro/src/ui.c#L42)), então `KEY_UP/KEY_DOWN/KEY_LEFT/KEY_RIGHT` já chegam em
  `wgetch`. Mapear em `ui_ler_comando()` ([ui.c:96-108](../../aventureiro/src/ui.c#L96-L108)):
  `KEY_UP`→Norte, `KEY_DOWN`→Sul, `KEY_RIGHT`→Leste, `KEY_LEFT`→Oeste (checar orientação com o
  usuário — o mapa ASCII do Pacote 14 pode já ter uma convenção N/S/L/O em tela pra bater).
- Decidir o contrato de retorno: mais simples é adicionar um terceiro pseudo-valor tipo
  `-3..-6` (um por direção) que `game_loop` traduz direto pra `comando_mover(jogador, mapa, bd, dir)`
  **pulando** `comando_mover_interativo`/o prompt "para que lado" — já que a seta já diz a direção.
- **Direção sem saída conectada**: decidir o comportamento (ignorar a tecla silenciosamente vs.
  mensagem "não há saída nesse sentido" sem consumir rodada) — original não tem equivalente direto
  pra isso, então é decisão de UX nova, não fidelidade.
- Atualizar a ajuda (`mostrar_ajuda()`, [game.c:150-164](../../aventureiro/src/game.c#L150-L164)) pra
  mencionar o atalho.

## Critério de aceite

Jogando manualmente: apertar uma seta move o jogador direto na direção correspondente (quando há
saída conectada), sem precisar digitar `0` antes nem escolher a letra depois. `0` + letra continua
funcionando do jeito antigo. `tests/smoke_test.py` atualizado pra exercitar as teclas de seta.

**Resolvido e confirmado.** Implementado:

- `ui.c` (`ui_ler_comando`): `KEY_UP`/`KEY_DOWN`/`KEY_RIGHT`/`KEY_LEFT` (já chegavam via `keypad()`,
  ligado desde o Pacote 7) retornam pseudo-valores `-3`/`-4`/`-5`/`-6` (Norte/Sul/Leste/Oeste) -
  mapeamento escolhido pra combinar com a orientação do painel de mapa (Pacote 17): Norte sobe,
  Leste anda pra direita, mesma convenção de `DELTA_LINHA`/`DELTA_COLUNA` em `combat.c`.
- `game.c` (`game_loop`): esses pseudo-valores pulam o `switch` de comandos 0-9 inteiro e chamam
  `comando_mover(jogador, mapa, bd, direcao)` **direto**, sem passar por
  `comando_mover_interativo`/o prompt "para que lado". Direção sem porta: decisão tomada foi
  reaproveitar a mensagem que já existe em `comando_mover` ("Não há saída pelo %s.") em vez de
  inventar um comportamento novo - essa mensagem já existia no código desde o Pacote 11 mas era
  inalcançável na prática (o prompt manual só oferece direções com porta de fato); o atalho de seta
  é o primeiro chamador real que pode passar uma direção inválida, então "ativa" esse código morto
  em vez de precisar de lógica nova. O resultado flui pro mesmo bloco de cauda compartilhado
  (narrar/redesenhar HUD+mapa/checar morte) que os comandos 0-9 já usam, evitando duplicar essa
  lógica.
- `mostrar_ajuda()`: nova linha mencionando o atalho.
- `tests/smoke_test.py`: nova `verificar_atalho_setas()` manda as 4 setas (seed fixa) e confirma que
  cada uma produz "Você entrou numa nova sala." OU "Não há saída pelo ..." - nunca o prompt "Para
  que lado" (o que indicaria que caiu no fluxo manual por engano). Fuzz loop também ganhou as 4
  sequências de escape das setas no alfabeto de teclas aleatórias. Achado durante a implementação:
  as sequências de escape das setas variam por terminfo/modo - sob `TERM=xterm-256color` com o
  keypad em modo aplicação (ligado pelo próprio ncurses ao iniciar), são `\EOA/\EOB/\EOC/\EOD`
  (SS3), não a variante `\E[A` mais comum fora do modo aplicação - confirmado com
  `TERM=xterm-256color infocmp -1`. Refatorada a lógica de polling/settle de
  `verificar_painel_mapa_visivel` (Pacote 17) pra uma função compartilhada (`_sessao_com_tela`),
  reaproveitada pela nova verificação em vez de duplicar.

Verificação: `ctest --test-dir build` e execução direta de `tests/smoke_test.py`, repetidos várias
vezes seguidas sem falha. Testado manualmente com pexpect+pyte (seed 1): as 3 setas sem porta na
sala de partida mostraram "Não há saída pelo [direção]." corretamente; a única com porta (Oeste,
nessa seed) moveu o jogador e redesenhou HUD/mapa/log exatamente como o fluxo `0`+letra faria.

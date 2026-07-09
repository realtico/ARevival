# Pacote 20 — pausas dramáticas (examinar sala / apresentar tripulante)

**Tamanho:** M · **Depende de:** [Pacote 6a](06a-combat-basico.md), [Pacote 15](15-narrar-arma-tripulante.md)

## Objetivo

O usuário lembra de pausas de ~1s no original em dois pontos: durante o comando Examinar (entre a
mensagem de luz e "Você encontrou...", e de novo antes de revelar o quê) e ao entrar numa sala com
tripulante (uma sequência de 3 falas espaçadas: "Há alguém..." / "Há alguma coisa aqui..." / "é um
XXX armado com YYY, frase de efeito ZZZ"). Fui checar o `aventureiro.p.bas` pra separar o que é
confirmável na fonte do que é reconstrução por sensação:

- **Confirmado no original (Examinar)**: linhas 5160-5200. `5160 PRINT ... "EXAMINANDO A SALA..."`
  é seguido por `5170 FOR I=U TO O` / `5180 NEXT I` — um loop vazio de 1 a 8 (`U=1`, `O=8`, linhas
  26-28) só pra passar tempo — antes de `5200 PRINT ,,"VOCE ENCONTROU... "`. É um delay deliberado,
  sem nenhum outro efeito. No ZX81 (Z80 a 3.25MHz, BASIC interpretado, modo SLOW com tela
  redesenhada via software) 8 iterações vazias plausivelmente já dava a sensação de ~1s.
- **Não confirmado como delay explícito (Examinar)**: a pausa entre "Você encontrou..." e a
  revelação (`5200`→`5250`/`5410` é sequencial, sem FOR/NEXT no meio) — reconstrução de fidelidade
  à experiência, não ao BASIC literal.
- **Confirmado no original (tripulante) — e é uma sequência de 3 falas, não 2**: linhas 6150-6270.
  - `6150 PRINT ,,"HA ALGUEM,"`
  - entre 6155-6200: `RAND 9500+(CODE R$(D,A,7)-37)*10` / `RAND USR RT` e mais três `LET
    ...=A$( TO USR RD)` — rotina em código de máquina decodificando nome/arma/frase do tripulante
    da memória.
  - `6210 PRINT "...HA ALGUMA COISA AQUI."` — uma segunda fala isolada, que a versão atual do jogo
    (Pacote 15) **não tem** — hoje tudo foi fundido numa mensagem só.
  - entre 6220-6255: mais um bloco `RAND 9310+VAL Z$*10` / `RAND USR RT` + `LET`s, decodificando
    dados de combate do tripulante (arma etc).
  - `6270 PRINT ,,"TWIN REPORTA...",," \"E UM ";M$,"ARMADO COM ";N$;".",X$;".\""` — a revelação
    final é atribuída a "TWIN" (o tradutor/conselheiro eletrônico do jogador, apresentado na tela
    de título), não narração impessoal.
  - Ou seja: os dois `RAND USR` são processamento genuíno do ZX81 (não um `FOR/NEXT` proposital),
    mas a estrutura de 3 `PRINT`s distintos com trabalho pesado entre eles é 100% fiel à fonte — a
    lembrança do usuário bate exatamente com o código.

Dado que o pedido é recriar a sensação de jogar o original (não simular ciclos de CPU), a proposta é
tratar todas as pausas como equivalentes na prática — todas ~1s — mas deixar registrado que a pausa
do Examinar antes da revelação é a única sem lastro direto de delay explícito no código-fonte; as
demais (incluindo as duas pausas da apresentação de tripulante) têm base direta na estrutura real do
BASIC, mesmo que o "porquê" da demora seja processamento e não um loop de espera proposital.

## Entregáveis

- **Mecanismo de pausa no buffer de mensagens** (`combat.h`/`types.h`): hoje `Resultado` é só um
  buffer de strings (`mensagens[MAX_MENSAGENS_RESULTADO]`) que `narrar()` em
  [game.c:58-62](../../aventureiro/src/game.c#L58-L62) despeja de uma vez, sem timing entre linhas.
  Adicionar um array paralelo (ex. `bool pausa_apos[MAX_MENSAGENS_RESULTADO]`) marcando quais
  mensagens devem ser seguidas de uma pausa antes da próxima. Um helper em `combat.c` tipo
  `marcar_pausa(Resultado *r)` seta o flag no último índice preenchido, chamado logo depois do
  `log_msg(...)` que deve pausar.
- **Aplicar a pausa em `narrar()`** (`game.c`): depois de `ui_log("%s", res->mensagens[i])`, se
  `res->pausa_apos[i]`, chamar uma nova função de UI (ex. `ui_pausar_dramatico()` em `ui.c`,
  provavelmente só um `napms(1000)` do ncurses — a janela de log já foi refrescada pelo `ui_log`
  anterior, então a mensagem fica visível durante a espera).
- **Pontos de uso**:
  - `comando_examinar_sala` (`combat.c`): marcar pausa depois da mensagem de luz/escuro (linha
    ~462/471) e antes de `"Voce encontrou..."` — este é o ponto com lastro direto na fonte.
  - Mesma função: marcar pausa depois de `"Voce encontrou..."`, antes da revelação (Nada/item) —
    reconstrução, não fidelidade literal (ver acima).
  - `narrar_sala` (`combat.c`, tripulante): hoje o Pacote 15 deixou tudo numa mensagem só
    (`"Ha alguem aqui: %s, armado com %s. ..."`). Separar de volta em **três** mensagens,
    espelhando 6150/6210/6270:
    1. "Ha alguem..." (equivalente a `6150 "HA ALGUEM,"`), pausa;
    2. "Ha alguma coisa aqui..." (equivalente a `6210 "...HA ALGUMA COISA AQUI."` — mensagem que
       não existe na versão atual, precisa ser adicionada), pausa;
    3. a revelação completa, atribuída a TWIN (equivalente a `6270 "TWIN REPORTA... \"E UM
       [nome] ARMADO COM [arma]. [frase].\""`) — ex. `"TWIN reporta... \"E um %s, armado com %s.
       %s.\""`.
  - Sala de Teleporte no início da partida (`game_loop`, `game.c:179-182`): hoje só marca
    `visitada = true`, sem chamar `combat_narrar_sala_atual`/`narrar` — o jogador começa o jogo sem
    ver a descrição da sala onde está. Tratar o início como uma "entrada" também: chamar
    `combat_narrar_sala_atual` + `narrar(&descricao)` antes do primeiro `ui_desenhar_hud`/loop de
    comando, do mesmo jeito que o Examinar interativo já faz (game.c:133-134). Sala de Teleporte
    nunca tem tripulante (nasce "segura", ver `map.c:106`), então essa narração inicial não deve
    disparar a sequência de pausas do tripulante — só tipo de sala + saídas.
- **Não deve afetar** `tests/smoke_test.py`: o fuzzer manda ~200 comandos aleatórios com timeout de
  5s por sessão; se cada Examinar/entrada-em-sala-com-tripulante passar a levar +1-2s reais, o
  orçamento de tempo do fuzzer explode. Adicionar uma forma de desligar as pausas em modo de teste
  (ex. variável de ambiente tipo `AVENTUREIRO_SEM_PAUSAS=1` checada em `ui_pausar_dramatico()`, ou
  flag `--sem-pausas` em `main.c` reaproveitando o padrão do `--seed`) e usar isso em
  `tests/smoke_test.py`.

## Critério de aceite

Jogando manualmente: Examinar sala mostra a mensagem de luz, pausa perceptível (~1s), "Você
encontrou...", pausa (~1s), revelação. Entrar em sala com tripulante mostra "Há alguém...", pausa
(~1s), "Há alguma coisa aqui...", pausa (~1s), depois a revelação de TWIN com nome/arma/frase.
Iniciar uma partida nova mostra a descrição (tipo de sala + saídas) da Sala de Teleporte antes do
primeiro comando, sem disparar a sequência de pausas de tripulante. `tests/smoke_test.py` roda no
modo sem pausas e continua passando sem estourar o timeout.

**Resolvido e confirmado.** Implementado:

- `combat.h`: novo `bool pausa_apos[MAX_MENSAGENS_RESULTADO]` em `Resultado`, paralelo a
  `mensagens` (`resultado_vazio()` já zera tudo via `memset`, nenhuma inicialização extra
  necessária). `combat.c`: helper `marcar_pausa(Resultado *r)` seta o flag no último índice
  preenchido.
- `ui.c`/`ui.h`: `ui_pausar_dramatico()` = `napms(1000)`, exceto se a variável de ambiente
  `AVENTUREIRO_SEM_PAUSAS` estiver definida (checada uma vez, cacheada em `static`).
- `game.c` (`narrar`): depois de cada `ui_log`, chama `ui_pausar_dramatico()` se
  `res->pausa_apos[i]`.
- `combat.c`:
  - `narrar_sala` (apresentação de tripulante): separada em 3 mensagens com pausa entre elas -
    "Há alguém...", "Há alguma coisa aqui...", e a revelação atribuída a TWIN (`"TWIN reporta...
    \"É um %s, armado com %s. %s.\""`).
  - `comando_examinar_sala`: pausa marcada na última mensagem antes de `"Você encontrou..."`
    (cobre tanto o caminho de luz normal quanto o de acidente no escuro) e outra depois de
    `"Você encontrou..."`, antes da revelação (Nada/item).
- `game.c` (`game_loop`): antes do loop principal, chama `combat_narrar_sala_atual` + `narrar` pra
  descrever a Sala de Teleporte no início da partida - antes o jogador começava sem ver nenhuma
  descrição. Sala de Teleporte nunca tem tripulante, então não dispara pausas.
- `tests/smoke_test.py`: `AVENTUREIRO_SEM_PAUSAS=1` adicionado a todos os `pexpect.spawn` (fuzz
  loop e as duas verificações via pyte) - sem isso o timeout de 5s por sessão do fuzzer estouraria
  toda vez que sorteasse Examinar ou uma sala com tripulante.

Verificação: `ctest --test-dir build` (modo sem pausas) rodado 3x seguidas, ~21.5s cada, sem
regressão de tempo. Testado manualmente com pexpect+pyte **com pausas ligadas** (sem a variável de
ambiente), medindo o tempo real entre mensagens: apresentação de tripulante (seed 1) — "Há
alguém..." → 1.01s → "Há alguma coisa aqui..." → 1.00s → "TWIN reporta... É um Carregador Arbock,
armado com Bumerangue. Pouco amigável."; Examinar sala (seed 7) — comando → 1.06s → "Você
encontrou..." → 1.00s → "Nada.". Narração inicial da Sala de Teleporte confirmada visível antes do
primeiro comando (seed 1): "Sala tipo: Sala de Teleporte" / "Saídas: Oeste" / "Não há ninguém
aqui.", sem pausas.

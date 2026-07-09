# Pacote 22 — fidelidade: medicamentos iniciais

**Tamanho:** S · **Depende de:** [Pacote 12](12-fidelidade-formulas.md)

## Objetivo

Usuário pediu pra verificar no `aventureiro.p.bas` (1) a frequência de aparição de medicamento e (2)
se o jogador realmente começa sem medicamentos. A frequência já batia (loot pós-combate 20%, linha
1830; quantidade do item de sala 60/25/15%, linha 5330-5340 — ambos já implementados desde o Pacote
12). O início, não: `player.c` inicializava `num_medicamentos = 0`, mas o original não começa com 0.

## Entregáveis

- **Achado**: linha 27 do original, `6 LET M=VAL "5"` — roda antes até da tela de título, e `M` é a
  mesma variável usada em todo o resto do jogo pra medicamentos (`1850 LET M=M+U` no loot, `4095
  LET M=M-U` ao usar, `4520 PRINT "MEDICAMENTOS >";M`). Nenhuma das 4 ocorrências de `LET M=` no
  arquivo inteiro reatribui `M` entre a linha 6 e o início de fato da partida (linhas 46-89, onde
  vida/energia/dinheiro/arma são inicializados) — o valor 5 sobrevive intacto.
- `data/config.json`: novo campo `jogador.medicamentos_iniciais: 5`, seguindo o padrão dos outros
  valores iniciais (`vida_inicial`, `energia_inicial`, `dinheiro_inicial`) — balanceamento fica no
  JSON, não em `#define`.
- `types.h` (`Config`), `data_loader.c` (`carregar_config`): campo novo carregado igual aos outros.
- `player.c` (`jogador_iniciar`): `jogador.num_medicamentos = cfg->medicamentos_iniciais;` em vez do
  `0` hardcoded anterior.

## Critério de aceite

Jogando manualmente: HUD e comando Situação (`7`) mostram "Medicamentos: 5" desde o primeiro
comando, sem precisar de nenhum loot. `ctest --test-dir build` continua passando.

**Resolvido e confirmado.** Verificado com pexpect+pyte (seed fixa): HUD mostra "Medicamentos: 5" e
o comando Situação também, logo na primeira tela após a Sala de Teleporte. `ctest` rodado 3x
seguidas sem falha após a mudança.

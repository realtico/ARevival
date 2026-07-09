# Pacote 23 — fidelidade: tripulante não deveria sumir quando você foge sem ser seguido

**Tamanho:** S · **Depende de:** [Pacote 13](13-perseguicao-fiel.md)

## Objetivo

Usuário reportou: "quando eu fujo e não sou perseguido... se eu retorno o inimigo é sorteado
novamente? [...] eu fujo e quando volto a sala tá vazia". Fui checar `aventureiro.p.bas` — não há
sorteio novo ao reentrar numa sala (presença de tripulante é estado fixo por sala, `R$(D,A,7)`,
só alterado por eventos específicos), mas achei um bug real na porta em como o comando Fugir (`2`)
trata a sala de origem.

O original tem **duas** sub-rotinas de fuga distintas, com comportamento diferente:

- **Tripulante foge de você durante combate** (linhas 6800-6870, `reacao_tripulante_apos_turno` →
  `tripulante_fugir_para_outra_sala`): a sub-rotina que move o tripulante (`GOSUB 6859`, que chama
  6900+6860+6870) roda **sempre**, mesmo se você responder "N" a "quer segui-lo?" (linha 6833:
  `IF INKEY$="N" THEN GOTO 6859` — vai pra 6859 de qualquer jeito). O "seguir ou não" só decide se
  **você** anda atrás dele pela trilha; ele já saiu da sala original de qualquer forma. Isso já
  estava certo na porta (Pacote 13).
- **Você foge do tripulante** (linhas 2040-2230, `comando_fugir`): diferente. `2120 IF R$(X,Y,7)<>"
  " OR RND<.5 THEN GOTO 2200` — a sub-rotina que limpa a sala de origem (`GOSUB 6860`, linha 2150)
  só roda dentro do ramo "ele decidiu te seguir" (sala de destino vazia + 50% de sorte). Se ele
  **não** te seguir, essa limpeza nunca acontece — `R$(D,A,7)` continua apontando pro mesmo
  tripulante, vivo, na sala original. A linha 2090 (`GOSUB 6900`, salva vida/energia atuais na sala
  de origem *antes* de saber se ele vai seguir) reforça isso: só faz sentido persistir o estado ali
  se ele puder continuar na sala.

A porta (`comando_fugir` em `combat.c`) zerava `celula->tem_tripulante` **incondicionalmente**,
antes até de sortear se ele segue — por isso a sala sempre ficava vazia ao fugir, seguido ou não.

## Entregáveis

- `combat.c` (`comando_fugir`): mover `celula->tem_tripulante = false;` pra dentro do `if` que já
  decide se o tripulante segue (`!destino->tem_tripulante && sorteio_chance(...)`) — só limpa a
  sala de origem nesse ramo. Fora dele (não seguiu), `celula` fica intocada: tripulante continua lá,
  vivo, com a vida/energia que já tinha (nada muda durante uma fuga bem-sucedida sem perseguição).

## Critério de aceite

Jogando manualmente: fugir de um tripulante que decide **não** seguir, depois voltar pra sala de
onde fugiu, mostra o mesmo tripulante ainda lá (nome/arma/vida preservados). Fugir de um que decide
seguir continua funcionando como antes (ele aparece na sala de destino, origem fica vazia).
`tests/smoke_test.py` continua passando.

**Resolvido e confirmado.** Verificado com pexpect+pyte, seed 3: jogador entra no Refeitório
("Engenheiro de Bordo, armado com Espada Laser"), foge com sucesso pelo lado Oeste sem ser seguido
(mensagem "Infelizmente o [...] veio atrás de você" ausente), volta pelo lado Leste (oposto) e o
Refeitório mostra a apresentação completa do mesmo tripulante de novo — confirma que ele nunca saiu
da sala. Comparado lado a lado com o comportamento anterior ao fix (sala aparecia vazia, "Não há
ninguém aqui.", exatamente o bug reportado pelo usuário). `ctest --test-dir build` rodado 3x
seguidas sem falha após a mudança.

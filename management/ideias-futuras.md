# Ideias futuras (pós-Pacote 10)

> Não fazem parte do backlog de construção em [backlog/](backlog/) — são melhorias a considerar
> **depois** que o Pacote 10 (polimento) estiver validado e o jogo completo estiver jogável e fiel
> ao original. Cada uma provavelmente vira seu próprio pacote pequeno quando chegar a vez.

- **Labirinto maior** — grid além de 8x8. Já é quase de graça: `grid_size` em `data/config.json` (Pacote 1), `map.c` (Pacote 4) já generaliza para qualquer tamanho até `MAX_SALAS` (32). Só validar performance/legibilidade em grids bem maiores.
- **Novos personagens (tripulantes)** — entradas novas em `data/crew.json`. Estrutura já suporta; só respeitar `id_arma` válido e `MAX_TRIPULANTES` (32) em `types.h`.
- **Novas armas** — entradas novas em `data/weapons.json`. Mesma ideia; respeitar `MAX_ARMAS` (16).
- **Mais aleatoriedade nas armas** — hoje dano é uniforme em `[1, dano_maximo]`. O Pacote 10
  decodificou `aventureiro.p.bas` e confirmou que o original também usa `INT(RND*MMD+1)` uniforme
  para dano de arma (não há viés aqui) — só os valores de loot/item têm viés não uniforme (ver
  [Pacote 12](backlog/12-fidelidade-formulas.md)). Se quiser variância por arma (críticos, dado com
  viés), é mecânica nova, não fidelidade ao original.

> Nota: as ideias de "revelar mapa conhecido" e "perseguição multi-sala" que estavam aqui viraram
> decisões confirmadas com o usuário no Pacote 10 — não são mais ideias soltas, são os pacotes
> [13 (perseguição fiel)](backlog/13-perseguicao-fiel.md) e [14 (mapa ASCII)](backlog/14-mapa-ascii.md).

- **Configurar quantidade/densidade de inimigos** — hoje `chances_percentual.tripulante_na_sala`
  (`data/config.json`) já controla a chance de haver *algum* tripulante por sala, mas não dá pra
  ajustar a densidade geral de inimigos no mapa além disso (ex.: um modo "mais vazio"/"mais cheio",
  ou variar a chance por tipo de sala). Pedido do usuário durante o Pacote 13, ao mexer na
  fuga/perseguição — avaliar se basta expor mais parâmetros em `chances_percentual` ou se vale um
  campo novo dedicado a densidade.
> Nota: "Scripts de instalação (Linux e macOS)" que estava aqui já foi implementado —
> `aventureiro/scripts/install-deps.sh` (checa/instala via `apt`/`dnf`/`pacman`/`brew`, com
> confirmação antes de rodar qualquer coisa), com instruções por SO no `aventureiro/README.md`.

- **Gerador de labirinto por sorteio independente de porta** — algoritmo alternativo ao atual
  (árvore geradora DFS + portas extras, `map.c`, ver [Pacote 25](backlog/25-densidade-labirinto.md)):
  sortear cada porta entre salas vizinhas de forma independente (mais perto da hipótese do
  `handover_aventureiro_c.md` sobre como o original gerava o mapa) e rodar um passo de reparo de
  conectividade (`mapa_totalmente_conectado()` já existe pra checar) unindo componentes
  desconectados no final. Produziria uma distribuição de portas mais orgânica em vez do viés
  "estrutura de árvore com atalhos por cima". Só vale a pena se ajustar
  `chance_porta_extra_labirinto` (a solução aplicada no Pacote 25) não for suficiente.

Guardar mais ideias aqui conforme surgirem durante a implementação dos pacotes atuais.

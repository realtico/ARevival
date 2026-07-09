# Pacote 26 — ajuda como barra de comandos permanente na base da tela

**Tamanho:** M · **Depende de:** [Pacote 11](11-melhorias-jogabilidade.md), [Pacote 17](17-mapa-continuo.md)

## Objetivo

Hoje a ajuda (`H`) é um pseudo-comando que limpa o log e imprime a lista completa dos 10 comandos
por extenso (`mostrar_ajuda()`, [game.c](../../aventureiro/src/game.c)) — o jogador precisa lembrar
de apertar `H` toda vez que esquecer um número. Sugestão do usuário: virar uma **barra permanente na
base da tela**, sempre visível, só com palavras-chave curtas (não a frase inteira de cada comando) —
mesmo espírito do Pacote 17, que tirou o mapa de "sob demanda via tecla" pra "painel sempre visível".

## Entregáveis

- **Layout**: nova `WINDOW*` (`janela_barra`) de 1 linha (ou 2, se não couber em uma só — decidir
  com base em largura mínima) fixada na última linha do terminal, largura total (`COLS`). Isso
  reduz a altura disponível pro log/painel de mapa em 1-2 linhas — ajustar
  `janela_log`/`janela_mapa` (hoje calculadas em `ui_iniciar()`, [ui.c](../../aventureiro/src/ui.c))
  pra descontar essa altura, do mesmo jeito que o painel de mapa já desconta largura do log.
- **Conteúdo**: palavras-chave curtas por comando, não frases completas. Ex. (a validar com
  contagem de colunas real, grid 8x8 desenhando o mapa some largura à direita):
  `0:Mover 1:Atacar 2:Fugir 3:Arma 4:Falar 5:Escudo 6:Remédio 7:Status 8:Examinar 9:Teleporte`.
  Isso é bem mais longo que 80 colunas com espaçamento legível — decidir entre: abreviar mais
  (`0-Mv 1-At ...`), ou aceitar que só cabe uma live nos terminais mais largos e cair pra um
  fallback (ex. só os números "0-9" sem legenda) em terminais estreitos, igual ao painel de mapa já
  faz (Pacote 17: unir/sumir com base em largura mínima).
- **Decidir o destino do comando `H`**: remover de vez (barra já cobre a necessidade, mesmo default
  do Pacote 17 quando o mapa virou painel) ou manter como forma de ver a versão "por extenso"
  (frases completas, pra quem quiser mais contexto que só a palavra-chave). Se mantido, atualizar
  `mostrar_ajuda()` pra não duplicar informação com a barra; se removido, tirar `ui_ler_comando()`
  (`-1`) e simplificar como o Pacote 19 fez pro `M`.
- Atualizar a tela de título (`game_tela_titulo()`) pra não listar mais os comandos por extenso se a
  barra permanente já cobre isso (evitar redundância de texto).
- Terminal pequeno demais pra barra + HUD + log/mapa: mesma filosofia do Pacote 17 (não corrompe a
  tela, barra vira no-op silencioso ou reduz pro fallback "só números" antes de sumir de vez).
- `tests/smoke_test.py` (pexpect+pyte): nova verificação confirmando a barra aparece sem apertar
  nada, igual `verificar_painel_mapa_visivel` (Pacote 17) fez pro mapa.

## Critério de aceite

Jogando manualmente: a barra de comandos aparece sempre na última linha, com palavras-chave legíveis
(não frases completas), sem precisar apertar `H`. HUD, log e painel de mapa continuam funcionando
normalmente acima dela. Terminal pequeno demais tem comportamento definido (não corrompe a tela).
`tests/smoke_test.py` atualizado e passando.

# Pacote 16 — bug: acentos quebrados no Linux

**Tamanho:** S · **Depende de:** [Pacote 10](10-polimento.md)

## Objetivo

No macOS os acentos renderizam certo. No Linux (relatado rodando dentro de um terminal PowerShell —
provavelmente WSL), nomes com acento saem como lixo tipo `DetenM-CM-'M-CM-#o` em vez de `Detenção`.

Hipótese (ver comentário já existente em
[ui.c:19-31](../../aventureiro/src/ui.c#L19-L31)): `setlocale(LC_ALL, "")` em `ui_iniciar()` depende
da locale do **ambiente** ter uma variante `.UTF-8` disponível (`LANG`/`LC_ALL`). No macOS isso quase
sempre vem configurado por padrão no terminal; em muitas instalações Linux mínimas/WSL/Docker, a
locale UTF-8 não está gerada ou as variáveis não estão exportadas nessa sessão — `setlocale` cai
silenciosamente pra `"C"`/`"POSIX"`, e aí volta o problema que o comentário já descreve (ncurses conta
cada byte de um caractere multi-byte como uma coluna, e o terminal mostra os bytes crus em notação
`M-x`).

## Entregáveis

- **Confirmar a hipótese**: rodar `locale` e `locale -a` no ambiente Linux que reproduz o bug; ver se
  existe alguma `*.UTF-8` na lista e se `LANG`/`LC_ALL` estão setados.
- **Fallback em `ui_iniciar()`** (`ui.c`): se `setlocale(LC_ALL, "")` não resultar numa locale UTF-8
  (checar o retorno / usar `nl_langinfo(CODESET)` pra confirmar `"UTF-8"`), tentar explicitamente
  uma locale UTF-8 conhecida (ex. `"C.UTF-8"`, disponível em quase toda distro Linux moderna mesmo
  sem locale extra gerada) antes de desistir.
- Se mesmo com fallback a distro não tiver nenhuma UTF-8 instalada, decidir se vale um aviso claro no
  início do jogo (em vez de tela quebrada silenciosa) e/ou nota no
  `aventureiro/scripts/install-deps.sh` / README pedindo pra gerar uma locale UTF-8 no Linux.

## Critério de aceite

No mesmo ambiente Linux onde o bug foi reproduzido (screenshot: sala mostrando
"Cela de DetenM-CM-'M-CM-#o" em vez de "Cela de Detenção"), rodar o binário mostra os acentos
corretos, sem depender do usuário já ter `LANG`/`LC_ALL` UTF-8 setados na shell.

**Resolvido e confirmado.** A hipótese original (locale não-UTF-8 no ambiente) era só metade da
causa. Reproduzindo o bug de verdade num Linux (Ubuntu 24.04, com `LANG=en_US.UTF-8` **já setado
corretamente** no ambiente — ou seja, com a locale supostamente "correta"), o texto acentuado ainda
saía como `DepM-CM-3sito` em vez de `Depósito`. Causa real: o `Makefile` linkava contra a
`libncurses` **narrow** (8-bit, via `pkg-config --libs ncurses`), que trata cada *byte* de uma
sequência UTF-8 multi-byte como uma célula separada — independente da locale do processo — e usa a
notação `M-x` do `unctrl()` pra desenhar bytes que não reconhece. Só a variante **wide**
(`libncursesw`, via `pkg-config --libs ncursesw`) sabe combinar bytes multi-byte num único
caractere. No macOS a libncurses do sistema é sempre wide por baixo dos panos (não existe uma
variante narrow separada), por isso o bug nunca reproduzia lá mesmo com o link "errado" — daí a
suspeita anterior de diferença BSD/Linux estar certa, só que a causa raiz era outra.

Correção: `Makefile` agora usa `pkg-config --cflags/--libs ncursesw` em vez de `ncurses`.
`garantir_locale_utf8()` em `ui.c` continua no código como a outra metade do fix (garante que a
locale seja UTF-8 pra `libncursesw` ter o que decodificar) — só trocar a lib não bastaria se o
processo ainda estivesse preso em locale `"C"`.

Verificação: capturado o output bruto do binário via `pexpect` (pty real, sem depender de olho
humano), navegando por várias salas com nomes acentuados. Com a lib narrow (código antigo): 17
ocorrências literais de `M-C` no stream de bytes — reproduz exatamente o bug do screenshot. Com
`libncursesw` (fix): 0 ocorrências de `M-C`, 17 sequências UTF-8 cruas (`\xc3\x..`) corretas no
lugar. `make test` (fuzzing via `tests/smoke_test.py`) permanece sem crash. Falta só validar em
WSL/Termux de verdade (ambientes citados originalmente) pra fechar o loop, mas o mecanismo do bug e
do fix já está provado, não é mais hipótese.

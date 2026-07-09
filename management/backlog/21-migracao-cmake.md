# Pacote 21 — migração para CMake com verificação de pré-requisitos

**Tamanho:** S · **Depende de:** [Pacote 9](09-main-testes.md), [Pacote 16](16-bug-acentos-linux.md)

## Objetivo

O build hoje é um `Makefile` artesanal + `scripts/install-deps.sh` (script bash que detecta
SO/gerenciador de pacotes por `case`, checa binários com `command -v`, e sugere/roda o comando de
instalação). Funciona, mas cada plataforma nova precisa de um `case` novo no script — e é
exatamente esse o problema real: `install-deps.sh` só reconhece `apt`/`dnf`/`pacman`/`brew`, então
quando o Termux (próximo alvo de teste, ver `management/README.md`) for testado, ele cai no branch
"SO não reconhecido" mesmo o Termux tendo `pkg-config`/`ncursesw`/`gcc` disponíveis via `pkg`.

CMake resolve isso generalizando a *verificação* (não a instalação): em vez de perguntar "que
distro é essa, que comando eu sugiro", ele simplesmente tenta achar o compilador/pkg-config/libs e
falha alto, na hora do `configure`, com uma mensagem específica de que faltou — funciona igual em
qualquer plataforma com CMake instalado, sem precisar de um branch por gerenciador de pacotes.

## Entregáveis

- **`CMakeLists.txt` na raiz de `aventureiro/`**, substituindo o `Makefile`, replicando exatamente
  o comportamento atual:
  - C11 (`set(CMAKE_C_STANDARD 11)` / `CMAKE_C_STANDARD_REQUIRED ON`), flags `-Wall -Wextra -Werror`
    em todos os fontes (hoje compilam limpos com isso, inclusive o `cJSON.c` vendorizado).
  - **ncursesw via `pkg-config`, não `find_package(Curses)`** — o `FindCurses` genérico do CMake
    historicamente não distingue de forma confiável a variante *wide* da *narrow*, e foi
    exatamente a lib narrow (`ncurses` em vez de `ncursesw`) a causa raiz do bug de acentos
    quebrados no Linux corrigido no Pacote 16. Usar `find_package(PkgConfig REQUIRED)` +
    `pkg_check_modules(NCURSESW REQUIRED IMPORTED_TARGET ncursesw)` mantém o mesmo mecanismo já
    validado, e falha o `configure` com mensagem clara ("A required package was not found:
    ncursesw") em vez de silenciosamente linkar a lib errada.
  - Binário final em `build/aventureiro` (mesmo caminho de hoje) — `tests/smoke_test.py` e o README
    já assumem esse caminho, não deve mudar.
- **Alvo de teste equivalente a `make test`**: `enable_testing()` + `add_test(...)` rodando
  `tests/smoke_test.py` contra o binário, mais um alvo nomeado `test` pra manter o comando
  reconhecível (`cmake --build build --target test`, ou `ctest --output-on-failure` direto).
  Python3 é dependência só do teste, não do build — se não for encontrado
  (`find_package(Python3 COMPONENTS Interpreter)`), o `configure` deve seguir normalmente e só
  avisar que o alvo de teste não vai estar disponível, nunca falhar o build do jogo por isso.
- **Remover o `Makefile`** (substituído, não duplicado — dois build systems paralelos divergem com
  o tempo).
- **Atualizar `scripts/install-deps.sh`**: trocar a checagem/instalação de `make` por `cmake` em
  cada branch de gerenciador (`apt`: adicionar `cmake` ao `apt-get install`; `dnf`: adicionar
  `cmake`; `pacman`: adicionar `cmake`; `brew`: adicionar `cmake`). Manter a mesma filosofia do
  script (mostra o comando, pede confirmação, não instala nada silenciosamente).
- **Atualizar `README.md`** (seções "Requisitos" e "Build e execução") pros novos comandos:
  `cmake -B build && cmake --build build`, `cmake --build build --target run` (ou continuar
  chamando `./build/aventureiro` direto), `ctest --test-dir build`.

## Critério de aceite

`rm -rf build && cmake -B build && cmake --build build` produz `build/aventureiro` sem warnings,
idêntico em comportamento ao binário do `Makefile` antigo. Com `ncursesw` fora do `PKG_CONFIG_PATH`
(simulando a ausência do pacote), `cmake -B build` falha na etapa de configure com uma mensagem
específica sobre o pacote faltante — não um erro de linker genérico depois de já ter compilado tudo.
`ctest --test-dir build` (ou `cmake --build build --target test`) roda o smoke test e passa.
`tests/smoke_test.py build/aventureiro` continua funcionando sem alteração.

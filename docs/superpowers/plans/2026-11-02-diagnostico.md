# Diagnóstico USB com interface — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Entregar ELF experimental que lê a guitarra conhecida e mostra os comandos ao vivo.
**Architecture:** Core C portátil para parsing/validação; adaptador PS5 ugen; loop limitado de diagnóstico e HTTP local com HTML embutido. Browser pelo exemplo oficial do SDK.
**Tech Stack:** C11, LLVM 18.1.8, SDK v0.45, PowerShell, Node para gerar assets/testes de replay.
**Spec:** `docs/superpowers/specs/2026-11-02-diagnostico.md`.

## Global Constraints
Todos os limites da especificação se aplicam; nenhuma execução no console. Trabalhar na branch `codex/diagnostic-elf` no diretório já escolhido pelo usuário; preservar o relatório e os clones originais. Não publicar conteúdo do GH5.

## Review Focus
- SDK CRT altera credenciais automaticamente: documentar e conferir dependências.
- Diferenças de ABI ioctl em 13.60: erros encerram a tentativa, sem fallback agressivo.
- Interface ocupada: não destacar driver; informar bloqueio.
- Reconexão/endereço USB reutilizado: descritores conferidos em toda abertura.
- Browser falha ou cliente HTTP lento: leitura e prazo não devem ficar presos.

## Task 1: Core e replay
Files: `src/guitar.h`, `src/guitar.c`, `tests/test_core.c`, `tests/replay.c`, `tools/test.ps1`.
Interfaces: `gh_decode(report,len,state)`; `gh_select_config(vid,pid,rev,cfg,len,target)`; `gh_accept(tracker,report,len,time_us)`.
- [x] Escrever testes de comandos capturados, acordes 06/1f, PS10, neutral0f, whammy, tamanho inválido, descritores malformados e dispositivos excluídos; observar RED.
- [x] Implementar core mínimo, validar GREEN e replay das três capturas.

## Task 2: USB, HTTP e interface
Files: `src/usb_ps5.c`, `src/usb_ps5.h`, `src/main.c`, `web/index.html`, `tools/build.ps1`.
Interfaces: usa core; USB via callbacks reais na produção e fake somente nos testes para erros/disconnect; HTTP sem bloquear E/S USB.
- [x] Escrever teste que rejeita endpoint sem identidade/descritor e encerra com EAGAIN/desconexão sem estados presos; observar RED.
- [x] Implementar enumeração limitada, endpoint validado, UI local, browser e stop/prazo.
- [x] Compilar ELF, inspecionar dependências/arquitetura e testar UI com backend de simulação claramente identificado.

## Task 3: Entrega verificável
Files: `README.md`, `THIRD_PARTY.md`, `LICENSE`, `dist/`, relatório atualizado.
- [x] Registrar investigação de ProsperoMgr/ShadowMountPlus e achados da imagem, com commits/hashes.
- [x] Executar suíte completa e revisão independente; corrigir falhas importantes.
- [x] Empacotar ELF, instruções, fontes correspondentes e checksum; registrar ausência de teste no PS5.

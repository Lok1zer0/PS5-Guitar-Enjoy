# Registro de validação local — 02/11/2026

Escopo: diagnóstico experimental, **sem execução no PS5**.

| Verificação | Resultado |
|---|---|
| `tools/test.ps1` | 122 core + 50 USB guard + 62 HTTP, zero falhas |
| Replay físico | 5975 + 5982 + 4477 relatórios, três capturas PASS |
| `tests/ui.cjs` | Edge headless, sessão nova/acorde/parada com resposta atrasada PASS |
| `tools/build.ps1` | LLVM 18.1.8 + SDK v0.45, ELF64 x86-64 PIE |
| Link | `--no-undefined`, alinhamento 0x4000, imports conferidos |
| Console 13.60 | Não executado; ABI/permissões/browser/USB pendentes |

Revisão independente realizada em contexto separado, com inspeção das fontes e
referências. Correções verificadas por regressão RED → GREEN:

1. Recebimento de transferência curta configurado no endpoint validado; falha
   de configuração fecha ambas as fds, preserva errno e não inicia leitura.
2. Identidade/configuração revalidadas na fd IN e descritores RX atuais
   conferidos, para detectar troca de dispositivo/alternate setting.
3. Validade de relatório por sessão; reconectar não apresenta estado antigo
   como entrada atual, e o intervalo de desconexão não entra na estatística.
4. Resposta/falha HTTP atrasada não altera a tela depois de confirmar parada.

Riscos do CRT documentados: ajustes de permissões ocorrem antes de `main`, sem
rollback pelo aplicativo; loader em processo separado recomendado. Timeout é
cooperativo, não watchdog de chamadas kernel. Não houve garantia de risco zero.

Não ficaram achados importantes da revisão sem correção. Continuam pendências
de teste no console e de integração futura com controle virtual/GH5.

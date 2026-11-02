# Dependências, fontes e atribuições

O diagnóstico está sob GPL-3.0-or-later. Os módulos de parser e guarda foram
escritos para este projeto a partir das capturas físicas e da documentação das
APIs. Não incluem o emulador PS5SX2, música ou executável de GH5.

## Código ligado ao ELF

- **ps5-payload-sdk v0.45**, commit
  `a7685ce8e97ce1f8a4f38253a41d54241c9124f1`:
  [fonte fixado](https://github.com/ps5-payload-dev/sdk/tree/a7685ce8e97ce1f8a4f38253a41d54241c9124f1).
  CRT, biblioteca C adaptadora, stubs de bibliotecas Sony e linker script.
  GPLv3 ou posterior por padrão; autores/avisos nos arquivos, incluindo
  John Törnblom. Headers FreeBSD mantêm suas licenças BSD individuais.
- Fluxo de abertura do navegador e suas assinaturas seguem
  [`samples/browser/main.c`](https://github.com/ps5-payload-dev/sdk/blob/a7685ce8e97ce1f8a4f38253a41d54241c9124f1/samples/browser/main.c),
  copyright 2023 John Törnblom, GPL-3.0-or-later.
- O arquivo `ps5-payload-sdk-v0.45-source.zip` incluído no pacote de entrega
  contém o fonte correspondente do SDK, com os avisos originais. O SDK binário
  oficial é utilizado no build; a biblioteca Sony do console não é distribuída.

## Projetos consultados, sem incorporar seus payloads

- ProsperoMgr `89987cacc20c236b4318a3dd327f3b0dc9de91d2`: UI web e formato da
  notificação simples. GPLv3, conferir LICENSE e avisos individuais.
- ShadowMountPlus `30c5a2b791ada7a78ccbda92f4bc3b1fa1cf27dd`: build SDK e HTML
  embutido. GPLv3; componentes de terceiros têm avisos próprios.
- AnyController `2eec5ec209f7e20369144de37e2358d1b2af0241`: padrão de acesso ugen
  e receive short transfer. GPLv3. Não foi copiada sua camada de pad virtual.
- PS5SX2 `b2a886b48739395de675377b03ecce75ab3368a8`: auditoria do parser e do
  leitor USB. Arquivos examinados GPL-3.0-or-later, copyrights swordpdf e
  respectivos autores herdados. O parser C deste diagnóstico não é uma cópia
  do header C++ do PS5SX2.
- FreeBSD: headers via SDK e referência de semântica ugen. Preservados avisos no
  pacote de fonte do SDK.

Os commits, caminhos, funções e limites de reutilização estão no relatório.
Copiar código adicional desses projetos no futuro exige preservar seus avisos
e aplicar a licença de cada arquivo efetivamente incorporado.

## Ferramentas de desenvolvimento

- LLVM/LLD 18.1.8: ferramenta de compilação, não redistribuída no pacote.
- GCC/MinGW: testes de host, não ligado ao ELF PS5.
- Node.js / Playwright / Edge: testes da interface no PC, não dependências PS5.
- Sleuth Kit 4.15.0: análise somente para leitura da imagem, fora do payload.

O pacote não contém chaves, firmware, biblioteca proprietária de SDK oficial
Sony, jogo, XEX ou SELF do GH5. A imagem original ficou inalterada.

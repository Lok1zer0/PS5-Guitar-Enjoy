# PS5 Guitar Enjoy

Diagnóstico experimental para a guitarra PS3 do usuário: receptor HID
**12BA:0100, revisão 0005**, atrás do hub **1430:4734**.

O ELF lê botões, palhetada, whammy e slider e tenta abrir uma tela no navegador
do PS5. **Ainda não encaminha comandos aos jogos.** Firmware pretendido: **13.60**;
o build foi validado no PC, mas **não foi executado no console**.

## Arquivos

- `dist/ps5-guitar-diagnostic.elf`: payload experimental.
- `dist/SHA256SUMS`: hash do ELF.
- `TESTE_NO_PS5.md`: procedimento e limitações para o primeiro teste.
- `RELATORIO_VIABILIDADE.md`: pesquisa, análise da imagem GH5 e evidências.
- `src/`, `web/`, `tests/`, `tools/`: fonte e testes correspondentes.

## Limites e runtime

O aplicativo não destaca drivers, não reclama interfaces, não envia relatórios
de saída/feature, não cria controle virtual e não modifica arquivos no console.
Confere descritores antes e depois de abrir a entrada; falha sem forçar acesso.
O servidor escuta somente no próprio PS5 (`127.0.0.1:18765`).

Há parada pela tela e limite cooperativo de três minutos. Isso não é watchdog
capaz de interromper chamadas kernel travadas.

**O CRT padrão do ps5-payload-sdk ajusta permissões do processo antes de `main`.**
Ele altera capacidades/atributos e limites de syscall, sem restauração neste
diagnóstico. Preferir o loader já funcional em **processo separado**. Parar o
diagnóstico libera seus recursos; não representa rollback do runtime. Um ELF
novo não herda garantia de estabilidade de outro projeto que use o mesmo SDK.

## Build no Windows

Ferramentas utilizadas: LLVM/LLD **18.1.8**, SDK **v0.45** oficial. Não requer WSL
para este projeto. Baixe as distribuições dos projetos oficiais; não há download
automático nem envio ao console nos scripts.

```powershell
./tools/build.ps1 -Sdk 'C:/toolchains/ps5-payload-sdk' -Llvm 'C:/toolchains/llvm18/bin'
```

O script compila C11 para `x86_64-sie-ps5` e liga com o CRT/linker script do SDK.
Não utilize MinGW/MSVC para gerar o ELF PS5. Os parâmetros padrão do script
apontam para a instalação portátil preparada nesta máquina; passe os caminhos
explicitamente em outro PC. A inspeção fica em `build/elf-inspection.txt`.

Testes de lógica no Windows, com GCC/MinGW C11:

```powershell
./tools/test.ps1 -CC 'C:/Strawberry/c/bin/gcc.exe'
```

Teste opcional da interface: Node.js, Playwright e Edge instalados:

```powershell
node tests/ui.cjs 'C:/caminho/node_modules/playwright'
```

Esse teste usa servidor de dados sintéticos local e **não se conecta ao PS5**.
As fixtures binárias são relatórios HID capturados no Windows em 02/11/2026;
seus hashes estão em `tests/fixtures/SHA256SUMS`. O passo de tempo do replay é
sintético; não mede a frequência real nem latência da guitarra.

## Achado sobre o GH5 fornecido

A imagem foi inspecionada somente para leitura. Contém `game/default.xex`, um
SELF x86-64, configuração `gh5_pad_mode = "guitar"` e `GuitarAdapter` no runtime.
O README interno documenta **Cross segurado + D-pad cima/baixo** para tocar o
fret verde. Outros frets: Circle, Triangle, Square e L1. Exige palhetada separada.
O hash do executável difere do registrado nesse README, portanto o mapeamento
precisa ser confirmado no console. Nenhum arquivo do jogo integra este projeto.

Licença do diagnóstico: **GPL-3.0-or-later**. Dependências e créditos em
[THIRD_PARTY.md](THIRD_PARTY.md).

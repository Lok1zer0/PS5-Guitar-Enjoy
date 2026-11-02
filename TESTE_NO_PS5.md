# Primeiro teste no PS5 — diagnóstico v0.1

**Estado:** ELF compilado e inspecionado no Windows; ainda não executado em PS5.
O objetivo é confirmar leitura do receptor no firmware 13.60. Este arquivo
**não libera a guitarra nos jogos** e não mede latência de gameplay.

## Antes de carregar

1. Termine o jogo e faça o primeiro teste na tela inicial, mantendo o DualSense
   disponível. Não configure autostart nem modo residente permanente.
2. Conecte o receptor original ao PS5 e sincronize a guitarra. O programa procura
   o filho `12BA:0100 rev.0005`, não o hub `1430:4734`. Outros modelos/revisões
   são ignorados nesta versão.
3. Use o loader ELF que já funciona no seu jailbreak. Prefira execução em
   **processo separado**, como a arquitetura do ps5-payload-elfldr. Não carregue
   dentro de ShellUI ou de um jogo por injeção de processo.
4. Confira o hash do arquivo com `Get-FileHash -Algorithm SHA256` no Windows e
   compare com `SHA256SUMS` do mesmo pacote. O script de build nunca envia o ELF.

O aplicativo não instala nada no armazenamento do console. Entretanto, o
**runtime padrão do SDK altera permissões do processo antes de iniciar o
diagnóstico** e não as restaura aqui. Um loader que reutilize um processo pode
manter esses ajustes depois que `main` retorna. Não há garantia de risco zero,
e o limite de três minutos não interrompe um ioctl que trave dentro do kernel.

## Carregar e observar

1. Envie/execute `ps5-guitar-diagnostic.elf` pelo procedimento habitual do seu
   loader. Não envie duas cópias simultâneas. A porta local impede duas sessões
   do aplicativo, mas o CRT é inicializado antes desse teste.
2. O ELF tenta abrir o navegador com a interface. A URL com token aparece no log
   do loader: `http://127.0.0.1:18765/<token>/`. Essa URL é **do próprio PS5**;
   abri-la no PC acessaria o PC e não o console.
3. Se a janela não abrir, aguarde o limite da sessão e consulte o log do loader.
   Não repita o envio em sequência para tentar destravar o navegador.
4. A tela deve passar de **Procurando o receptor** para **Receptor aberto**, e
   só depois para **Guitarra detectada**, quando chegar um relatório válido.
5. Teste verde, vermelho, amarelo, azul e laranja individualmente. Depois teste
   verde+vermelho e cinco frets, strum cima/baixo, Start, Select, PS e whammy.
   Os contadores devem registrar pulsos curtos mesmo que a tela não os mostre
   durante a atualização a 10 Hz. Slider e aceleração são valores brutos.
6. Inclinação/Star Power não está validada; o acelerômetro não variou na captura
   Windows. Não interpretar isso como diagnóstico conclusivo de defeito físico.
7. Desconecte e reconecte o dongle uma vez: indicadores devem apagar e aguardar
   novos relatórios. A reenumeração é feita a cada três segundos.

## Encerrar e registrar o resultado

Clique **Encerrar diagnóstico** e confirme a mensagem. O programa fecha USB e
servidor; a janela do navegador pode permanecer aberta e pode ser fechada
manualmente. Sem ação, ele tenta encerrar após 180 segundos de tempo monotônico.
Se a comunicação sumir, a tela informa indisponibilidade; isso, isoladamente,
não comprova que o processo terminou.

O botão **Copiar dados** copia o último diagnóstico apresentado. Registre:

- endpoint, tamanho máximo, relatórios válidos/inválidos;
- resposta de todos os botões, acordes e whammy;
- reconexão e mensagem de encerramento;
- `errno` e linhas `[GuitarEnjoy]` do log do loader, se não houver leitura;
- eventual perda do DualSense/USB, travamento ou mensagem do sistema.

Falha em ioctl, endpoint ocupado ou identidade divergente **não aciona detach,
patch ou tentativa em endpoints desconhecidos**. Esse erro é um resultado útil
para decidir a próxima alteração. Não há patch de kernel novo neste diagnóstico.

## Teste separado no GH5 intacto

Sem o diagnóstico em execução, tente **segurar Cross e usar D-pad cima/baixo**
no gameplay. O arquivo interno documenta esse esquema de guitarra Xbox 360:

| Comando | DualSense documentado |
|---|---|
| Verde / vermelho / amarelo / azul / laranja | Cross / Circle / Triangle / Square / L1 |
| Strum | D-pad cima/baixo ou analógico esquerdo vertical |
| Whammy | Analógico direito X ou R2 |
| Star Power | R1 ou clique no touchpad |
| Pausa | Options |

Ainda não é o esquema PS2 com notas disparadas só pelos frets. O README interno
pode corresponder a outro build; se não funcionar, investigar os logs do port e
a configuração realmente carregada, sem modificar a imagem por tentativa.

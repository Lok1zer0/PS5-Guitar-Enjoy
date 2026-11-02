# Diagnóstico PS5 Guitar Enjoy v0.1

Autorizado pelo usuário em 2026-11-02: ELF com leitura ao vivo e interface simples.
O relatório anterior continua sendo a pesquisa de referência; esta etapa valida
USB no console, antes da integração com o sistema ou com o jogo.

- C11, ps5-payload-sdk v0.45; compilação local Windows com LLVM 18.1.8.
- Interface HTML embutida, servidor HTTP somente em 127.0.0.1:18765;
  abertura pelo sample `samples/browser/main.c` do SDK (UserService + SystemService).
- Reconhecer somente 12ba:0100 revisão 0005, interface HID não boot, alt 0,
  endpoint interrupt IN validado no descritor. Nunca ler endpoints de outros IDs.
- Não destacar drivers, reclamar interfaces, enviar relatórios/feature requests,
  criar controles virtuais ou alterar bibliotecas/arquivos do console.
- Abertura de endpoint somente O_RDONLY|O_NONBLOCK; poll com espera, desconexão
  limpa e reenumeração limitada. Falha de permissão gera diagnóstico, sem escalada.
- Revalidar dispositivo/configuração na fd IN, conferir descritores RX ativos e
  ativar USB_SET_RX_SHORT_XFER apenas nessa fd (política de recepção no host).
- Relatórios exatamente 27 bytes; parser baseado nas capturas físicas Windows.
  Mostrar cinco frets, strum, Start, Select, PS, whammy, slider e aceleração bruta.
  Inclinação/Star Power não validada, sem inventar detecção.
- Contar transições de cada botão, inclusive pulsos entre atualizações da tela.
- Encerrar por botão ou após 180 segundos de relógio monotônico. Sem instalação
  permanente, autostart ou persistência. O runtime padrão do SDK tem patches de
  credenciais/syscall: o artefato deve documentar isso explicitamente.
- Testes no host para parser, seleção de interface e falhas de E/S; compilar ELF,
  inspecionar cabeçalhos/imports e revisão independente. Sem executar no PS5.
- Segurança absoluta, compatibilidade 13.60 e latência final não são comprovadas
  por esses testes. O diagnóstico não habilita guitarra em jogos.

Imagem GH5 fornecida: somente leitura com Sleuth Kit; não distribuir executável,
imagens, músicas ou recursos do jogo no repositório/pacote.

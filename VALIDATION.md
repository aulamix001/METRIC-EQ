# Validação da fundação

## Escopo

A implementação atual é somente a Fase 1, com **um parâmetro: bypass**. [AUDIT.md](docs/AUDIT.md) relaciona os comandos ausentes. A imagem conceitual da conversa não é uma captura do binário.

## Testes executados localmente (Linux x86_64)

GCC 14.2.0, CMake 4.4.4, Ninja 1.13.2 e JUCE 8.0.12, commit `29396c22c93392d6738e021b83196283d6e4d850`.

- `foundation`: 160 casos de preservação exata de áudio (5 sample rates × 2 buffers × 2 layouts × bypass ativo/inativo × 4 sinais), 20 restaurações de estado e entradas inválidas.
- `parameter_mapping`: inventário de exatamente um parâmetro, metadados, índice do controle no host, GUI → host com gesto de automação completo, host → GUI e estado → GUI.
- `lifecycle`: 50 ciclos de criação/redimensionamento/destruição de editor, instâncias independentes, prepare/release com alteração de sample rate e buffer, bypass pelo callback do host e rejeição de buses incompatíveis.
- Testes CTest executados em Release e Debug. Os resultados da execução mais recente ficam em `build-*/Testing/Temporary/LastTest.log`.

As verificações do editor via CTest criam componentes, sem apresentar uma janela de host. O pluginval adiciona testes do wrapper VST3 em processo separado, inclusive editor quando há um display.

## pluginval local

pluginval 1.0.4 nível 5 passou com seed 12345, sample rates 44.1/48/88.2/96/192 kHz e buffers 32/2048, incluindo o teste de editor em Xvfb. O primeiro ensaio sem display falhou ao abrir o editor; o ensaio sem GUI passou, e depois a suíte completa passou com display virtual. Isso separa a limitação do executor dos resultados do plugin. Log local: `/workspace/tools/pluginval/linux-validation-xvfb.log`.

## Windows x64

O workflow `Windows VST3` recompila o commit selecionado em Release e Debug com Visual Studio 2022, executa as três suítes e valida o Release com pluginval 1.0.4 nível 5 (sem desativar testes de GUI), cobrindo 44.1/48/88.2/96/192 kHz e buffers 32/2048. Logs são preservados nos artefatos da execução.

Um teste remoto só deve ser considerado aprovado após a conclusão daquela execução. `BUILD-INFO.json` vincula o ZIP ao código e ao binário. Este texto descreve o procedimento; os logs e o estado do workflow são a evidência do resultado.

O primeiro ensaio Windows desta auditoria compilou Release/Debug, passou as três suítes e o pluginval escreveu `SUCCESS`, mas o workflow marcou erro porque a invocação direta do executável GUI retornou ao PowerShell antes de sua conclusão. `scripts/validate-windows.ps1` corrige a espera com `Process.WaitForExit`, captura stdout/stderr sem bloqueio e exige tanto código de saída 0 quanto confirmação `SUCCESS`. Uma nova execução completa verifica esse fluxo corrigido.

## Pacote e download

Antes de publicar: validação PE x64 do binário, extração do ZIP e comparação de SHA256 do VST3. Depois de publicar: download por HTTPS e comparação de SHA256 do pacote. O arquivo é identificado pelo commit da branch de downloads. A disponibilidade futura do GitHub não é garantida por uma verificação pontual.

O ZIP Windows é um bundle para Windows. Os artefatos locais em `build-release` e `build-debug` são Linux, com RPATH para dependências deste executor, e não devem ser renomeados ou anunciados como Windows.

## Limitações

Sem teste em Studio One 8 ou REAPER 7. Sem teste de sessão profissional, desempenho de longa duração ou underruns. Referências, A/B, filtros, FFT, EQ Match e medidores do manual não estão implementados nem validados. Não há certificação de conformidade BS.1770/EBU R128 de um medidor que não existe.

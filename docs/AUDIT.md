# Auditoria 0.2.0 — interface e processamento reais

A versão 0.1 era apenas uma fundação com Bypass. Na 0.2, o editor anterior foi substituído e o comparador possui player, análise e monitoração reais. A captura `plugin-v02.png` vem do editor compilado, com sinais de calibração processados; não é o mockup.

## Implementado e testado

- Isolamento A/B (não soma permanente), crossfade, ganho suavizado e bypass seco.
- Player RAM em worker próprio, biblioteca de 16 slots, leitura fora do áudio, resampling linear, cues e estado com caminhos dos arquivos.
- Monitoração Stereo/Mono/L/R/Sides e Solo In Place.
- Filtros IIR estáveis de monitoração, transição de bancos, todas as inclinações/regiões, rejeição de valores não finitos.
- FIFO SPSC entre áudio/editor, FFT Hann fora do callback, magnitude calibrada, média de potência, hold e views independentes por página.
- Correlação e balanço L/R por frequência; histórico de crest factor. Os controles sem efeito nesses modos são desabilitados, em vez de se apresentarem como funcionais.
- Editor com faders, medidores segmentados, botão circular, cabeçalho/abas, gráfico central, controles e quatro colunas de referências.
- VST3 e adaptador de interoperabilidade VST2.4 independente. O adaptador legado tem fábrica, áudio, estado, automação e janela próprios. Não é um VST3 renomeado.

## Automação

`CHiTests --inventory` enumera 37 parâmetros reais. Bypass conserva o ID persistente `bypass`, índice 0, padrão 0, dois estados. Ganhos A/B: -18 a +18 dB. AB é booleano; seleção de referência/cue/monitor são parâmetros de escolha.

Os testes verificam os valores normalizados do manual:

- Referências 1–16: 1, 8, 15, 20, 25, 35, 40, 48, 54, 58, 64, 70, 77, 86, 92, 100%.
- Cues 1–4: 1, 30, 60, 100%.
- Stereo/Mono/L/R/Sides: 1, 30, 50, 80, 100%.

O arredondamento dos parâmetros de escolha foi definido explicitamente para que 70% selecione a referência 12, sem a ambiguidade de arredondamento para par. Existem cinco parâmetros de view por página, em vez de uma escolha compartilhada que se perca ao trocar de aba.

## Limitações — sem certificação de funções ausentes

Não implementados: LUFS, True Peak, PSR, Loudness Match, EQ Match, streaming por disco, ganho por referência, loop de região, Sync Offset/PDC, compact UI, presets em arquivo/arquivo portátil, banco de defaults do usuário e relink assistido completo. As abas Loudness e EQ Match estão desabilitadas com explicação. Correlation/Stereo nesta etapa mostram a média, sem todo o sistema Range/Hold do manual. Dynamics mostra crest factor, não PSR.

O limite de RAM é 256 MB por instância, incluindo versões retidas até a destruição. Isso garante que a thread de áudio não seja responsável por liberar arquivos enquanto o worker publica referências. Slots cancelados têm revisão de carregamento para impedir que um decoder atrasado ressuscite um arquivo removido. Arquivos ausentes produzem status de erro, não áudio fictício.

O resampling do player é linear; não é um SRC mastering de banda limitada. VST2 é estéreo 2-in/2-out; VST3 aceita mono/estéreo. Os dois formatos precisam de testes em DAWs reais, além de pluginval. Não há alegação de certificação no Studio One/REAPER, consumo máximo de CPU medido ou implementação integral do manual.

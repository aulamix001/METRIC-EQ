# Auditoria funcional e de automação

## Resultado

**Este código é somente a Fase 1. A imagem conceitual gerada na conversa NÃO corresponde à interface compilada. Os comandos do manual e da proposta visual ainda não estão implementados.**

O inventário executado com `CHiTests --inventory` encontra exatamente **um parâmetro de host: `bypass`**. Não há biblioteca de referências, caminho B, FFT nem engine EQ Match no código atual. Um build aprovado ou um download íntegro não certifica funções ausentes.

## Comandos reais

| Recurso | Situação | Evidência |
|---|---|---|
| Passagem de áudio mono/estéreo | Implementado | 160 casos sintéticos com comparação exata de samples |
| Bypass | Implementado; ativo e bypass são idênticos nesta fase sem DSP | GUI, automação e callback de bypass do host testados |
| Salvar/restaurar estado | Implementado para bypass | 20 round trips e rejeição de estado inválido |
| Redimensionar editor | Implementado | 50 aberturas/fechamentos, tamanhos inicial e extremos |
| Identificação do botão para o host | Corrigida nesta auditoria | `getControlParameterIndex` retorna o índice de bypass somente para esse botão |

## Mapeamento implementado

| ID persistente | Nome | Índice | Valor normalizado | Padrão | Passos | Automatizável |
|---|---|---:|---|---:|---:|---|
| bypass | Bypass | 0 | 0 = desligado, 1 = ligado | 0 | 2 | Sim |

O teste `parameter_mapping` verifica a identidade do parâmetro, vínculo de host, GUI → parâmetro, notificação com início/fim do gesto, automação → GUI e restauração de estado → GUI. Componentes sem parâmetro retornam -1. A capacidade de MIDI direto permanece desativada.

## Comandos do manual sem implementação/mapeamento

| Grupo solicitado | Situação real |
|---|---|
| Faders A/B ±18 dB e ganho por referência | Ausentes |
| Alternância A/B, transição e fonte B | Ausentes |
| Biblioteca de 16 referências, drag/drop, codecs, Disk/RAM | Ausentes |
| Play/Pause, Rewind, Latch/Cue/Sync/Manual | Ausentes |
| Cues 1–4, edição e exclusão | Ausentes |
| Loop, deslocamento, 1/2, 2x e marcadores | Ausentes |
| Sync Offset individual/global, PDC e Listen A+B | Ausentes |
| Stereo/Mono/L/R/Sides e Solo In Place | Ausentes |
| Filtros LOW/HIGH, slope, modos, presets e Spot | Ausentes |
| Zoom, LINK e edição de filtros no gráfico | Ausentes |
| Single/Dual/Layered e preferências por página | Ausentes |
| Spectrum, Correlation, Stereo Image, Dynamics | Ausentes |
| Peak/RMS/LUFS/True Peak e históricos | Ausentes |
| Loudness Match Single/All e seus quatro modos | Ausentes |
| EQ Match e captura de perfis | Ausentes |
| Compact UI, tamanhos discretos, Settings e padrões | Ausentes; existe somente redimensionamento contínuo |
| Presets nomeados, arquivo portátil e busca de áudios ausentes | Ausentes; existe somente o estado APVTS de bypass |

Não é possível certificar esses comandos como funcionais nem verificar seus percentuais no binário atual, pois os parâmetros não existem. Os campos da proposta visual não são botões clicáveis do plugin.

## Valores de seleção solicitados (requisitos futuros, NÃO implementados)

- Monitoração: Stereo 1%, Mono 30%, L 50%, R 80%, Sides 100%. O S de Solo In Place é um modificador separado.
- Cues 1–4: 1%, 30%, 60%, 100%.
- Referências 1–16: 1%, 8%, 15%, 20%, 25%, 35%, 40%, 48%, 54%, 58%, 64%, 70%, 77%, 86%, 92%, 100%.
- Cues/referências usam valores High e Low iguais; os percentuais são exemplos de seleção, não fronteiras exatas entre faixas normalizadas.

## Publicação auditável

O pacote Windows inclui este relatório e `BUILD-INFO.json` com o SHA do código-fonte, inventário de parâmetros e SHA256 do binário. O pipeline só empacota depois de CTest em Release/Debug e pluginval 1.0.4 nível 5. Verifica PE x64 e extração do ZIP, publica e baixa o arquivo novamente para comparar o SHA256. A branch `windows-downloads` é atualizada sem reescrever o histórico. Downloads por SHA de commit identificam um pacote imutável.

Essas verificações não substituem a execução no Studio One 8 ou REAPER 7. A compatibilidade nessas DAWs, desempenho em sessão, latência perceptível e estabilidade de longa duração permanecem sem teste.

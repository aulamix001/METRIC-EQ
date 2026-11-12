# Validação 0.2.0

## Suítes automatizadas

1. `foundation`: 160 casos de passagem exata, mono/estéreo, 44.1/48/88.2/96/192 kHz, buffers 32/2048, bypass ativo/inativo; estado inválido e round trips.
2. `parameter_mapping`: 37 parâmetros; GUI ↔ host, gesto de bypass, estado ↔ GUI, percentuais de referência/cue/stereo e views independentes.
3. `lifecycle`: 50 ciclos de editor, redimensionamento, instâncias independentes, prepare/release, callback de bypass e layouts rejeitados.
4. `reference_playback`: WAV real decodificado em worker, isolamento A/B, limites da transição, B analisado ao ouvir A, ganho, mono/solo/sides, bypass seco, cues/caminhos, arquivo ausente e cancelamento de load.
5. `monitor_filters`: resposta conhecida do low-pass, 12 combinações região/inclinação, automação e sanitização NaN/Inf.
6. `spectrum_calibration`: senoide calibrada no caminho FIFO → Hann → FFT do editor, magnitude esperada em dBFS e ausência de sinal fictício em B silencioso.

Os comandos cloud compilam e executam CTest em Release/Debug. Logs locais: `/workspace/tools/core-build.log`, `/workspace/tools/core-debug-build.log`. pluginval 1.0.4 nível 5 é executado separadamente para VST3 e para o adaptador VST2; no Linux, com Xvfb. Logs locais: `/workspace/tools/pluginval/v02-vst3.log` e `v02-vst2.log`.

## Windows e publicação

O workflow recompila em Visual Studio 2022 x64, roda as seis suítes em Release e Debug e executa pluginval nos dois formatos. O validador Windows é aguardado explicitamente com Process.WaitForExit: não se interpreta o LASTEXITCODE antes de um executável GUI terminar.

O empacotamento exige dois binários PE x64 diferentes, registra seus hashes e o inventário real em BUILD-INFO.json, extrai o ZIP e compara os dois arquivos. Publicação e download são verificados por SHA256. Logs de uma execução anterior não certificam o commit atual; confira o sourceCommit no manifesto.

## Interface capturada

`CHiTests --snapshot-signal docs/plugin-v02.png` gera a captura do editor real com uma referência WAV de calibração e entrada de teste processadas pelo próprio plugin. A imagem não contém músicas/leituras simuladas. `--snapshot` mostra o editor sem áudio.

## Limites

Veja [AUDIT.md](docs/AUDIT.md). Não houve teste em Studio One ou REAPER neste executor. Peak/RMS/crest não são apresentados como LUFS/True Peak/PSR. O resampling linear e o orçamento RAM de 256 MB são limitações conhecidas desta etapa. Não há afirmação de implementação integral do manual ou de metas de CPU/underruns sem medição.

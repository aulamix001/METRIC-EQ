# Validação da Fase 1

Executada no executor Linux x86_64 em 12/11/2026, com GCC 14.2.0, CMake 4.4.4, Ninja 1.13.2 e JUCE 8.0.12 (commit `29396c22c93392d6738e021b83196283d6e4d850`).

## Executado

- `./scripts/build-cloud.sh Release`: configuração, VST3 e CTest.
- `./scripts/build-cloud.sh Debug`: configuração, VST3 e CTest.
- Uma suíte `foundation` em cada configuração: 160 casos de áudio passaram (5 sample rates × 2 buffers × 2 layouts × 2 estados de bypass × 4 sinais).
- 20 ciclos de serialização/restauração de estado por configuração; estado inválido preservou os parâmetros existentes.
- Binário Release identificado como ELF x86_64; símbolos `GetPluginFactory`, `ModuleEntry` e `ModuleExit` presentes.
- Dependências dinâmicas Release resolvidas neste executor; geração de `moduleinfo.json` pelo helper VST3 do JUCE concluída.

## Artefatos locais

- `build-release/CHiMetricMatchPro_artefacts/Release/VST3/CHi Metric Match Pro.vst3`
- `build-debug/CHiMetricMatchPro_artefacts/Debug/VST3/CHi Metric Match Pro.vst3`

São bundles **Linux**, com dependências locais e RPATH para `/workspace/tools/sysroot/usr/lib/x86_64-linux-gnu`. Não são distribuíveis como binários Windows nem pacotes Linux universais. Para Windows, use o script PowerShell no Windows com Visual Studio 2022.

## Não executado / não implementado

Não houve execução no Windows, Studio One, REAPER ou pluginval. O editor foi compilado, mas sua interação visual e abertura/fechamento repetidos não foram testados. Não há medição de CPU em DAW, teste de underruns ou evidência de estabilidade em sessões profissionais.

As fases 2–6 permanecem pendentes: reprodução de referências, A/B, FFT, EQ Match, medidores normativos, interface final e validação de integração. Não são simuladas pela interface inicial.

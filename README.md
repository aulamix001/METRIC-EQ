# CHi Metric Match Pro

Implementação original de CHi Audio, com referência funcional no ADPTR Metric AB.

## Estado: Fase 1

Fundação C++20/JUCE 8.0.12: plugin VST3 mono/estéreo, passagem de áudio em ganho unitário, parâmetro de bypass automatizável, serialização APVTS e editor vetorial redimensionável de 1200 × 720. Como ainda não há processamento, áudio ativo e bypass são idênticos. Nenhum controle de recurso futuro é apresentado como funcional.

Ainda **não** implementados: A/B com arquivos, FFT, EQ Match, loudness, correlação, imagem estéreo, dinâmica e biblioteca. Não é ainda um substituto funcional do Metric AB. O próximo marco é a Fase 2 do briefing.

## Estrutura

- `Source/PluginProcessor.*`: buses, áudio, bypass e estado.
- `Source/PluginEditor.*`: interface original e controle de bypass.
- `Tests/DSPTests.cpp`: áudio e persistência de estado.
- `scripts/build-windows.ps1`: build, testes e instalação opcional.

## Windows 11 / Visual Studio 2022

Instale Visual Studio 2022 com Desktop development with C++, Windows SDK, CMake ≥3.22 e Git. Em PowerShell:

```powershell
./scripts/build-windows.ps1 -Configuration Release
./scripts/build-windows.ps1 -Configuration Debug
```

O script baixa JUCE pelo CMake, compila x64 e executa CTest. Artefato: `build-windows/CHiMetricMatchPro_artefacts/Release/VST3/CHi Metric Match Pro.vst3`.

Use `-Install` somente para solicitar instalação explícita. O script pede confirmação e não sobrescreve um plugin existente. Abra um PowerShell com permissões apropriadas se a pasta Common Files exigir administrador. A instalação não é feita automaticamente.

## Linux

Requer compilador C++20, CMake, Ninja, pkg-config, desenvolvimento ALSA, FreeType, Fontconfig e X11. Com dependências disponíveis:

```sh
cmake -S . -B build-release -G Ninja -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build build-release --parallel 2
ctest --test-dir build-release --output-on-failure
```

Para reutilizar um checkout local de JUCE, adicione `-DFETCHCONTENT_SOURCE_DIR_JUCE=/caminho/JUCE`. Repita com `build-debug` e `Debug`.

Neste executor de nuvem, as dependências foram extraídas localmente em `/workspace/tools/sysroot`, sem instalação no sistema. Use `./scripts/build-cloud.sh Release` ou `./scripts/build-cloud.sh Debug`; esse auxiliar exige o checkout JUCE em `/workspace/deps/JUCE` e o ambiente Python de ferramentas em `/workspace/tools/build`. O procedimento Linux genérico acima serve para outras máquinas com dependências instaladas.

## Validação e limites

Os testes verificam preservação exata de silêncio, impulso, senoide de 1 kHz e estéreo invertido em 5 sample rates, buffers 32/2048, mono/estéreo e bypass ativo/inativo (160 casos), além de 20 restaurações de estado e entrada de estado inválida. Não substituem testes de integração em DAW, stress, abertura/fechamento de editor ou pluginval.

Windows, Studio One 8 e REAPER 7 exigem validação nas respectivas plataformas. Um build Linux não gera um VST3 Windows.

## Uso inicial

Insira como efeito em uma faixa mono ou estéreo. A saída preserva a entrada. BYPASS é automatizável e salvo com a sessão. Nenhum áudio de referência é carregado nesta fase.

## Licenciamento

JUCE possui termos AGPLv3/comerciais próprios; revise `JUCE/LICENSE.md` antes de distribuir. A ausência de dependências comerciais obrigatórias não implica autorização para distribuir um produto fechado sem atender à licença JUCE. Nenhum código, marca ou asset proprietário da ADPTR é incluído.

# Build verificado 0.2.0

A interface de fundação da versão 0.1 foi substituída pelo editor real mostrado em [plugin-v02.png](plugin-v02.png). A versão 0.2 implementa comparação de referência; as funções restantes do manual estão discriminadas em [AUDIT.md](AUDIT.md).

- Código compilado: [`6f308ae794b972ccd61e4335ba03e9f4a6f15046`](https://github.com/aulamix001/METRIC-EQ/commit/6f308ae794b972ccd61e4335ba03e9f4a6f15046).
- [Execução Windows concluída com sucesso](https://github.com/aulamix001/METRIC-EQ/actions/runs/43258053738): Visual Studio 2022 x64, seis suítes em Release e Debug e pluginval 1.0.4 nível 5 nos dois formatos.
- Publicação imutável: [`6461815e6c684b74d5bc6d4115f3e3282339db53`](https://github.com/aulamix001/METRIC-EQ/tree/6461815e6c684b74d5bc6d4115f3e3282339db53).
- [Manifesto do build](https://raw.githubusercontent.com/aulamix001/METRIC-EQ/6461815e6c684b74d5bc6d4115f3e3282339db53/BUILD-INFO.json): versão 0.2.0, 37 parâmetros, commit de origem e hashes dos dois binários.

Os três arquivos publicados foram baixados novamente e conferidos por SHA256 e CRC. A inspeção dos executáveis confirmou PE x64 e entradas distintas: `GetPluginFactory` no VST3 e `VSTPluginMain` no VST2. O manifesto e a captura do editor estão incluídos em cada ZIP.

| Pacote | Tamanho | SHA256 |
| --- | ---: | --- |
| [VST3 Windows x64](https://raw.githubusercontent.com/aulamix001/METRIC-EQ/6461815e6c684b74d5bc6d4115f3e3282339db53/CHi-Metric-Match-Pro-VST3-Windows-x64.zip) | 3.510.462 bytes | `28045d596a944c4ca280bd142521fdef43e9a44aff8c32b94a999fbf95d0a100` |
| [VST2 Windows x64](https://raw.githubusercontent.com/aulamix001/METRIC-EQ/6461815e6c684b74d5bc6d4115f3e3282339db53/CHi-Metric-Match-Pro-VST2-Windows-x64.zip) | 3.460.972 bytes | `eab37933f6530dbe6fb2091c625d70bfbf2c333c63a5206810d9dd93a2b80681` |
| [VST3 + VST2](https://raw.githubusercontent.com/aulamix001/METRIC-EQ/6461815e6c684b74d5bc6d4115f3e3282339db53/CHi-Metric-Match-Pro-Windows-x64.zip) | 6.734.780 bytes | `69f1560b9ca2ad5fe43cfcb6a924be5d32f9bcd5e7346ac6f398208daea707f0` |

Não houve teste em REAPER ou Studio One neste executor. A aprovação no pluginval e nos testes automatizados não certifica as funções ausentes listadas na auditoria.

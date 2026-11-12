# Comprovante do build Windows auditado

## Resultado verificado

- Código-fonte: `c6ed14f2ec08b44868425c5e493e9b0f0d6d2a25`.
- [Execução Windows aprovada](https://github.com/aulamix001/METRIC-EQ/actions/runs/43229407076).
- Release e Debug: três suítes CTest aprovadas em cada configuração.
- pluginval 1.0.4 nível 5: aprovado no Windows, incluindo editor e automação.
- Empacotamento, pré-release, branch de downloads e download pelo próprio pipeline: aprovados.
- Verificação independente neste executor: ZIP baixado por HTTPS, SHA256 e CRC aprovados, PE x64 confirmado e hash do binário igual ao manifesto.
- Tamanho do ZIP: **2.826.214 bytes**. Binário interno: **6.139.392 bytes**.
- SHA256 do ZIP: `2653a4c1947f13f864fca3225969cfd9e285e9f03cb58f4b212459bb852e4c92`.
- SHA256 do VST3 interno: `80c55683d608e840e1de5b5d944c0d7bd6c65299fca90e8642f6a2e1fa00f5f7`.

## Downloads imutáveis

- [ZIP Windows x64](https://raw.githubusercontent.com/aulamix001/METRIC-EQ/7d182ebf5b8518a9a391cb93f27787777bb50c8d/CHi-Metric-Match-Pro-Windows-x64.zip)
- [SHA256SUMS.txt](https://raw.githubusercontent.com/aulamix001/METRIC-EQ/7d182ebf5b8518a9a391cb93f27787777bb50c8d/SHA256SUMS.txt)
- [Manifesto BUILD-INFO.json](https://raw.githubusercontent.com/aulamix001/METRIC-EQ/7d182ebf5b8518a9a391cb93f27787777bb50c8d/BUILD-INFO.json)

## Escopo

**Somente fundação.** Inventário real: `bypass`, índice 0, dois estados normalizados 0/1, padrão 0, automatizável. [Auditoria funcional](AUDIT.md). Os demais comandos do manual, o player A/B, os analisadores, os medidores, o EQ Match e a interface conceitual ainda não existem neste binário.

Não testado em Studio One 8 ou REAPER 7. Um download verificado neste momento não garante disponibilidade futura do serviço.

# Mapas do BedWars

O jogo inclui três arenas geradas e 18 arenas avançadas. **Layout padrão**
ligado mostra Fortaleza, Ruínas e Jardins com escolha de Rush. Desligado,
mostra os mapas importados em páginas de seis cartões. NC mantém duas equipes;
os demais mapas avançados têm oito equipes.

## Mapas importados

| Mapa | Blocos convertidos | Diamantes | Esmeraldas |
| --- | ---: | ---: | ---: |
| NC | 39.240 | 1 | 1 |
| Airshow | 120.067 | 4 | 4 |
| Amazon | 136.794 | 4 | 4 |
| Ashfire | 77.662 | 4 | 4 |
| Crogorm | 51.790 | 4 | 4 |
| Dragonstar | 72.116 | 4 | 2 |
| Easter Basket | 59.952 | 4 | 2 |
| Egg Hunt | 78.122 | 4 | 4 |
| Glacier | 69.157 | 4 | 4 |
| Hollow | 60.349 | 4 | 4 |
| Lighthouse | 70.905 | 4 | 2 |
| Orchestra | 97.526 | 4 | 4 |
| Pernicious | 89.517 | 4 | 4 |
| RoofTop | 544.898 | 4 | 2 |
| Solace | 113.626 | 4 | 4 |
| Speedway | 45.625 | 4 | 2 |
| Waterfall | 98.417 | 4 | 4 |
| Zarzul | 128.575 | 4 | 2 |

## Conversão e serviços

As arenas são lidas dos arquivos Anvil/NBT em `mapas externos/`. A geometria
é transladada para a janela ativa sem redimensionamento. Construções remotas
de edição fora da área da arena, como as existentes no mundo RoofTop,
não fazem parte da importação. A altura ativa de 192 blocos preserva as
construções altas; saves e chunks anteriores com altura 80 ou 128 são migrados
durante o carregamento.

Camas e baús mantêm suas posições. Spawns, aldeões e geradores são
configurados a partir dos marcadores e pisos das arenas e validados contra
a colisão. Os nichos de loja exigem espaço vazio para o corpo e acesso pela
frente; aldeões originais são usados como referência quando existem no arquivo.
Os geradores usam as plataformas de slabs originais, com altura de coleta
ajustada à superfície. Nenhum bloco do mapa importado é substituído para
construir geradores. Camas e baús também mantêm suas posições.
As regras, lojas e temporizadores são os do MineClone; plugins,
command blocks e entidades do servidor Java não são executados.

Materiais e estados legados são convertidos para o registro do jogo. O suporte
inclui escadas invertidas, lajes, paredes, painéis de vidro, portas, alçapões,
portões, trilhos, vinhas, plantas, bigornas, cabeças, cacau, sinalizadores,
dispensadores e barreiras invisíveis. Troncos mantêm seu eixo. As texturas
são procedurais, como no restante do projeto. Blocos desconhecidos interrompem
a conversão com diagnóstico; não são substituídos silenciosamente por ar.

Os dados comprimidos ficam incorporados ao executável. As pastas de mundos
Java não precisam ser distribuídas junto com o jogo.

## Reimportar

Na raiz do projeto, com Python 3:

```sh
python3 tools/import_nc_map.py "mapas externos/NC Bedwars 1v1"
python3 tools/import_nc_map.py "mapas externos/Airshow" --airshow --output include/airshow_map_data.inc
python3 tools/import_bedwars_maps.py
make -j"$(nproc)"
```

`data/external-maps.json` registra contagem de blocos, estados, limites e pontos
de serviço de cada arena. `include/external_maps.inc` contém os dados gerados.

## Validação

```sh
make test-maps
./build/external_maps_test --render
```

Os testes comparam todos os blocos e estados convertidos, verificam as oito
camas e seus cabeçotes, testam o piso e espaço livre dos serviços e iniciam
cada arena como host LAN. A execução gráfica também gera capturas em `build/`.
Todos os participantes precisam da mesma versão do protocolo LAN (12).

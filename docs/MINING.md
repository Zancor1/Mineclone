# Mineração

Os tempos básicos seguem Minecraft Java 1.8.9, sem encantamentos. A dureza,
os materiais e os requisitos de coleta vêm dos dados 1.8 já incluídos em
`data/minecraft-1.8/blocks.json`, com licença em
`LICENSES/PrismarineJS-minecraft-data.txt`.

A velocidade da ferramenta adequada é 2 para madeira, 4 para pedra, 6 para
ferro, 8 para diamante e 12 para ouro. Ferramentas inadequadas não recebem
esse bônus. Ouro mantém o nível de coleta da madeira.

O tempo em ticks é `ceil(dureza × divisor / velocidade)`, com divisor 30
quando a ferramenta permite coletar o bloco e 100 quando não permite.
Cada tick dura 0,05 segundo. Blocos instantâneos usam o mínimo de um tick
no jogo para evitar divisão por zero. No cliente, minerar no ar ou debaixo
d'água aplica uma penalidade de velocidade de 5 vezes para cada condição.
Slabs, escadas e outros blocos importados usam seu material e formato.

| Bloco | Mão | Madeira | Pedra | Ferro | Diamante | Ouro |
|---|---:|---:|---:|---:|---:|---:|
| Pedra, com picareta | 7,5 s | 1,15 s | 0,6 s | 0,4 s | 0,3 s | 0,2 s |
| Tábuas, com machado | 3 s | 1,5 s | 0,75 s | 0,5 s | 0,4 s | 0,25 s |
| End stone, com picareta | 15 s | 2,25 s | 1,15 s | 0,75 s | 0,6 s | 0,4 s |
| Obsidiana, com picareta | 250 s | 125 s | 62,5 s | 41,7 s | 9,4 s | 20,85 s |

Quebrar não garante a coleta: por exemplo, obsidiana exige picareta de
diamante para produzir o bloco.

A cama é uma regra própria do MineClone: ambas as metades levam **0,15 s**,
com qualquer item. O host valida o tempo antes de destruir a cama e despertar
o pinguim. A validação admite um tick de diferença entre os relógios do cliente
e do host.

## Referências

- [Minecraft Forum: tabela histórica de velocidade das picaretas](https://www.minecraftforum.net/forums/minecraft-java-edition/suggestions/55132-pick-axe-speed-reduction-things-are-going-too-fast).
- [Minecraft 1.8.9: cálculo de dureza relativa](https://github.com/Marcelektro/MavenMCP-1.8.9/blob/master/src/main/java/net/minecraft/block/Block.java).
- [Minecraft 1.8.9: materiais das ferramentas](https://github.com/Marcelektro/MavenMCP-1.8.9/blob/master/src/main/java/net/minecraft/item/Item.java).
- [Minecraft 1.8.9: eficiência e requisitos das picaretas](https://github.com/Marcelektro/MavenMCP-1.8.9/blob/master/src/main/java/net/minecraft/item/ItemPickaxe.java).

Para regenerar os metadados: `python3 tools/generate_mining18.py`.

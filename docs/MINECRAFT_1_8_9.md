# Referência de conteúdo Java 1.8.9

Este documento separa registro de conteúdo de equivalência de comportamento.
**MineClone não implementa integralmente Minecraft 1.8.9.**

## Fonte e cobertura

- [Minecraft Wiki: Java Edition 1.8.9](https://minecraft.wiki/w/Java_Edition_1.8.9)
- [Valores anteriores ao Flattening](https://minecraft.wiki/w/Java_Edition_pre-flattening_data_values)
- [PrismarineJS/minecraft-data: registros 1.8](https://github.com/PrismarineJS/minecraft-data/tree/1df9432c07c81b47326e4fb1e06d3c8d723bf4aa/data/pc/1.8)

As tabelas locais fixam o commit acima: 336 IDs de itens e 198 IDs de blocos
na fonte. O catálogo normalizado contém 631 registros, incluindo variantes e
blocos sem item correspondente. As variantes incorretas de acácia/carvalho escuro
em `17:4` e `17:5` da fonte foram omitidas; essas madeiras usam `162:0` e `162:1`.
Não é uma enumeração de todas as combinações de NBT, encantamentos, poções ou
estados direcionais. Campos de reparo/encantamento da fonte não são usados como
especificação de comportamento da versão.

Os arquivos em `include/content18_*.inc` são gerados por
`python tools/generate_content18.py`, sem dependências externas. IDs existentes
não podem ser reordenados: são persistidos nos saves. O jogo não exige Python
nem acesso à internet para compilar ou jogar.

## O que funciona nesta implementação

- Blocos cúbicos de construção: colocação, colisão, faces oclusas e persistência.
- Vidros coloridos com transparência; glowstone e lanterna do mar emitem luz.
- Ferramentas das famílias madeira, pedra, ferro, ouro e diamante: receitas,
  pilhas unitárias e desgaste ao minerar. Picaretas têm requisitos de coleta;
  machados e pás aceleram materiais compatíveis.
- Minérios de ouro, diamante, esmeralda, redstone e lápis-lazúli em regiões novas,
  com coleta condicionada à picareta adequada.
- Fundição de ferro e ouro, areia em vidro, pedregulho em pedra, argila em
  terracota/tijolo e netherrack em tijolo do Nether.
- Fabricação de ferramentas, compactação/descompactação de metais e gemas,
  tábuas por espécie, blocos 2×2, tingimento de lã, vidro e argila endurecida.

## Diferenças e sistemas ausentes

"Suporte básico" na tabela significa que o conteúdo pode ser usado pelo sistema
atual de blocos/itens. **Não significa equivalência completa ao original.**

Ainda faltam, entre outros: redstone e mecanismos, trilhos e veículos, portas e
pistões, entidades e criaturas, PvP e acertos de projéteis em entidades, encantamentos e poções,
dimensões/portais, mapas e livros editáveis, agricultura e plantio
com enxadas. Gelo não altera atrito, areia das almas não reduz velocidade,
folhas não decaem e blocos não têm orientação. As texturas são procedurais,
não as texturas originais. Espadas ainda não têm entidades para atacar.

A antiga tela textual de referência foi retirada do jogo. O inventário mostra
seleções com ícones, incluindo a aba Minigames. Esta tabela permanece somente
como documentação de cobertura; entradas indisponíveis não viram itens fictícios.

## Itens de minigames

Pérolas, bolas de fogo, TNT, maçãs douradas, baldes, arco/flechas, ovos, bolas de
neve, armaduras, cama e escada têm implementações locais descritas no README.
A bola de fogo segue a convenção de minigames (projétil), não o uso padrão de
fire charge no Minecraft. Não há PvP, entidades vivas ou regras de partidas.

## Tabela

| ID legado | Variante | Conteúdo | Suporte no MineClone |
| --- | --- | --- | --- |
| 1 | 0 | Stone | Básico: `Stone` |
| 1 | 1 | Granite | Básico: `Granite` |
| 1 | 2 | Polished Granite | Básico: `PolishedGranite` |
| 1 | 3 | Diorite | Básico: `Diorite` |
| 1 | 4 | Polished Diorite | Básico: `PolishedDiorite` |
| 1 | 5 | Andesite | Básico: `Andesite` |
| 1 | 6 | Polished Andesite | Básico: `PolishedAndesite` |
| 2 | 0 | Grass Block | Básico: `Grass` |
| 3 | 0 | Dirt | Básico: `Dirt` |
| 3 | 1 | Coarse Dirt | Básico: `CoarseDirt` |
| 3 | 2 | Podzol | Básico: `Podzol` |
| 4 | 0 | Cobblestone | Básico: `Cobblestone` |
| 5 | 0 | Oak Wood Planks | Básico: `Planks` |
| 5 | 1 | Spruce Wood Planks | Básico: `SpruceWoodPlanks` |
| 5 | 2 | Birch Wood Planks | Básico: `BirchWoodPlanks` |
| 5 | 3 | Jungle Wood Planks | Básico: `JungleWoodPlanks` |
| 5 | 4 | Acacia Wood Planks | Básico: `AcaciaPlanks` |
| 5 | 5 | Dark Oak Wood Planks | Básico: `DarkOakPlanks` |
| 6 | 0 | Oak Sapling | Indisponível |
| 6 | 1 | Spruce Sapling | Indisponível |
| 6 | 2 | Birch Sapling | Indisponível |
| 6 | 3 | Jungle Sapling | Indisponível |
| 6 | 4 | Acacia Sapling | Indisponível |
| 6 | 5 | Dark Oak Sapling | Indisponível |
| 7 | 0 | Bedrock | Básico: `Bedrock` |
| 12 | 0 | Sand | Básico: `Sand` |
| 12 | 1 | Red Sand | Básico: `RedSand` |
| 13 | 0 | Gravel | Básico: `Gravel` |
| 14 | 0 | Gold Ore | Básico: `GoldOre` |
| 15 | 0 | Iron Ore | Básico: `IronOre` |
| 16 | 0 | Coal Ore | Básico: `CoalOre` |
| 17 | 0 | Oak Wood | Básico: `Wood` |
| 17 | 1 | Spruce Wood | Básico: `SpruceWood` |
| 17 | 2 | Birch Wood | Básico: `BirchWood` |
| 17 | 3 | Jungle Wood | Básico: `JungleWood` |
| 18 | 0 | Oak Leaves | Básico: `Leaves` |
| 18 | 1 | Spruce Leaves | Básico: `SpruceLeaves` |
| 18 | 2 | Birch Leaves | Básico: `BirchLeaves` |
| 18 | 3 | Jungle Leaves | Básico: `JungleLeaves` |
| 19 | 0 | Sponge | Indisponível |
| 19 | 1 | Wet Sponge | Indisponível |
| 20 | 0 | Glass | Básico: `Glass` |
| 21 | 0 | Lapis Lazuli Ore | Básico: `LapisLazuliOre` |
| 22 | 0 | Lapis Lazuli Block | Básico: `LapisLazuliBlock` |
| 23 | 0 | Dispenser | Indisponível |
| 24 | 0 | Sandstone | Básico: `Sandstone` |
| 24 | 1 | Chiseled Sandstone | Básico: `ChiseledSandstone` |
| 24 | 2 | Smooth Sandstone | Básico: `SmoothSandstone` |
| 25 | 0 | Note Block | Indisponível |
| 27 | 0 | Powered Rail | Indisponível |
| 28 | 0 | Detector Rail | Indisponível |
| 29 | 0 | Sticky Piston | Indisponível |
| 30 | 0 | Cobweb | Indisponível |
| 31 | 0 | Shrub | Indisponível |
| 31 | 1 | Tall Grass | Básico: `TallGrass` |
| 31 | 2 | Fern | Indisponível |
| 32 | 0 | Dead Bush | Básico: `DeadBush` |
| 33 | 0 | Piston | Indisponível |
| 35 | 0 | White Wool | Básico: `WhiteWool` |
| 35 | 1 | Orange Wool | Básico: `OrangeWool` |
| 35 | 2 | Magenta Wool | Básico: `MagentaWool` |
| 35 | 3 | Light blue Wool | Básico: `LightBlueWool` |
| 35 | 4 | Yellow Wool | Básico: `YellowWool` |
| 35 | 5 | Lime Wool | Básico: `LimeWool` |
| 35 | 6 | Pink Wool | Básico: `PinkWool` |
| 35 | 7 | Gray Wool | Básico: `GrayWool` |
| 35 | 8 | Light gray Wool | Básico: `LightGrayWool` |
| 35 | 9 | Cyan Wool | Básico: `CyanWool` |
| 35 | 10 | Purple Wool | Básico: `PurpleWool` |
| 35 | 11 | Blue Wool | Básico: `BlueWool` |
| 35 | 12 | Brown Wool | Básico: `BrownWool` |
| 35 | 13 | Green Wool | Básico: `GreenWool` |
| 35 | 14 | Red Wool | Básico: `RedWool` |
| 35 | 15 | Black Wool | Básico: `BlackWool` |
| 37 | 0 | Dandelion | Básico: `Dandelion` |
| 38 | 0 | Poppy | Básico: `Rose` |
| 38 | 1 | Blue Orchid | Básico: `Orchid` |
| 38 | 2 | Allium | Básico: `Allium` |
| 38 | 3 | Azure Bluet | Indisponível |
| 38 | 4 | Red Tulip | Básico: `RedTulip` |
| 38 | 5 | Orange Tulip | Indisponível |
| 38 | 6 | White Tulip | Indisponível |
| 38 | 7 | Pink Tulip | Básico: `PinkTulip` |
| 38 | 8 | Oxeye Daisy | Indisponível |
| 39 | 0 | Brown Mushroom | Indisponível |
| 40 | 0 | Red Mushroom | Indisponível |
| 41 | 0 | Block of Gold | Básico: `BlockOfGold` |
| 42 | 0 | Block of Iron | Básico: `BlockOfIron` |
| 44 | 0 | Stone Slab | Indisponível |
| 44 | 1 | Sandstone Slab | Indisponível |
| 44 | 2 | Wooden Slab | Indisponível |
| 44 | 3 | Cobblestone Slab | Indisponível |
| 44 | 4 | Bricks Slab | Indisponível |
| 44 | 5 | Stone Bricks Slab | Indisponível |
| 44 | 6 | Nether Brick Slab | Indisponível |
| 44 | 7 | Quartz Slab | Indisponível |
| 45 | 0 | Brick | Básico: `Bricks` |
| 46 | 0 | TNT | Básico: `Tnt` |
| 47 | 0 | Bookshelf | Básico: `Bookshelf` |
| 48 | 0 | Moss Stone | Básico: `MossStone` |
| 49 | 0 | Obsidian | Básico: `Obsidian` |
| 50 | 0 | Torch | Indisponível |
| 52 | 0 | Monster Spawner | Indisponível |
| 53 | 0 | Oak Wood Stairs | Indisponível |
| 54 | 0 | Chest | Básico: `Chest` |
| 56 | 0 | Diamond Ore | Básico: `DiamondOre` |
| 57 | 0 | Block of Diamond | Básico: `BlockOfDiamond` |
| 58 | 0 | Crafting Table | Básico: `CraftingTable` |
| 60 | 0 | Farmland | Indisponível |
| 61 | 0 | Furnace | Básico: `Furnace` |
| 65 | 0 | Ladder | Básico: `Ladder` |
| 66 | 0 | Rail | Indisponível |
| 67 | 0 | Cobblestone Stairs | Indisponível |
| 69 | 0 | Lever | Indisponível |
| 70 | 0 | Stone Pressure Plate | Indisponível |
| 72 | 0 | Wooden Pressure Plate | Indisponível |
| 73 | 0 | Redstone Ore | Básico: `RedstoneOre` |
| 76 | 0 | Redstone Torch | Indisponível |
| 77 | 0 | Stone Button | Indisponível |
| 78 | 0 | Snow | Indisponível |
| 79 | 0 | Ice | Básico: `Ice` |
| 80 | 0 | Snow | Básico: `Snow` |
| 81 | 0 | Cactus | Básico: `Cactus` |
| 82 | 0 | Clay | Básico: `Clay` |
| 84 | 0 | Jukebox | Indisponível |
| 85 | 0 | Oak Fence | Indisponível |
| 86 | 0 | Pumpkin | Indisponível |
| 87 | 0 | Netherrack | Básico: `Netherrack` |
| 88 | 0 | Soul Sand | Básico: `SoulSand` |
| 89 | 0 | Glowstone | Básico: `Glowstone` |
| 91 | 0 | Jack o'Lantern | Indisponível |
| 95 | 0 | White Stained Glass | Básico: `WhiteStainedGlass` |
| 95 | 1 | Orange Stained Glass | Básico: `OrangeStainedGlass` |
| 95 | 2 | Magenta Stained Glass | Básico: `MagentaStainedGlass` |
| 95 | 3 | Light Blue Stained Glass | Básico: `LightBlueStainedGlass` |
| 95 | 4 | Yellow Stained Glass | Básico: `YellowStainedGlass` |
| 95 | 5 | Lime Stained Glass | Básico: `LimeStainedGlass` |
| 95 | 6 | Pink Stained Glass | Básico: `PinkStainedGlass` |
| 95 | 7 | Gray Stained Glass | Básico: `GrayStainedGlass` |
| 95 | 8 | Light Gray Stained Glass | Básico: `LightGrayStainedGlass` |
| 95 | 9 | Cyan Stained Glass | Básico: `CyanStainedGlass` |
| 95 | 10 | Purple Stained Glass | Básico: `PurpleStainedGlass` |
| 95 | 11 | Blue Stained Glass | Básico: `BlueStainedGlass` |
| 95 | 12 | Brown Stained Glass | Básico: `BrownStainedGlass` |
| 95 | 13 | Green Stained Glass | Básico: `GreenStainedGlass` |
| 95 | 14 | Red Stained Glass | Básico: `RedStainedGlass` |
| 95 | 15 | Black Stained Glass | Básico: `BlackStainedGlass` |
| 96 | 0 | Wooden Trapdoor | Indisponível |
| 97 | 0 | Stone Monster Egg | Indisponível |
| 97 | 1 | Cobblestone Monster Egg | Indisponível |
| 97 | 2 | Stone Brick Monster Egg | Indisponível |
| 97 | 3 | Mossy Stone Brick Monster Egg | Indisponível |
| 97 | 4 | Cracked Stone Brick Monster Egg | Indisponível |
| 97 | 5 | Chiseled Stone Brick Monster Egg | Indisponível |
| 98 | 0 | Stone Bricks | Básico: `StoneBricks` |
| 98 | 1 | Mossy Stone Bricks | Básico: `MossyStoneBricks` |
| 98 | 2 | Cracked Stone Bricks | Básico: `CrackedStoneBricks` |
| 98 | 3 | Chiseled Stone Bricks | Básico: `CarvedStone` |
| 99 | 0 | Brown Mushroom Block | Indisponível |
| 100 | 0 | Red Mushroom Block | Indisponível |
| 101 | 0 | Iron Bars | Indisponível |
| 102 | 0 | Glass Pane | Indisponível |
| 103 | 0 | Melon | Indisponível |
| 106 | 0 | Vines | Indisponível |
| 107 | 0 | Oak Fence Gate | Indisponível |
| 108 | 0 | Brick Stairs | Indisponível |
| 109 | 0 | Stone Brick Stairs | Indisponível |
| 110 | 0 | Mycelium | Básico: `Mycelium` |
| 111 | 0 | Lily Pad | Indisponível |
| 112 | 0 | Nether Brick | Básico: `NetherBrick` |
| 113 | 0 | Nether Brick Fence | Indisponível |
| 114 | 0 | Nether Brick Stairs | Indisponível |
| 116 | 0 | Enchantment Table | Indisponível |
| 120 | 0 | End Portal Frame | Indisponível |
| 121 | 0 | End Stone | Básico: `EndStone` |
| 122 | 0 | Dragon Egg | Indisponível |
| 123 | 0 | Redstone Lamp | Básico: `LampOff` |
| 126 | 0 | Oak Wood Slab | Indisponível |
| 126 | 1 | Spruce Wood Slab | Indisponível |
| 126 | 2 | Birch Wood Slab | Indisponível |
| 126 | 3 | Jungle Wood Slab | Indisponível |
| 126 | 4 | Acacia Wood Slab | Indisponível |
| 126 | 5 | Dark Oak Wood Slab | Indisponível |
| 128 | 0 | Sandstone Stairs | Indisponível |
| 129 | 0 | Emerald Ore | Básico: `EmeraldOre` |
| 130 | 0 | Ender Chest | Indisponível |
| 131 | 0 | Tripwire Hook | Indisponível |
| 133 | 0 | Block of Emerald | Básico: `BlockOfEmerald` |
| 134 | 0 | Spruce Wood Stairs | Indisponível |
| 135 | 0 | Birch Wood Stairs | Indisponível |
| 136 | 0 | Jungle Wood Stairs | Indisponível |
| 137 | 0 | Command Block | Indisponível |
| 138 | 0 | Beacon | Indisponível |
| 139 | 0 | Cobblestone Wall | Indisponível |
| 139 | 1 | Mossy Cobblestone Wall | Indisponível |
| 143 | 0 | Wooden Button | Indisponível |
| 145 | 0 | Anvil | Indisponível |
| 145 | 1 | Slightly Damaged Anvil | Indisponível |
| 145 | 2 | Very Damaged Anvil | Indisponível |
| 146 | 0 | Trapped Chest | Indisponível |
| 147 | 0 | Weighted Pressure Plate (Light) | Indisponível |
| 148 | 0 | Weighted Pressure Plate (Heavy) | Indisponível |
| 151 | 0 | Daylight Detector | Indisponível |
| 152 | 0 | Block of Redstone | Indisponível |
| 153 | 0 | Nether Quartz | Básico: `NetherQuartzOre` |
| 154 | 0 | Hopper | Indisponível |
| 155 | 0 | Block of Quartz | Básico: `BlockOfQuartz` |
| 155 | 1 | Chiseled Quartz Block | Básico: `ChiseledQuartzBlock` |
| 155 | 2 | Pillar Quartz Block | Básico: `PillarQuartzBlock` |
| 156 | 0 | Quartz Stairs | Indisponível |
| 157 | 0 | Activator Rail | Indisponível |
| 158 | 0 | Dropper | Indisponível |
| 159 | 0 | White Stained Clay | Básico: `WhiteStainedClay` |
| 159 | 1 | Orange Stained Clay | Básico: `OrangeStainedClay` |
| 159 | 2 | Magenta Stained Clay | Básico: `MagentaStainedClay` |
| 159 | 3 | Light Blue Stained Clay | Básico: `LightBlueStainedClay` |
| 159 | 4 | Yellow Stained Clay | Básico: `YellowStainedClay` |
| 159 | 5 | Lime Stained Clay | Básico: `LimeStainedClay` |
| 159 | 6 | Pink Stained Clay | Básico: `PinkStainedClay` |
| 159 | 7 | Gray Stained Clay | Básico: `GrayStainedClay` |
| 159 | 8 | Light Gray Stained Clay | Básico: `LightGrayStainedClay` |
| 159 | 9 | Cyan Stained Clay | Básico: `CyanStainedClay` |
| 159 | 10 | Purple Stained Clay | Básico: `PurpleStainedClay` |
| 159 | 11 | Blue Stained Clay | Básico: `BlueStainedClay` |
| 159 | 12 | Brown Stained Clay | Básico: `BrownStainedClay` |
| 159 | 13 | Green Stained Clay | Básico: `GreenStainedClay` |
| 159 | 14 | Red Stained Clay | Básico: `RedStainedClay` |
| 159 | 15 | Black Stained Clay | Básico: `BlackStainedClay` |
| 160 | 0 | White Stained Glass Pane | Indisponível |
| 160 | 1 | Orange Stained Glass Pane | Indisponível |
| 160 | 2 | Magenta Stained Glass Pane | Indisponível |
| 160 | 3 | Light Blue Stained Glass Pane | Indisponível |
| 160 | 4 | Yellow Stained Glass Pane | Indisponível |
| 160 | 5 | Lime Stained Glass Pane | Indisponível |
| 160 | 6 | Pink Stained Glass Pane | Indisponível |
| 160 | 7 | Gray Stained Glass Pane | Indisponível |
| 160 | 8 | Light Gray Stained Glass Pane | Indisponível |
| 160 | 9 | Cyan Stained Glass Pane | Indisponível |
| 160 | 10 | Purple Stained Glass Pane | Indisponível |
| 160 | 11 | Blue Stained Glass Pane | Indisponível |
| 160 | 12 | Brown Stained Glass Pane | Indisponível |
| 160 | 13 | Green Stained Glass Pane | Indisponível |
| 160 | 14 | Red Stained Glass Pane | Indisponível |
| 160 | 15 | Black Stained Glass Pane | Indisponível |
| 161 | 0 | Acacia Leaves | Básico: `AcaciaLeaves` |
| 161 | 1 | Dark Oak Leaves | Básico: `DarkOakLeaves` |
| 162 | 0 | Acacia Wood | Básico: `AcaciaWood0` |
| 162 | 1 | Dark Oak Wood | Básico: `DarkOakWood1` |
| 163 | 0 | Acacia Wood Stairs | Indisponível |
| 164 | 0 | Dark Oak Wood Stairs | Indisponível |
| 165 | 0 | Slime Block | Básico: `Slime` |
| 166 | 0 | Barrier | Indisponível |
| 167 | 0 | Iron Trapdoor | Indisponível |
| 168 | 0 | Prismarine | Básico: `Prismarine` |
| 168 | 1 | Prismarine Bricks | Básico: `PrismarineBricks` |
| 168 | 2 | Dark Prismarine | Básico: `DarkPrismarine` |
| 169 | 0 | Sea Lantern | Básico: `SeaLantern` |
| 170 | 0 | Hay Bale | Básico: `HayBale` |
| 171 | 0 | White Carpet | Indisponível |
| 171 | 1 | Orange Carpet | Indisponível |
| 171 | 2 | Magenta Carpet | Indisponível |
| 171 | 3 | Light Blue Carpet | Indisponível |
| 171 | 4 | Yellow Carpet | Indisponível |
| 171 | 5 | Lime Carpet | Indisponível |
| 171 | 6 | Pink Carpet | Indisponível |
| 171 | 7 | Gray Carpet | Indisponível |
| 171 | 8 | Light Gray Carpet | Indisponível |
| 171 | 9 | Cyan Carpet | Indisponível |
| 171 | 10 | Purple Carpet | Indisponível |
| 171 | 11 | Blue Carpet | Indisponível |
| 171 | 12 | Brown Carpet | Indisponível |
| 171 | 13 | Green Carpet | Indisponível |
| 171 | 14 | Red Carpet | Indisponível |
| 171 | 15 | Black Carpet | Indisponível |
| 172 | 0 | Hardened Clay | Básico: `HardenedClay` |
| 173 | 0 | Block of Coal | Básico: `BlockOfCoal` |
| 174 | 0 | Packed Ice | Básico: `PackedIce` |
| 175 | 0 | Sunflower | Indisponível |
| 175 | 1 | Lilac | Indisponível |
| 175 | 2 | Double Tallgrass | Indisponível |
| 175 | 3 | Large Fern | Indisponível |
| 175 | 4 | Rose Bush | Indisponível |
| 175 | 5 | Peony | Indisponível |
| 179 | 0 | Red Sandstone | Básico: `RedSandstone` |
| 179 | 1 | Chiseled Red Sandstone | Básico: `ChiseledRedSandstone` |
| 179 | 2 | Smooth Red Sandstone | Básico: `SmoothRedSandstone` |
| 180 | 0 | Red Sandstone Stairs | Indisponível |
| 182 | 0 | Red Sandstone Slab | Indisponível |
| 183 | 0 | Spruce Fence Gate | Indisponível |
| 184 | 0 | Birch Fence Gate | Indisponível |
| 185 | 0 | Jungle Fence Gate | Indisponível |
| 186 | 0 | Dark Oak Fence Gate | Indisponível |
| 187 | 0 | Acacia Fence Gate | Indisponível |
| 188 | 0 | Spruce Fence | Indisponível |
| 189 | 0 | Birch Fence | Indisponível |
| 190 | 0 | Jungle Fence | Indisponível |
| 191 | 0 | Dark Oak Fence | Indisponível |
| 192 | 0 | Acacia Fence | Indisponível |
| 256 | 0 | Iron Shovel | Básico: `IronShovel` |
| 257 | 0 | Iron Pickaxe | Básico: `IronPickaxe` |
| 258 | 0 | Iron Axe | Básico: `IronAxe` |
| 259 | 0 | Flint and Steel | Básico: `FlintAndSteel` |
| 260 | 0 | Apple | Básico: `Apple` |
| 261 | 0 | Bow | Básico: `Bow` |
| 262 | 0 | Arrow | Básico: `Arrow` |
| 263 | 0 | Coal | Básico: `Coal` |
| 263 | 1 | Charcoal | Indisponível |
| 264 | 0 | Diamond | Básico: `Diamond` |
| 265 | 0 | Iron Ingot | Básico: `IronIngot` |
| 266 | 0 | Gold Ingot | Básico: `GoldIngot` |
| 267 | 0 | Iron Sword | Básico: `IronSword` |
| 268 | 0 | Wooden Sword | Básico: `WoodenSword` |
| 269 | 0 | Wooden Shovel | Básico: `WoodenShovel` |
| 270 | 0 | Wooden Pickaxe | Básico: `WoodenPickaxe` |
| 271 | 0 | Wooden Axe | Básico: `WoodenAxe` |
| 272 | 0 | Stone Sword | Básico: `StoneSword` |
| 273 | 0 | Stone Shovel | Básico: `StoneShovel` |
| 274 | 0 | Stone Pickaxe | Básico: `StonePickaxe` |
| 275 | 0 | Stone Axe | Básico: `StoneAxe` |
| 276 | 0 | Diamond Sword | Básico: `DiamondSword` |
| 277 | 0 | Diamond Shovel | Básico: `DiamondShovel` |
| 278 | 0 | Diamond Pickaxe | Básico: `DiamondPickaxe` |
| 279 | 0 | Diamond Axe | Básico: `DiamondAxe` |
| 280 | 0 | Stick | Básico: `Stick` |
| 281 | 0 | Bowl | Indisponível |
| 282 | 0 | Mushroom Stew | Indisponível |
| 283 | 0 | Golden Sword | Básico: `GoldenSword` |
| 284 | 0 | Golden Shovel | Básico: `GoldenShovel` |
| 285 | 0 | Golden Pickaxe | Básico: `GoldenPickaxe` |
| 286 | 0 | Golden Axe | Básico: `GoldenAxe` |
| 287 | 0 | String | Básico: `String` |
| 288 | 0 | Feather | Básico: `Feather` |
| 289 | 0 | Gunpowder | Básico: `Gunpowder` |
| 290 | 0 | Wooden Hoe | Básico: `WoodenHoe` |
| 291 | 0 | Stone Hoe | Básico: `StoneHoe` |
| 292 | 0 | Iron Hoe | Básico: `IronHoe` |
| 293 | 0 | Diamond Hoe | Básico: `DiamondHoe` |
| 294 | 0 | Golden Hoe | Básico: `GoldenHoe` |
| 295 | 0 | Seeds | Indisponível |
| 296 | 0 | Wheat | Indisponível |
| 297 | 0 | Bread | Indisponível |
| 298 | 0 | Leather Cap | Básico: `LeatherHelmet` |
| 299 | 0 | Leather Tunic | Básico: `LeatherChestplate` |
| 300 | 0 | Leather Pants | Básico: `LeatherLeggings` |
| 301 | 0 | Leather Boots | Básico: `LeatherBoots` |
| 302 | 0 | Chain Helmet | Básico: `ChainmailHelmet` |
| 303 | 0 | Chain Chestplate | Básico: `ChainmailChestplate` |
| 304 | 0 | Chain Leggings | Básico: `ChainmailLeggings` |
| 305 | 0 | Chain Boots | Básico: `ChainmailBoots` |
| 306 | 0 | Iron Helmet | Básico: `IronHelmet` |
| 307 | 0 | Iron Chestplate | Básico: `IronChestplate` |
| 308 | 0 | Iron Leggings | Básico: `IronLeggings` |
| 309 | 0 | Iron Boots | Básico: `IronBoots` |
| 310 | 0 | Diamond Helmet | Básico: `DiamondHelmet` |
| 311 | 0 | Diamond Chestplate | Básico: `DiamondChestplate` |
| 312 | 0 | Diamond Leggings | Básico: `DiamondLeggings` |
| 313 | 0 | Diamond Boots | Básico: `DiamondBoots` |
| 314 | 0 | Golden Helmet | Básico: `GoldenHelmet` |
| 315 | 0 | Golden Chestplate | Básico: `GoldenChestplate` |
| 316 | 0 | Golden Leggings | Básico: `GoldenLeggings` |
| 317 | 0 | Golden Boots | Básico: `GoldenBoots` |
| 318 | 0 | Flint | Básico: `Flint` |
| 319 | 0 | Raw Porkchop | Indisponível |
| 320 | 0 | Cooked Porkchop | Indisponível |
| 321 | 0 | Painting | Indisponível |
| 322 | 0 | Golden Apple | Básico: `GoldenApple` |
| 322 | 1 | Enchanted Golden Apple | Indisponível |
| 323 | 0 | Sign | Indisponível |
| 324 | 0 | Oak Door | Indisponível |
| 325 | 0 | Bucket | Básico: `Bucket` |
| 326 | 0 | Water Bucket | Básico: `WaterBucket` |
| 327 | 0 | Lava Bucket | Básico: `LavaBucket` |
| 328 | 0 | Minecart | Indisponível |
| 329 | 0 | Saddle | Indisponível |
| 330 | 0 | Iron Door | Indisponível |
| 331 | 0 | Redstone | Básico: `Redstone` |
| 332 | 0 | Snowball | Básico: `Snowball` |
| 333 | 0 | Boat | Indisponível |
| 334 | 0 | Leather | Básico: `Leather` |
| 335 | 0 | Milk | Indisponível |
| 336 | 0 | Brick | Básico: `Brick` |
| 337 | 0 | Clay | Básico: `ClayBall` |
| 338 | 0 | Sugar Canes | Indisponível |
| 339 | 0 | Paper | Indisponível |
| 340 | 0 | Book | Indisponível |
| 341 | 0 | Slimeball | Indisponível |
| 342 | 0 | Minecart with Chest | Indisponível |
| 343 | 0 | Minecart with Furnace | Indisponível |
| 344 | 0 | Egg | Básico: `Egg` |
| 345 | 0 | Compass | Indisponível |
| 346 | 0 | Fishing Rod | Indisponível |
| 347 | 0 | Clock | Indisponível |
| 348 | 0 | Glowstone Dust | Básico: `GlowstoneDust` |
| 349 | 0 | Raw Fish | Indisponível |
| 349 | 1 | Raw Salmon | Indisponível |
| 349 | 2 | Clownfish | Indisponível |
| 349 | 3 | Pufferfish | Indisponível |
| 350 | 0 | Cooked Fish | Indisponível |
| 350 | 1 | Cooked Salmon | Indisponível |
| 351 | 0 | Ink Sac | Básico: `InkSac` |
| 351 | 1 | Rose Red | Básico: `RoseRed` |
| 351 | 2 | Cactus Green | Básico: `CactusGreen` |
| 351 | 3 | Cocoa Beans | Básico: `CocoaBeans` |
| 351 | 4 | Lapis Lazuli | Básico: `LapisLazuli` |
| 351 | 5 | Purple Dye | Básico: `PurpleDye` |
| 351 | 6 | Cyan Dye | Básico: `CyanDye` |
| 351 | 7 | Light Gray Dye | Básico: `LightGrayDye` |
| 351 | 8 | Gray Dye | Básico: `GrayDye` |
| 351 | 9 | Pink Dye | Básico: `PinkDye` |
| 351 | 10 | Lime Dye | Básico: `LimeDye` |
| 351 | 11 | Dandelion Yellow | Básico: `DandelionYellow` |
| 351 | 12 | Light Blue Dye | Básico: `LightBlueDye` |
| 351 | 13 | Magenta Dye | Básico: `MagentaDye` |
| 351 | 14 | Orange Dye | Básico: `OrangeDye` |
| 351 | 15 | Bone Meal | Básico: `BoneMeal` |
| 352 | 0 | Bone | Indisponível |
| 353 | 0 | Sugar | Básico: `Sugar` |
| 354 | 0 | Cake | Indisponível |
| 355 | 0 | Bed | Básico: `Bed` |
| 356 | 0 | Redstone Repeater | Indisponível |
| 357 | 0 | Cookie | Indisponível |
| 358 | 0 | Map | Indisponível |
| 359 | 0 | Shears | Indisponível |
| 360 | 0 | Melon | Indisponível |
| 361 | 0 | Pumpkin Seeds | Indisponível |
| 362 | 0 | Melon Seeds | Indisponível |
| 363 | 0 | Raw Beef | Indisponível |
| 364 | 0 | Steak | Indisponível |
| 365 | 0 | Raw Chicken | Indisponível |
| 366 | 0 | Cooked Chicken | Indisponível |
| 367 | 0 | Rotten Flesh | Indisponível |
| 368 | 0 | Ender Pearl | Básico: `EnderPearl` |
| 369 | 0 | Blaze Rod | Indisponível |
| 370 | 0 | Ghast Tear | Indisponível |
| 371 | 0 | Gold Nugget | Indisponível |
| 372 | 0 | Nether Wart | Indisponível |
| 373 | 0 | Potion | Indisponível |
| 374 | 0 | Glass Bottle | Indisponível |
| 375 | 0 | Spider Eye | Indisponível |
| 376 | 0 | Fermented Spider Eye | Indisponível |
| 377 | 0 | Blaze Powder | Indisponível |
| 378 | 0 | Magma Cream | Indisponível |
| 379 | 0 | Brewing Stand | Indisponível |
| 380 | 0 | Cauldron | Indisponível |
| 381 | 0 | Eye of Ender | Indisponível |
| 382 | 0 | Glistering Melon | Indisponível |
| 383 | 0 | Spawn | Indisponível |
| 383 | 1 | Spawn Dropped item | Indisponível |
| 383 | 7 | Spawn Thrown egg | Indisponível |
| 383 | 8 | Spawn Lead knot | Indisponível |
| 383 | 10 | Spawn Shot arrow | Indisponível |
| 383 | 11 | Spawn Thrown snowball | Indisponível |
| 383 | 12 | Spawn Ghast fireball | Indisponível |
| 383 | 13 | Spawn Blaze fireball | Indisponível |
| 383 | 14 | Spawn Thrown Ender Pearl | Indisponível |
| 383 | 15 | Spawn Thrown Eye of Ender | Indisponível |
| 383 | 16 | Spawn Thrown splash potion | Indisponível |
| 383 | 17 | Spawn Thrown Bottle o' Enchanting | Indisponível |
| 383 | 18 | Spawn Item Frame | Indisponível |
| 383 | 19 | Spawn Wither Skull | Indisponível |
| 383 | 20 | Spawn Primed TNT | Indisponível |
| 383 | 21 | Spawn Falling block | Indisponível |
| 383 | 21 | Spawn Falling block | Indisponível |
| 383 | 22 | Spawn Firework Rocket | Indisponível |
| 383 | 30 | Spawn Armor Stand | Indisponível |
| 383 | 41 | Spawn Boat | Indisponível |
| 383 | 42 | Spawn Minecart | Indisponível |
| 383 | 42 | Spawn Minecart | Indisponível |
| 383 | 42 | Spawn Minecart | Indisponível |
| 383 | 48 | Spawn Mob | Indisponível |
| 383 | 49 | Spawn Monster | Indisponível |
| 383 | 50 | Spawn Creeper | Indisponível |
| 383 | 51 | Spawn Skeleton | Indisponível |
| 383 | 52 | Spawn Spider | Indisponível |
| 383 | 53 | Spawn Giant | Indisponível |
| 383 | 54 | Spawn Zombie | Indisponível |
| 383 | 55 | Spawn Slime | Indisponível |
| 383 | 56 | Spawn Ghast | Indisponível |
| 383 | 57 | Spawn Zombie Pigman | Indisponível |
| 383 | 58 | Spawn Enderman | Indisponível |
| 383 | 59 | Spawn Cave Spider | Indisponível |
| 383 | 60 | Spawn Silverfish | Indisponível |
| 383 | 61 | Spawn Blaze | Indisponível |
| 383 | 62 | Spawn Magma Cube | Indisponível |
| 383 | 63 | Spawn Ender Dragon | Indisponível |
| 383 | 64 | Spawn Wither | Indisponível |
| 383 | 65 | Spawn Bat | Indisponível |
| 383 | 66 | Spawn Witch | Indisponível |
| 383 | 67 | Spawn Endermite | Indisponível |
| 383 | 68 | Spawn Guardian | Indisponível |
| 383 | 90 | Spawn Pig | Indisponível |
| 383 | 91 | Spawn Sheep | Indisponível |
| 383 | 92 | Spawn Cow | Indisponível |
| 383 | 93 | Spawn Chicken | Indisponível |
| 383 | 94 | Spawn Squid | Indisponível |
| 383 | 95 | Spawn Wolf | Indisponível |
| 383 | 96 | Spawn Mooshroom | Indisponível |
| 383 | 97 | Spawn Snow Golem | Indisponível |
| 383 | 98 | Spawn Ocelot | Indisponível |
| 383 | 99 | Spawn Iron Golem | Indisponível |
| 383 | 100 | Spawn Horse | Indisponível |
| 383 | 101 | Spawn Rabbit | Indisponível |
| 383 | 120 | Spawn Villager | Indisponível |
| 383 | 200 | Spawn Ender Crystal | Indisponível |
| 384 | 0 | Bottle o' Enchanting | Indisponível |
| 385 | 0 | Fire Charge | Básico: `Fireball` |
| 386 | 0 | Book and Quill | Indisponível |
| 387 | 0 | Written Book | Indisponível |
| 388 | 0 | Emerald | Básico: `Emerald` |
| 389 | 0 | Item Frame | Indisponível |
| 390 | 0 | Flower Pot | Indisponível |
| 391 | 0 | Carrot | Indisponível |
| 392 | 0 | Potato | Indisponível |
| 393 | 0 | Baked Potato | Indisponível |
| 394 | 0 | Poisonous Potato | Indisponível |
| 395 | 0 | Empty Map | Indisponível |
| 396 | 0 | Golden Carrot | Indisponível |
| 397 | 0 | Skeleton Skull | Indisponível |
| 397 | 1 | Wither Skeleton Skull | Indisponível |
| 397 | 2 | Zombie Head | Indisponível |
| 397 | 3 | Head | Indisponível |
| 397 | 4 | Creeper Head | Indisponível |
| 398 | 0 | Carrot on a Stick | Indisponível |
| 399 | 0 | Nether Star | Indisponível |
| 400 | 0 | Pumpkin Pie | Indisponível |
| 401 | 0 | Firework Rocket | Indisponível |
| 402 | 0 | Firework Star | Indisponível |
| 403 | 0 | Enchanted Book | Indisponível |
| 404 | 0 | Redstone Comparator | Indisponível |
| 405 | 0 | Nether Brick | Básico: `NetherBrickItem` |
| 406 | 0 | Nether Quartz | Básico: `Quartz` |
| 407 | 0 | Minecart with TNT | Indisponível |
| 408 | 0 | Minecart with Hopper | Indisponível |
| 409 | 0 | Prismarine Shard | Indisponível |
| 410 | 0 | Prismarine Crystals | Indisponível |
| 411 | 0 | Raw Rabbit | Indisponível |
| 412 | 0 | Cooked Rabbit | Indisponível |
| 413 | 0 | Rabbit Stew | Indisponível |
| 414 | 0 | Rabbit's Foot | Indisponível |
| 415 | 0 | Rabbit Hide | Indisponível |
| 416 | 0 | Armor Stand | Indisponível |
| 417 | 0 | Iron Horse Armor | Indisponível |
| 418 | 0 | Gold Horse Armor | Indisponível |
| 419 | 0 | Diamond Horse Armor | Indisponível |
| 420 | 0 | Lead | Indisponível |
| 421 | 0 | Name Tag | Indisponível |
| 422 | 0 | Minecart with Command Block | Indisponível |
| 423 | 0 | Raw Mutton | Indisponível |
| 424 | 0 | Cooked Mutton | Indisponível |
| 425 | 0 | Black Banner | Indisponível |
| 425 | 1 | Red Banner | Indisponível |
| 425 | 2 | Green Banner | Indisponível |
| 425 | 3 | Brown Banner | Indisponível |
| 425 | 4 | Blue Banner | Indisponível |
| 425 | 5 | Purple Banner | Indisponível |
| 425 | 6 | Cyan Banner | Indisponível |
| 425 | 7 | Light Gray Banner | Indisponível |
| 425 | 8 | Gray Banner | Indisponível |
| 425 | 9 | Pink Banner | Indisponível |
| 425 | 10 | Lime Banner | Indisponível |
| 425 | 11 | Yellow Banner | Indisponível |
| 425 | 12 | Light Blue Banner | Indisponível |
| 425 | 13 | Magenta Banner | Indisponível |
| 425 | 14 | Orange Banner | Indisponível |
| 425 | 15 | White Banner | Indisponível |
| 427 | 0 | Spruce Door | Indisponível |
| 428 | 0 | Birch Door | Indisponível |
| 429 | 0 | Jungle Door | Indisponível |
| 430 | 0 | Acacia Door | Indisponível |
| 431 | 0 | Dark Oak Door | Indisponível |
| 2256 | 0 | 13 Disc | Indisponível |
| 2257 | 0 | Cat Disc | Indisponível |
| 2258 | 0 | Blocks Disc | Indisponível |
| 2259 | 0 | Chirp Disc | Indisponível |
| 2260 | 0 | Far Disc | Indisponível |
| 2261 | 0 | Mall Disc | Indisponível |
| 2262 | 0 | Mellohi Disc | Indisponível |
| 2263 | 0 | Stal Disc | Indisponível |
| 2264 | 0 | Strad Disc | Indisponível |
| 2265 | 0 | Ward Disc | Indisponível |
| 2266 | 0 | 11 Disc | Indisponível |
| 2267 | 0 | Wait Disc | Indisponível |
| 0 | 0 | Air | Indisponível |
| 8 | 0 | Water | Básico: `Water` |
| 9 | 0 | Stationary Water | Básico: `Water` |
| 10 | 0 | Lava | Básico: `Lava` |
| 11 | 0 | Stationary Lava | Básico: `Lava` |
| 26 | 0 | Bed | Indisponível |
| 34 | 0 | Piston Head | Indisponível |
| 36 | 0 | Block moved by Piston | Indisponível |
| 43 | 0 | Double Stone Slab | Indisponível |
| 51 | 0 | Fire | Indisponível |
| 55 | 0 | Redstone Wire | Indisponível |
| 59 | 0 | Wheat | Indisponível |
| 62 | 0 | Burning Furnace | Indisponível |
| 63 | 0 | Standing Sign | Indisponível |
| 64 | 0 | Oak Door | Indisponível |
| 68 | 0 | Wall Sign | Indisponível |
| 71 | 0 | Iron Door | Indisponível |
| 74 | 0 | Glowing Redstone Ore | Indisponível |
| 75 | 0 | Redstone Torch (inactive) | Indisponível |
| 83 | 0 | Sugar Cane | Indisponível |
| 90 | 0 | Nether Portal | Indisponível |
| 92 | 0 | Cake | Indisponível |
| 93 | 0 | Redstone Repeater (inactive) | Indisponível |
| 94 | 0 | Redstone Repeater (active) | Indisponível |
| 104 | 0 | Pumpkin Stem | Indisponível |
| 105 | 0 | Melon Stem | Indisponível |
| 115 | 0 | Nether Wart | Indisponível |
| 117 | 0 | Brewing Stand | Indisponível |
| 118 | 0 | Cauldron | Indisponível |
| 119 | 0 | End Portal | Indisponível |
| 124 | 0 | Redstone Lamp (active) | Básico: `LampOn` |
| 125 | 0 | Double Wooden Slab | Indisponível |
| 127 | 0 | Cocoa | Indisponível |
| 132 | 0 | Tripwire | Indisponível |
| 140 | 0 | Flower Pot | Indisponível |
| 141 | 0 | Carrot | Indisponível |
| 142 | 0 | Potato | Indisponível |
| 144 | 0 | Mob head | Indisponível |
| 149 | 0 | Redstone Comparator | Indisponível |
| 150 | 0 | Redstone Comparator (deprecated) | Indisponível |
| 176 | 0 | Standing Banner | Indisponível |
| 177 | 0 | Wall Banner | Indisponível |
| 178 | 0 | Inverted Daylight Sensor | Indisponível |
| 181 | 0 | Double Red Sandstone Slab | Indisponível |
| 193 | 0 | Spruce Door | Indisponível |
| 194 | 0 | Birch Door | Indisponível |
| 195 | 0 | Jungle Door | Indisponível |
| 196 | 0 | Acacia Door | Indisponível |
| 197 | 0 | Dark Oak Door | Indisponível |

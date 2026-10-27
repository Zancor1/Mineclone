# SkyWars

Em Multiplayer, escolha SkyWars, crie uma sala com nome no modo Solo.
No lobby, escolha um time; o host usa P ou o botão touch Iniciar partida.
Todos precisam do protocolo LAN 23, inclusive para BedWars.

Bosque Celeste possui oito ilhas separadas, árvores, minas e três baús por ilha.
Os jogadores começam sem itens nem armadura; todo equipamento vem dos baús.
As ilhas não têm bancada nem fornalha. O centro tem quatro baús com loot superior. Não existem camas, lojas, geradores
nem respawn: vence o último time vivo. O vencedor recebe 25 XP; cada kill dá 10 XP.
Itens carregados e armaduras equipadas caem ao morrer. Após a vitória, o lobby
retorna automaticamente. F8 reinicia a rodada pelo host.

Os 28 baús têm inventários independentes e compartilhados entre os jogadores.
O host valida distância e movimentações, evitando duplicação por cliques simultâneos.
O loot varia em conteúdo, quantidade e posição dos slots. Cada ilha combina
baús de combate, suprimentos e utilidades, com variações de armas e armaduras.
O centro reserva espadas e armaduras de diamante, pérolas, poções e mais maçãs.
Cada sala e rodada recebem uma semente própria; os baús são reabastecidos aos
2, 4 e 6 minutos. Somente o host gera o loot e transmite o inventário compartilhado:
retirar ou depositar um item atualiza a visão dos outros jogadores.

## Equipamentos e utilidades

- Espadas de madeira, pedra, ferro e diamante; picaretas de madeira, ferro e diamante.
- Pedra, tábuas, obsidiana e materiais obtidos minerando as ilhas.
- Ovos e bolas de neve arremessáveis, consumidos ao usar, com colisão e knockback.
- Arco e flechas com puxada, dano e knockback; pérolas de teleporte.
- Armaduras individuais, com redução de dano calculada pelas peças equipadas.
  Use a peça na hotbar para equipar; no inventário, clique no slot de armadura
  para retirar ou trocar a peça com o cursor.
- Maçãs douradas, poções de velocidade, TNT e baldes de água/lava.
  O centro tem equipamentos de diamante e pérolas.

A seleção de itens representa este modo do MineClone, não todo preset de todos
os servidores Minecraft. Ainda não há vara de pesca, encantamento de equipamentos,
frascos de experiência nem poções arremessáveis. O registro atual de blocos/itens
ocupa os 256 IDs do formato; adicionar novas famílias requer ampliar o formato e
migrar saves e protocolo, sem reutilizar IDs de itens existentes.

## Verificação

`make test-skywars` verifica loot, limites das pilhas, baús, spawns, início por LAN,
eliminação no void, vitória e recompensa. Os testes BedWars, anticheat e migração
continuam separados. Builds Android e Windows precisam de testes em aparelhos reais.

No Tester, o inventário inclui 16 ovos e 16 bolas de neve para testar knockback
nos dummies.

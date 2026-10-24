# BedWars LAN

Partida local com oito equipes: **Solo**, até oito jogadores (um por time),
ou **Dupla**, até dezesseis jogadores (dois por time). Todos
precisam usar a mesma versão do MineClone. Este modo usa um protocolo próprio;
a conexão Java 1.8 do menu Multiplayer não participa da partida.

## Hospedar e entrar

1. No menu principal, abra **Multiplayer → BedWars**.
2. Informe seu nick e clique em **Criar sala**. Escolha o **nome do servidor**
   (até 32 caracteres), **Solo** ou **Dupla** e depois o modo **Rush**:
   rápido (22), médio (38) ou lento (70 blocos de vazio entre as bordas das
   ilhas vizinhas).
3. Seus amigos abrem a mesma tela, informam o IPv4 do seu computador seguido
   de `:25566` e clicam em **Conectar**, ou selecionam a sala descoberta na lista.
4. Todos chegam ao lobby de vidro acima das ilhas. O host pressiona **P**
   com pelo menos dois jogadores conectados para uma contagem regressiva de
   cinco segundos na tela de todos; cada jogador vai à ilha do seu time. Se restar
   apenas um jogador durante a contagem, o início é cancelado.

Exemplo: `192.168.1.20:25566`. No Linux, `hostname -I` mostra os endereços do
host. Os computadores precisam conseguir se alcançar pela rede e a porta
**TCP 25566** precisa estar liberada no firewall do host. O servidor aceita
IPv4 e também funciona em uma rede virtual quando o IP é alcançável.

São necessários pelo menos dois jogadores para iniciar. Amigos entram enquanto a partida
estiver no lobby; **F8**, no host, reinicia o mapa e retorna todos ao lobby.
Não há seleção automática de partidas nem bots.

## Regras implementadas

- Quatro ilhas de equipe, ilhas de diamante, centro de esmeraldas e vazio.
- Cama formada por duas partes; a própria cama não pode ser destruída.
- Terreno da arena protegido. Blocos colocados por jogadores podem ser removidos.
- Geradores com estoque limitado: ferro, ouro, diamantes e esmeraldas.
- Loja com recursos, blocos, espadas, picaretas, armaduras e itens de combate.
- Melhorias permanentes na rodada: afiação, proteção e velocidade do gerador.
- Inventário com 27 slots mais nove da hotbar; clique na origem e no destino
  para mover ou juntar pilhas. O baú da equipe também usa essas operações.
- Dano, intervalo entre golpes, knockback, flechas, explosivos e pérolas.
- Morte no vazio: renascimento em cinco segundos enquanto a cama existir.
- Sem cama, cada morte elimina o jogador. A última equipe com jogadores vivos vence.
- As camas restantes são destruídas após dez minutos. Após quinze minutos,
  inicia-se dano periódico. Uma vitória retorna todos ao lobby em dez segundos.

O computador que hospeda valida compras, inventários, construção, mineração,
combate, camas e resultados. A posição usa previsão no cliente com limites
básicos no host; este modo é destinado a partidas entre amigos, sem um sistema
completo de anticheat ou autenticação de contas.

## Controles

| Ação | Controle |
| --- | --- |
| Andar | WASD |
| Pular | Espaço |
| Correr / agachar | Ctrl / Shift |
| Atacar / minerar | Botão esquerdo |
| Colocar / usar item | Botão direito |
| Abrir loja próxima | Botão direito olhando para o aldeão de itens ou melhorias |
| Inventário | E |
| Baú | Botão direito no baú da própria ilha |
| Selecionar hotbar | 1–9 / roda do mouse |
| Chat | T, Enter envia |
| Menu / fechar interface | Esc |
| Iniciar contagem | P, somente host (minimo 2 jogadores) |
| Refazer arena e voltar ao lobby | F8, somente host |

## Estado da partida

As rodadas são temporárias e não substituem os mundos salvos do single-player.
O host precisa manter o jogo aberto. Se ele sair, os clientes voltam à tela de
BedWars com uma mensagem de desconexão. Uma partida em andamento não é salva
para retomada posterior.

Este é um modo BedWars próprio do MineClone, não uma reprodução completa das
regras, interfaces, cosméticos ou infraestrutura de um servidor comercial.

## Validação

`make test` cobre uma conexão TCP de dois jogadores: lobby, equipes, geradores,
compras, consumo de blocos, mineração, cama adversária, eliminação, vitória e
reinício. Também inicia um segundo processo com o cliente LAN do MineClone.
`make test-renderer` grava uma prévia em `build/bedwars-lobby.png`.
Os testes usam loopback; uma partida em dois computadores distintos ainda deve
ser validada na rede dos jogadores.

As lojas são dois aldeões em cada base: **Loja de itens** e **Melhorias**.
Abra a loja olhando para o aldeão e clicando com o botão direito. Melhorias
incluem afiação, proteção e geração de recursos; armaduras ficam na loja de itens.
O terreno original da arena não pode ser quebrado. Apenas blocos colocados
pelos jogadores e camas adversárias podem ser destruídos.

O item selecionado e a armadura de cada jogador são exibidos aos adversários.
Bolas de fogo aplicam impulso reforçado e permitem saltos com explosão sem
dano ao lançador; adversários recebem dano e empurrão. Todos devem usar a
mesma versão do MineClone (protocolo BedWars LAN 11).

Golpes corpo a corpo têm alcance máximo de 3,3 blocos, medidos dos olhos até
a caixa de colisão do adversário, e são bloqueados pelo terreno. O impulso
horizontal dos golpes é limitado para evitar lançamentos excessivos; a cabeça acompanha também o olhar vertical.

## Descoberta de salas

Em **Multiplayer → BedWars**, a busca de hosts MineClone começa automaticamente.
Use **Atualizar** para repetir a busca. À esquerda, filtre por nome, mapa,
ping máximo e vagas. A lista central mostra mapa, ocupação e ping medido.
Selecione uma sala para consultar os jogadores à direita; **Conectar** usa
o endereço exibido embaixo, que também aceita edição manual.
**Criar sala** abre a seleção de modo, distância e mapa.
A busca dura cinco segundos e consulta as interfaces IPv4 ativas por broadcast
por sub-rede e multicast `239.255.77.67`, incluindo Wi-Fi, cabo e interfaces
virtuais de VPN. A lista mostra nick do host, jogadores, distância Rush e estado
da partida. Clique em uma sala disponível para entrar; **Atualizar busca**
consulta a rede novamente. Salas cheias ou em partida ficam indisponíveis.

O host precisa permitir **UDP 25567** para descoberta e **TCP 25566** para jogo.
Linux e Windows usam o mesmo anúncio. A busca encontra salas desta versão do
MineClone, não servidores Minecraft Java convencionais.

Hamachi/Radmin podem filtrar broadcast ou multicast, conforme a configuração
da VPN e do firewall. A descoberta não pode atravessar esse bloqueio; nesses
casos, informe o IP virtual do host na conexão manual. Isolamento de clientes
em Wi-Fi e redes de convidados também pode impedir a descoberta e a conexão.

## Mapas e criação da sala

Depois de escolher Rush rápido, médio ou lento, escolha uma das cinco prévias:

- **Fortaleza:** torres e muralhas sobre ilhas de grama.
- **Ruínas:** terreno de pedra com colinas, plataformas e arcos.
- **Jardins:** terraços, pavilhões de madeira e árvores de copa rosa.

As prévias são renderizadas a partir da mesma geometria da partida. O host
transmite o mapa escolhido e cada cliente gera a mesma arena. As distâncias
Rush continuam sendo medidas entre as bordas das ilhas vizinhas.

## Categorias da loja

A loja de itens usa uma grade com ícones, inventário e hotbar. As abas são:
blocos, espadas, armaduras, ferramentas, poções e utilitários. O preço aparece
ao passar o mouse no item. Não há abas vazias. Arco e flechas ficam junto dos
utilitários; o graveto com knockback não foi adicionado.

Espadas disponíveis: madeira, pedra, ferro, ouro e diamante. Armaduras de
malha, ferro e diamante substituem **somente calça e botas** e permanecem após
morrer. Capacete e peitoral continuam de couro da equipe.

Picareta e machado têm progressão independente: **madeira → pedra → ferro**.
A aba oferece somente o próximo nível. A compra substitui a ferramenta anterior;
no nível máximo, ela fica identificada como máxima e não pode ser recomprada.
Ao morrer, ferro vira pedra e pedra vira madeira; madeira permanece. Ferramentas
persistentes não podem ser guardadas no baú da equipe.

Poções são consumidas com botão direito e têm duração limitada:

| Poção | Custo | Efeito |
|---|---|---|
| Velocidade | 1 esmeralda | Movimento 40% mais rápido por 45 s |
| Salto | 1 esmeralda | Impulso de salto aumentado por 45 s |
| Invisibilidade | 2 esmeraldas | Oculta o jogador e seu nome por 30 s; receber dano encerra o efeito |

Os efeitos somem ao morrer e seus tempos são sincronizados pelo host. O
agachamento também é sincronizado: o modelo se abaixa e inclina o corpo, e o
nome fica oculto enquanto o jogador está agachado.

Estas mudanças usam o protocolo BedWars LAN **11**. Todos os participantes
precisam atualizar o jogo, tanto no Linux quanto no Windows.

## Resposta dos ataques

O alcance corpo a corpo é de até **3,3 blocos** dos olhos até a caixa de colisão
do adversário. O clique envia a posição e direção atuais antes do ataque.
Pacotes prontos tentam sair imediatamente pelo socket não bloqueante e os
acertos são confirmados pelo host sem esperar o snapshot periódico. Não há cooldown artificial entre cliques válidos de corpo a corpo nem bloqueio
de novos acertos pela imunidade de dano. Latência da rede ainda influencia a resposta.

## Toggle Sprint e impulso dos golpes

**Toggle Sprint** vem ligado por padrão em Opções. Um toque na tecla de correr
ativa a corrida automática ao andar para frente; outro toque desativa. Agachar
suspende a corrida e soltá-lo permite retomá-la. Desligar a opção restaura o
comportamento de segurar a tecla de corrida. A opção respeita a bind escolhida
em Keyboard e é salva em `settings.cfg`.

Golpes corpo a corpo usam impulso horizontal e vertical menor, amortecem o
movimento anterior e limitam a soma de impactos sucessivos. Explosões mantêm
seu impulso próprio. Cada clique que passa pelas verificações de alcance de
3,3 blocos, direção e paredes pode causar dano imediatamente.

## Ritmo dos geradores

Sem melhorias da equipe, a base produz **1 ferro a cada 2,5 segundos** e
**1 ouro a cada 8 segundos**. As melhorias de forja continuam acelerando a base.

Diamantes começam no nível 1 com intervalo de **25 segundos**; esmeraldas
começam com **35 segundos**. A cada **2 minutos e 50 segundos de partida**,
o nível aumenta e o intervalo diminui 5 segundos, até o mínimo de 5 segundos.
Diamantes chegam ao nível máximo 5, e esmeraldas ao nível máximo 7.

O nível e a contagem para a próxima geração aparecem acima de cada gerador
de diamante/esmeralda. O host sincroniza o progresso dos relógios com todos
os clientes; o tempo não depende do relógio do computador de quem entrou.
Estoques máximos continuam limitados para evitar acúmulo infinito de recursos.

O knockback corpo a corpo tem recuo horizontal maior e gravidade mais forte
até aterrissar. O pulo normal e os impulsos de explosões mantêm sua gravidade.

## Escolha de times

Todos recebem uma **lã branca** no lobby. Segure-a e clique com o botão
esquerdo ou direito para abrir a seleção de equipes. A tela mostra as vagas
e seu time atual. O host valida cada escolha: Solo permite um jogador por
time; Dupla permite dois. Entre os times ocupados, a diferença máxima é de
um jogador. Com apenas dois participantes, eles ficam em times diferentes.
Desconexões no lobby também recalculam o equilíbrio antes do início.

Em Dupla, companheiros compartilham ilha, cama, baú e melhorias de equipe.
Não há fogo amigo. Armadura, inventário e ferramentas permanecem individuais.

Os textos dos geradores de diamante e esmeralda aparecem apenas a até
**5 blocos de distância** da câmera ao gerador.

## Movimento e separação das ilhas

O sprint local usa 5,2 blocos por segundo. Acertar um adversário interrompe o
sprint do atacante por 0,1 segundo; Toggle Sprint retoma a corrida depois desse
intervalo. O impulso vertical dos golpes foi ajustado para 6,0, com amortecimento exponencial, limite horizontal de
8,5 e gravidade de combate de 28 blocos/s². Golpes sucessivos no ar não
reiniciam a subida.

As bases têm seis blocos adicionais de vazio entre as bordas: 22, 38 ou 70
blocos conforme a opção Rush. As plataformas de diamante foram reduzidas para
eliminar os atalhos de pulo. Centro e ilhas de diamante têm construções próprias
nos temas Fortaleza, Ruínas e Jardins. As prévias usam essa mesma geometria.

## Mapa NC

**NC** importa a arena de `NC Bedwars 1v1`, mantendo as duas bases e a
construção original. Está disponível em Solo (1 contra 1) e Dupla (2 contra 2).
A distância é a do mapa original; a opção Rush não redimensiona essa arena.
As duas camas são restauradas ao criar ou reiniciar a partida.

As lojas, os baús e os geradores usam os sistemas do MineClone. O lobby de
espera fica acima da arena. A construção é protegida, e apenas blocos dos
jogadores e camas adversárias podem ser quebrados.

A conversão inclui 39.240 blocos e 119 estados legados, com materiais e
orientação de lajes, escadas, cercas, grades e carpetes. Decorações e os baús
especiais recebem representações locais. Command blocks não executam comandos
Java e os limites invisíveis de barreira do mundo original não são importados.

O mapa convertido fica incorporado ao executável: convidados não precisam da
pasta original. O importador `tools/import_nc_map.py` lê o formato Anvil legado
com a biblioteca padrão do Python e gera `include/nc_map_data.inc` e o relatório
`data/nc-import.json`. Para refazer a conversão:

```sh
python3 tools/import_nc_map.py 'NC Bedwars 1v1'
make -j"$(nproc)"
```

Todos os participantes precisam usar o protocolo BedWars LAN **11**.

A compra de espada substitui a espada segurada; se outra categoria estiver
selecionada, substitui a primeira espada do inventário. A loja indica recursos
suficientes, recursos faltantes e melhorias adquiridas. A mineração só mostra
progresso em blocos permitidos; ar e terreno protegido não são mineráveis.

## Consumo, cura e alcance

Lojas de qualquer equipe atendem todos os jogadores. Melhorias sempre pertencem
à equipe de quem paga. Madeira e ouro foram retirados das ofertas de espadas;
a espada inicial de madeira continua no equipamento de renascimento. O graveto
com Knockback II custa cinco ouros e duplica o impulso horizontal, sem aumentar
a subida.

Segure o botão de uso por 1,61 segundo para comer uma maçã dourada. Soltar o
botão, trocar de item ou abrir uma interface cancela o consumo. A maçã concede
quatro pontos de absorção (dois corações dourados) e regenera um ponto de vida
por segundo durante cinco segundos. A cura natural restaura dois pontos a cada
quatro segundos; receber dano reinicia sua contagem. Durante a regeneração da
maçã, os pulsos de cura natural ficam suspensos. Dano de queda é calculado pelo
host ao aterrissar.

Construção e mineração no BedWars têm alcance máximo de quatro blocos dos
olhos até o bloco. O alcance de combate continua sendo 3,3 blocos.

### Disposição das arenas

As equipes são Vermelho, Azul, Verde, Amarelo, Ciano, Branco, Rosa e Cinza.
Fortaleza, Ruínas e Jardins têm oito bases no contorno da arena, com ilhas
de diamante entre vizinhos e esmeralda no centro. Cada cenário tem contorno,
relevo e construções próprios. As distâncias Rush só alteram os mapas gerados.

Airshow foi convertido de `mapas externos/Airshow/`: 120.067 blocos e 86
estados legados, incluindo as oito camas, balões, aviões, escadas invertidas,
lajes, cercas, vidros e argilas coloridos. As barreiras originais permanecem
invisíveis e com colisão. O fogo decorativo usa geometria cruzada iluminada.
Há oito geradores de base, quatro de diamante e quatro de esmeralda.
A geometria original é mantida; as lojas e regras são executadas pelo MineClone.

NC foi reimportado de `mapas externos/NC Bedwars 1v1/` e mantém duas equipes,
pois é uma arena 1v1. Os arquivos convertidos ficam incorporados ao executável;
os jogadores não precisam copiar as pastas de mundos Java.

```sh
python3 tools/import_nc_map.py "mapas externos/NC Bedwars 1v1"
python3 tools/import_nc_map.py "mapas externos/Airshow" --airshow --output include/airshow_map_data.inc
```

A altura ativa passou de 80 para 128 blocos para preservar o Airshow completo.
Saves e arquivos de chunks antigos com altura 80 são migrados ao carregar.
Todos na sala precisam usar esta mesma versão do protocolo LAN (11).

### Escolha do layout na criação

Abaixo de **Dupla**, **Layout padrão** começa ligado. Com ele ligado,
a escolha de Solo/Dupla leva à tela Rush e depois mostra apenas Fortaleza,
Ruínas e Jardins. Desligado, a escolha leva diretamente a **Mapas avançados**,
com NC e Airshow, que mantêm suas distâncias originais.

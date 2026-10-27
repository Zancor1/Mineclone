# MineClone

Jogo de construção e exploração em primeira pessoa, inspirado no Minecraft clássico e desenvolvido
em **C++17 com Raylib**. Explore terrenos procedurais, construa com blocos e salve
mundos independentes ou jogue BedWars com amigos em salas hospedadas pelo próprio jogador.

O projeto utiliza malhas por chunk, simulação de água por níveis e iluminação
por blocos com lanternas, sombras e ciclo de dia e noite. Texturas e shaders são
criados no código, sem pacotes de assets externos.

## Recursos

- **Geração procedural contínua:** novas regiões surgem conforme você explora,
  com biomas, montanhas, árvores, cactos e lagos. Uma janela de 256 × 256 × 192
  blocos permanece ativa ao redor do jogador.
- **Construção livre:** blocos, ferramentas e plantas, inventário criativo paginado e
  hotbar com nove posições configuráveis. Inclui lã em 16 cores.
- **Água e lava dinâmicas:** prioridade de queda, busca pelos caminhos mais curtos
  até desníveis, propagação limitada e secagem após a remoção da fonte. A lava
  avança à metade da velocidade da água.
- **Iluminação:** lanternas com bloqueio de luz por obstáculos, luz interpolada
  nos vértices, oclusão ambiente e neblina.
- **Dia e noite:** ciclo de 20 minutos, ajuste de horário entre 00:00 e 23:59 e
  opção de manter o relógio travado.
- **Primeira pessoa:** mão visível apenas com o slot vazio; itens e blocos em 3D,
  acompanhando a seleção da hotbar, com animações de movimento e uso.
- **Movimentação:** câmera em primeira pessoa, colisão AABB, deslizamento nas
  paredes, pulo contínuo, natação e voo criativo.
- **Crafting e fornalhas:** fabricação 2 × 2 no inventário e 3 × 3 na mesa,
  consumo de ingredientes, ferramentas, equipamentos e fundição de minérios.
- **Multiplayer:** BedWars LAN entre clientes MineClone e cliente experimental
  para servidores Minecraft Java 1.8–1.8.9 (protocolo 47).
- **Progressão no BedWars:** XP por kills, camas e vitórias, caixas gratuitas e
  skins coloridas para ferramentas, com perfil local que acompanha o jogador entre hosts.
- **Mundos persistentes:** criação, seleção e exclusão de saves, com restauração
  da posição, orientação e horário.

## Conteúdo da versão 1.8.9

A seleção com ícones em **E → Todos os itens** permite pegar itens no criativo e
depositá-los no inventário ou na hotbar. O registro inclui blocos e itens de
construção, ferramentas, equipamentos, consumíveis e variantes de formas
usadas no mapa NC. **Não há paridade completa com a 1.8.9**;
redstone, entidades, dimensões e outros sistemas ainda estão ausentes.

Veja a [tabela de cobertura](docs/MINECRAFT_1_8_9.md) para consultar cada registro,
suas fontes e as diferenças de comportamento. Adicionar o nome de um item ao
catálogo não implementa sua mecânica.

## Compilar e executar

### Dependências no Arch Linux

```sh
sudo pacman -Syu --needed gcc make pkgconf raylib zlib
```

É necessária uma sessão gráfica com suporte a **OpenGL 3.3**. O projeto utiliza
GCC, GNU Make e `pkg-config` para localizar a Raylib.

### Gerar Windows x64 pelo Linux

```sh
sudo pacman -S --needed mingw-w64-gcc cmake make curl
make windows
```

O executável fica em **`dist/windows/minecraft_clone.exe`**. O script baixa
Raylib 5.5 e zlib 1.3.1 dos repositórios oficiais e compila ambas para Windows.
Objetos, bibliotecas e fontes dessa compilação ficam em `build/windows/`,
separados dos objetos Linux. Os downloads exigem internet na primeira execução.

Copie a pasta `dist/windows/` para um computador com **Windows 10/11 x64** e
OpenGL 3.3. Não é necessário instalar Raylib, zlib ou DLLs do MinGW. Saves e
configurações são criados no diretório de trabalho; use uma pasta com permissão
de escrita. Perfis e skins são salvos em `%APPDATA%\MineClone`, separados
do executável. Linux e Windows usam o protocolo **BedWars LAN 23** e devem estar
na mesma versão do jogo. A porta TCP 25566 precisa estar liberada no host.

Veja [build e validação Windows](docs/WINDOWS.md) para detalhes e testes.

### Build Linux

Baixe ou clone este repositório e, na raiz do projeto, execute:

```sh
make -j"$(nproc)"
./minecraft_clone
```

Os objetos e arquivos de dependências ficam em `build/`. O Makefile recompila
apenas os módulos afetados pelas alterações.

Para limpar os arquivos de compilação:

```sh
make clean
```

Para gerar um executável com símbolos de depuração:

```sh
make clean
make CXXFLAGS='-std=c++17 -O0 -g -Wall -Wextra -Wpedantic'
```

## Como jogar

No menu principal, selecione **Jogar** para abrir a lista de mundos. Escolha um
save existente ou use **Criar Novo Mundo** para gerar um mapa com uma nova seed.

| Ação | Controle |
| --- | --- |
| Movimentar | W, A, S, D |
| Olhar | Mouse |
| Pular continuamente / nadar ou voar para cima | Segurar Espaço |
| Ativar / desativar voo criativo | Dois toques rápidos em Espaço |
| Acelerar durante o voo | Ctrl esquerdo |
| Nadar / voar para baixo | Shift esquerdo |
| Quebrar bloco | Botão esquerdo do mouse |
| Colocar bloco | Botão direito do mouse |
| Selecionar posição da hotbar | 1–9 ou roda do mouse |
| Abrir / fechar inventário | E |
| Abrir pausa / voltar | Esc |
| Incluir água e lava na seleção de blocos | Segurar Alt |
| Ligar / desligar a lanterna apontada | F |
| Avançar um quarto do ciclo solar | F6, no criativo com o horário destravado |
| Trocar página do inventário | Setas esquerda/direita, Page Up/Down ou botões |

**E** abre o inventário pessoal com fabricação 2 × 2, **27 slots (3 × 9)** e a **hotbar de 9 slots**
abaixo. Clique para pegar ou depositar pilhas. Passe o mouse sobre um slot e pressione a tecla configurada de uma posição da hotbar para trocar as duas pilhas, preservando quantidades e propriedades. No inventário criativo, **Shift + clique
esquerdo descarta a pilha** do slot. A tela do baú utiliza exatamente o mesmo inventário e a mesma hotbar.

Para obter novos blocos no modo criativo, clique em **Blocos criativos**. O
catálogo oferece uma pilha de 64 no cursor; deposite-a na hotbar ou use
**Voltar ao inventário** para guardá-la nos slots pessoais. Apenas o catálogo
cria itens; transferências entre slots e baús mantêm as quantidades. No modo sobrevivência, o catálogo não está disponível e construir consome itens. Câmera e simulação ficam pausadas
com o inventário aberto.

### Opções

Acesse **Opções...** pelo menu principal ou pelo menu de pausa.

- **Horário:** arraste a barra entre **00:00 e 23:59**. Pela pausa, a iluminação
  do mundo ao fundo acompanha o ajuste.
- **Travar horário:** mantém a hora escolhida sem interromper a física ou a
  simulação de água. Para um dia permanente, selecione **12:00** e ative a trava.
- **Neblina:** ligue/desligue o efeito externo e ajuste sua distância entre 8 e
  192 blocos. Debaixo d'água, a limitação de visibilidade permanece ativa.
- **Distância de visão:** entre 1 e 6 chunks (16 a 96 blocos). A alteração afeta
  quais malhas são desenhadas; é independente da neblina e não apaga regiões.
- **Sensibilidade:** ajuste o mouse entre **10% e 300%**. O valor de **100%**
  corresponde à sensibilidade padrão.

Clique em **Concluído** ou pressione **Esc** para salvar as preferências em
`settings.cfg` e retornar ao menu anterior. O horário escolhido também define
como novos mundos começam; mundos existentes restauram seu próprio horário salvo.

### Baús

Encontre **Bau** no inventário criativo, equipe-o na hotbar e coloque-o no mundo.
Use o **botão direito** para abrir; um bloco sólido sobre o baú impede a abertura.
Para construir sobre ele sem abrir, segure **Shift** ao colocar o bloco.

Cada baú tem **27 slots**, com pilhas de até **64 itens**, e a tela oferece mais
27 slots de inventário pessoal. A hotbar possui nove pilhas reais: pegar ou transferir itens retira a quantidade
do slot de origem, sem criar cópias. Shift + clique também envia itens do
inventário ou da hotbar para o baú, preservando sobras quando não cabe tudo.

| Operação | Controle |
| --- | --- |
| Pegar, depositar, juntar ou trocar uma pilha | Clique esquerdo |
| Pegar metade / depositar uma unidade | Clique direito |
| Recolher uma pilha do baú para o inventário | Shift + clique esquerdo |
| Fechar | E, Esc ou X |

Conteúdo dos baús, inventário pessoal e pilha no cursor são salvos com o mundo,
inclusive quando a região é descarregada. Ao quebrar um baú, os itens são
recolhidos para o inventário pessoal; se não couberem, a quebra é cancelada.
Baús são individuais e não se unem em baús duplos.

### Fabricação e ferro

O inventário oferece fabricação **2 × 2**. A **mesa de trabalho**, aberta com o
botão direito, oferece **3 × 3**. Coloque ingredientes e clique no resultado:
o jogo consome uma unidade de cada ingrediente por receita. As grades ficam
salvas com o mundo.

| Receita | Disposição | Resultado |
| --- | --- | --- |
| Tábuas | 1 tronco | 4 tábuas |
| Gravetos | 2 tábuas na vertical | 4 gravetos |
| Mesa de trabalho | 4 tábuas em quadrado 2 × 2 | 1 mesa |
| Fornalha | 8 pedregulhos, deixando o centro vazio | 1 fornalha |
| Espada de madeira | 2 tábuas na vertical, com graveto abaixo | 1 espada |
| Espada de pedra | 2 pedregulhos na vertical, com graveto abaixo | 1 espada |
| Espada de ferro | 2 lingotes na vertical, com graveto abaixo | 1 espada |
| Picareta de madeira | 3 tábuas na primeira linha; 2 gravetos na coluna central abaixo | 1 picareta |
| Picareta de pedra | Mesmo formato, usando pedregulhos | 1 picareta |
| Picareta de ferro | Mesmo formato, usando lingotes | 1 picareta |

As espadas usam uma coluna de três slots na mesa, em qualquer coluna. São itens
não colocáveis, com limite de uma ferramenta por slot. Os demais ingredientes usam
pilhas de até 64. O catálogo criativo também oferece os novos itens.

O minério de ferro aparece sob a superfície em regiões novas. Quebrar pedra
recolhe pedregulho; minério de carvão fornece carvão e minério de ferro fornece
o próprio bloco, desde que haja espaço no inventário. Coloque o ferro no slot
superior da **fornalha**, combustível no inferior e retire o lingote à direita.
Cada lingote leva **10 segundos**; um carvão dura **80 segundos**. Troncos,
tábuas e gravetos também servem como combustível. Conteúdo, combustível restante
e progresso de cada fornalha são persistidos, inclusive entre regiões.

As ferramentas têm durabilidade: madeira suporta 59 usos, pedra 131 e ferro 250.
Minerar com espada consome dois usos. O single-player ainda não tem criaturas;
o combate entre jogadores está disponível no BedWars LAN.

### Sobrevivência

Na seleção de mundos, altere **Modo** antes de criar um mundo. Sobrevivência
começa com inventário vazio, sem catálogo criativo e sem voo. Segure o botão
esquerdo para minerar; o indicador junto à mira mostra o progresso. Os materiais
recolhidos vão diretamente ao inventário, e construir consome uma unidade.

Pedra e carvão exigem picareta para fornecer materiais; ferro exige pelo menos
picareta de pedra. Use madeira para fazer tábuas, gravetos, mesa e a primeira
picareta. A picareta de pedra permite obter ferro e fundi-lo na fornalha.

O HUD exibe vida e ar ao mergulhar. Quedas, lava, fogo, cactos, sufocamento e
afogamento causam dano. Ao morrer, escolha **Renascer** ou volte ao menu.
Renascer esvazia o inventário e as grades de fabricação e procura um ponto seguro
na região atual. Os itens perdidos não aparecem no chão. Fome e criaturas ainda
não estão implementadas. Mundos antigos continuam no modo criativo.

### Itens de minigames

Acesse **E → Todos os itens** no criativo, clique no item e deposite-o na hotbar. Há armas, ferramentas, armaduras, lã, vidro, end stone,
obsidiana e consumíveis de construção e mobilidade.

| Item | Uso |
| --- | --- |
| Pérola do End | Modelo 3D na mão e em voo, com rastro de partículas; botão direito lança; o impacto teleporta para uma posição livre e causa 5 de dano na sobrevivência |
| Bola de fogo | Modelo 3D com crosta incandescente e rastro de fogo; botão direito lança um projétil explosivo |
| TNT | Botão direito coloca TNT ativada, com contador de 5 segundos |
| Arco | Segure o botão direito e solte para disparar; consome flechas na sobrevivência |
| Bola de neve / ovo | Botão direito lança um projétil com gravidade |
| Maçã dourada | Modelo 3D com animação de comer e farelos; segure o botão direito por 1,6 s; recupera 1 coração por segundo durante 5 s e concede 4 pontos de absorção por 120 s |
| Balde vazio | Botão direito coleta uma fonte de água/lava |
| Balde cheio | Botão direito coloca uma fonte; devolve o balde na sobrevivência |
| Armadura | Botão direito na mão ou no inventário equipa; pode ser retirada dos quatro slots de equipamento |
| Cama | Botão direito coloca duas metades sobre piso; camas na região ativa são usadas ao renascer |
| Escada de mão | Coloque-a e avance/pule dentro da célula para subir; Shift desce |

Armaduras de couro, malha, ferro, ouro e diamante possuem proteção e desgaste.
A proteção afeta explosões, lava e cactos; não evita dano de queda ou afogamento.
Equipamento e efeitos da maçã são salvos com o jogador.

Baldes, arco, flechas, maçã dourada, TNT, cama, escada, isqueiro e armaduras
fabricáveis têm receitas na mesa. Couro, linha, pólvora, maçãs e penas estão na
seleção criativa; o jogo local ainda não tem criaturas para fornecer esses materiais.
O isqueiro ativa TNT que já esteja colocada no mundo.

No single-player, são **itens para construção e testes locais**. O modo
BedWars LAN possui regras próprias de equipes, compras, PvP e vitória,
descritas abaixo. No single-player, flechas, ovos e bolas de neve não causam
dano a entidades. Explosões locais usam regras simplificadas e
preservam baús/fornalhas, obsidiana e rocha matriz. A cama tem orientação fixa;
o retorno funciona apenas para camas na região ativa. Projéteis e TNT ativada
não são persistidos ao sair do mundo.

## Multiplayer

### Servidores Minecraft Java (experimental)

O menu principal inclui **Multiplayer**. O cliente usa o protocolo Java 1.8
(47), com conexão TCP, chunks comprimidos, outros jogadores, inventários, chat,
ataques e aplicação de vida/knockback enviados pelo servidor.

**Não há suporte completo e validado ao MushMC.** Autenticação Microsoft e
criptografia de sessão ainda não foram implementadas. Os testes de rede usam
um servidor de teste local, não uma partida pública.

Veja [compatibilidade, controles e teste manual](docs/MULTIPLAYER.md).

### BedWars LAN

**Protocolo atual: 20.** Host e convidados precisam atualizar juntos,
inclusive em partidas entre Linux e Windows. Salas incompatíveis aparecem
na busca com aviso de versão diferente.

Em **Multiplayer → BedWars**, é possível escolher entre 21 mapas com prévias e lobby suspenso. Há Solo (até oito jogadores) e Dupla (até dezesseis),
com seleção de time pela lã branca no lobby. Há camas, equipes, loja, geradores, PvP, placar,
renascimento e vitória. O host abre a porta TCP **25566**; os amigos entram
pelo seu IPv4. Com pelo menos dois jogadores, o host pressiona **P** para
iniciar a contagem de cinco segundos na tela de todos. **F8** volta ao lobby.
O navegador de servidores lista salas, mapa, jogadores e ping. Os filtros
permitem buscar por nome, mapa e ping máximo; selecione uma sala para ver
os nicks à direita e clique em **Conectar**. **Atualizar** busca salas em Wi-Fi, cabo e interfaces VPN
quando a rede permite descoberta; também é possível entrar pelo IP manual.
Autorize UDP **25567** para busca e TCP **25566** para jogo.
As lojas de itens e melhorias são aldeões acessados pelo botão direito. A loja
de itens tem categorias com ícones, progressão de picaretas e machados,
armaduras permanentes para calça/botas e poções temporárias.
A loja de melhorias mostra preços e níveis de afiação, proteção e forja em
cartões separados. Nos mapas básicos, os aldeões ficam em barracas; nos
importados, as lojas usam os espaços das construções e os geradores ficam
sobre as plataformas originais, sem substituir os blocos do mapa.

O inventário tem **27 slots mais 9 de hotbar**. **Shift + clique esquerdo**
transfere a pilha entre inventário e hotbar, ou entre jogador e baú aberto.
As teclas da hotbar trocam o item com o slot sob o cursor. Cliques separados
colocam blocos sem intervalo artificial; segurar o botão repete a cada 0,2 s.

Em **Multiplayer → Tester**, o botão no canto inferior direito abre uma arena
local para testes, sem esperar outros jogadores. Ela tem um gerador rápido de
ferro, ouro, diamante e esmeralda; quatro camas separadas (sem defesa, madeira,
end stone e obsidiana); um dummy passivo e outro que persegue e ataca depois
de receber um golpe. Ambos mostram a vida acima da cabeça. O inventário já
vem com equipamentos e utilitários. **F8** restaura a arena, e esse modo não
concede XP.

Ao minerar, rachaduras aparecem no próprio bloco e aumentam com o progresso,
sem barra no centro da tela.

A cama quebra em **0,15 segundo**. Os tempos básicos dos demais blocos e
ferramentas seguem a dureza, o material e os ticks da Java 1.8.9; veja a
[tabela de mineração](docs/MINING.md).

Ao destruir uma cama, o pinguim acorda e foge com os braços levantados até cair
no void. A animação é sincronizada pelo host, e `sounds/ahh.mp3` toca por
proximidade enquanto ele corre e cai.

Durante a parte inicial do knockback, novos ataques de jogadores não causam
dano nem reiniciam o empurrão; novos hits são liberados 0,15 s antes do fim
estimado do impulso. Acertos confirmados deixam o jogador vermelho
por 0,2 s e reproduzem `sounds/Hit.mp3`.

A bola de fogo voa a 24 blocos/s e aplica seu impulso horizontal gradualmente,
sem causar dano direto ao dono ou aos inimigos.
O graveto de knockback arremessa aproximadamente 6,5 blocos em espaço livre;
obstáculos e movimento alteram a distância final. Flechas têm haste, ponta e
penas em 3D, acompanham a direção do voo e ficam fincadas no chão ou em paredes
por até 60 segundos. Remover o bloco de apoio remove a flecha. Esse estado é
sincronizado entre os jogadores. O arco dispara ao soltar o botão, com três níveis de puxada: curta
(até 0,1 s), média (até antes de 0,9 s) e máxima (a partir de 0,9 s). O intervalo
entre disparos é 0,3 s. A velocidade máxima da flecha é 60 blocos/s, com arrasto
e gravidade por ticks de 20 Hz.

O dano base é de **1 coração** para espada de madeira/ouro, **1,5** para pedra,
**2** para ferro e **2,5** para diamante. Flechas causam **0,5**, **1** ou **2 corações** de dano base conforme a puxada.
Após 1 s, há **10% de chance** de um crítico de **3 corações**, com partículas
de brilho. Armadura e proteção reduzem esses valores.

O terreno da arena é protegido; blocos colocados pelos jogadores e camas
adversárias podem ser quebrados.
Segure **TAB** para listar os jogadores e seus times. No BedWars, a TNT explode
após cinco segundos, aplica knockback sem dano direto e destrói construções de lã até quatro blocos, madeira
até três e pedra do fim até dois. Obsidiana resiste à explosão.
A TNT aparece no bloco apontado, mostra o tempo restante acima dela e pulsa
cada vez mais rápido conforme a explosão se aproxima.

Consulte [BedWars LAN](docs/BEDWARS.md) para controles, regras e limitações.
As partidas são temporárias e separadas dos saves do single-player.
Com **Layout padrão** ligado, aparecem Rush e os três mapas gerados.
Desligado, a seleção mostra 18 mapas avançados em páginas de seis cartões.
A lista e o processo de importação estão em [Mapas](docs/MAPS.md).
As equipes são Vermelho, Azul, Verde, Amarelo, Ciano, Branco, Rosa e Cinza.
O Airshow importado preserva as oito bases e os balões originais; o NC mantém
duas equipes por ser um mapa 1v1. Os saves antigos de altura 80 ou 128 continuam
carregando na janela de altura 192.

O mapa **NC** inclui a arena importada para dois times, em Solo ou Dupla.
As lajes, escadas, cercas, grades e carpetes preservam formas e materiais da
construção. Consulte [BedWars LAN](docs/BEDWARS.md#mapa-nc) para detalhes.

### Progressão no BedWars

No SkyWars, cada kill concede **10 XP** e a vitória concede **25 XP**.
No BedWars, kills normais concedem **5 XP**, kills finais (vítima sem cama) **15 XP** no total,
camas destruídas **10 XP** e cada jogador da equipe
vencedora recebe **25 XP** uma única vez por partida. Empates não dão o bônus
de vitória. Cada **50 XP acumulados**
sobe um nível: nível 2 com 50 XP, nível 3 com 100 XP, etc. Cada subida deixa uma
caixa pendente, entregue quando o jogador volta ao lobby.

No inventário (`E`), **Abrir caixa** mostra uma roleta gratuita; **Inventário de
skins** permite escolher cores para espadas, picaretas e machados, separadamente.
São 36 skins de itens, com raridades e possibilidade de repetição, visíveis em
primeira pessoa e pelos outros jogadores. Não alteram atributos ou o personagem.

O perfil local leva nível, caixas e skins para salas de outros hosts. Os dados
ficam em `%APPDATA%\MineClone` no Windows e `~/.local/share/MineClone` no Linux
(ou `$XDG_DATA_HOME/MineClone`). Os diretórios `profiles/` e `skins/` guardam
progresso e cores de ferramentas; perfis antigos são migrados automaticamente.
O host mantém um cache de recuperação e o cliente salva as atualizações
recebidas. Maiúsculas e minúsculas identificam o mesmo nick. Para levar o perfil
a outro computador, copie sua pasta de dados; usar apenas o nick não é um login.
Não há autenticação ou banco de dados central. A pasta `skins/` guarda as cores
das ferramentas; não é um carregador de skins PNG do personagem.
Veja [Níveis, caixas e skins](docs/BEDWARS.md#níveis-caixas-e-skins).

**Q** joga uma unidade do item selecionado no chão; **Ctrl + Q** joga a pilha
inteira. Os itens têm física e podem ser recolhidos ao chegar perto, após uma
pequena espera de 0,8 segundo. No BedWars, os drops são sincronizados pelo host.
A tecla pode ser alterada nas opções de controles. Com uma pilha no cursor,
clicar fora da janela do inventário com o botão esquerdo dropa a pilha inteira;
o botão direito dropa uma unidade.

## Controles por toque

O botão central do D-pad alterna o agachamento: toque para agachar e toque
novamente para levantar, sem precisar segurar. Abrir um menu cancela o agachamento.

No lobby BedWars, **Escolher time** abre a seleção e **Iniciar partida** aparece
somente para o host. Esses botões ficam no topo e dispensam teclado no celular.

Ao detectar um toque real na tela, o jogo mostra controles no estilo do Minecraft
Pocket Edition antigo. Cliques comuns do mouse não ativam a interface touch.
O direcional permite andar em diagonal e o círculo central permite agachar;
os botões à direita permitem pular, usar itens e minerar. É possível andar,
olhar e pular simultaneamente com vários dedos.

- Toque rápido com bloco na mão e local válido: colocar um bloco. Nos outros casos, atacar.
- Segure parado por **0,3 segundo**: começar a quebrar; continue segurando para minerar.
- Arraste na área do mundo: olhar, sem atacar ou colocar por acidente.
- **Usar**: usar imediatamente, inclusive segurar arco, comida ou poção.
- **Picar**: segurar para quebrar blocos.
- Toque na hotbar para selecionar; **...** abre o inventário e **Voltar** fecha.
- **Correr** controla a corrida; **II** abre a pausa e **Q** dropa uma unidade.

Em **Opções → Controls → Touch: editar botoes**, selecione e arraste os controles
para mudar sua posição. **Menor -** e **Maior +** alteram o tamanho do controle
selecionado, incluindo a hotbar. O botão **D-pad / Joystick** alterna o movimento;
**Restaurar** recupera as posições e tamanhos padrão. **Salvar / voltar** guarda o
layout para a próxima execução. O editor também aceita mouse no computador.

O suporte nativo desktop usa Windows Touch e XInput 2.2 no Linux com X11.
A leitura no Android usa os pontos de toque do Raylib. Os testes de gestos podem
ser executados com `make test-touch`. Veja [Compilar para Android](docs/ANDROID.md)
para gerar o APK no Arch Linux. A versão Android ainda precisa de validação em
aparelho físico.

## Teclas e campo de visão

Em **Opções → Controls → Keyboard**, clique numa ação e pressione a nova tecla ou botão
do mouse. Use a roda para ver as demais ações. Backspace remove a bind durante
a captura; ESC cancela. Teclas já usadas por outra ação são rejeitadas. **Reset**
restaura os controles padrão. Navegação e edição de texto dos menus mantêm seus
atalhos convencionais; ESC continua disponível para sair dos menus.

As opções estão separadas em **Visuals**, **Controls** e **Extra**. Visuals reúne
FOV, distância de visão, neblina e horário; Controls contém a sensibilidade do
mouse, Toggle Sprint e as teclas. Extra oferece céu com gradiente, nuvens de dia
e estrelas à noite, além de **Full Bright**, que ilumina a cena sem alterar o
horário ou a simulação. Os dois recursos podem ser ligados ou desligados.

**FOV ao correr**, em Visuals, aumenta suavemente o campo de visão em 8 graus
durante a corrida e retorna ao valor normal ao parar. Pode ser desligado.
A corrida também tem passos visuais mais rápidos, com um balanço leve da câmera
e dos itens, sem alterar a velocidade física de 5,612 blocos/s.

O slider **FOV** ajusta o campo de visão de 40 a 110 (padrão 70). FOV e binds
são salvos em `settings.cfg`; configurações de versões anteriores são migradas
preservando suas opções existentes.

A caminhada usa **4,317 blocos/s** e a corrida **5,612 blocos/s**, cerca de 30% a mais.

**Toggle Sprint**, ligado por padrão nas opções, permite tocar Ctrl uma vez
para ativar a corrida e outra vez para desativar. Funciona com a tecla de correr
configurada em Keyboard. Desligue a opção para voltar a segurar a tecla.

## Salvamento

Os mundos ficam em `saves/`, cada um com seu arquivo `world.dat`. A seleção de
mundos também reconhece arquivos `.dat` na raiz do projeto.

O jogo salva blocos, níveis e fontes dos líquidos, posição, orientação, velocidade
vertical, estado de voo, modo de jogo, vida, ar, durabilidade e horário ao pausar, abrir o inventário, perder foco ou encerrar
normalmente. **Salvar e Voltar ao Menu** grava o mundo e retorna à tela inicial.
Os 27 slots pessoais, os 9 slots da hotbar e a pilha no cursor também são persistidos.

Os saves usam formato binário versionado, checksum e validação antes do
carregamento. A gravação utiliza um arquivo temporário seguido de substituição
por rename. O formato atual é a versão 12, com leitura das versões anteriores.

**Excluir Mundo** remove o save selecionado e sua pasta caso ela fique vazia.
Outros arquivos no diretório são preservados.

Execute o jogo a partir da raiz do projeto: os caminhos de saves e configurações
são relativos ao diretório de execução.

### Exploração e regiões

Ao se aproximar da margem, a janela ativa avança em passos de 32 blocos.
Saves antigos de 128 × 128 são ampliados ao carregar, preservando posição,
inventários e construções em chunks arquivados. O terreno usa coordenadas globais e a seed do mundo para manter continuidade
entre regiões. Não há troca de mapa nem teletransporte: jogador e coordenadas
locais são deslocados juntos, preservando a posição global e a câmera. Com a neblina ligada, a névoa aumenta perto da margem para suavizar o limite
da janela carregada. Desligá-la também desliga essa contribuição nas bordas.

Chunks que saem da janela são gravados em `world.dat.chunks/`, ao lado do save;
quando você retorna, os blocos construídos e os estados de água e lava são
restaurados. O arquivo `world.dat` mantém a região ativa e a posição do jogador.
**Ao fazer backup ou compartilhar um mundo, copie também `world.dat.chunks/`.**

Regiões fora da janela não simulam fluidos. A geração, leitura e reconstrução das
malhas ainda são síncronas e podem causar uma breve pausa na transição. A altura
permanece limitada a 192 blocos; a exploração horizontal se estende até cerca de
100 milhões de blocos por eixo e ocupa mais espaço em disco conforme o mapa cresce.

## Arquitetura

```text
include/      Interfaces e definições compartilhadas
src/          Implementação dos módulos
  main.cpp      Janela e inicialização
  game.cpp      Loop principal e transições de estado
  world.cpp     Geração, blocos, fluidos, raycasting e saves
  player.cpp    Câmera, movimentação, colisão e natação
  renderer.cpp  Atlas, malhas, transparência e shaders GLSL
  lighting.cpp  Propagação, visibilidade e amostragem de luz
  ui.cpp        Menus, inventário, hotbar e HUD
  settings.cpp  Leitura e gravação das preferências
  chest.cpp     Inventários, pilhas e persistência de baús
  crafting.cpp  Receitas, fabricação e fornalhas
  survival.cpp  Mineração, ferramentas, equipamento, vida e efeitos
  items.cpp     Projéteis, explosões, consumíveis e interação de itens
  network.cpp   Cliente experimental do protocolo Java 1.8
  bedwars.cpp   Host LAN, equipes, lojas, combate e regras da partida
  bedwars_maps.cpp Arenas procedurais, NC e Airshow incorporados
  external_arenas.cpp Dados comprimidos dos demais mapas importados
  progression.cpp Perfis locais, XP, caixas e desbloqueios
  cosmetics.cpp   Inventário de skins e animação das caixas
  lan_transport.cpp Transporte TCP e descoberta de salas por UDP
  map_blocks.cpp Estados, formas e colisão de blocos importados
tests/        Testes de simulação, geometria, iluminação e opções
LICENSES/     Licenças de código de terceiros
Makefile      Build incremental e alvos de teste
```

### Renderização

O mundo é dividido em chunks de 16 × 16 blocos na horizontal. Cada chunk combina
as faces expostas em uma malha. Edições atualizam as revisões dos chunks afetados,
incluindo vizinhos necessários para recalcular faces e iluminação nos limites.

Durante a partida, o recálculo de luz das edições é dividido entre quadros, com
aproximadamente 2 ms de computação por quadro. A luz anterior permanece visível
até o novo resultado estar completo. Novas edições são agrupadas e substituem
trabalhos pendentes, evitando publicar iluminação de um estado antigo do mundo.
Carregamentos e trocas de mundo calculam a luz completa antes de desenhar a cena.

`make test-edit-frames` mede colocação e remoção repetidas no mapa NC em uma
janela oculta; os tempos dependem da máquina e do driver gráfico.

A geometria opaca é desenhada primeiro. As faces transparentes são ordenadas por
profundidade e desenhadas em seguida, sem escrita no buffer de profundidade.
A água utiliza altura proporcional ao nível e faces laterais que fecham os
degraus entre células vizinhas.

Lanternas emitem luz em 24 níveis. A iluminação direta verifica obstáculos pela
grade; a contribuição indireta usa propagação por vizinhança. Os vértices
amostram células externas às faces e o shader interpola a iluminação. As sombras
possuem resolução de blocos, sem shadow maps por pixel.

### Simulação de líquidos

Água e lava compartilham o mesmo algoritmo, com relógios independentes: **150 ms**
por tick para água e **300 ms** para lava. Suporte e propostas de fluxo são
calculados antes de aplicar as alterações à grade.

1. A queda vertical tem prioridade e preenche a célula inferior com nível 7.
2. Sobre um piso sólido, a água procura os caminhos acessíveis mais curtos até
   uma queda dentro do alcance restante. Caminhos de mesmo comprimento dividem
   o fluxo.
3. Cada passo horizontal reduz o nível em uma unidade. O nível 1 encerra a
   propagação lateral; sem queda acessível, o fluxo ocupa o piso respeitando
   esse limite.
4. Ao alcançar o fundo de uma queda, o escoamento continua no novo patamar.
   A queda renova o nível, mas não cria uma fonte permanente.
5. Água sem conexão válida com uma fonte perde um nível por tick até secar.

Água e lava mantêm fontes e níveis separados por célula, sem substituir uma à
outra. A lava usa a mesma geometria de superfície por nível e conserva sua
emissão de luz. Ao carregar um save anterior à versão 6, os blocos antigos de
lava estática passam a ser fontes; níveis de água e estado do jogador são mantidos.

As tabelas de referência 1.8 são fornecidas por
[PrismarineJS/minecraft-data](https://github.com/PrismarineJS/minecraft-data).
A atribuição está em `LICENSES/PrismarineJS-minecraft-data.txt`.

## Testes

Execute os testes sem abrir janela:

```sh
make test
```

As suítes executam os mesmos cenários de propagação e secagem para água e lava:
rotas com obstáculos, quedas
consecutivas, limites de nível, geometria entre chunks, bloqueio e remoção de
luz, interpolação nos vértices, ciclo solar, saves, trava de horário e sensibilidade. Há verificações específicas da velocidade da
lava, persistência dos níveis e conversão de saves com lava estática. O voo e o
carregamento de regiões são testados com ida e volta, coordenadas negativas,
preservação de construções e detecção de arquivos de chunk corrompidos. Há testes
de migração de 128 para 256 blocos, neblina, distância de visão, catálogo,
ferramentas, tingimento, compactação e fundição de ouro.

Para verificar os shaders e renderizar quatro fases solares em uma janela
oculta, execute em uma sessão gráfica:

```sh
make test-renderer
```

Para verificar progressão, rede local e mapas importados separadamente:

```sh
make test-progression
make test-platform
make test-maps
```

### Cobertura e evidências

Os testes verificam cenários específicos; não comprovam ausência de bugs nem
compatibilidade completa com servidores públicos. A suíte inclui crafting,
consumo de ingredientes, fornalhas, inventários, saves e migrações. Os testes de
BedWars exercitam host e clientes em loopback, seleção e equilíbrio de equipes,
compras, construção protegida, dano, knockback, consumo de maçãs, regeneração,
vitória e reinício. Também há conexão entre dois processos MineClone e
verificação de envio da animação de ataque, transporte de perfil local entre
hosts, bônus único de vitória e sincronização de flechas fincadas. Há testes
de impacto no chão e em paredes, remoção do suporte e expiração das flechas.
Os testes de mapas verificam colisão das estações e preservação dos blocos
originais ao iniciar uma partida.

`make test-renderer` verifica shaders e renderização em uma sessão gráfica,
com capturas em `build/`. A versão Windows foi compilada com MinGW e sua
abertura foi verificada pelo Wine. Isso não substitui uma partida prolongada
entre computadores Linux e Windows em uma rede real.

### Desempenho

O projeto usa malhas por chunk, descarte de faces internas, frustum culling e
atualização incremental das malhas. Essas são decisões de implementação;
**não há benchmark padronizado publicado nem garantia de uma taxa de FPS**.
O desempenho depende do hardware, resolução, distância de visão, quantidade de
luzes e líquidos e estado da partida.

Para publicar uma medição reproduzível, registre CPU, GPU, RAM, sistema,
compilador, resolução, configurações, mapa, número de jogadores e duração do
teste. Meça também as pausas ao editar blocos e carregar regiões, além do FPS.

## Limitações atuais

A região ativa é limitada, mas a exploração horizontal carrega novos chunks.
Água e lava usam propagação por níveis e fontes: são autômatos celulares,
não uma simulação física com conservação de volume. Uma fonte pode criar
novas células de líquido, e a secagem reduz níveis ao perder alimentação.
Essa limitação continua presente. Água e lava ainda não reagem ao se tocar.
A lava causa dano no modo sobrevivência.

Crafting, fornalhas e multiplayer estão implementados, com a cobertura descrita
nas seções acima. A compatibilidade Java é parcial; o BedWars LAN usa protocolo
próprio e exige versões compatíveis do MineClone nos participantes. O NC usa
as regras locais do BedWars, sem executar os command blocks do mundo Java.

Áudio e crescimento de plantas não estão implementados. O carregamento de
regiões não é assíncrono. Não há paridade completa com Minecraft 1.8.9.

## Créditos

Os sistemas de blocos, propagação de luz e variação solar foram adaptados a
partir de **swr06/Minecraft**, de **Samuel Rasquinha**. A licença MIT original
está preservada em [LICENSES/swr06-Minecraft.txt](LICENSES/swr06-Minecraft.txt).
As texturas utilizadas pelo executável são procedurais.

MineClone é um projeto independente inspirado em Minecraft, sem afiliação com
Mojang ou Microsoft.

## Anticheat

No BedWars LAN, o host valida inventário, alcance e tempo de mineração. O sistema
novo limita deslocamentos acumulados, subida, voo parado e posições dentro de
blocos, com tolerância para lag, poções, respawn, pérolas e knockback. Cada cliente
acompanha as amostras de movimento e usa seu próprio verificador para confirmar
episódios suspeitos. Confirmações repetidas, vencidas ou sem evidência não contam.

O host também passa pelas verificações. Se for expulso, a sala é transferida para
o jogador conectado há mais tempo depois dele. Mapa, times, inventários, perfis,
efeitos e entidades em andamento são preservados, com uma pausa breve para
reconectar. A conexão com o sucessor é testada antes de desconectar o host antigo.

O kick exige 1 confirmação com até 3 participantes, 2 com 4–5, 3 com 6–8, 4 com
9–11, e assim por diante. Não há ban por nick. No Windows há também verificações
locais de ferramentas conhecidas, módulos suspeitos, integridade do código e
handles externos com permissão de ler ou modificar a memória, com tolerância para componentes com assinatura válida da Microsoft
(ferramentas legítimas também podem disparar a regra). Na detecção local, o jogo fecha a janela
e mostra **“LoL try to cheating.”**. Ao clicar em **OK**, o processo encerra sem
reabrir automaticamente; uma nova abertura deve ser feita manualmente. Não há
ban de hardware nem bloqueio persistente de sala. O motivo fica em
`%LOCALAPPDATA%\MineClone\anticheat.log`.

Veja [funcionamento e limites](docs/ANTICHEAT.md). A detecção Windows precisa de
validação em Windows real; ela não garante detectar todo injetor, DLL ou leitura.
Host e convidados precisam atualizar para o **protocolo LAN 23**.

## SkyWars

O cartão SkyWars abre salas solo no mapa Bosque Celeste, com oito ilhas,
28 baús com conteúdos variados e compartilhados, loot superior no centro e vitória do último time vivo,
sem camas nem respawn. Todos começam sem itens e armadura. Ovos e bolas de neve funcionam com knockback multiplayer;
há equipamentos, blocos, arco, pérolas, maçãs, poções, TNT e baldes.
Veja [regras, itens e limites](docs/SKYWARS.md). Todos devem atualizar para LAN 23.

# MineClone

Jogo de construção e exploração em primeira pessoa, inspirado no Minecraft clássico e desenvolvido
em **C++17 com Raylib**. Explore terrenos procedurais, construa com blocos e salve
mundos independentes.

O projeto utiliza malhas por chunk, simulação de água por níveis e iluminação
por blocos com lanternas, sombras e ciclo de dia e noite. Texturas e shaders são
criados no código, sem pacotes de assets externos.

## Recursos

- **Geração procedural:** mundos de 128 × 128 × 80 blocos com seed aleatória,
  biomas, montanhas, árvores, cactos e lagos.
- **Construção livre:** 40 blocos e plantas, inventário criativo paginado e
  hotbar com nove posições configuráveis.
- **Água dinâmica:** prioridade de queda, busca pelos caminhos mais curtos até
  desníveis, propagação limitada e secagem após a remoção da fonte.
- **Iluminação:** lanternas com bloqueio de luz por obstáculos, luz interpolada
  nos vértices, oclusão ambiente e neblina.
- **Dia e noite:** ciclo de 20 minutos, ajuste de horário entre 00:00 e 23:59 e
  opção de manter o relógio travado.
- **Movimentação:** câmera em primeira pessoa, colisão AABB, deslizamento nas
  paredes, pulo contínuo e natação.
- **Mundos persistentes:** criação, seleção e exclusão de saves, com restauração
  da posição, orientação e horário.

## Compilar e executar

### Dependências no Arch Linux

```sh
sudo pacman -Syu --needed gcc make pkgconf raylib
```

É necessária uma sessão gráfica com suporte a **OpenGL 3.3**. O projeto utiliza
GCC, GNU Make e `pkg-config` para localizar a Raylib.

### Build

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
| Pular continuamente / nadar para cima | Segurar Espaço |
| Nadar para baixo | Shift esquerdo |
| Quebrar bloco | Botão esquerdo do mouse |
| Colocar bloco | Botão direito do mouse |
| Selecionar posição da hotbar | 1–9 ou roda do mouse |
| Abrir / fechar inventário | E |
| Abrir pausa / voltar | Esc |
| Incluir água na seleção de blocos | Segurar Alt |
| Ligar / desligar a lanterna apontada | F |
| Avançar um quarto do ciclo solar | F6, com o horário destravado |
| Trocar página do inventário | Setas esquerda/direita, Page Up/Down ou botões |

No inventário, escolha um bloco e clique em uma posição da hotbar, ou pressione
**1–9** para equipá-lo. A câmera e a simulação ficam pausadas enquanto o
inventário está aberto.

### Opções

Acesse **Opções...** pelo menu principal ou pelo menu de pausa.

- **Horário:** arraste a barra entre **00:00 e 23:59**. Pela pausa, a iluminação
  do mundo ao fundo acompanha o ajuste.
- **Travar horário:** mantém a hora escolhida sem interromper a física ou a
  simulação de água. Para um dia permanente, selecione **12:00** e ative a trava.
- **Sensibilidade:** ajuste o mouse entre **10% e 300%**. O valor de **100%**
  corresponde à sensibilidade padrão.

Clique em **Concluído** ou pressione **Esc** para salvar as preferências em
`settings.cfg` e retornar ao menu anterior. O horário escolhido também define
como novos mundos começam; mundos existentes restauram seu próprio horário salvo.

## Salvamento

Os mundos ficam em `saves/`, cada um com seu arquivo `world.dat`. A seleção de
mundos também reconhece arquivos `.dat` na raiz do projeto.

O jogo salva blocos, níveis e fontes de água, posição, orientação, velocidade
vertical e horário ao pausar, abrir o inventário, perder foco ou encerrar
normalmente. **Salvar e Voltar ao Menu** grava o mundo e retorna à tela inicial.
A organização da hotbar não é persistida.

Os saves usam formato binário versionado, checksum e validação antes do
carregamento. A gravação utiliza um arquivo temporário seguido de substituição
por rename. O formato atual é a versão 5, com leitura das versões anteriores.

**Excluir Mundo** remove o save selecionado e sua pasta caso ela fique vazia.
Outros arquivos no diretório são preservados.

Execute o jogo a partir da raiz do projeto: os caminhos de saves e configurações
são relativos ao diretório de execução.

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
tests/        Testes de simulação, geometria, iluminação e opções
LICENSES/     Licenças de código de terceiros
Makefile      Build incremental e alvos de teste
```

### Renderização

O mundo é dividido em chunks de 16 × 16 blocos na horizontal. Cada chunk combina
as faces expostas em uma malha. Edições atualizam as revisões dos chunks afetados,
incluindo vizinhos necessários para recalcular faces e iluminação nos limites.

A geometria opaca é desenhada primeiro. As faces transparentes são ordenadas por
profundidade e desenhadas em seguida, sem escrita no buffer de profundidade.
A água utiliza altura proporcional ao nível e faces laterais que fecham os
degraus entre células vizinhas.

Lanternas emitem luz em 24 níveis. A iluminação direta verifica obstáculos pela
grade; a contribuição indireta usa propagação por vizinhança. Os vértices
amostram células externas às faces e o shader interpola a iluminação. As sombras
possuem resolução de blocos, sem shadow maps por pixel.

### Simulação de água

O fluido avança em ticks de **150 ms**, calculando suporte e propostas de fluxo
antes de aplicar as alterações à grade.

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

## Testes

Execute os testes sem abrir janela:

```sh
make test
```

As suítes cobrem propagação e secagem da água, rotas com obstáculos, quedas
consecutivas, limites de nível, geometria entre chunks, bloqueio e remoção de
luz, interpolação nos vértices, ciclo solar, saves, trava de horário e sensibilidade.

Para verificar os shaders e renderizar quatro fases solares em uma janela
oculta, execute em uma sessão gráfica:

```sh
make test-renderer
```

## Limitações atuais

O mundo é finito. A simulação de água não conserva volume, e a lava é apenas um
bloco visual, sem propagação ou dano. Crafting, áudio, multiplayer, crescimento
de plantas e carregamento infinito de chunks não estão implementados.

## Créditos

Os sistemas de blocos, propagação de luz e variação solar foram adaptados a
partir de **swr06/Minecraft**, de **Samuel Rasquinha**. A licença MIT original
está preservada em [LICENSES/swr06-Minecraft.txt](LICENSES/swr06-Minecraft.txt).
As texturas utilizadas pelo executável são procedurais.

MineClone é um projeto independente inspirado em Minecraft, sem afiliação com
Mojang ou Microsoft.

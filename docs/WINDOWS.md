# Windows x64 a partir do Linux

## Preparação no Arch Linux

```sh
sudo pacman -S --needed mingw-w64-gcc cmake make curl
make windows
```

O [MinGW-w64](https://www.mingw-w64.org/build-systems/cmake/) gera código Windows
sem executar o compilador dentro do Windows. O build baixa as versões fixadas
Raylib 5.5 e zlib 1.3.1 por HTTPS dos repositórios oficiais. As bibliotecas Linux
do sistema não são usadas no executável Windows.

O limite padrão é de quatro tarefas simultâneas. Para alterá-lo:

```sh
JOBS=8 make windows
```

## Arquivos gerados

- `dist/windows/minecraft_clone.exe`: jogo Windows x64.
- `dist/windows/LICENSE*`: licenças das dependências distribuídas.
- `build/windows/objects/`: objetos exclusivos do MinGW.
- `build/windows/prefix/`: cabeçalhos e bibliotecas Windows.
- `build/windows/sources/`: fontes das dependências.

Copie a pasta `dist/windows/` para Windows 10 ou 11 de 64 bits e execute o jogo
em uma pasta com permissão de escrita. É necessário suporte a OpenGL 3.3.
Raylib, zlib e os runtimes do compilador são ligados estaticamente; as DLLs
importadas pelo executável pertencem ao Windows.

O build Linux continua disponível com `make`. Os dois builds podem coexistir.
`make clean` apaga o diretório `build`, incluindo o cache das dependências Windows;
o próximo `make windows` baixa e compila novamente essas dependências.

## Rede e dados

A camada de sockets usa POSIX no Linux e Winsock no Windows, preservando o
formato dos pacotes. A resolução SRV usa o resolvedor do sistema correspondente
(`libresolv` ou `DnsQuery_A`). O BedWars LAN permite clientes dos dois sistemas
na mesma sala quando estão usando a mesma versão do jogo. Autorize TCP 25566
no firewall da máquina que hospeda.

Saves e configurações usam substituição do arquivo temporário após a gravação.
No Windows, `MoveFileExW` permite substituir um save já existente, incluindo
caminhos representáveis por `std::filesystem::path`.

## Verificação

```sh
make test
make test-renderer
make test-platform
```

O teste de plataforma verifica inicialização de sockets, conexão LAN local,
troca de pacotes binários, substituição de save existente e parsing SRV.
Para executar sua versão Windows no Linux, instale Wine e execute:

```sh
sudo pacman -S --needed wine
make test-windows-platform
```

O prefixo Wine usado pelo teste fica em `build/windows/wine-test/`, separado
do prefixo pessoal. Os testes via Wine não substituem uma partida entre dois
computadores Windows/Linux reais.

## Descoberta de salas

Na tela BedWars LAN, **Procurar na rede local** abre a lista de hosts MineClone.
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

# Multiplayer Java 1.8 experimental

MineClone implementa um cliente TCP do protocolo **47**, usado pela família Java
1.8. O menu **Multiplayer** aceita `endereco:porta` (porta padrão 25565) e um nome
de 3 a 16 letras, números ou `_`.

**A compatibilidade com MushMC não foi validada. O cliente ainda não substitui
um cliente Minecraft completo para servidores públicos.** O endereço do Mush é
apenas um campo inicial editável; não representa uma certificação de suporte.

## Implementado e testado

- Handshake, login sem criptografia, VarInts, quadros e compressão zlib.
- Chunks individuais/em lote, alterações de blocos e origem vertical móvel.
- Posição do jogador, correções do servidor, corrida/agachamento e voo permitido.
- Outros jogadores com representação simples, envio de ataques e animação.
- Vida, morte, renascimento e velocidade de knockback recebidos do servidor.
- Chat (`T` ou `/`) com componentes traduzidos e nick, keep-alive e desconexão.
- Hologramas de armor stands com nomes visíveis e cores de texto.
- Lista de jogadores (`Tab`), nomes formatados, ping, cabeçalho e rodapé.
- Imagens de mapas Java 1.8 na mão e em molduras, com atualizações parciais.
  Isso permite exibir verificações enviadas como mapas para resposta manual;
  a verificação específica do MushMC ainda não foi validada.
- Hotbar e inventários do servidor, cliques, confirmação de transações e NBT de
  itens preservado nos pacotes enviados.
- O inventário criativo local e a simulação local de líquidos/consumíveis não
  alteram o mundo conectado. Combate, inventário e alterações são decididos pelo
  servidor. O mundo recebido não é salvo sobre um save local.

`make test` executa um servidor TCP de teste em `127.0.0.1`, com porta aleatória.
Ele verifica login, compressão, chunks, jogadores, dano, knockback, inventário,
ataque, chat com nick, lista de jogadores, mapas, keep-alive e encerramento.
`make test-renderer` também captura mapas e Tab usando esse servidor local.
Os testes não fazem login em servidores públicos.

## Limitações importantes

- Não há login Microsoft, validação de sessão ou criptografia RSA/AES. Servidores
  que exigem autenticação são recusados com uma mensagem explícita.
- Não há negociação de versões além de 1.8, packs de recursos,
  mods Forge no cliente, skins, NPCs/mobs completos, placares, efeitos sonoros nem todas as
  entidades e estados de blocos.
- Blocos sem equivalente local usam geometria simplificada. Isso também limita
  colisões e movimentação; compatibilidade com anticheats não foi validada.
- Não foram realizados testes de partidas, sincronização prolongada ou partidas
  no MushMC. Receber um login aceito não prova que todos os seus modos funcionem.
- Dano e impulso são aplicados a partir dos pacotes recebidos; o servidor é quem
  calcula o resultado do PvP. Não se implementa dano arbitrário no cliente.

## Teste manual

Use um servidor **Java 1.8.x local ou de testes** configurado para aceitar
clientes sem autenticação de sessão. Abra Multiplayer, informe o endereço e o
nome, e clique em Conectar. O jogo entra somente após receber posição e chunks.
Esc abre a pausa; **Desconectar** retorna ao menu. `E` abre o inventário remoto.
No multiplayer, o menu não pausa o servidor nem interrompe os keep-alives.

## Referências

- [PrismarineJS: protocolo 1.8](https://github.com/PrismarineJS/minecraft-data/blob/1df9432c07c81b47326e4fb1e06d3c8d723bf4aa/data/pc/1.8/protocol.json)
- [Minecraft Wiki: protocolo](https://minecraft.wiki/w/Java_Edition_protocol)
- [Autenticação e criptografia](https://minecraft.wiki/w/Java_Edition_protocol/Encryption)

A tabela de protocolo utilizada como referência está em
`data/minecraft-1.8/protocol.json`; atribuição em
`LICENSES/PrismarineJS-minecraft-data.txt`.

## Endereços e perfil local

O nick e o último endereço usado são guardados em `settings.cfg` ao conectar.
O campo aceita domínios, `localhost:25565`, IPs da rede local como
`192.168.1.20:25565` e IPv6 no formato `[::1]:25565`.
Para jogar com amigos, use o IP e a porta de um servidor Java 1.8.x acessível
pela sua rede. O cliente não hospeda um servidor integrado.

Outros servidores públicos podem ser informados no mesmo campo, incluindo
FlameMC, mas não foram validados. Domínios sem porta explícita consultam `_minecraft._tcp` (DNS SRV).
Uma porta explícita usa diretamente o host informado.
Uma restrição de movimento imposta pelo servidor durante uma verificação
continua dependendo da conclusão normal dessa verificação.

## Movimentação

Em piso normal, a física online avança a 20 ticks/s, com aceleração e atrito
horizontais. Os testes medem velocidades estáveis de aproximadamente
4,317 blocos/s andando, 5,612 com `Ctrl` + avanço e 1,295 com `Shift`.
Agachar reduz a altura da câmera e evita caminhar para fora de uma borda.
`Shift` cancela a corrida. O cliente envia os estados de corrida e agachamento
ao servidor. Isso não certifica compatibilidade com anticheats: blocos especiais,
efeitos e demais diferenças de física ainda podem causar correções do servidor.

## Forge 1.8.x

O handshake inicial identifica a capacidade FML. Ao receber `FML|HS`, o cliente
registra os canais, envia a lista vazia de mods e responde às fases de
negociação. Aceita apenas IDs vanilla conhecidos, sem remapeamentos ou
substituições de mods. A lista vazia não promete executar mods Java.

O teste TCP local cobre a conclusão da negociação e a recusa explícita de um
bloco de mod desconhecido. Não houve validação no servidor do usuário.
Servidores com mods obrigatórios, dimensões adicionais ou registros próprios
continuam exigindo Minecraft Java com Forge e os mods correspondentes.
O suporte não abrange Forge de outras versões do Minecraft.

Referência: [mensagens oficiais FML 1.8.9](https://github.com/MinecraftForge/MinecraftForge/blob/1.8.9/src/main/java/net/minecraftforge/fml/common/network/handshake/FMLHandshakeMessage.java)
e [máquina de estados oficial](https://github.com/MinecraftForge/FML/blob/1.8/src/main/java/net/minecraftforge/fml/common/network/handshake/FMLHandshakeClientState.java).

## Diagnóstico de conexão

A entrada tem prazos absolutos: 35 segundos para concluir o login e mais
45 segundos para receber posição e chunks. Keep-alives não renovam esses
prazos. O motivo da falha aparece no menu, em múltiplas linhas.
Um timeout identifica a etapa que falhou; não certifica suporte ao servidor.

### Pedido de criptografia em LAN

O diagnóstico de `Encryption Request` indica falta de suporte a RSA/AES no
cliente. Esse pacote sozinho não identifica o tipo da conta nem comprova
exigência de autenticação Microsoft. Usar uma conta offline no launcher do
host não altera o suporte criptográfico do MineClone. O handshake FML também
não implementa criptografia ou autenticação de sessão.

## Mapas de minigames e correções de posição

O cache remoto guarda até 1024 colunas e, ao atingir o limite, descarta a mais
distante da última posição enviada pelo servidor. Chunks recebidos antes de um
teleporte são reaplicados à janela local ao trocar de origem. O atributo `generic.movementSpeed` recebido do servidor e seus modificadores
são aplicados à movimentação, sem duplicar o modificador de corrida. A física inclui
subida automática de obstáculos de até 0,6 bloco quando há apoio e espaço.

O teste TCP cobre respawn, dois chunks em lote recebidos antes do teleporte,
coordenadas distantes/negativas, mudança de faixa vertical e caminhada na arena.
Isso não valida uma partida real de Boxing no Mush.

`multiplayer.log` registra coordenadas de chunks, posições enviadas e correções
do servidor para diagnóstico. É recriado a cada tentativa de conexão e não
registra mensagens do chat ou senhas. Após reproduzir um problema, preserve esse
arquivo antes de conectar novamente. O arquivo é ignorado pelo Git.

## Identificação do cliente

O canal `MC|Brand` envia `fml,forge` como marca de compatibilidade solicitada.
Esse texto não autentica uma conta, não instala Forge/mods e não garante
compatibilidade com anticheats. A lista FML continua declarando zero mods.
O contato com o piso online usa as coordenadas exatas da colisão, sem o
pequeno afastamento usado no modo local.

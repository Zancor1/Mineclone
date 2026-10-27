# Anticheat do BedWars LAN

O host continua sendo a autoridade da sala. Os clientes enviam intenções de ação;
dano, recursos, inventário, compras, tiros, camas e edição do mundo são decididos
pelo host. Ataques corpo a corpo já respeitam 3,3 blocos e a obstrução do cenário.
Bater no ar não é tratado como trapaça.

## Verificações durante a partida

O host aplica um orçamento de deslocamento acumulado. Mandar cem posições no
mesmo instante não dá cem vezes a tolerância de distância. O orçamento admite
pequenas rajadas de rede e aumenta com poções e impulsos autorizados pelo host.
Também há limites de subida, detecção de hover prolongado sem suporte e rejeição
de posições dentro das caixas reais de colisão, incluindo slabs e escadas.
Respawn e teleporte legítimos reinicializam os verificadores. O Tester usa a
simulação livre dos dummies e não recebe essas punições de movimento.

As amostras de movimento, inclusive as rejeitadas, são distribuídas pelo host com
identidade autenticada e tempo do servidor. Cada cliente mantém seu próprio
orçamento e consulta sua cópia do mapa para detectar movimentos impossíveis.
Não há botão de denúncia: a confirmação é automática, depois de repetidas
violações observadas. As tolerâncias são conservadoras e não detectam todo speed
hack sutil ou toda trajetória de voo; não substituem simulação física completa
no servidor.

Mineração também valida o tempo e a ferramenta do início/fim da ação. Há limite
de 400 pedidos por segundo simulado por convidado, para limitar spam de ações.
Mineração e spam usam a evidência numérica do host; movimento exige também uma
detecção no verificador do observador.

## Confirmações e kick

| Pessoas conectadas, incluindo o acusado | Confirmações distintas |
| --- | --- |
| 1–3 | 1 |
| 4–5 | 2 |
| 6–8 | 3 |
| 9–11 | 4 |
| 12–14 | 5 |
| 15–16 | 6 |

O host conta como um observador quando o acusado é outro jogador. Quando o
próprio host é acusado, apenas os demais clientes podem confirmar. O acusado não pode confirmar a própria acusação.
Cada observador só conta uma vez no mesmo episódio. A confirmação deve citar
uma evidência criada pelo host, na rodada atual, há no máximo oito segundos.
Desconexão ou reutilização de slot não transfere confirmações para outro jogador.
Sem consenso, a ação inválida continua sendo rejeitada, mas não há kick por aquele
episódio. Um pacote de protocolo corrompido pode causar desconexão imediata.

O kick mostra `LoL try to cheating.` e não cria ban por nick ou IP. O host continua sendo a autoridade do estado, mas seu movimento também é
observado pelos demais clientes e ele pode ser expulso. Nesse caso há transferência
da sala, descrita abaixo. Um host com o protocolo adulterado ainda pode fabricar
estado ou recusar a transferência; isso não fornece consenso criptográfico contra
um servidor totalmente malicioso. Outros servidores externos seguem suas próprias
regras; esta implementação é para o BedWars LAN do MineClone.

## Expulsão do host e transferência da sala

O sucessor é o participante conectado há mais tempo depois do host. A ordem de
entrada é mantida separadamente dos slots, inclusive após reconexões e novas
rodadas. Se o primeiro candidato não conseguir hospedar, o protocolo tenta os
seguintes participantes nessa ordem.

A simulação pausa enquanto o host envia um checkpoint ao sucessor em blocos de
até 48 KB, com confirmação por bloco. O checkpoint contém os jogadores e seus
inventários, HP, XP, times, melhorias e efeitos; também contém baús, geradores,
relógio e fase da partida, blocos alterados e colocados, drops com sua física,
projéteis com dano e tempo de vida, TNT e pinguins. O limite do checkpoint é 32 MB.

O sucessor abre uma porta TCP livre. Antes do kick, o host antigo e os demais
clientes testam uma conexão autenticada com o sucessor. Depois de todos confirmarem,
os clientes reconectam usando tickets de uso único vinculados a seus IDs. O host
antigo recebe o kick e o sucessor continua a mesma partida. IDs e times não são
trocados, e a sala passa a ser anunciada no endereço/porta do novo host. O mesmo
procedimento funciona se outro host for expulso mais tarde.

É necessário que os participantes consigam conectar diretamente ao sucessor na
rede, com o firewall permitindo o jogo. A troca não cria um relay de internet nem
configura roteadores. Se nenhum candidato acessível conseguir receber a sala, a
transferência falha com mensagem. Esta é uma transferência cooperativa antes do
encerramento: não recupera automaticamente a sala se o processo do host for morto
abruptamente antes de fornecer o checkpoint. Os tickets protegem contra outro
convidado reivindicando um slot alheio, mas não contra o host emissor adulterado.

## Proteção local no Windows

O monitor verifica na abertura e, em uma thread separada, aproximadamente a cada
500 milissegundos entre varreduras (o tempo de cada consulta pode aumentar esse intervalo):

- Nomes conhecidos de ferramentas, incluindo Extreme Injector, Xenos, Process
  Hacker/System Informer e executáveis conhecidos do Cheat Engine.
- DLLs carregadas fora dos diretórios de sistema que não tenham assinatura
  Authenticode válida. O executável do jogo é excluído dessa regra.
- Handles de outros processos com direitos de leitura, escrita, operação de
  memória ou criação de thread sobre o jogo, quando o Windows expõe essa
  informação. Componentes com assinatura válida da Microsoft recebem tolerância;
  processos protegidos cuja origem não pode ser consultada não geram acusação. Não é
  necessário confirmar uma leitura: a permissão no handle já dispara a detecção.
- Threads do jogo iniciadas em memória executável privada, um sinal possível de
  injeção sem módulo convencional.
- Alterações nas seções executáveis do jogo em relação ao estado inicial em memória.

Assinaturas são verificadas usando o cache local, sem consulta de revogação pela
rede. DLLs recebem tolerância por diretório protegido ou assinatura válida.
Para handles externos, a tolerância exige assinatura válida da Microsoft.
Outras ferramentas legítimas de diagnóstico
e segurança também podem disparar essa regra estrita. Overlays não assinados,
ferramentas de desenvolvimento ou JITs podem causar detecção. Permissões do
Windows podem impedir consultas: falhas de consulta não são prova de trapaça.
A consulta é periódica: handles de duração curta podem passar entre verificações.
Renomear ferramentas, modificar o próprio anticheat ou atuar no kernel pode burlar
verificações de modo usuário. A proteção não
atesta que o jogo está 100% intacto e não tenta verificar processos de outros PCs.

Na detecção local, o jogo sai da sessão de forma controlada, fecha a janela e
mostra `LoL try to cheating.`. Ao confirmar em OK, o processo encerra sem reabrir
o jogo. Uma nova abertura precisa ser feita manualmente e será bloqueada se a
ferramenta continuar detectável. Não há crash deliberado.

A detecção local apenas encerra o jogo: não há ban de hardware, nick, IP ou bloqueio
persistente de sala. Arquivos antigos `anticheat-rooms.txt` são ignorados.
Após fechar a ferramenta detectada, o jogador pode abrir o jogo manualmente.
O motivo da detecção é registrado em `%LOCALAPPDATA%\MineClone\anticheat.log`
(nome da ferramenta, DLL, processo com handle de memória ou regra de integridade).
Esse registro serve somente para diagnóstico e não bloqueia futuras aberturas.

## Validação

`make test-anticheat` cobre os limites de população, duplicação, expiração,
orçamento de movimento, acusações sem evidência, kick por consenso em seis
participantes e detecção automática por um cliente real em processo separado.
`make test-host-migration` usa três processos reais, expulsa dois hosts em
sequência e verifica reconexão, mundo, times, perfis, inventário e TNT. As fixtures
antigas de gameplay usam um objeto de teste separado para seus reposicionamentos
artificiais; produção e testes de anticheat/migração não usam essa exceção.
A compilação cruzada Windows não valida o comportamento das APIs em Windows real.
Todos precisam usar a versão LAN 23. O monitor de processos é exclusivo do Windows;
a validação da partida e as confirmações também funcionam em Linux e Android.

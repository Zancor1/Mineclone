# Android

O projeto Android fica em `android/`, separado do Makefile desktop. Requer Android
8.0 ou superior e OpenGL ES 2.0. O APK padrão inclui ARM de 32 e 64 bits.

## Compilar no Arch Linux

```sh
sudo pacman -S --needed base-devel cmake ninja android-tools jdk17-openjdk curl unzip
./tools/setup-android-sdk.sh
make android
```

O instalador pede a leitura e aceitação das licenças do SDK. SDK, NDK, Gradle e
arquivos de build ficam em `build/android/`. O primeiro build precisa de internet
e baixa vários gigabytes. `android-tools` sozinho não fornece SDK nem NDK.
Se já tiver SDK pelo Android Studio, use `ANDROID_HOME=/caminho/do/sdk` antes dos
comandos. Para selecionar outro JDK 17, configure `ANDROID_JAVA_HOME`.

Saída: `dist/android/MineClone-debug.apk`, assinado com uma chave de desenvolvimento
local. Preserve `build/android/user/` para atualizar instalações com a mesma chave.
Essa é uma versão de teste; publicação em loja exige assinatura de distribuição.

Com depuração USB habilitada e o aparelho autorizado:

```sh
adb install -r dist/android/MineClone-debug.apk
```

Para compilar somente celulares ARM de 64 bits:

```sh
ANDROID_ABIS=arm64-v8a make android
```

## Adaptações

- Controles por toque e vários dedos, com seleção automática ao tocar na tela.
- Teclado virtual nos campos de nome, endereço e filtros de servidores.
- Renderização OpenGL ES e atlas compacto para GPUs móveis.
- Sons incluídos no APK; saves, configurações e perfis no armazenamento privado do app.
- Distância de visão inicial reduzida e limite inicial de 60 FPS.
- Permissões de rede e multicast para descoberta LAN; voltar do Android abre/fecha menus.

Desinstalar o aplicativo apaga seus dados privados. O multiplayer continua usando
um host acessível pela rede: compilar para Android não cria um serviço de salas
pela internet. Builds que se conectam precisam usar versões compatíveis.

A compilação do APK e os testes automatizados não substituem testes em aparelhos.
Ainda é necessário verificar desempenho, interface em diferentes telas, retomada
do aplicativo e conexão entre Android, Linux e Windows em uma rede real.

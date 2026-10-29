# Redes Neurais

## Compilando

A interface usa [Clay](https://github.com/nicbarker/clay) (incluído em `third_party/clay`) e
[raylib](https://www.raylib.com/) 5.5 ou superior. O mesmo `Makefile` atende macOS, Linux e Windows:

```
make            # build/NeuralNetwork (interface) e build/RedesNeurais (console)
make run        # abre a interface
make test       # executa o console com testes_treinamento.txt
```

Se o sistema não tiver raylib 5.5 ou superior, `make raylib` baixa e compila o raylib 6.2 em
`third_party/` e o build passa a usá-lo.

### macOS

Requer as Command Line Tools do Xcode e `brew install raylib`. `make app` gera `build/NeuralNetwork.app`.

### Linux

No Fedora: `sudo dnf install gcc-c++ make raylib-devel`. Debian e Ubuntu não têm pacote do raylib,
instale as dependências e use o `make raylib`:

```
sudo apt install build-essential curl pkg-config libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl-dev
make raylib
make
```

### Windows

No terminal UCRT64 do [MSYS2](https://www.msys2.org/):

```
pacman -S make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-raylib
make
```

Os executáveis são linkados estáticos e rodam fora do MSYS2.

### Arquivo da rede

A rede desenhada na interface é salva no mesmo formato da versão Windows original em:

- macOS: `~/Library/Application Support/RedesNeurais/NeuralNetWork.bin`
- Linux: `~/.local/share/RedesNeurais/NeuralNetWork.bin`
- Windows: `%APPDATA%\RedesNeurais\NeuralNetWork.bin`

Para abrir outra rede, arraste o arquivo para a janela. O título mostra o arquivo aberto, com `*`
quando há alterações não salvas, e "Salvar" grava nesse arquivo.

## Exemplos

A pasta `exemplos/` tem redes prontas para as funções lógicas AND, OR e XOR, com neurônios de
função degrau:

- `and.bin`, `or.bin` e `xor.bin`: arraste para a janela da interface.
- `and.txt`, `or.txt` e `xor.txt`: a mesma rede no formato do console, com as 4 amostras da tabela
  verdade: `build/RedesNeurais < exemplos/xor.txt`.

A primeira entrada é o bias, sempre -1, e o peso dela é o limiar do neurônio: no AND o neurônio
dispara quando `x1 + x2 - 1,5 >= 0`, no OR quando `x1 + x2 - 0,5 >= 0`. O XOR não é linearmente
separável e usa uma camada oculta: um neurônio OR, um AND e a saída `OR - AND - 0,5`.

## Console

```
Como usar:

RedesNeurais.exe < dados.txt

Estutura do arquivo de entrada:
(quantidade de neurônios n) (quantidade de sinais de entrada sn) (quantidade totais de sinais tn)
(id do neurônio 1) (função de ativação) [constante da função]
... n vezes
(id do neurônio n) (degrau|bipolar|logistica|hiperbolica|linear) [constante da função]

(método de ligação) (id do neurônio ou entrada) [peso da ligação] [id do neurônio de destino]
... tn vezes
(entrada|conectar|saida) (id do neurônio ou entrada) [peso da ligação] [id do neurônio de destino]

amostras (quantidade de amostras)
(vetor de entradas separadas por espaço)
[saidas] [vetor de saidas desejadas separadas por espaço]

exemplo:
;1 neurônio
;3 sinais de entrada
;4 sinais, 3 de entrada + 1 de saida
1 3 4
1 degrau ; função degrau

entrada 1  1.5 1 ; entrada 1 liga ao neurônio 1 com peso 1.5
entrada 2  1.0 1
entrada 3  1.0 1
saida   1        ; o neurônio 1 é de saida

amostras 4
-1.0 0.0 0.0
-1.0 0.0 1.0
-1.0 1.0 0.0
-1.0 1.0 1.0


as funções logistica e hiperbolica necessitam de uma contante, exemplo:
3 logistica 1.0 ; o terceiro neurônio usa a função logística com beta igual a 1.0

Analisando a Saida:
Saida 1: 0.0805225
Esperado 1: 1
Saida 2: -0.38891
Esperado 2: 0

Saida para o primeiro
```

# Redes Neurais

## Compilando

A interface usa [Clay](https://github.com/nicbarker/clay) (incluído em `third_party/clay`) e
[raylib](https://www.raylib.com/) 5.5 ou superior. O mesmo `Makefile` atende macOS, Linux e Windows:

```
make            # build/NeuralNetwork (interface), build/rna (linha de comando) e
                # build/RedesNeurais (console)
make run        # abre a interface
make test       # executa o console com testes_treinamento.txt
make check      # testes do treino: derivadas, convergência, arquivos e, com o MNIST
                # baixado, o acerto e o tempo das épocas
make mnist      # baixa o MNIST para dados/mnist e treina exemplos/mnist.bin
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

A rede desenhada na interface é salva em:

- macOS: `~/Library/Application Support/RedesNeurais/NeuralNetWork.bin`
- Linux: `~/.local/share/RedesNeurais/NeuralNetWork.bin`
- Windows: `%APPDATA%\RedesNeurais\NeuralNetWork.bin`

Para abrir outra rede, arraste o arquivo para a janela. O título mostra o arquivo aberto, com `*`
quando há alterações não salvas, e "Salvar" grava nesse arquivo.

Ao sair, a posição, o tamanho e a maximização da janela, o tamanho da interface e o zoom e o
deslocamento da vista ficam em `estado.txt`, na mesma pasta. A janela volta como estava, centralizada se o monitor dela não estiver
mais ligado, e a vista volta se a rede aberta ao iniciar for a última que estava aberta.

A rede é gravada no formato 2.0: a quantidade de entradas, o lado da imagem que elas formam e, para
cada camada, a quantidade de neurônios, a função, os pesos e os bias, em float. A interface também
abre os formatos 1.1 e 1.2, da rede em grafo das versões anteriores, quando os nós estão em camadas:
cada ligação vai de uma camada à seguinte, os neurônios de uma camada têm a mesma função e as saídas
vêm da última. A ligação ausente vira peso 0 e a entrada de bias é somada ao bias dos neurônios
que ela alimenta. A rede que não está em camadas não abre, e a mensagem diz a ligação que pula uma
camada. A versão Windows original não abre o formato 2.0.

### Editando e executando

A rede é feita de camadas densas: cada neurônio recebe todas as saídas da camada anterior, a
primeira recebe as entradas, e os neurônios da última são as saídas. As camadas ficam em colunas,
da esquerda para a direita, cada uma com o nome em cima ("Oculta 1 · ReLU · 16"). Os botões ficam
num painel flutuante no canto da área de edição:

- "Entrada" acrescenta uma entrada, ligada a todos os neurônios da primeira camada.
- "Neurônio" acrescenta um neurônio na camada selecionada (clique num neurônio ou no nome da
  camada) ou, sem seleção, na última oculta; sem oculta cria uma.
- "Camada" cria uma camada oculta de 2 neurônios logísticos depois da selecionada ou antes da de
  saída. "Saída" acrescenta um neurônio na camada de saída.
- Os pesos e os bias novos são sorteados, menores quanto mais ligações a camada tem.
- Clique direito (ou duplo) num neurônio ou no nome da camada: menu com a função de ativação da
  camada, "Excluir neurônio" e "Excluir camada". Nas entradas o menu as transforma numa imagem de 8
  x 8, 16 x 16 ou 28 x 28 pixels, ou de volta em entradas soltas, e exclui a entrada.
- Peso e bias: o primeiro clique seleciona e o segundo (ou um clique duplo) edita no lugar. Enter
  ou clicar fora aplica, Esc cancela. Com mais de 50 ligações numa camada os rótulos de peso
  somem para não cobrir a rede: clique na linha para ver o peso dela.
- Delete exclui a entrada, o neurônio ou a camada selecionada, Esc tira a seleção.
- A roda do mouse aproxima e afasta a rede em volta do ponteiro, de 10% a 400%. Arraste o fundo
  vazio, ou qualquer ponto com o botão do meio, para mover a rede na tela. "Ajustar à janela"
  centraliza a rede com o zoom que a mostra inteira, no máximo 100%, como ao abrir um arquivo.
- Ctrl + e Ctrl - (Cmd no Mac) aumentam e diminuem o tamanho da interface, de 75% a 200%: botões,
  menus, diálogos, dicas, status e legenda. Ctrl 0 volta para 100%. A rede mantém o tamanho, que
  muda com a roda do mouse.

Redes grandes ficam mais simples na tela:

- Uma camada de mais de 32 neurônios vira um bloco com o mapa dos pesos: uma linha por neurônio e
  uma coluna por entrada, em azul os negativos e em vermelho os positivos. Depois de uma imagem
  cada neurônio vira um ladrilho com os pesos dele na forma da imagem, o que mostra os traços a que
  ele responde.
- Entradas de uma imagem de até 16 x 16 aparecem como uma grade de pixels; maiores, como um quadro
  com a imagem. Mais de 32 entradas soltas viram um bloco.
- Entre colunas com muitas ligações, ou com um bloco, as ligações viram uma faixa.

Cada neurônio tem um bias (`b`, mostrado acima dele) somado à entrada da função de ativação, que faz
o papel do limiar negativo. Ele é treinado junto com os pesos.

As funções de ativação são linear, degrau, bipolar, logística, hiperbólica, ReLU (o próprio valor
quando positivo, senão 0) e softmax. A softmax é calculada para os neurônios da camada juntos: cada
um dá `e^x` dividido pela soma dos `e^x` de todos, então os valores somam 1 e o maior é a classe
escolhida. No treino o erro da softmax na saída é a diferença para o alvo, o da entropia cruzada,
que não some quando a saída está longe do alvo como na logística.

A cor e a espessura de cada ligação mostram a influência dela na saída, do cinza ao vermelho: na
edição pelo peso, na execução pelo peso vezes o valor que passa por ela, relativos à maior da
camada.

"Executar" liga o modo de execução: clique nas entradas para alternar o valor entre 0 e 1 e nos
alvos das saídas para alterná-los. Cada neurônio mostra o valor calculado e a saída fica verde
quando é igual ao alvo.

"Treinar" guarda a combinação da tela, os valores das entradas e os alvos das saídas, como uma
amostra, e ajusta os pesos e os bias por retropropagação até as saídas de todas as amostras
guardadas ficarem iguais aos alvos (no máximo 1000 iterações), destacando os valores alterados.
Com softmax, cujas saídas somam 1, a combinação da tela sem nenhum alvo marcado não é guardada.
Assim cada linha da tabela verdade é ensinada com um clique sem desfazer as anteriores: configure as
entradas e os alvos e clique em "Treinar", linha a linha. Treinar de novo uma combinação já
guardada troca os alvos dela. O treino parte dos pesos atuais e cada peso e bias tem um passo próprio
(Rprop), que cresce enquanto o sentido do ajuste se mantém e cai à metade quando ele troca; a rede
termina com os melhores pesos encontrados, nunca pior do que começou.

O treino roda em segundo plano: o status mostra a iteração e o erro, ou a época e o acerto, e o
botão vira "Parar treino", que interrompe e fica com os pesos até ali. Com mais de 2000 amostras,
como as do MNIST, cada passo com todas elas seria lento e o treino passa a ser por épocas: em cada
uma as amostras são embaralhadas e os pesos andam um pouco a cada lote de 128 (SGD, taxa 0,1), por
10 épocas.

- "Sortear pesos" sorteia de novo os bias e os pesos, para recomeçar quando o treino não chega ao
  alvo ou a rede tem valores inválidos. As amostras continuam guardadas.
- "Zerar amostras" esquece as amostras e as imagens arrastadas para o treino, e o próximo treino
  usa só a combinação da tela. As amostras também são esquecidas ao mudar a quantidade de entradas
  ou de saídas.
- "Salvar" grava as amostras ao lado da rede, `rede.amostras.txt` para `rede.bin`, no mesmo bloco
  dos arquivos do console: `amostras` e a quantidade, depois, para cada amostra, os valores das
  entradas e `saidas` com os alvos, com ponto decimal. Sem amostras o arquivo é apagado. Ao abrir
  a rede ele é lido de volta; se não combina com a rede, ela abre sem amostras. As imagens
  arrastadas para o treino não são gravadas.
- "Parar" volta à edição.

## Exemplos

A pasta `exemplos/` tem redes prontas para as funções lógicas AND, OR e XOR, com neurônios de
função degrau:

- `and.bin`, `or.bin` e `xor.bin`: arraste para a janela da interface. Têm as 2 entradas da função
  e o bias em cada neurônio.
- `and.txt`, `or.txt` e `xor.txt`: a mesma rede no formato do console, com as 4 amostras da tabela
  verdade: `build/RedesNeurais < exemplos/xor.txt`. O console não tem bias no neurônio, então a
  primeira entrada é o bias, sempre 1, e o peso dela faz o papel do bias.
- `digitos.bin`, `digitos28.bin` e `mnist.bin`: reconhecem dígitos escritos à mão, veja abaixo.

O bias é o limiar negativo do neurônio: no AND o neurônio dispara quando `x1 + x2 - 1,5 >= 0`, no OR
quando `x1 + x2 - 0,5 >= 0`. O XOR não é linearmente separável e usa uma camada oculta: um neurônio
OR, um AND e a saída `OR - AND - 0,5`.

## Reconhecendo dígitos

`exemplos/digitos.bin` é uma versão reduzida da rede do notebook
[Reconhecendo Números usando Dataset MNIST](https://github.com/clebrw/Reconhecendo-Numeros-usando-Dataset-MNIST),
que usa 784 entradas (28 x 28 pixels), 256 neurônios ReLU e 10 softmax. Aqui são 64 entradas numa
imagem de 8 x 8 pixels, 10 neurônios ReLU e 10 softmax, que são as saídas: a de cima para o dígito
0 e a de baixo para o 9. Ao lado ficam as 100 amostras, `digitos.amostras.txt`, feitas das imagens
da pasta `exemplos/digitos`: 10 de cada dígito, com o dígito no começo do nome (`7_3.png`).

1. Arraste `digitos.bin` para a janela, clique em "Executar" e em "Treinar". As 100 amostras são
   aprendidas em poucas dezenas de iterações.
2. Arraste a imagem de um dígito para a janela. As entradas mostram a imagem reduzida e a saída de
   maior valor é o dígito reconhecido, destacado ao lado dela e no status: "Reconhecido 7", em verde
   se é o dígito do nome do arquivo, em laranja se não é e em azul se o nome não começa com um
   dígito.
3. Para ensinar uma imagem reconhecida errado, clique no alvo da saída certa, se o nome não o
   marcou, e em "Treinar". Ela entra nas amostras e "Salvar" a grava com as outras.

Treinada com as 100 imagens, a rede acerta em média 66% dos 500 dígitos de `exemplos/digitos28`,
que nunca viu (de 53% a 70% em 10 sorteios dos pesos).

### Uma entrada por pixel

`exemplos/digitos28.bin` tem uma entrada para cada pixel das imagens do MNIST: 784 entradas (28 x 28),
16 neurônios ReLU e 10 softmax, com 12.704 pesos. Ao lado ficam 500 amostras, feitas das imagens da
pasta `exemplos/digitos28`: os 50 primeiros exemplos de cada dígito do conjunto de treino do MNIST.
Os 100 dígitos de `exemplos/digitos`, do conjunto de teste, não estão entre elas e servem para
testar a rede com dígitos que ela nunca viu.

O uso é o mesmo, e o quadro das entradas mostra o dígito arrastado como uma imagem. "Treinar" leva
cerca de um décimo de segundo.

A rede treinada acerta em média 78% dos 100 dígitos de teste (de 75% a 80% em 10 sorteios dos
pesos). Com as mesmas 500 imagens reduzidas a 8 x 8, uma rede como a de `digitos.bin` acerta 75%:
a média dos pixels ignora pequenas diferenças de posição e de espessura do traço, e os pixels a
mais ajudam pouco com poucos exemplos. O que mais aumenta o acerto é a quantidade de exemplos.

### O MNIST inteiro

`exemplos/mnist.bin` é a rede do notebook: 784 entradas, 256 neurônios ReLU e 10 softmax, com
203.530 pesos e bias, treinada por 10 épocas com as 60 mil imagens de treino do MNIST. Ela acerta
97% das 10 mil imagens de teste e 97 dos 100 dígitos de `exemplos/digitos`. Arraste-a para a
janela, clique em "Executar" e arraste dígitos para reconhecer.

As imagens do MNIST não estão no repositório. `make mnist` baixa os quatro arquivos (11 MB), confere
o sha256 de cada um, descompacta em `dados/mnist`, fora do git, e treina a rede de novo, em cerca de
8 segundos.

Para treinar com o MNIST na interface, arraste a pasta `dados/mnist` (ou o arquivo
`train-images-idx3-ubyte`) para a janela com uma rede cujas entradas formem uma imagem e clique em
"Treinar". As imagens passam pela mesma redução das imagens arrastadas, no tamanho das entradas da
rede, e as de teste, ao lado, medem o acerto a cada época. Sem a redução a rede acertaria 97% das
imagens de teste do arquivo, mas só 77% das mesmas imagens arrastadas, recortadas e centralizadas
de outro jeito. Até a rede de 8 x 8 de `digitos.bin` aproveita: treinada assim por 5 épocas, passa
de 66% para 90% de acerto nos dígitos de `exemplos/digitos`.

Os pesos são float e cada época leva cerca de 0,8 segundo numa thread de um Mac com M2 Pro: as
entradas em 0, a maior parte dos pixels, não entram na conta.

### Imagens arrastadas

Uma imagem pode ser arrastada em qualquer rede cujas entradas formem uma imagem (clique direito
nas entradas para escolher o tamanho). Os pixels vão para as entradas linha a linha, de cima para
baixo. Antes a imagem é reduzida como no MNIST: o fundo claro é invertido, para o traço escuro no
papel ficar claro como no dataset, o contraste vai de 0 a 1 e o traço é recortado, centralizado e
reduzido pela média dos pixels para dentro de uma borda vazia de 1/7 do lado, 1 pixel em 8 x 8 e 4
em 28 x 28. Os valores ficam com 3 casas. O dígito no começo do nome
do arquivo marca o alvo da saída dele; sem ele todos os alvos ficam em 0. As imagens são lidas pelo
raylib: PNG sempre, os outros formatos só se o raylib foi compilado com eles.

Arrastar uma pasta de imagens com o dígito no começo do nome, ou o MNIST, faz o treino usar as
imagens dela no lugar das amostras.

`make digitos` refaz as duas redes e as amostras a partir das imagens das pastas, com a mesma redução
das imagens arrastadas para a janela.

### Imagens

As imagens são as originais, de 28 x 28 pixels, do [MNIST](https://yann.lecun.com/exdb/mnist/), de
Yann LeCun, Corinna Cortes e Christopher J. C. Burges, licenciado em CC BY-SA 3.0. As de
`exemplos/digitos28` são os 50 primeiros exemplos de cada dígito do conjunto de treino, na ordem
dele: `7_00.png` é o primeiro 7. As de `exemplos/digitos` são do conjunto de teste e são os 100
dígitos da figura do notebook, na posição deles nela:
`7_3.png` é o 7 da quarta linha. Os índices no conjunto de teste, da primeira à última linha da
figura:

| Dígito | Índices |
|---|---|
| 0 | 6131, 305, 5531, 2081, 3646, 312, 71, 3445, 2127, 6351 |
| 1 | 767, 1351, 8459, 4602, 9499, 96, 6527, 3050, 1736, 6233 |
| 2 | 7237, 6519, 4504, 6345, 9072, 4801, 9849, 8532, 6240, 1374 |
| 3 | 1384, 4552, 938, 4770, 9231, 734, 1615, 6664, 9925, 7554 |
| 4 | 308, 3405, 4377, 6102, 8329, 5175, 740, 1767, 6734, 4590 |
| 5 | 6880, 4942, 102, 812, 7974, 4463, 2670, 1370, 4219, 3890 |
| 6 | 3304, 9627, 6498, 2000, 1436, 3690, 9617, 6605, 5568, 4897 |
| 7 | 2666, 626, 4148, 2967, 7145, 5234, 6490, 6944, 7028, 9204 |
| 8 | 2246, 936, 9375, 6493, 7244, 7146, 2004, 7767, 1202, 8476 |
| 9 | 5268, 2494, 5671, 1428, 4078, 6168, 8962, 2740, 9740, 3693 |

## Linha de comando

`build/rna` treina e usa pelo terminal as redes da interface (`.bin`), com o mesmo código da
interface, sem abrir a janela:

```
build/rna treinar <rede.bin> [dados] [--epocas N] [--saida saida.bin]
build/rna reconhecer <rede.bin> <imagem>...
```

- `treinar` faz o mesmo que o botão "Treinar" e grava a rede treinada em `saida.bin` ou, sem ele,
  na própria rede. Sem `dados` usa as amostras gravadas ao lado da rede e as grava ao lado da
  saída; `dados` é uma pasta de imagens com o dígito no começo do nome ou o MNIST (veja
  [O MNIST inteiro](#o-mnist-inteiro)). Com mais de 2000 amostras treina por épocas, 10 sem `--epocas`, e mostra o acerto de
  cada uma. Termina com 0 quando as saídas chegam aos alvos ou as épocas terminam e com 2 quando
  não; rodar de novo continua o treino.
- `reconhecer` mostra, para cada imagem, a saída de maior valor e o valor dela, como ao arrastar a
  imagem para a janela. Com o dígito no começo do nome do arquivo mostra o esperado quando erra e,
  com mais de uma imagem, os acertos.

```
$ build/rna treinar exemplos/digitos28.bin --saida treinada.bin
Treinado com 500 amostras em 38 iterações: erro de 896,88 para 1,14.
Rede salva em treinada.bin.
$ build/rna reconhecer treinada.bin exemplos/digitos/*.png
0_0.png: 0 (1)
0_4.png: 6 (1), esperado 0
...
Acertos: 79 de 100 (79%)
$ build/rna treinar exemplos/mnist.bin dados/mnist --epocas 2 --saida nova.bin
Época 1 de 2: acerto de 97,8% no treino e 97,1% no teste, 0,8 s
Época 2 de 2: acerto de 97,9% no treino e 97,3% no teste, 1,6 s
Treinado por 2 épocas com 60000 amostras.
Rede salva em nova.bin.
```

## Console

`build/RedesNeurais` lê da entrada padrão uma ou mais redes num formato texto, treina cada uma com as
amostras que têm saídas desejadas, mostra a saída de cada amostra e escreve a rede treinada no mesmo
formato, que pode ser lida de novo para continuar o treino:

```
build/RedesNeurais < exemplos/xor.txt
make test       # o mesmo com testes_treinamento.txt
```

Formato, com comentários de `;` até o fim da linha:

```
(quantidade de neurônios n) (quantidade de entradas) (quantidade de ligações)
(número do neurônio) (degrau|bipolar|logistica|hiperbolica|linear|relu|softmax) [constante]
... n vezes, a logística e a hiperbólica com a constante (beta), como 1.0
(entrada|conectar|saida) ... uma linha por ligação:
  entrada (número da entrada) (peso) (neurônio de destino)
  conectar (neurônio de origem) (peso) (neurônio de destino)
  saida (neurônio), que vira uma saída da rede

amostras (quantidade de amostras)
(valores das entradas separados por espaço)
[saidas (valores desejados de cada saída)]
... uma vez por amostra
```

O console treina a rede em camadas, como a interface: cada neurônio fica uma camada depois do mais
alto que o alimenta, cada ligação precisa ir de uma camada à seguinte, os neurônios de uma camada
precisam ter a mesma função e as saídas vêm da última. A ligação ausente entre camadas vale 0 e
também é treinada; as que o treino cria aparecem no fim das ligações da rede treinada. A rede fora
desse formato é recusada com o motivo.

O console não tem bias no neurônio: o bias é uma entrada que vale sempre 1 (ou -1) e o peso dela.
A entrada que liga além da primeira camada, como a do bias no `xor.txt`, precisa ter o mesmo valor
em todas as amostras; o treino ajusta o peso dela. Exemplo, um AND com o peso do bias de um OR:

```
1 3 4
1 degrau

entrada 1 0.5 1 ; a entrada 1 vale sempre -1 e faz o papel do bias
entrada 2 1.0 1
entrada 3 1.0 1
saida   1

amostras 4
-1 0 0
saidas 0
-1 0 1
saidas 0
-1 1 0
saidas 0
-1 1 1
saidas 1
```

Saída:

```
Rede 1: treinada com 4 amostras em 4 iterações, erro de 2 para 0.
Amostra 1: saídas 0 (desejadas 0)
Amostra 2: saídas 0 (desejadas 0)
Amostra 3: saídas 0 (desejadas 0)
Amostra 4: saídas 1 (desejadas 1)

Rede treinada:
1 3 4
1 degrau

entrada  1 0.7684001 1
entrada  2 0.7315999 1
entrada  3 0.7315999 1
saida    1

amostras 4
...
```

O treino é o mesmo da interface e da linha de comando, do diretório `training/`, com os pesos em
float.

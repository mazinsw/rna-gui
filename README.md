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

A rede desenhada na interface é salva em:

- macOS: `~/Library/Application Support/RedesNeurais/NeuralNetWork.bin`
- Linux: `~/.local/share/RedesNeurais/NeuralNetWork.bin`
- Windows: `%APPDATA%\RedesNeurais\NeuralNetWork.bin`

Para abrir outra rede, arraste o arquivo para a janela. O título mostra o arquivo aberto, com `*`
quando há alterações não salvas, e "Salvar" grava nesse arquivo.

Ao sair, a posição, o tamanho e a maximização da janela, o tamanho da interface e o zoom e o
deslocamento da vista ficam em `estado.txt`, na mesma pasta. A janela volta como estava, centralizada se o monitor dela não estiver
mais ligado, e a vista volta se a rede aberta ao iniciar for a última que estava aberta.

O arquivo é gravado no formato 1.1 da versão Windows original enquanto nenhum bias é usado. Com bias
em algum neurônio, uma entrada de bias ou um neurônio ReLU ou softmax ele passa para o formato 1.2,
que a versão original não abre. A interface lê os dois.

### Editando e executando

Os botões ficam num painel flutuante no canto da área de edição. Clique em um nó e depois em outro
para conectá-los. A conexão começa com peso aleatório entre -1 e 1, e a conexão com uma saída com
peso 1.

- Segure Shift e arraste no fundo para criar uma camada: um nó a cada 120 pixels arrastados, para
  baixo ou para cima, Esc cancela. Na rede sem entradas a camada é de entradas, depois delas de
  neurônios; começando sobre uma camada existente os nós entram nela, com o tipo dela, que é como
  se acrescentam saídas pelo arraste. Ao soltar, cada nó da camada anterior é ligado a cada nó novo
  e cada nó novo a cada nó da camada seguinte. Uma camada criada entre duas remove as conexões
  diretas entre elas.
- "Neurônio", "Entrada" e "Saída" adicionam o nó abaixo do último da camada dele, já ligado às
  camadas vizinhas, e redistribuem a camada com espaçamento igual no centro vertical da rede. O
  neurônio vai na camada do neurônio selecionado, ou na última com neurônios; a entrada na primeira
  camada com entradas e a saída na última com saídas. Sem essa camada o botão cria uma: a de
  entradas antes de todas, a de saídas depois de todas e a de neurônios depois das entradas. O nó
  novo fica selecionado, então cliques seguidos continuam na mesma camada.
- Clique direito (ou duplo) no neurônio: menu com a função de ativação e "Excluir neurônio". Na
  entrada o menu escolhe o tipo, dado ou bias, e na conexão ou na saída exclui.
- Peso e bias: o primeiro clique seleciona e o segundo (ou um clique duplo) edita no lugar. Enter
  ou clicar fora aplica, Esc cancela. Com mais de 100 conexões os rótulos de peso somem para não
  cobrir a rede: clique na linha da conexão para ver o peso dela.
- Delete exclui o nó ou a conexão selecionada.
- Arrastar um nó, ou o início de uma camada nova, alinha o centro dele aos pontos da grade. A menos de 8
  pixels do x de outro nó ele entra na camada desse nó, e do y, na linha dele, com prioridade sobre
  a grade. Uma linha azul mostra o alinhamento com outro nó e uma tracejada, com a grade. Segure Alt
  (Option no Mac) para mover sem alinhar.
- "Reorganizar" refaz a posição de todos os nós em camadas: as entradas na primeira, cada neurônio
  logo depois do nó mais distante que o alimenta e as saídas na última. Dentro da camada os nós
  ficam na altura média dos que os alimentam, para as conexões cruzarem menos. As entradas de uma
  imagem (veja [Reconhecendo dígitos](#reconhecendo-dígitos)) ficam em quadrado, na ordem dos
  pixels. Depois centraliza a rede com o zoom que a mostra inteira, no máximo 100%.
- A roda do mouse aproxima e afasta a rede em volta do ponteiro, de 25% a 400%. Arraste o fundo
  vazio sem Shift, ou qualquer ponto com o botão do meio, para mover a rede na tela. Abrir um
  arquivo arrastado volta a vista para 100%, ou para o zoom que mostra a rede inteira se ela não
  cabe.
- Ctrl + e Ctrl - (Cmd no Mac) aumentam e diminuem o tamanho da interface, de 75% a 200%: botões,
  menus, diálogos, dicas, status e legenda. Ctrl 0 volta para 100%. A rede mantém o tamanho, que
  muda com a roda do mouse.

Cada neurônio tem um bias (`b`, mostrado acima dele) somado à entrada da função de ativação, que faz
o papel do limiar negativo. Ele começa aleatório e é treinado junto com os pesos. Para modelar o
bias como entrada, use uma entrada do tipo bias: ela vale sempre 1 e o peso da conexão é o bias.

As funções de ativação são linear, degrau, bipolar, logística, hiperbólica, ReLU (o próprio valor
quando positivo, senão 0) e softmax. A softmax é calculada junto para todos os neurônios softmax da
rede: cada um dá `e^x` dividido pela soma dos `e^x` de todos, então os valores somam 1 e o maior é
a classe escolhida. No treino o erro do neurônio softmax é a diferença para o alvo, o da entropia
cruzada, que não some quando a saída está longe do alvo como na logística.

A cor e a espessura de cada conexão mostram a influência dela na saída, do cinza ao vermelho: na
edição pelo peso, na execução pelo peso vezes o valor que passa por ela, relativos à maior da rede.

"Executar" liga o modo de execução: clique nas entradas para alternar o valor entre 0 e 1 e nas
saídas para alternar o alvo. Cada neurônio mostra o valor calculado e a saída fica verde quando é
igual ao alvo.

"Treinar" guarda a combinação da tela, os valores das entradas e os alvos das saídas, como uma
amostra, e ajusta os pesos e os bias por retropropagação até as saídas de todas as amostras
guardadas ficarem iguais aos alvos (no máximo 1000 iterações), destacando os valores alterados.
Com softmax, cujas saídas somam 1, a combinação da tela sem nenhum alvo marcado não é guardada.
Assim cada linha da tabela verdade é ensinada com um clique sem desfazer as anteriores: configure as
entradas e os alvos e clique em "Treinar", linha a linha. Treinar de novo uma combinação já
guardada troca os alvos dela. O treino parte dos pesos atuais e cada peso e bias tem um passo próprio
(Rprop), que cresce enquanto o sentido do ajuste se mantém e cai à metade quando ele troca; a rede
termina com os melhores pesos encontrados, nunca pior do que começou. A conexão que é a única de uma
saída repassa o valor do nó e o peso dela fica fixo; numa saída ligada a vários nós, que soma os
valores deles, os pesos também são treinados.

- "Sortear pesos" sorteia de novo, entre -1 e 1, os bias e os pesos treinados, para recomeçar
  quando o treino não chega ao alvo ou a rede tem valores inválidos. As amostras continuam guardadas.
- "Zerar amostras" esquece as amostras, e o próximo treino usa só a combinação da tela. Elas também
  são esquecidas ao adicionar ou excluir uma entrada ou saída e ao trocar o tipo de uma entrada.
- "Salvar" grava as amostras ao lado da rede, `rede.amostras.txt` para `rede.bin`, no mesmo bloco
  dos arquivos do console: `amostras` e a quantidade, depois, para cada amostra, os valores das
  entradas na ordem dos nós (a entrada de bias vale 1) e `saidas` com os alvos, com ponto decimal.
  Sem amostras o arquivo é apagado. Ao abrir a rede ele é lido de volta; se não combina com a rede,
  ela abre sem amostras.
- "Parar" volta à edição.

## Exemplos

A pasta `exemplos/` tem redes prontas para as funções lógicas AND, OR e XOR, com neurônios de
função degrau:

- `and.bin`, `or.bin` e `xor.bin`: arraste para a janela da interface. Têm as 2 entradas da função
  e o bias em cada neurônio.
- `and.txt`, `or.txt` e `xor.txt`: a mesma rede no formato do console, com as 4 amostras da tabela
  verdade: `build/RedesNeurais < exemplos/xor.txt`. O console não tem bias no neurônio, então a
  primeira entrada é o bias, sempre 1, e o peso dela faz o papel do bias.
- `digitos.bin` e `digitos28.bin`: reconhecem dígitos escritos à mão, veja abaixo.

O bias é o limiar negativo do neurônio: no AND o neurônio dispara quando `x1 + x2 - 1,5 >= 0`, no OR
quando `x1 + x2 - 0,5 >= 0`. O XOR não é linearmente separável e usa uma camada oculta: um neurônio
OR, um AND e a saída `OR - AND - 0,5`.

## Reconhecendo dígitos

`exemplos/digitos.bin` é uma versão reduzida da rede do notebook
[Reconhecendo Números usando Dataset MNIST](https://github.com/clebrw/Reconhecendo-Numeros-usando-Dataset-MNIST),
que usa 784 entradas (28 x 28 pixels), 256 neurônios ReLU e 10 softmax. Aqui são 64 entradas em
quadrado (8 x 8 pixels), 10 neurônios ReLU, 10 softmax e 10 saídas, a de cima para o dígito 0 e a de
baixo para o 9. Ao lado ficam as 100 amostras, `digitos.amostras.txt`, feitas das imagens da pasta
`exemplos/digitos`: 10 de cada dígito, com o dígito no começo do nome (`7_3.png`).

1. Arraste `digitos.bin` para a janela, clique em "Executar" e em "Treinar". As 100 amostras são
   aprendidas em poucas dezenas de iterações.
2. Arraste a imagem de um dígito para a janela. As entradas mostram a imagem reduzida e a saída de
   maior valor é o dígito reconhecido, destacado ao lado dela e no status: "Reconhecido 7", em verde
   se é o dígito do nome do arquivo, em laranja se não é e em azul se o nome não começa com um
   dígito.
3. Para ensinar uma imagem reconhecida errado, clique no alvo da saída certa, se o nome não o
   marcou, e em "Treinar". Ela entra nas amostras e "Salvar" a grava com as outras.

### Uma entrada por pixel

`exemplos/digitos28.bin` tem uma entrada para cada pixel das imagens do MNIST: 784 entradas (28 x 28),
16 neurônios ReLU, 10 softmax e 10 saídas, com 12.714 conexões. Ao lado ficam 500 amostras, feitas
das imagens da pasta `exemplos/digitos28`: os 50 primeiros exemplos de cada dígito do conjunto de
treino do MNIST. Os 100 dígitos de `exemplos/digitos`, do conjunto de teste, não estão entre elas e
servem para testar a rede com dígitos que ela nunca viu.

O uso é o mesmo, e a grade de entradas mostra o dígito arrastado como uma imagem. "Treinar" leva de 1 a
3 segundos, com a janela parada enquanto isso.

A rede treinada acerta cerca de 65% dos 100 dígitos de teste (de 62% a 72%, conforme os pesos
sorteados). Mais pixels não querem dizer mais acertos: com as mesmas 500 imagens reduzidas a 8 x 8 uma
rede como a de `digitos.bin` acerta cerca de 75%, porque a média dos pixels ignora pequenas diferenças
de posição e de espessura do traço, e com poucos exemplos as 784 entradas deixam a rede decorar
detalhes de cada imagem. O que mais aumenta o acerto é a quantidade de exemplos.

### Imagens arrastadas

Uma imagem pode ser arrastada em qualquer rede cujas entradas de dado formem um quadrado (64 para 8 x
8). Os pixels vão para as entradas na ordem de leitura delas, de cima para baixo e da esquerda para a
direita. Antes a imagem é reduzida como no MNIST: o fundo claro é invertido, para o traço escuro no
papel ficar claro como no dataset, o contraste vai de 0 a 1 e o traço é recortado, centralizado e
reduzido pela média dos pixels para dentro de uma borda vazia de 1/7 do lado, 1 pixel em 8 x 8 e 4
em 28 x 28. Os valores ficam com 3 casas. O dígito no começo do nome
do arquivo marca o alvo da saída dele; sem ele todos os alvos ficam em 0. As imagens são lidas pelo
raylib: PNG sempre, os outros formatos só se o raylib foi compilado com eles.

Com 10 exemplos de cada dígito a rede acerta todos os que aprendeu, mas só cerca de 6 em 10 dígitos
que nunca viu. O notebook chega a 95% porque treina com as 60 mil imagens do MNIST.

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

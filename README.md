# Redes Neurais

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

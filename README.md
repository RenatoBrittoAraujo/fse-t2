# Trabalho 2 - Air Fryer

Baseado no código do projeto 1.

O projeto, infelizmente, não foi concluído à tempo.

O arquivo mais relevante do app é o `main/src/main.c`.

Variáveis relevantes de ambiente (como os períodos de tempo para leituras de botões) em `.env`.

*Como o código está em C, a instrução dada em aula é que fosse utilizado o bme280. Ou seja, ele foi testado apenas na placa 3* 

**Shared**: pasta contendo código feito pra ser reutilizado.

**Main**: pasta contendo código principal do app, nenhum código considerado apto para ser reutilizado.

A solução está toda concentrada em um único arquivo, porém o plano inicial era dividir a implentação em módulos coerentes. Isso não aconteceu por falta de tempo. 

O sistema que está na `main.c` foi inicialmente desenhado para usar uma fila de mensagens por cada meio de comunicação (UART MODBUS, I2C e GPIO), mas essa ideia foi abandonada por falta de tempo. 

Cada execução do programa cria um log novo no `logs/logs_{timestamp}.csv`.

## Como instalar, fazer ssh, compilar e rodar

1. Altera o arquivo `Makefile` com a configuração de SSH da máquina alvo.
```Makefile
IP = <ip da maquina>
USER = <username ssh>
PASS = <senha ssh>
PORT = <porta da maquina>
PRJ_NAME = <pasta do projeto DENTRO da pasta root após ssh na máquina>
SERVER_PROJECT_PATH = /home/$(USER)/$(PRJ_NAME) # altere pra pasta onde o código está na MÁQUINA ALVO
PATH_MY_OS = /home/renato/Desktop/fse/$(PRJ_NAME) # altere pra pasta onde o código está na SUA MÁQUINA
...
```

2. Rode `make install`, isso colocará o projeto na máquino dentro da pasta `$(SERVER_PROJECT_PATH)`

3. Rode `make ssh` para realizar ssh na máquina.

4. Dentro da máquina agora, entre na pasta do projeto chamada de `$(PRJ_NAME)`.

5. Execute `make all` para compilar e rodar o projeto inteiro.

Esse processo foi feito assim porque é muito fácil pra modificar código, instalar, compilar e rodar rapidamente, embora a linguagem C complique tudo um pouco.

### Excutar em dev

*projeto não roda em dev, dá muito trabalho pra configurar dependencias, compilar e mockar*

## Sistema de arquivos

Supondo que as condições de pressão e temperatura fossem ideais, o sistema de arquivos seria assim:

| padrão do arquivo                             | significado                                                                                                             |
| --------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| `shared`                                      | código compartilhado no projeto.                                                                                        |
| `(shared,*)/inc/`                             | arquivos include `.h`.                                                                                                  |
| `(shared,*)/src/`                             | arquivos `.c`.                                                                                                          |
| `(shared,*)/inc/`                             | arquivos `.obj`.                                                                                                        |
| `(shared,*)/test_inc/`                        | arquivos include `.h` de testes.                                                                                        |
| `(shared,*)/test_src/`                        | arquivos `.c` de testes.                                                                                                |
| `(shared,*)/test_obj/`                        | arquivos `.obj` de testes.                                                                                              |
| `(shared,*)/src/Makefile`                     | makefile para orquestrar compilação daquele projeto. Deve implementar comandos `dev`, `dev_test` e `prod`.              |
| `(shared,*)/src/Makefile_(dev,dev_test,prod)` | makefile específico para compilar o environment em seu nome.                                                            |
| `shared/inc/proto.h`                          | inspirado no golang, serve para compartilhar structs comuns entre os apps, por exemplo as mensagens que usam (binário). |


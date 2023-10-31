# {project_name}

## Como rodar

### Excutar em dev

`sudo docker-compose up`

### Executar em prod

## Proximos passos

## Sistema de arquivos

| padrão do arquivo                             | significado                                                                                                             |
| --------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------- |
| `shared`                                      | código compartilhadas no projeto.                                                                                       |
| `(shared,*)/inc/`                             | arquivos include `.h`.                                                                                                  |
| `(shared,*)/src/`                             | arquivos `.c`.                                                                                                          |
| `(shared,*)/inc/`                             | arquivos `.obj`.                                                                                                        |
| `(shared,*)/test_inc/`                        | arquivos include `.h` de testes.                                                                                        |
| `(shared,*)/test_src/`                        | arquivos `.c` de testes.                                                                                                |
| `(shared,*)/test_obj/`                        | arquivos `.obj` de testes.                                                                                              |
| `(shared,*)/src/Makefile`                     | makefile para orquestrar compilação daquele projeto. Deve implementar comandos `dev`, `dev_test` e `prod`.              |
| `(shared,*)/src/Makefile_(dev,dev_test,prod)` | makefile específico para compilar o environment em seu nome.                                                            |
| `shared/inc/proto.h`                          | inspirado no golang, serve para compartilhar structs comuns entre os apps, por exemplo as mensagens que usam (binário). |


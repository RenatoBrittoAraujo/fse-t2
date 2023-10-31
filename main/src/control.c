// #include <stdbool.h>
// #include <stdlib.h>
// #include <string.h>
// #include <stdio.h>
// #include <signal.h>
// #include <stdint.h>

// #include <shared/inc/proto.h>
// #include <shared/inc/shared_util.h>
// #include <shared/inc/time.h>
// #include <shared/inc/threads.h>
// #include <shared/inc/tcp_ip.h>

// #define SEGUNDO 1000 // milissegundos
// #define TEMPO_MAXIMO_CANCELA_ABERTA 10 * SEGUNDO
// #define TEMPO_MAXIMO_DE_DESCONEXAO 10 * SEGUNDO
// #define PERIODO_MINIMO_EXEC 50

// void set_entradas_inicial()
// {
//     log_print("[MAIN] set_entradas_inicial()\n", LEVEL_DEBUG);
// }

// bool is_nova_conexao(EstadoEstacionamento *e)
// {
//     log_print("[MAIN] is_nova_conexao()\n", LEVEL_DEBUG);
//     if (e == NULL)
//     {
//         return true;
//     }
//     if (e->tempo_ultima_execucao < get_time_mcs() - TEMPO_MAXIMO_DE_DESCONEXAO)
//     {
//         return true;
//     }
//     return false;
// }

// void envia_mensagem_pra_dependente(ThreadState *ts, void *args)
// {
//     log_print("[MAIN THREAD] envia_mensagem_pra_dependente()\n", LEVEL_DEBUG);
//     EstadoEstacionamento *e = (EstadoEstacionamento *)args;
//     log_print("[MAIN THREAD] envia msg dep 1\n", LEVEL_DEBUG);

//     MensagemIn *req = monta_request(e);
//     log_print("[MAIN THREAD] envia msg dep 6\n", LEVEL_DEBUG);
//     char *req_str = tranformar_request_em_string(req);
//     log_print("[MAIN THREAD] envia msg dep 2\n", LEVEL_DEBUG);

//     printf("t_id = %lu t_dep1_id = %lu t_dep2_id = %lu\n", ts->thread_id, e->t_dep_1->thread_id, e->t_dep_2->thread_id);
//     fflush(stdout);

//     log_print("[MAIN THREAD] envia msg dep 5\n", LEVEL_DEBUG);

//     char *ip;
//     int porta;

//     log_print("[MAIN THREAD] envia msg dep 3\n", LEVEL_DEBUG);
//     if (ts->thread_id == e->t_dep_1->thread_id)
//     {
//         log_print("[MAIN THREAD] msg pro andar 1\n", LEVEL_DEBUG);
//         ip = e->andares[0]->endereco->ip;
//         porta = e->andares[0]->endereco->porta;
//     }
//     else if (ts->thread_id == e->t_dep_2->thread_id)
//     {
//         log_print("[MAIN THREAD] msg pro andar 2\n", LEVEL_DEBUG);
//         ip = e->andares[1]->endereco->ip;
//         porta = e->andares[1]->endereco->porta;
//     }
//     else
//     {
//         log_print("[MAIN THREAD] ip e porta da mensagem não encontrado\n", LEVEL_ERROR);
//         exit(1);
//     }

//     printf("[MAIN THREAD] enviando para %s:%d\n", ip, porta);
//     fflush(stdout);

//     log_print("[MAIN THREAD] envia msg deeeep 4\n", LEVEL_DEBUG);
//     char *res_str = message_tcp_ip_port(req_str, ip, porta);
//     log_print("[MAIN THREAD] envia msg deeeep 7\n", LEVEL_DEBUG);
//     MensagemOut *res = parse_string_resposta(res_str);

//     log_print("[MAIN THREAD] envia msg deeep 3\n", LEVEL_DEBUG);
//     if (ts->thread_id == e->t_dep_1->thread_id)
//     {
//         log_print("[MAIN THREAD] resposta do andar 1\n", LEVEL_DEBUG);
//         memcpy((void *)e->t_dep_1->memory->out, (void *)res->e, sizeof(EstadoEstacionamento));
//     }
//     else if (ts->thread_id == e->t_dep_2->thread_id)
//     {
//         log_print("[MAIN THREAD] resposta do andar 2\n", LEVEL_DEBUG);
//         memcpy((void *)e->t_dep_2->memory->out, (void *)res->e, sizeof(EstadoEstacionamento));
//     }
//     else
//     {
//         log_print("[ERROR] [MAIN THREAD] endereco para esrever resposta não encontrado\n", LEVEL_ERROR);
//         exit(1);
//     }
// }

// ThreadState *criar_thread_comunicar_dependente(EstadoEstacionamento *e)
// {
//     log_print("[MAIN] criar_thread_comunicar_dependente()\n", LEVEL_DEBUG);

//     ThreadState *t = create_thread_state();
//     t->routine = envia_mensagem_pra_dependente;
//     t->memory->in = copiar_estado(e);
//     t->memory->out = copiar_estado(e);
//     t->args = copiar_estado(e);

//     log_print("[MAIN] thread criada\n", LEVEL_DEBUG);

//     return t;
// }

// void set_thread_args(ThreadState *t, EstadoEstacionamento *e)
// {
//     log_print("[MAIN] set_thread_args()\n", LEVEL_DEBUG);
//     printf("e->tdep_1 e->tdpe2%p %p\n", e->t_dep_1, e->t_dep_2);
//     fflush(stdout);
//     t->args = (void *)malloc(sizeof(EstadoEstacionamento));
//     log_print("[MAIN] set_thread_args 2\n", LEVEL_DEBUG);
//     memcpy(t->args, e, sizeof(EstadoEstacionamento));
//     log_print("[MAIN] set_thread_args ret\n", LEVEL_DEBUG);
// }

// // ================== MODIFICAR ESTADO ==================

// int is_todas_as_vagas_andar_ocupadas(EstadoAndar *a)
// {
//     log_print("[MAIN] is_todas_as_vagas_andar_ocupadas()\n", LEVEL_DEBUG);
//     int nv = 0;
//     for (int i = 0; i < a->num_vagas; i++)
//     {
//         nv += a->vagas[i];
//     }
//     return nv == a->num_vagas;
// }

// int is_todas_as_vagas_ocupadas(EstadoEstacionamento *e)
// {
//     log_print("[MAIN] is_todas_as_vagas_ocupadas()\n", LEVEL_DEBUG);
//     printf("e->num_andares = %d\n", e->num_andares);
//     fflush(stdout);
//     int nv = 0;
//     for (int i = 0; i < e->num_andares; i++)
//     {
//         nv += is_todas_as_vagas_andar_ocupadas(e->andares[i]);
//     }
//     return nv == e->num_andares;
// }

// int is_sensor_de_presenca_na_saida_ocupado(EstadoEstacionamento *e)
// {
//     log_print("[MAIN] is_sensor_de_presenca_na_saida_ocupado()\n", LEVEL_DEBUG);
//     e->entrada->motor_saida_ligado = 0;
// }

// int get_estado_motor_cancela(time_mcs sensor_de_presenca, time_mcs sensor_de_passagem)
// {
//     log_print("[MAIN] get_estado_motor_cancela()\n", LEVEL_DEBUG);

//     // se o sensor de presenca capturou algo ou o sensor de passagem está capturando algo
//     if (is_newer(TEMPO_MAXIMO_CANCELA_ABERTA + sensor_de_presenca) || is_newer(TEMPO_MAXIMO_CANCELA_ABERTA + sensor_de_passagem))
//     {
//         // cancela levanta
//         return 1;
//     }
//     // cancela abaixa
//     return 0;
// }

// int decidir_estado_motor_cancela_saida(EstadoEstacionamento *e)
// {
//     log_print("[MAIN] decidir_estado_motor_cancela_saida()\n", LEVEL_DEBUG);

//     if (is_newer(TEMPO_MAXIMO_CANCELA_ABERTA + e->entrada->sensor_de_presenca_saida))
//     {
//         e->entrada->motor_saida_ligado = 1;
//     }
//     else
//     {
//         if (!is_newer(TEMPO_MAXIMO_CANCELA_ABERTA + e->entrada->sensor_de_passagem_saida))
//         {
//             e->entrada->motor_saida_ligado = 0;
//         }
//     }
// }

// // ================== FIM MODIFICAR ESTADO ==================

// EstadoEstacionamento *controla(EstadoEstacionamento *e)
// {
//     log_print("[MAIN] controla()\n", LEVEL_DEBUG);

//     if (is_nova_conexao(e))
//     {
//         e = inicializar_estado("MAIN");
//         set_entradas_inicial();

//         e->t_dep_1 = criar_thread_comunicar_dependente(e);
//         e->t_dep_2 = criar_thread_comunicar_dependente(e);
//     }
//     e->tempo_ultima_execucao = get_time_mcs();

//     // ========= CANCELAS

//     e->entrada->motor_entrada_ligado = get_estado_motor_cancela(
//         e->entrada->sensor_de_presenca_entrada,
//         e->entrada->sensor_de_passagem_entrada);

//     e->entrada->motor_saida_ligado = get_estado_motor_cancela(
//         e->entrada->sensor_de_presenca_saida,
//         e->entrada->sensor_de_passagem_saida);

//     // ========= VAGAS

//     int todos_andares_lotados = 1;
//     for (int i = 1; i <= e->num_andares; i++)
//     {
//         char buff[1000];
//         sprintf(buff, "[MAIN CONTROLA] Analisando vagas do andar %d\n", i);
//         log_print(buff, LEVEL_DEBUG);

//         EstadoAndar *a = e->andares[i - 1];
//         int andar_lotado = is_todas_as_vagas_andar_ocupadas(a);
//         todos_andares_lotados *= andar_lotado;
//         a->andar_lotado = andar_lotado;
//     }
//     e->estacionamento_lotado = is_todas_as_vagas_ocupadas(e);

//     log_print("[MAIN CONTROLA] enviando mensagem pra dependente\n", LEVEL_DEBUG);

//     set_thread_args(e->t_dep_1, e);
//     start_thread(e->t_dep_1);
//     set_thread_args(e->t_dep_2, e);
//     start_thread(e->t_dep_2);

//     // ========= FIM

//     time_mcs esperar_proximo = get_time_mcs() - e->tempo_ultima_execucao - PERIODO_MINIMO_EXEC;

//     if (esperar_proximo > 0)
//     {
//         log_print("[MAIN CONTROLA] wait()\n", LEVEL_DEBUG);
//         wait(esperar_proximo);
//     }

//     log_print("[MAIN CONTROLA] fim\n", LEVEL_DEBUG);

//     return e;
// }

// #define PERIODO_MINIMO_ENTRE_EXECUCOES 100 // milissegundos

// // ========== COMEÇA CONFIG DE PINOS =============
// #define RASP_ESTACIONAMENTO_1_3 1
// #ifdef RASP_ESTACIONAMENTO_1_3

// #define OUT_A1_ENDERECO_01 22
// #define OUT_A1_ENDERECO_02 26
// #define OUT_A1_ENDERECO_03 19
// #define INP_A1_SENSOR_VAGA 18
// #define OUT_A1_SINAL_DE_LOTADO_FECHADO 27
// #define INP_A1_SENSOR_ABERTURA_CANCELA_ENTRADA 23
// #define INP_A1_SENSOR_FECHAMENTO_CANCELA_ENTRADA 24
// #define OUT_A1_MOTOR_CANCELA_ENTRADA 10
// #define INP_A1_SENSOR_ABERTURA_CANCELA_SAIDA 25
// #define INP_A1_SENSOR_FECHAMENTO_CANCELA_SAIDA 12
// #define OUT_A1_MOTOR_CANCELA_SAIDA 17

// #define OUT_A2_ENDERECO_01 13
// #define OUT_A2_ENDERECO_02 6
// #define OUT_A2_ENDERECO_03 5
// #define INP_A2_SENSOR_VAGA 20
// #define OUT_A2_SINAL_DE_LOTADO_FECHADO 8
// #define INP_A2_SENSOR_ABERTURA_CANCELA 16
// #define INP_A2_SENSOR_FECHAMENTO_CANCELA 21

// #define INP_SENSOR_DE_PASSAGEM_1 16
// #define INP_SENSOR_DE_PASSAGEM_2 21

// #else

// // NOTE QUE ESSES AQUI SÃO OS PINOS ERRADOS DA PLACA 4
// // [TODO] define os pinos para a outra rasp de estacionamento
// #define OUT_A1_ENDERECO_01 22
// #define OUT_A1_ENDERECO_02 26
// #define OUT_A1_ENDERECO_03 19
// #define INP_A1_SENSOR_VAGA 18
// #define OUT_A1_SINAL_DE_LOTADO_FECHADO 27
// #define INP_A1_SENSOR_ABERTURA_CANCELA_ENTRADA 23
// #define INP_A1_SENSOR_FECHAMENTO_CANCELA_ENTRADA 24
// #define OUT_A1_MOTOR_CANCELA_ENTRADA 10
// #define INP_A1_SENSOR_ABERTURA_CANCELA_SAIDA 25
// #define INP_A1_SENSOR_FECHAMENTO_CANCELA_SAIDA 12
// #define OUT_A1_MOTOR_CANCELA_SAIDA 17

// #define OUT_A2_ENDERECO_01 13
// #define OUT_A2_ENDERECO_02 6
// #define OUT_A2_ENDERECO_03 5
// #define INP_A2_SENSOR_VAGA 20
// #define OUT_A2_SINAL_DE_LOTADO_FECHADO 8
// #define INP_A2_SENSOR_ABERTURA_CANCELA 16
// #define INP_A2_SENSOR_FECHAMENTO_CANCELA 21

// #define INP_SENSOR_DE_PASSAGEM_1 16
// #define INP_SENSOR_DE_PASSAGEM_2 21

// #endif
// // ========== TERMINA CONFIG DE PINOS =============

// // ========== COMEÇA CONFIG DE AMBIENTE =============
// #ifdef TEST_MODE

// // CONFIG DE TESTE
// // #include <test_time_util.h>

// #define BCM2835_GPIO_FSEL_OUTP 1
// #define BCM2835_GPIO_FSEL_INPT 0

// void bcm2835_gpio_fsel(uint8_t pin, uint8_t mode)
// {
//     log_print("[BCM MOCK] bcm2835_gpio_fsel()\n", LEVEL_DEBUG);
// }
// int bcm2835_close(void) { log_print("[BCM MOCK] bcm2835_close()\n", LEVEL_DEBUG); }
// int bcm2835_init(void)
// {
//     log_print("[BCM MOCK] bcm2835_init()\n", LEVEL_DEBUG);
//     return 1;
// }
// int bcm2835_gpio_lev(uint8_t pin) { log_print("[BCM MOCK] bcm2835_gpio_lev()\n", LEVEL_DEBUG); }
// void bcm2835_gpio_write(uint8_t pin, uint8_t on) { log_print("[BCM MOCK] bcm2835_gpio_write()\n", LEVEL_DEBUG); }

// #else

// // CONFIG DE PROD

// #include <bcm2835.h>
// #include <shared/inc/time.h>

// #endif
// // ========== TERMINA CONFIG DE AMBIENTE =============

// #define TEMPO_ESPERA_DETECTOR_VAGA 10

// int id_andar;

// void configura_pinos()
// {
//     log_print("[DEP] configura_pinos\n", LEVEL_DEBUG);
//     bcm2835_gpio_fsel(OUT_A1_ENDERECO_01, BCM2835_GPIO_FSEL_OUTP);
//     bcm2835_gpio_fsel(OUT_A1_ENDERECO_02, BCM2835_GPIO_FSEL_OUTP);
//     bcm2835_gpio_fsel(OUT_A1_ENDERECO_03, BCM2835_GPIO_FSEL_OUTP);
//     bcm2835_gpio_fsel(INP_A1_SENSOR_VAGA, BCM2835_GPIO_FSEL_INPT);
//     bcm2835_gpio_fsel(OUT_A1_SINAL_DE_LOTADO_FECHADO, BCM2835_GPIO_FSEL_OUTP);
//     bcm2835_gpio_fsel(INP_A1_SENSOR_ABERTURA_CANCELA_ENTRADA, BCM2835_GPIO_FSEL_INPT);
//     bcm2835_gpio_fsel(INP_A1_SENSOR_FECHAMENTO_CANCELA_ENTRADA, BCM2835_GPIO_FSEL_INPT);
//     bcm2835_gpio_fsel(OUT_A1_MOTOR_CANCELA_ENTRADA, BCM2835_GPIO_FSEL_OUTP);
//     bcm2835_gpio_fsel(INP_A1_SENSOR_ABERTURA_CANCELA_SAIDA, BCM2835_GPIO_FSEL_INPT);
//     bcm2835_gpio_fsel(INP_A1_SENSOR_FECHAMENTO_CANCELA_SAIDA, BCM2835_GPIO_FSEL_INPT);
//     bcm2835_gpio_fsel(OUT_A1_MOTOR_CANCELA_SAIDA, BCM2835_GPIO_FSEL_OUTP);
//     bcm2835_gpio_fsel(OUT_A2_ENDERECO_01, BCM2835_GPIO_FSEL_OUTP);
//     bcm2835_gpio_fsel(OUT_A2_ENDERECO_02, BCM2835_GPIO_FSEL_OUTP);
//     bcm2835_gpio_fsel(OUT_A2_ENDERECO_03, BCM2835_GPIO_FSEL_OUTP);
//     bcm2835_gpio_fsel(INP_A2_SENSOR_VAGA, BCM2835_GPIO_FSEL_INPT);
//     bcm2835_gpio_fsel(OUT_A2_SINAL_DE_LOTADO_FECHADO, BCM2835_GPIO_FSEL_OUTP);
//     bcm2835_gpio_fsel(INP_A2_SENSOR_ABERTURA_CANCELA, BCM2835_GPIO_FSEL_INPT);
//     bcm2835_gpio_fsel(INP_A2_SENSOR_FECHAMENTO_CANCELA, BCM2835_GPIO_FSEL_INPT);
// }

// void handle_interruption(int sinal)
// {
//     log_print("[DEP] handle_interruption\n", LEVEL_DEBUG);
//     bcm2835_close();
//     printf("interruption! %d\n", sinal);
//     exit(0);
// }

// EstadoEstacionamento *request = NULL;
// char *resposta = NULL;
// char *get_resposta(void *c_request)
// {
//     char *n_request = NULL;
//     log_print("[DEP] get_resposta\n", LEVEL_DEBUG);
//     if (c_request != NULL)
//     {
//         log_print("[DEP] get_resposta memcpy\n", LEVEL_DEBUG);
//         n_request = (char *)malloc(sizeof(EstadoEstacionamento));
//         memcpy(n_request, c_request, sizeof(EstadoEstacionamento));
//         log_print("[DEP] request setada\n", LEVEL_DEBUG);
//     }
//     if (request != NULL)
//     {
//         log_print("[DEP] limpa antiga request\n", LEVEL_DEBUG);
//         free(request);
//     }
//     log_print("[DEP] seta nova request\n", LEVEL_DEBUG);
//     request = c_request;
//     log_print("[DEP] request feita\n", LEVEL_DEBUG);

//     log_print("[DEP] get_resposta\n", LEVEL_DEBUG);
//     return resposta;
// }

// void set_resposta(EstadoEstacionamento *e)
// {
//     log_print("[DEP] set_resposta\n", LEVEL_DEBUG);
//     if (resposta == NULL)
//     {
//         log_print("[DEP] aloca resposta\n", LEVEL_DEBUG);
//         resposta = (char *)malloc(sizeof(EstadoEstacionamento));
//     }
//     if (e != NULL)
//     {

//         log_print("[DEP] memcpy resposta\n", LEVEL_DEBUG);
//         memcpy(resposta, e, sizeof(EstadoEstacionamento));
//     }
//     log_print("[DEP] fim set_resposta\n", LEVEL_DEBUG);
// }

// void escuta_main(ThreadState *ts, void *args)
// {
//     log_print("[DEP] escuta_main\n", LEVEL_DEBUG);
//     EstadoEstacionamento *e = (EstadoEstacionamento *)args;
//     printf("ABRINDO IP:PORTA %s:%d\n", e->endereco->ip, e->endereco->porta);
//     fflush(stdout);
//     log_print("[DEP] deu parse do estacionamento\n", LEVEL_DEBUG);
//     listen_tcp_ip_port(get_resposta, e->endereco->ip, e->endereco->porta);
//     log_print("[DEP] escutando porta!\n", LEVEL_DEBUG);
// }

// void abrir_thread_server_dependente(EstadoEstacionamento *e)
// {
//     log_print("[DEP] abrir_thread_server_dependente\n", LEVEL_DEBUG);

//     ThreadState *t = create_thread_state();
//     e->t_main = t;
//     t->routine = escuta_main;
//     t->args = e;
//     start_thread(t);
// }

// int atualiza_tempo(time_mcs *attr, int atualizar)
// {
//     log_print("[DEP] atualiza_tempo\n", LEVEL_DEBUG);
//     if (atualizar)
//         *attr = get_time_mcs();
//     return atualizar;
// }

// EstadoEstacionamento *le_aplica_estado(EstadoEstacionamento *e, int id_andar)
// {
//     log_print("[DEP] le_aplica_estado\n", LEVEL_DEBUG);
//     if (request != NULL)
//     {
//         log_print("[DEP] tem request1!!\n", LEVEL_DEBUG);
//         memcpy(e, request, sizeof(EstadoEstacionamento));
//         log_print("[DEP] tem request2!!\n", LEVEL_DEBUG);
//         free(request);
//         log_print("[DEP] tem request3!!\n", LEVEL_DEBUG);
//         request = NULL;
//         log_print("[DEP] tem request4!!\n", LEVEL_DEBUG);
//     }
//     log_print("[DEP] le_aplica_estado iniciando\n", LEVEL_DEBUG);
//     printf("e->num_andares = %d\n", e->num_andares);

//     if (e->num_andares < id_andar - 1)
//     {
//         log_print("[ERROR] [DEP] le_aplica_estado numero da andar < id do andar atual\n", LEVEL_ERROR);
//     }

//     EstadoAndar *a = e->andares[id_andar - 1];
//     int andar_entrada = read_env_int_index("ID_ANDAR_DA_ENTRADA", -1);

//     log_print("[DEP] le_aplica_estado iniciando\n", LEVEL_DEBUG);
//     uint8_t end1, end2, end3;
//     if (id_andar == 1 || 1)
//     {
//         end1 = OUT_A1_ENDERECO_01;
//         end2 = OUT_A1_ENDERECO_02;
//         end3 = OUT_A1_ENDERECO_03;
//     }
//     else
//     {
//         end1 = OUT_A2_ENDERECO_03;
//         end2 = OUT_A2_ENDERECO_02;
//         end3 = OUT_A2_ENDERECO_01;
//     }

//     log_print("[DEP] le_aplica_estado verificando vagas\n", LEVEL_DEBUG);
//     // Itera por todas as vagas
//     for (int i = 0; i < a->num_vagas; i++)
//     {
//         int vend1 = LOW, vend2 = LOW, vend3 = LOW;
//         if ((1 << 0) & i)
//             vend1 = HIGH;
//         if ((1 << 1) & i)
//             vend2 = HIGH;
//         if ((1 << 2) & i)
//             vend3 = HIGH;

//         bcm2835_gpio_write(end1, vend1);
//         bcm2835_gpio_write(end2, vend2);
//         bcm2835_gpio_write(end3, vend3);

//         printf("%d %d %d\n",
//                bcm2835_gpio_lev(end3),
//                bcm2835_gpio_lev(end2),
//                bcm2835_gpio_lev(end1));
//         fflush(stdout);

//         // claramente essa solução não suporta mais que 8 vagas, mas foi oq deu
//         wait(5);
//         int read = bcm2835_gpio_lev(INP_A1_SENSOR_VAGA);
//         a->vagas[i] = read;
//         printf("vaga %d é %d!\n", i + 1, read);
//         fflush(stdout);
//     }

//     if (id_andar == andar_entrada)
//     {
//         log_print("[DEP LE APLICA ESTADO] analisando entrada\n", LEVEL_DEBUG);

//         atualiza_tempo(&e->entrada->sensor_de_presenca_entrada, bcm2835_gpio_lev(INP_A1_SENSOR_ABERTURA_CANCELA_ENTRADA));
//         atualiza_tempo(&e->entrada->sensor_de_passagem_saida, bcm2835_gpio_lev(INP_A1_SENSOR_FECHAMENTO_CANCELA_SAIDA));
//         bcm2835_gpio_write(OUT_A1_MOTOR_CANCELA_ENTRADA, e->entrada->motor_entrada_ligado);

//         atualiza_tempo(&e->entrada->sensor_de_passagem_entrada, bcm2835_gpio_lev(INP_A1_SENSOR_FECHAMENTO_CANCELA_ENTRADA));
//         atualiza_tempo(&e->entrada->sensor_de_presenca_saida, bcm2835_gpio_lev(INP_A1_SENSOR_ABERTURA_CANCELA_SAIDA));
//         bcm2835_gpio_write(OUT_A1_MOTOR_CANCELA_SAIDA, e->entrada->motor_saida_ligado);

//         bcm2835_gpio_write(OUT_A1_SINAL_DE_LOTADO_FECHADO, e->estacionamento_fechado || e->estacionamento_lotado);
//     }
//     else
//     {
//         log_print("[DEP LE APLICA ESTADO] analisando passagem de andar\n", LEVEL_DEBUG);
//         int id_sensores = id_andar;
//         if (andar_entrada < id_andar)
//         {
//             id_sensores--;
//         }

//         log_print("le sensor de entrada \n", LEVEL_DEBUG);
//         if (bcm2835_gpio_lev(INP_SENSOR_DE_PASSAGEM_1))
//         {
//             e->sensor_de_subida_de_andar[id_sensores - 1]++;
//             e->tempo_sensores_de_andar_atualizados = get_time_mcs();
//         }

//         if (bcm2835_gpio_lev(INP_SENSOR_DE_PASSAGEM_2))
//         {
//             e->sensor_de_descida_de_andar[id_sensores - 1]++;
//             e->tempo_sensores_de_andar_atualizados = get_time_mcs();
//         }

//         bcm2835_gpio_write(OUT_A2_SINAL_DE_LOTADO_FECHADO, e->estacionamento_fechado || e->estacionamento_lotado);
//     }

//     return e;
// }

// // int main()
// // {
// //     set_env();
// //     set_level(LEVEL_DEBUG);
// //     set_time_wait_ignore(0);

// //     log_print("[DEP MAIN] incializando\n", LEVEL_INFO);

// //     if (!bcm2835_init())
// //         exit(1);

// //     log_print("[DEP MAIN] bcm iniciado\n", LEVEL_INFO);

// //     configura_pinos();
// //     log_print("[DEP MAIN] pinos configurados\n", LEVEL_INFO);

// //     signal(SIGINT, handle_interruption);
// //     log_print("[DEP MAIN] signal settado\n", LEVEL_INFO);

// //     id_andar = read_env_int_index("ID_ANDAR", -1);
// //     char BUFF[1000];
// //     sprintf(BUFF, "%s%d", "ANDAR_", id_andar);
// //     log_print(BUFF, 1);
// //     EstadoEstacionamento *e = inicializar_estado(BUFF);

// //     log_print("[DEP MAIN] estado inicializado\n", LEVEL_INFO);

// //     abrir_thread_server_dependente(e);
// //     log_print("[DEP MAIN] thread de abrir porta chamada\n", LEVEL_DEBUG);

// //     time_mcs last_exec = 0;

// //     while (1)
// //     {
// //         log_print("[DEP MAIN] chama le aplica estado\n", LEVEL_DEBUG);
// //         e = le_aplica_estado(e, id_andar);
// //         log_print("[DEP MAIN] le aplica estado retornou!!!\n", LEVEL_DEBUG);
// //         set_resposta(e);

// //         printf("ESCUTANDO %s:%d\n", e->endereco->ip, e->endereco->porta);
// //         if (is_newer(PERIODO_MINIMO_ENTRE_EXECUCOES + last_exec))
// //         {
// //             time_mcs wait_time = PERIODO_MINIMO_ENTRE_EXECUCOES + last_exec - get_time_mcs();
// //             printf("Waiting %lu ms\n", wait_time);
// //             wait(wait_time);
// //         }
// //         last_exec = get_time_mcs();
// //     }
// // }

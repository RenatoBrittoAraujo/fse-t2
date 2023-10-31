#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <softPwm.h>

#include <shared/inc/threads.h>
#include <shared/inc/shared_util.h>
#include <shared/inc/time.h>
#include <shared/inc/lcd_16_2_controller.h>
#include <shared/inc/pid.h>

#include <main/inc/modbus.h>
#include <main/inc/bme280_2.h>
#include <main/inc/encoder_rotatorio.h>

#define STR_FRAME 10000

#define ENCODER_ROTATORIO_INVALIDO -1e9
#define TEMPERATURA_INVALIDA -1e9

#define THREAD_EXEC_IGNORED (void *)1

struct PeriodicThread
{
    char *name;

    time_mcs last_exec;
    time_mcs period;
    ThreadState *thread;
};
typedef struct PeriodicThread PeriodicThread;

struct Estado
{
    // ========== COMUNICAÇÃO

    // Descontinuado:

    // I2CCommEstado *i2c;
    // GPIOCommEstado *gpio;
    // UARTMODBUSCommEstado *uart_modbus;

    // ========== CONSTANTES GERAIS

    int deadline_io;

    int pin_ventoinha;
    int pin_resistencia;

    double pid_kp;
    double pid_ki;
    double pid_kd;

    int bme280_i2c_fd;

    FILE *logs_csv_file;
    char *logs_file_name;
    char *logs_folder;

    // ========== THREADS

    // PeriodicThread t_gpio;
    // PeriodicThread t_i2c;
    // PeriodicThread t_uart_modbus;

    PeriodicThread t_controle;

    PeriodicThread t_botoes;
    PeriodicThread t_leitura_encoder_rotatorio;
    PeriodicThread t_leitura_temperatura_ambiente;
    PeriodicThread t_leitura_temperatura_interna;

    PeriodicThread t_escrita_potencia;
    PeriodicThread t_escrita_lcd;

    // ========== SAIDAS
    // todas as estruturas aqui que começam com 'out_' devem ser modificada apenas pela thread de controle.

    // [0,100] % (período máximo de 10ms)
    int out_valor_ventoinha;

    // [0,100] % (período máximo de 10ms)
    int out_valor_resistencia;

    int out_sinal_de_controle;

    double out_sinal_de_referencia;

    // 0 = ligado / 1 = desligado
    int out_estado_sistema;

    // 0 = dashboard / 1 = curva ou terminal
    int out_modo_de_controle;

    // 1 = aberto / 0 = fechado
    int out_menu_aberto;

    // 0 = parado / 1 = funcionando
    int out_estado_funcionamento;

    int out_contador_tempo;

    char *out_display_lcd;

    // ========== ENTRADAS
    // todas as estruturas aqui que começam com 'in_' devem ser modificada apenas pelas threads durante leitura de sensores externos.

    double in_last_posicao_encoder_rotatorio;
    double in_temperatura_interna;
    double in_temperatura_referencia;
    double in_temperatura_ambiente;

    int in_last_comando_usuario;

    // 0 = dashboard / 1 = curva ou terminal
    int in_modo_de_controle;

    // 1 = aberto / 0 = fechado
    int in_menu_aberto;

    // 0 = parado / 1 = funcionando
    int in_estado_funcionamento;

    // ====== INUTIL
    // as três variáveis abaixo são inúteis pro projeto
    // mas vou deixar poque servem também pra aeroespacial.
    double in_pressao;
    double in_umidade;
    double in_altitude;

    // exemplo
    // - in_pressao = 884.98
    // - in_umidade = 42.25
    // - in_altitude = 1125.21

    // note como a altitude está em quase perfeita concordância
    // com a real altitude de brasília, 1172m. isso é muito foda
    // ====== FIM INUTIL
};
typedef struct Estado Estado;

void print_estado(Estado *e)
{
    printf("====== PRINT DO ESTADO ATUAL ======\n", e);
    fflush(stdout);
    printf("estado ptr = %p\n\n", e);
    fflush(stdout);
    printf("- bme280_i2c_fd = %d\n", e->bme280_i2c_fd);
    printf("- deadline_io = %d\n", e->deadline_io);
    printf("- pin_ventoinha = %d\n", e->pin_ventoinha);
    printf("- pin_resistencia = %d\n", e->pin_resistencia);
    printf("- out_valor_ventoinha = %d\n", e->out_valor_ventoinha);
    printf("- out_valor_resistencia = %d\n", e->out_valor_resistencia);
    printf("- out_sinal_de_controle = %d\n", e->out_sinal_de_controle);
    printf("- out_estado_sistema = %d\n", e->out_estado_sistema);
    printf("- out_modo_de_controle = %d\n", e->out_modo_de_controle);
    printf("- out_menu_aberto = %d\n", e->out_menu_aberto);
    printf("- out_estado_funcionamento = %d\n", e->out_estado_funcionamento);
    printf("- out_contador_tempo = %d\n", e->out_contador_tempo);
    printf("- in_last_comando_usuario = %d\n", e->in_last_comando_usuario);
    printf("- in_modo_de_controle = %d\n", e->in_modo_de_controle);
    printf("- in_menu_aberto = %d\n", e->in_menu_aberto);
    printf("- in_estado_funcionamento = %d\n", e->in_estado_funcionamento);
    printf("- pid_kp = %.2f\n", e->pid_kp);
    printf("- pid_ki = %.2f\n", e->pid_ki);
    printf("- pid_kd = %.2f\n", e->pid_kd);
    printf("- out_sinal_de_referencia = %.2f\n", e->out_sinal_de_referencia);
    printf("- in_temperatura_interna = %.2f\n", e->in_temperatura_interna);
    printf("- in_temperatura_referencia = %.2f\n", e->in_temperatura_referencia);
    printf("- in_temperatura_ambiente = %.2f\n", e->in_temperatura_ambiente);
    printf("- in_pressao = %.2f\n", e->in_pressao);
    printf("- in_umidade = %.2f\n", e->in_umidade);
    printf("- in_altitude = %.2f\n", e->in_altitude);
    printf("- out_display_lcd = '%s'\n", e->out_display_lcd);
    printf("====== FIM PRINT DO ESTADO ======\n", e);
    fflush(stdout);
}

////////////// INICIALIZAÇÃO

Estado *copia_estado(Estado *e)
{
    Estado *novo = malloc(sizeof(Estado));
    memcpy(novo, e, sizeof(Estado));
    return novo;
}

int should_thread_run(PeriodicThread *p)
{
    return !p->thread->__running && !is_newer(p->last_exec + p->period);
}

void *run_thread(PeriodicThread *t, void *arg)
{
    if (!should_thread_run(t))
    {
        return THREAD_EXEC_IGNORED;
    }

    t->thread->args = arg;

    start_thread(t->thread);

    void *res_void_ptr = wait_thread_response_with_deadline(t->thread, t->period);
    t->last_exec = get_time_mcs();

    return res_void_ptr;
}

void processa_comando_usuario(int comando, Estado *e)
{
    // Lê comandos do usuário
    // - 0x01: botao de ligar
    // - 0x02: botao de desligar
    // - 0x03: botao de inciar aquecimento
    // - 0x04: botao de cancelar processo
    // - 0x05: botao de tempo + 1 minuto
    // - 0x06: botao de tempo - 1 minuto
    // - 0x07: aciona modo de alimentos pre-programados
    char *s;

    if (comando != 0)
    {
        printf("[====================================] comando de usuário: %d\n", comando);
    }
    switch (comando)
    {
    case 0x01:
        e->out_estado_sistema = 1;
        break;
    case 0x02:
        e->out_estado_sistema = 0;
        break;
    case 0x03:
        e->out_estado_funcionamento = 1;
        break;
    case 0x04:
        e->out_estado_funcionamento = 0;
        break;
    case 0x05:
        e->out_contador_tempo += 1;
        break;
    case 0x06:
        e->out_contador_tempo -= 1;
        break;
    case 0x07:
        // range de [0,1]
        e->out_menu_aberto = 1 - e->out_menu_aberto;
        break;
    case 0x0:
        // noop
        break;
    default:
        sprintf(s, "[thread_controle] comando de usuário desconhecido: %d", comando);
        log_print(s, LEVEL_ERROR);
        break;
    }
}

void processa_ventoinha_e_resistencia(Estado *e)
{
    pid_atualiza_referencia(e->in_temperatura_referencia);
    e->out_sinal_de_referencia = e->in_temperatura_referencia;

    // periodo de [-100,100]
    double sinal_de_controle = pid_controle(e->in_temperatura_interna);
    e->out_sinal_de_controle = sinal_de_controle;

    // quando sinal diz que temperatura deve aumentar, aumenta resistencia e desliga ventoinha
    if (sinal_de_controle > 0)
    {
        // note como isso será aplicado pelo uso de pwd que "amortece" a mudança
        // ou seja, pode ser alterado bruscamente à vontade, porque o controle
        // disso vai amaciar cada uma das mundaças sem danificar aparelho
        e->out_valor_resistencia = sinal_de_controle;
        e->out_valor_ventoinha = 0;
    }
    // caso contrário, aumenta ventoinha e diminui resistencia
    else
    {
        e->out_valor_resistencia = 0;
        e->out_valor_ventoinha = -sinal_de_controle;
    }
}

void controle_thread(ThreadState *ts, void *e_ptr)
{
    log_print("[controle_thread] rodando thread", LEVEL_INFO);
    // NESSA THREAD, NENHUM INPUT COM PREFIXO 'in_' PODE SER MODIFICADO
    // TODOS OS INPUTS 'in_' DEVEM SER EXPLICITAMENTE LIDOS OU IGNORADOS COM JUSTIFICATIVA
    // TODOS OS INPUTS 'out_' DEVEM SER EXPLICITAMENTE ESCRITOS OU IGNORADOS COM JUSTIFICATIVA
    Estado *e = (Estado *)e_ptr;

    // => processa input 'in_last_comando_usuario'
    // => escreve output 'out_menu_aberto'
    // => escreve output 'out_contador_tempo'
    // => escreve output 'out_estado_sistema'
    // => escreve output 'out_estado_funcionamento'
    processa_comando_usuario(e->in_last_comando_usuario, e);
    // limpa para evitar processamento duplicado
    e->in_last_comando_usuario = 0x0;

    // => processa input in_temperatura_interna
    // => processa input in_temperatura_referencia
    // => escreve output out_sinal_de_controle
    // => escreve output out_sinal_de_referencia
    // => escreve output out_valor_resistencia
    // => escreve output out_valor_ventoinha
    processa_ventoinha_e_resistencia(e);

    // int out_modo_de_controle
    // int in_modo_de_controle
    // int in_menu_aberto

    // OUTPUTS DESNECESSARIOS, SERÃO IGNORADOS:
    // char *out_display_lcd - descontinuado por professor

    // INPUTS INUTEIS, SERÃO IGNORADOS:
    // double in_pressao  - guardado apenas como referência para bme280
    // double in_umidade  - guardado apenas como referência para bme280
    // double in_altitude - guardado apenas como referência para bme280

    log_print("[controle_thread] fim", LEVEL_INFO);

    pthread_exit(e);
}

void controle(Estado *e)
{
    void *res_void_ptr = run_thread(&e->t_controle, copia_estado(e));
    if (res_void_ptr == THREAD_EXEC_IGNORED)
        return;

    if (res_void_ptr == NULL)
    {
        log_print("[controle] thread de controle nao retornou a tempo", LEVEL_ERROR);
    }
    else
    {
        // prefiro definir explicitamente assim pra evitar qualquer escrita bizarra
        Estado *res = (Estado *)res_void_ptr;

        e->out_contador_tempo = res->out_contador_tempo;
        e->out_display_lcd = res->out_display_lcd;

        e->out_estado_funcionamento = res->out_estado_funcionamento;
        e->out_estado_sistema = res->out_estado_sistema;
        e->out_modo_de_controle = res->out_modo_de_controle;

        e->out_sinal_de_controle = res->out_sinal_de_controle;
        e->out_sinal_de_referencia = res->out_sinal_de_referencia;

        e->out_valor_resistencia = res->out_valor_resistencia;
        e->out_valor_ventoinha = res->out_valor_ventoinha;
    }
}

void escreve_lcd_thread(ThreadState *ts, void *e_void_ptr)
{
    log_print("[escreve_lcd_thread] rodando thread", LEVEL_INFO);
    Estado *e = (Estado *)e_void_ptr;

    ModbusRequest req = new_modbus_request(0x1, 0x16, 0xD8, REQ_TYPE_SEND_STR, "7239", RES_TYPE_NONE);
    set_req_str(&req, e->out_display_lcd);

    make_modbus_request(&req);
    log_print("[escreve_lcd_thread] fim", LEVEL_INFO);
    pthread_exit(e);
}
void escreve_lcd(Estado *e)
{
    // usa ic2 no lcd 16x2 (professor deu o código)
    void *res_void_ptr = run_thread(&e->t_escrita_lcd, copia_estado(e));
    if (res_void_ptr == THREAD_EXEC_IGNORED)
        return;
    if (res_void_ptr == NULL)
    {
        log_print("[escreve_lcd] thread escrita de lcd nao retornou a tempo", LEVEL_ERROR);
    }
}

void escreve_potencia_thread(ThreadState *ts, void *e_void_ptr)
{
    log_print("[escreve_potencia_thread] rodando thread", LEVEL_INFO);
    Estado *e = (Estado *)e_void_ptr;

    softPwmWrite(e->pin_resistencia, e->out_valor_resistencia);
    softPwmWrite(e->pin_ventoinha, e->out_valor_ventoinha);
    pthread_exit(e);
}
void escreve_potencia(Estado *e)
{
    // usa gpio no circuito de potencia
    void *res_void_ptr = run_thread(&e->t_controle, copia_estado(e));
    if (res_void_ptr == THREAD_EXEC_IGNORED)
        return;
    if (res_void_ptr == NULL)
    {
        log_print("[escreve_potencia] thread de escrita de potencia nao retornou a tempo", LEVEL_ERROR);
    }
}

void le_encoder_rotatorio_thread(ThreadState *ts, void *e_void_ptr)
{
    log_print("[le_encoder_rotatorio_thread] rodando thread", LEVEL_INFO);
    Estado *e = (Estado *)e_void_ptr;

    EstadoEncoderRotatorio een = get_estado_encoder_rotatorio();
    e->in_last_posicao_encoder_rotatorio = een.posicao;

    log_print("[le_encoder_rotatorio_thread] fim", LEVEL_INFO);
    pthread_exit(e);
}
void le_encoder_rotatorio(Estado *e)
{
    // usa gpio no potenciometro usado em aula
    void *res_void_ptr = run_thread(&e->t_leitura_encoder_rotatorio, copia_estado(e));
    if (res_void_ptr == THREAD_EXEC_IGNORED)
        return;
    if (res_void_ptr == NULL)
    {
        log_print("[le_encoder_rotatorio] thread de leitura encoder rotatorio nao retornou a tempo", LEVEL_ERROR);
    }
    else
    {
        Estado *res = (Estado *)res_void_ptr;
        if (e->in_last_posicao_encoder_rotatorio == ENCODER_ROTATORIO_INVALIDO)
        {
            e->in_last_posicao_encoder_rotatorio = res->in_last_posicao_encoder_rotatorio;
        }
        else
        {
            double diff = res->in_last_posicao_encoder_rotatorio - e->in_last_posicao_encoder_rotatorio;
            e->in_last_posicao_encoder_rotatorio = res->in_last_posicao_encoder_rotatorio;
            e->in_temperatura_referencia += diff;
        }
    }
}

void le_temperatura_ambiente_thread(ThreadState *ts, void *e_void_ptr)
{
    log_print("[le_temperatura_ambiente_thread] rodando thread", LEVEL_INFO);
    Estado *e = (Estado *)e_void_ptr;

    bme280_calib_data cal;
    readCalibrationData(e->bme280_i2c_fd, &cal);

    wiringPiI2CWriteReg8(e->bme280_i2c_fd, 0xf2, 0x01); // humidity oversampling x 1
    wiringPiI2CWriteReg8(e->bme280_i2c_fd, 0xf4, 0x25); // pressure and temperature oversampling x 1, mode normal

    bme280_raw_data raw;
    getRawData(e->bme280_i2c_fd, &raw);

    int32_t t_fine = getTemperatureCalibration(&cal, raw.temperature);
    double t = compensateTemperature(t_fine);                        // C
    double p = compensatePressure(raw.pressure, &cal, t_fine) / 100; // hPa
    double h = compensateHumidity(raw.humidity, &cal, t_fine);       // %
    double a = getAltitude(p);

    e->in_temperatura_ambiente = t;
    // note como essas variáveis são inúteis, mas quero guardar porque serve pra algum projeto aeroespacial
    e->in_pressao = p;
    e->in_umidade = h;
    e->in_altitude = a;

    ModbusRequest req = new_modbus_request(0x01, 0x23, 0xC2, REQ_TYPE_REQUEST_FLT, "7239", RES_TYPE_FLT);
    ModbusResponse *res = make_modbus_request(&req);
    if (res == NULL)
    {
        log_print("[le_temperatura_ambiente_thread] erro ao fazer request modbus pra temperatura referência", LEVEL_ERROR);
    }
    e->in_temperatura_referencia = (double)htonl((int)res->float_num);

    log_print("[le_temperatura_ambiente_thread] fim", LEVEL_INFO);
    pthread_exit(e);
}
void le_temperatura_ambiente(Estado *e)
{
    // usa ic2 no bmp280 (para placas 1,2,4) OU
    // usa ic2 no bme280 (para placa 3)
    void *res_void_ptr = run_thread(&e->t_leitura_temperatura_ambiente, copia_estado(e));
    if (res_void_ptr == THREAD_EXEC_IGNORED)
        return;
    if (res_void_ptr == NULL)
    {
        log_print("[le_temperatura_ambiente] thread de temperatura ambiente nao retornou a tempo", LEVEL_ERROR);
    }
    else
    {
        Estado *res = (Estado *)res_void_ptr;
        e->in_temperatura_ambiente = res->in_temperatura_ambiente;
        e->in_pressao = res->in_pressao;
        e->in_altitude = res->in_altitude;
        e->in_umidade = res->in_umidade;
        e->in_temperatura_referencia = res->in_temperatura_referencia;
    }
}

void le_temperatura_interna_thread(ThreadState *ts, void *e_void_ptr)
{
    log_print("[le_temperatura_interna_thread] rodando thread", LEVEL_INFO);
    Estado *e = (Estado *)e_void_ptr;

    ModbusRequest req = new_modbus_request(0x1, 0x23, 0xC1, REQ_TYPE_REQUEST_FLT, "7239", RES_TYPE_FLT);

    ModbusResponse *res = make_modbus_request(&req);

    if (res == NULL)
    {
        log_print("[le_temperatura_interna_thread] falhou em ler temperatura interna", LEVEL_ERROR);
        pthread_exit(NULL);
        return;
    }

    e->in_temperatura_interna = (double)htonl((int)res->float_num);

    log_print("[le_temperatura_interna_thread] fim", LEVEL_INFO);
    pthread_exit(e);
}
void le_temperatura_interna(Estado *e)
{
    // usa uart  no ds18b20
    void *res_void_ptr = run_thread(&e->t_leitura_temperatura_interna, copia_estado(e));
    if (res_void_ptr == THREAD_EXEC_IGNORED)
        return;
    if (res_void_ptr == NULL)
    {
        log_print("[le_temperatura_interna] thread de leitura interna nao retornou a tempo", LEVEL_ERROR);
    }
    else
    {
        Estado *res = (Estado *)res_void_ptr;
        e->in_temperatura_interna = res->in_temperatura_interna;
    }
}

void le_escreve_botoes_thread(ThreadState *ts, void *e_void_ptr)
{
    log_print("[le_escreve_botoes_thread] rodando thread", LEVEL_INFO);

    Estado *e = (Estado *)e_void_ptr;

    ModbusRequest req = new_modbus_request(0x1, 0x23, 0xC3, REQ_TYPE_REQUEST_INT, "7239", RES_TYPE_INT);

    ModbusResponse *res = make_modbus_request(&req);
    if (res == NULL)
    {
        log_print("[le_escreve_botoes_thread] falha na leitura do botão", LEVEL_ERROR);
        pthread_exit(e);
    }
    int command = res->int_num;
    e->in_last_comando_usuario = htonl(command);

    // Escreve o LED de ligado/desligado
    req = new_modbus_request(
        0x1,
        0x16,
        0xD3,
        REQ_TYPE_SEND_INT,
        "7239",
        RES_TYPE_NONE);
    req.int_num = e->out_estado_sistema;
    printf("============sending: %d\n", req.int_num);
    fflush(stdout);
    make_modbus_request(&req);

    // Escreve o LED de funcionando ou nao
    req = new_modbus_request(
        0x1,
        0x16,
        0xD5,
        REQ_TYPE_SEND_INT,
        "7239",
        RES_TYPE_NONE);
    e->out_estado_funcionamento = 0;
    req.int_num = e->out_estado_funcionamento;
    printf("============sending func: %d\n", req.int_num);
    fflush(stdout);
    make_modbus_request(&req);

    // envia o contador de tempo
    req = new_modbus_request(
        0x1,
        0x16,
        0xD7,
        REQ_TYPE_SEND_INT,
        "7239",
        RES_TYPE_NONE);
    req.int_num = htonl(e->out_contador_tempo);
    make_modbus_request(&req);

    // envia sinal de controle
    req = new_modbus_request(
        0x1,
        0x16,
        0xD1,
        REQ_TYPE_SEND_INT,
        "7239",
        RES_TYPE_NONE);
    req.int_num = htonl(e->out_sinal_de_controle);
    make_modbus_request(&req);

    // envia sinal de referencia
    req = new_modbus_request(
        0x1,
        0x16,
        0xD2,
        REQ_TYPE_SEND_INT,
        "7239",
        RES_TYPE_NONE);
    req.int_num = htonl(e->out_sinal_de_referencia);
    make_modbus_request(&req);

    log_print("[le_escreve_botoes_thread] fim", LEVEL_INFO);

    pthread_exit(e);
}
void le_escreve_botoes(Estado *e)
{
    void *res_void_ptr = run_thread(&e->t_botoes, copia_estado(e));
    if (res_void_ptr == THREAD_EXEC_IGNORED)
        return;
    if (res_void_ptr == NULL)
    {
        log_print("[le_escreve_botoes] thread de leitura de botoes nao retornou a tempo", LEVEL_ERROR);
    }
    else
    {
        Estado *res = (Estado *)res_void_ptr;
        e->in_last_comando_usuario = res->in_last_comando_usuario;
    }
}

void init_threads(Estado *e)
{
    PeriodicThread *threads[] = {
        &e->t_controle,
        &e->t_escrita_lcd,
        &e->t_escrita_potencia,
        // &e->t_gpio,
        // &e->t_i2c,
        // &e->t_uart_modbus,
        &e->t_leitura_encoder_rotatorio,
        &e->t_botoes,
        &e->t_leitura_temperatura_ambiente,
        &e->t_leitura_temperatura_interna,
    };
    size_t sz = 7;

    for (size_t i = 0; i < sz; i++)
    {
        threads[i]->thread = create_thread_state(-1);
        threads[i]->last_exec = 0;
        threads[i]->name = (char *)malloc(sizeof(char) * 100);
        threads[i]->thread->response_size = sizeof(Estado);
        // threads[i]->period = lido do ambiente
    }

    e->t_controle.thread->routine = controle_thread;
    e->t_escrita_lcd.thread->routine = escreve_lcd_thread;
    e->t_escrita_potencia.thread->routine = escreve_potencia_thread;
    // e->t_gpio.thread->routine = gpio_thread;
    // e->t_i2c.thread->routine = ic2_thread;
    // e->t_uart_modbus.thread->routine = uart_modbus_thread;
    e->t_leitura_encoder_rotatorio.thread->routine = le_encoder_rotatorio_thread;
    e->t_botoes.thread->routine = le_escreve_botoes_thread;
    e->t_leitura_temperatura_ambiente.thread->routine = le_temperatura_ambiente_thread;
    e->t_leitura_temperatura_interna.thread->routine = le_temperatura_interna_thread;

    e->t_controle.name = "controle_thread";
    e->t_escrita_lcd.name = "escreve_lcd_thread";
    e->t_escrita_potencia.name = "escreve_potencia_thread";
    // e->t_gpio.name = "gpio_thread";
    // e->t_i2c.name = "ic2_thread";
    // e->t_uart_modbus.name = "uart_modbus_thread";
    e->t_leitura_encoder_rotatorio.name = "le_encoder_rotatorio_thread";
    e->t_botoes.name = "le_escreve_botoes_thread";
    e->t_leitura_temperatura_ambiente.name = "le_temperatura_ambiente_thread";
    e->t_leitura_temperatura_interna.name = "le_temperatura_interna_thread";
}

Estado *init_estado()
{
    log_print("[init_estado] inicializando estado", LEVEL_DEBUG);
    Estado *e = (Estado *)malloc(sizeof(Estado));

    log_print("[init_estado] lendo variaveis de ambiente", LEVEL_DEBUG);
    e->deadline_io = read_env_int_index("DEADLINE_IO", -1);
    e->logs_folder = read_env_str_index("LOGS_FOLDER", -1);
    e->t_botoes.period = read_env_int_index("PERIODO_LEITURA_BOTOES_MS", -1) * MILLI;
    e->t_leitura_temperatura_ambiente.period = read_env_int_index("PERIODO_LEITURA_SENSOR_DS18B20_MS", -1) * MILLI;
    e->t_leitura_encoder_rotatorio.period = read_env_int_index("PERIODO_LEITURA_ENCODER_ROTATORIO_MS", -1) * MILLI;
    e->t_escrita_potencia.period = read_env_int_index("PERIODO_ESCRITA_POTENCIA_MS", -1) * MILLI;
    e->t_leitura_temperatura_interna.period = read_env_int_index("PERIODO_LEITURA_SENSOR_BMP280_MS", -1) * MILLI;
    e->t_escrita_lcd.period = read_env_int_index("PERIODO_ESCRITA_LCD_MS", -1) * MILLI;
    // e->t_gpio.period = read_env_int_index("PERIODO_COMUNICACAO_GPIO_MS", -1) * MILLI;
    // e->t_i2c.period = read_env_int_index("PERIODO_COMUNICACAO_I2C_MS", -1) * MILLI;
    // e->t_uart_modbus.period = read_env_int_index("PERIODO_COMUNICACAO_UART_MODBUS_MS", -1) * MILLI;
    e->t_controle.period = read_env_int_index("PERIODO_CONTROLE_MS", -1) * MILLI;
    e->pin_ventoinha = read_env_int_index("PIN_VENTOINHA", -1);
    e->pin_resistencia = read_env_int_index("PIN_RESISTENCIA", -1);
    e->pid_kp = read_env_double_index("PID_KP", -1);
    e->pid_ki = read_env_double_index("PID_KI", -1);
    e->pid_kd = read_env_double_index("PID_KD", -1);

    // log_print("[init_estado] inicializando i2c", LEVEL_DEBUG);
    // init_i2c(e);

    // log_print("[init_estado] inicializando gpio", LEVEL_DEBUG);
    // init_gpio(e);gpio

    // log_print("[init_estado] inicializando uart modbus", LEVEL_DEBUG);
    // init_uart_modbus(e);

    log_print("[init_estado] inicializando threads", LEVEL_DEBUG);
    init_threads(e);

    log_print("[init_estado] inicializando valores estado", LEVEL_DEBUG);
    e->out_display_lcd = (char *)malloc(sizeof(char) * STR_FRAME);
    e->out_contador_tempo = 0;
    e->out_valor_resistencia = 0;
    e->out_valor_ventoinha = 0;
    e->out_estado_funcionamento = 0;
    e->out_estado_sistema = 0;
    e->out_sinal_de_controle = 0;
    e->out_sinal_de_referencia = 0;
    e->in_temperatura_referencia = TEMPERATURA_INVALIDA;
    e->in_temperatura_ambiente = TEMPERATURA_INVALIDA;
    e->in_temperatura_interna = TEMPERATURA_INVALIDA;
    e->in_last_posicao_encoder_rotatorio = ENCODER_ROTATORIO_INVALIDO;

    log_print("[init_estado] inicializando pinagem gpio", LEVEL_DEBUG);
    wiringPiSetup();
    softPwmCreate(e->pin_ventoinha, e->out_valor_ventoinha, 100);
    softPwmCreate(e->pin_resistencia, e->out_valor_resistencia, 100);

    log_print("[init_estado] inicializando pid", LEVEL_DEBUG);
    pid_configura_constantes(e->pid_kp, e->pid_ki, e->pid_kd);

    log_print("[init_estado] inicializando bme280", LEVEL_DEBUG);
    e->bme280_i2c_fd = wiringPiI2CSetup(BME280_ADDRESS);
    if (e->bme280_i2c_fd < 0)
    {
        log_print("[init_estado] bme280 falhou em iniciar file descriptor", LEVEL_DEBUG);
        return NULL;
    }

    log_print("[init_estado] estado inicializado com sucesso!", LEVEL_DEBUG);

    return e;
}

////////////// LEITURAS

Estado *le_escreve_dispositivos(Estado *e)
{
    log_print("[le_escreve_dispositivos] inicia", LEVEL_INFO);
    // executa as threads de leitura
    le_temperatura_ambiente(e);
    le_temperatura_interna(e);
    le_encoder_rotatorio(e);
    le_escreve_botoes(e);
    escreve_potencia(e);

    // decontinuado:
    // - escreve_lcd(e);

    wait_ms(e->deadline_io);

    log_print("[le_escreve_dispositivos] fim", LEVEL_INFO);
    return e;
}

////////////// CONTROLE

Estado *controla_estado(Estado *e)
{

    log_print("[controla_estado] inicia", LEVEL_INFO);
    controle(e);
    log_print("[controla_estado] fim", LEVEL_INFO);
    return e;
}

void append_to_csv(Estado *e)
{
    // pra evitar algo no buffer indo pros arquivos
    fflush(stdout);
    log_print("[init_estado] inicializando arquivo de log csv", LEVEL_DEBUG);

    if (e->logs_file_name == NULL)
    {
        log_print("[init_estado] inicializando nome arquivo de log csv", LEVEL_DEBUG);
        e->logs_file_name = (char *)malloc(sizeof(char) * STR_FRAME);
        sprintf(e->logs_file_name, "%slogs_%ld.csv", e->logs_folder, get_time_mcs());

        // touch file with csv headers
        FILE *tmp = fopen(e->logs_file_name, "w");
        if (tmp == NULL)
        {
            log_print("[init_estado] falha ao criar arquivo de log csv", LEVEL_DEBUG);
            exit(1);
        }
        fwrite("contador_tempo,valor_resistencia,valor_ventoinha,estado_funcionamento,estado_sistema,sinal_de_controle,sinal_de_referencia,temperatura_referencia,temperatura_ambiente,temperatura_interna,last_posicao_encoder_rotatorio,menu_aberto\n", sizeof(char), 500, tmp);
        fclose(tmp);
    }

    e->logs_csv_file = fopen(e->logs_file_name, "a");
    if (e->logs_csv_file == NULL)
    {
        log_print("[append_to_csv] could not open log file!", LEVEL_ERROR);
        exit(1);
    }
    fprintf(e->logs_csv_file, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            e->out_contador_tempo,
            e->out_valor_resistencia,
            e->out_valor_ventoinha,
            e->out_estado_funcionamento,
            e->out_estado_sistema,
            e->out_sinal_de_controle,
            e->out_sinal_de_referencia,
            e->in_temperatura_referencia,
            e->in_temperatura_ambiente,
            e->in_temperatura_interna,
            e->in_last_posicao_encoder_rotatorio,
            e->in_menu_aberto);

    fclose(e->logs_csv_file);
    log_print("[init_estado] end arquivo de log csv", LEVEL_DEBUG);
}

void test_lcd(Estado *e)
{
    log_print("[test_lcd] STARTING LCD 16x2 TEST", LEVEL_INFO);

    log_print("[test_lcd] WRITE 'https://files.catbox.moe/kkmi6a.pdf{linebreak}'", LEVEL_INFO);
    sprintf(e->out_display_lcd, "https://files.catbox.moe/kkmi6a.pdf\n\n");
    escreve_lcd(e);

    wait_ms(3 * SECOND);

    log_print("[test_lcd] WRITE '{linebreak}https://files.catbox.moe/d9cfgh.pdf'", LEVEL_INFO);
    sprintf(e->out_display_lcd, "\nhttps://files.catbox.moe/d9cfgh.pdf\n");
    escreve_lcd(e);

    wait_ms(3 * SECOND);

    log_print("[test_lcd] WRITE 'https://files.catbox.moe/{linebreak}pu31u7.pdf'", LEVEL_INFO);
    sprintf(e->out_display_lcd, "https://files.catbox.moe/\npu31u7.pdf\n");
    escreve_lcd(e);

    wait_ms(3 * SECOND);

    log_print("[test_lcd] WRITE ''", LEVEL_INFO);
    sprintf(e->out_display_lcd, "\n\n");
    escreve_lcd(e);

    wait_ms(3 * SECOND);

    log_print("[test_lcd] TEST FINISHED", LEVEL_INFO);
}

void test_led(Estado *e)
{
    log_print("[test_led] STARTING LED ON/OFF TEST", LEVEL_INFO);

    log_print("[test_led] Ligando LED", LEVEL_INFO);
    e->out_estado_sistema = 1;
    le_escreve_botoes(e);

    wait_ms(3 * SECOND);

    log_print("[test_led] Desligando LED", LEVEL_INFO);
    e->out_estado_sistema = 0;
    le_escreve_botoes(e);

    wait_ms(3 * SECOND);

    log_print("[test_led] TEST FINISHED", LEVEL_INFO);
}

Estado *ESTADO_GLOBAL;

void handle_sigint()
{
    log_print("[handle_sigint] SIGINT RECEIVED", LEVEL_INFO);

    if (ESTADO_GLOBAL != NULL)
    {
        free(ESTADO_GLOBAL->logs_file_name);
        free(ESTADO_GLOBAL->logs_folder);
        finalizar_leitura_encoder_rotatorio();
        force_thread_end(ESTADO_GLOBAL->t_botoes.thread);
        force_thread_end(ESTADO_GLOBAL->t_controle.thread);
        force_thread_end(ESTADO_GLOBAL->t_escrita_lcd.thread);
        force_thread_end(ESTADO_GLOBAL->t_escrita_potencia.thread);
        // force_thread_end(ESTADO_GLOBAL->t_gpio.thread);
        // force_thread_end(ESTADO_GLOBAL->t_i2c.thread);
        // force_thread_end(ESTADO_GLOBAL->t_uart_modbus.thread);
        force_thread_end(ESTADO_GLOBAL->t_leitura_encoder_rotatorio.thread);
        force_thread_end(ESTADO_GLOBAL->t_leitura_temperatura_ambiente.thread);
        force_thread_end(ESTADO_GLOBAL->t_leitura_temperatura_interna.thread);
    }

    exit(0);
}

// #define TEST_MODE 1

int main()
{
    log_print("[main] incializando app", LEVEL_DEBUG);
    set_level(LEVEL_DEBUG);
    signal(SIGINT, handle_sigint);

    log_print("[main] incializando estado", LEVEL_DEBUG);
    ESTADO_GLOBAL = init_estado();

#ifndef TEST_MODE
    log_print("[main] incializando loop principal", LEVEL_DEBUG);

    while (1)
    {
        log_print("[main] lendo e escrevendo dispositivos", LEVEL_DEBUG);
        ESTADO_GLOBAL = le_escreve_dispositivos(ESTADO_GLOBAL);

        log_print("[main] controlando baseado no estado", LEVEL_DEBUG);
        ESTADO_GLOBAL = controla_estado(ESTADO_GLOBAL);

        print_estado(ESTADO_GLOBAL);
        append_to_csv(ESTADO_GLOBAL);
    }

#else
    log_print("[main] incializando testes", LEVEL_DEBUG);
    test_led(ESTADO_GLOBAL);

    // descontinuado:
    // - test_lcd(e_test);

#endif
}

// ======= CODIGO DESCONTINUADO ==========

// struct I2CCommEstado
// {
//     char **message_queue;
//     int size;
// };
// typedef struct I2CCommEstado I2CCommEstado;

// struct GPIOCommEstado
// {
//     char **message_queue;
//     int size;
// };
// typedef struct GPIOCommEstado GPIOCommEstado;

// struct UARTMODBUSCommEstado
// {
//     char **message_queue;
//     int size;
// };
// typedef struct UARTMODBUSCommEstado UARTMODBUSCommEstado;

// void *gpio_thread(ThreadState *ts, void *e_void_ptr)
// {
//     Estado *e = (Estado *)e_void_ptr;
// }

// void *ic2_thread(ThreadState *ts, void *e_void_ptr)
// {
//     Estado *e = (Estado *)e_void_ptr;
// }

// void *uart_modbus_thread(ThreadState *ts, void *e_void_ptr)
// {
//     Estado *e = (Estado *)e_void_ptr;
//     // pra cada mensagem na fila de mensagens:
//     // - envia
//     // - recebe
//     // - copia
//     // - retorna pra quem pediu
// }

// void init_message_queue(char ***message_queue, int *size)
// {
//     log_print("[init_message_queue] iniciando fila de mensagens", LEVEL_INFO);
//     *message_queue = (char **)malloc(sizeof(char **));
//     *size = 0;
//     log_print("[init_message_queue] iniciada fila de mensagens", LEVEL_INFO);
// }

// void add_to_message_queue(char ***message_queue, int *size, char *message)
// {
//     *message_queue = (char **)realloc(*message_queue, size + 1);
//     *size = size + 1;
// }

// void init_i2c(Estado *e)
// {
//     e->i2c = (I2CCommEstado *)malloc(sizeof(I2CCommEstado));
//     init_message_queue(&e->i2c->message_queue, &e->i2c->size);
// }

// void init_gpio(Estado *e)
// {
//     e->gpio = (GPIOCommEstado *)malloc(sizeof(GPIOCommEstado));
//     init_message_queue(&e->gpio->message_queue, &e->gpio->size);
// }

// void init_uart_modbus(Estado *e)
// {
//     e->uart_modbus = (UARTMODBUSCommEstado *)malloc(sizeof(UARTMODBUSCommEstado));
//     init_message_queue(&e->uart_modbus->message_queue, &e->uart_modbus->size);
// }

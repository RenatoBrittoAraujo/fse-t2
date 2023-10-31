#include <stdio.h>
#include <bcm2835.h>

#include <shared/inc/time.h>
#include <shared/inc/threads.h>

#include <main/inc/encoder_rotatorio.h>

#define WAIT_ENCODER_LEITURA_MS 100

EstadoEncoderRotatorio *estado;
ThreadState *ts;
int started = 0;

void init_encoder_rotatorio()
{
    // gpio
    int CLK = 25;
    int DT = 24;
    int SW = 23;
    int CLK_F = 23;
    int DT_F = 24;
    int SW_F = 25;

    int iter = 0;
    int last_dt = 0;
    int last_clk = 0;
    int last_botao = 0;
    int estado_botao = 0;

    int dt = 0;
    int clk = 0;
    int position = 0;
    int direction = 1;

    time_ms t_repeat_ms = 100;

    bcm2835_init();

    bcm2835_gpio_fsel(DT, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(CLK, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(SW, BCM2835_GPIO_FSEL_INPT);

    bcm2835_gpio_ren(DT);
    bcm2835_gpio_ren(CLK);
    bcm2835_gpio_ren(SW);

    while (1)
    {
        wait_ms(WAIT_ENCODER_LEITURA_MS);

        int updated = 0;
        int expecting = 0;

        dt = bcm2835_gpio_lev(DT);
        if (bcm2835_gpio_eds(DT))
        {
            dt = 1;
        }

        clk = bcm2835_gpio_lev(CLK);
        if (bcm2835_gpio_eds(CLK))
        {
            clk = 1;
        }

        if (bcm2835_gpio_eds(SW))
        {
            estado_botao = 0;
        }
        else
        {
            estado_botao = 1;
        }

        if (clk != last_clk || last_dt != dt)
        {
            if (!expecting)
            {
                updated = 1;
                if (clk != last_clk)
                {
                }
                else if (dt != last_dt)
                {

                    direction = -1;
                }
                expecting = 1;
            }
            else
            {
                expecting = 0;
                position += direction;
            }

            last_dt = dt;
            last_clk = clk;
        }

        if (last_botao != estado_botao)
        {
            updated = 1;
            last_botao = estado_botao;
        }

        if (updated)
        {
            estado->botao_apertado = last_botao;
            estado->posicao = position;
            estado->direcao = direction;
        }
    }
}

EstadoEncoderRotatorio get_estado_encoder_rotatorio()
{
    if (!started)
    {
        started = 1;
        estado = malloc(sizeof(EstadoEncoderRotatorio));
        estado->botao_apertado = 0;
        estado->posicao = 0;
        estado->direcao = 0;

        // NOTA: isso está falhando e derruba a rasp!
        // desabilitei

        // ts = create_thread_state(-1);
        // ts->routine = init_encoder_rotatorio;
        // ts->args = estado;
        // start_thread(ts);
    }
    EstadoEncoderRotatorio e;
    e.botao_apertado = estado->botao_apertado;
    e.posicao = estado->posicao;
    e.direcao = estado->direcao;
    return e;
}

void finalizar_leitura_encoder_rotatorio()
{
    if (ts != NULL)
    {
        force_thread_end(ts);
        free(ts);
        free(estado);
    }
}
#ifndef ENCODER_ROTATORIO_H
#define ENCODER_ROTATORIO_H

struct EstadoEncoderRotatorio
{
    int direcao;
    int posicao;
    int botao_apertado;
};
typedef struct EstadoEncoderRotatorio EstadoEncoderRotatorio;

void finalizar_leitura_encoder_rotatorio();
EstadoEncoderRotatorio get_estado_encoder_rotatorio();

#endif
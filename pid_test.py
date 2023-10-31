from math import fabs
from time import sleep


class PID:

    saida_medida = 0
    sinal_de_controle = 0
    referencia = 0.0
    Kp = 0.0
    Ki = 0.0
    Kd = 0.0
    T = 1.0
    last_time = 0
    erro_total = 0
    erro_anterior = 0.0
    sinal_de_controle_MAX = 100.0
    sinal_de_controle_MIN = -100.0

    def __init__(self):
        self.saida_medida = 0
        self.sinal_de_controle = 0
        self.referencia = 0.0
        self.Kp = 0.0
        self.Ki = 0.0
        self.Kd = 0.0
        self.T = 1.0
        self.last_time = 0
        self.erro_total = 0
        self.erro_anterior = 0.0
        self.sinal_de_controle_MAX = 100.0
        self.sinal_de_controle_MIN = -100.0

    def pid_configura_constantes(self,  Kp_,  Ki_,  Kd_):
        self.Kp = Kp_
        self.Ki = Ki_
        self.Kd = Kd_

    def pid_atualiza_referencia(self, referencia_):
        self.referencia = referencia_

    def pid_controle(self,  saida_medida):
        erro = self.referencia - saida_medida

        self.erro_total = self.erro_total + erro

        if self.erro_total >= self.sinal_de_controle_MAX:
            self.erro_total = self.sinal_de_controle_MAX

        elif self.erro_total <= self.sinal_de_controle_MIN:
            self.erro_total = self.sinal_de_controle_MIN

        delta_error = erro - self.erro_anterior

        sinal_de_controle = self.Kp*erro + \
            (self.Ki*self.T)*self.erro_total + (self.Kd/self.T)*delta_error

        if sinal_de_controle >= self.sinal_de_controle_MAX:
            sinal_de_controle = self.sinal_de_controle_MAX

        elif sinal_de_controle <= self.sinal_de_controle_MIN:
            sinal_de_controle = self.sinal_de_controle_MIN

        self.erro_anterior = erro

        return sinal_de_controle


d_kp = 30.0
d_ki = 0.2
d_kd = 400.0


def apply_out_1(out):
    return out*0.003


def apply_out_2(out):
    m_out = fabs(out)
    return 0.00003*m_out*out


def apply_out_3(out):
    # como se a função da taxa de variação mudasse de acordo com o dispositivo usado
    # eg. aquecedor esquenta em diferente proporção de como ventoinha esfria
    if out > 0:
        return apply_out_2(out)
    else:
        return apply_out_1(out)


def test():

    p = PID()
    p.pid_configura_constantes(d_kp, d_ki, d_kd)
    p.pid_atualiza_referencia(200.0)

    t = 198

    while True:
        out = p.pid_controle(t)
        t_old = t
        t += apply_out_2(out)
        print(f"t={round(t,2)}\t\tout={round(out,2)}\tdiff={round(t - t_old,2)}")

        sleep(0.1)


test()

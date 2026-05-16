#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <termios.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdlib.h>
void limpaTela() {
    printf("\033[H\033[2J");
}

void enable_raw_mode()
{
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    tcflush(STDIN_FILENO, TCIFLUSH);

    // Entra no modo de tela alternativa e esconde o cursor piscante (\033[?25l)
    printf("\033[?1049h\033[?25l");
    fflush(stdout);
}

void disable_raw_mode()
{
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);

    // Sai da tela alternativa, volta para a tela normal e mostra o cursor (\033[?25h)
    printf("\033[?1049l\033[?25h");
    fflush(stdout);
}
enum SnakePart {
    SNAKE_HEAD = 1,
    SNAKE_BODY = 2,
    SNAKE_TAIL = 3,
};

enum Direction {
    DIRECTION_UP = 'w',
    DIRECTION_DOWN = 's',
    DIRECTION_LEFT = 'a',
    DIRECTION_RIGHT = 'd',
};

void start_tick_loop(int frequency)
{
    long long interval_ns = 1000000000LL / frequency;

    struct timespec sleep_time = {
        .tv_sec = interval_ns / 1000000000LL,
        .tv_nsec = interval_ns % 1000000000LL
    };

    char ultima_tecla = 'w';

    int counter = 0;

    int matriz[40][40] = {0};

    matriz[27][9] = 1;
    matriz[28][9] = 2;
    matriz[29][9] = 3;

    while (1) {
        limpaTela();
        bool found = false;

        ssize_t result = read(STDIN_FILENO, &ultima_tecla, 1);

        if (result < 0) {
            printf("\033[H\033[2J");
        }
        printf("total de frames: %d\n", counter);
        counter++;
        /**
         * O primeiro passo é encontrar onde na matriz está a snake.
         * Por enquanto estou atuando apenas com a identificação da cabeça (SNAKE_HEAD).
         *
         * **/
        for (int i = 0; i < 40; i++) {
            for (int j = 0; j < 40; j++) {
                /**
                 * Esse IF sinaliza que encontramos a posição da SNAKE_HEAD na matriz.
                 * **/
                if (matriz[i][j] ==  SNAKE_HEAD) {
                    /**
                     * E agora que encontramos, precisamos manipular a direção de movimento da SNAKE_HEAD.
                     * Pra isso precisamos etender a ordem de renderização da matriz:
                     * (i) refere-se à linha, enquanto (j) à coluna.
                     *
                     * Supondo que a SNAKE_HEAD esteja na posição i,j = [5,3] e a direção seja de subida,
                     * então o único ponteiro a ser atualizado é o (i), mantendo o (j): [4,3].
                     *
                     * Dessa forma, quando falamos em "subir", estamos decrementando a posição (i).
                     * Por lógica ao descer, estamos incrementando o (i).
                     *
                     * Direita deve incrementar o (j) e não alterar o (i): [4,3]; [4,4]; [4,5].
                     * E da mesma forma, a esquerda decrementa o (j), sem alterar o (i): [4,5]; [4;4], [4;3].
                     *
                     * Essa regra se repete até que implementemos uma regra de colisão. Como não há colisão,
                     * as posições SNAKE ficarão negativos e sumirão da tela, pois nosso MAPA é uma matriz 40X40.
                     *
                     **/
                    if (ultima_tecla == 'w') { // w = CIMA
                        matriz[i][j] = 0;
                        matriz[i-1][j] = 1;
                    }

                    if (ultima_tecla == 's') { // s = BAIXO
                        matriz[i][j] = 0;
                        matriz[i+1][j] = 1;
                    }

                    if (ultima_tecla == 'a') { // a = ESQUERDA
                        matriz[i][j] = 0;
                        matriz[i][j -1] = 1;
                    }

                    if (ultima_tecla == 'd') { // d = DIREITA
                        matriz[i][j] = 0;
                        matriz[i][j + 1] = 1;
                    }

                    /**
                     * Quando encontramos a SNAKE_HEAD precisamos parar de procurá-la na matriz.
                     * Como estamos em um for(j) dentro de outro for(i), precisamos definir
                     * a variável [found] = true, e só então sair do for(j) com break;
                     *
                     **/
                    found = true;
                    break;
                }
                /**
                 * Ao sair do for(j) sinalizando o found = true, então ele será verificado no for(i) que também
                 * forçará sua saída.
                 ***/
                if (found) break;
            }
        }

        /**
         * Aqui estamos desenhando o mapa que é uma matriz 40x40.
         * As células vazias são as células que não contém partes da SNAKE, ou paredes e frutinhas que ainda
         * vamos implementar.
         * Toda célula vazia é printada com um espaço vazio, para que gere "volume" no tabuleiro. Do contrário os
         * espaços vazios não existiriam e o movimento da snake ficaria errado.
         * **/
        for (int i = 0; i < 40; i++) {
            for (int j = 0; j < 40; j++) {
                /**
                 * No passo anterior validamos a posição da HEAD e o seu deslocamento baseado na direção.
                 * Aqui precisamos apenas renderizar a SNAKE_HEAD na matriz do mapa:
                 ***/
                if (matriz[i][j] ==  SNAKE_HEAD) {
                    printf("\033[42m H \033[0m");
                } else {
                    printf("  ");
                }

            }
            printf("\n");
        }

        fflush(stdout);
        nanosleep(&sleep_time, NULL);

    }
}

int escolheDificuldade() {
    struct MenuItem {
        int level;
        char name[10];
    };

    struct MenuItem items[4] = { {1, "Facil"}, {3, "Medio"}, {10, "Dificil"}, {15, "Extreme"} };
    int dificuldade = 0;

    for (int i = 1; i <= 4; i++) {
        printf("%d - %s\n", i, items[i -1].name);
    }

    printf("Escolha a dificuldade: ");

    int difSelecionada;
    scanf("%d", &difSelecionada);

    if (difSelecionada < 1 || difSelecionada > 4) {
        limpaTela();
        printf("\033[37;41mDificuldade Invalida!\033[0m\n");
        escolheDificuldade();
    }

    dificuldade = items[difSelecionada -1 ].level;

    return dificuldade;
}

int main()
{
    limpaTela();
    int dificuldade;

    dificuldade = escolheDificuldade();

    enable_raw_mode();

    start_tick_loop(dificuldade);

    disable_raw_mode();

    return 0;
}
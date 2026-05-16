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
        // Agora limpamos apenas a tela alternativa atual (sem mexer no seu histórico real)
        limpaTela();
        bool found = false;

        ssize_t result = read(STDIN_FILENO, &ultima_tecla, 1);

        if (result < 0) {
            printf("\033[H\033[2J");
        }
        printf("total de frames: %d\n", counter);
        counter++;

        for (int i = 0; i < 40; i++) {
            for (int j = 0; j < 40; j++) {
                if (matriz[i][j] ==  SNAKE_HEAD) {
                    if (ultima_tecla == 'w') {
                        matriz[i][j] = 0;
                        matriz[i-1][j] = 1;
                    }

                    if (ultima_tecla == 's') {
                        matriz[i][j] = 0;
                        matriz[i+1][j] = 1;
                    }

                    if (ultima_tecla == 'a') {
                        matriz[i][j] = 0;
                        matriz[i][j -1] = 1;
                    }

                    if (ultima_tecla == 'd') {
                        matriz[i][j] = 0;
                        matriz[i][j + 1] = 1;
                    }
                    found = true;
                    break;
                }

                if (found) break;
            }
        }

        for (int i = 0; i < 40; i++) {
            for (int j = 0; j < 40; j++) {
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
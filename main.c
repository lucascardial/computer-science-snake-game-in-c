#include <locale.h>
#include <ncurses.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>

#define FPS 30
#define GAME_TICK 8

#define FRAME_TIME_US (1000000 / FPS)
#define GAME_TIME_US  (1000000 / GAME_TICK)

#define MAP_SIZE_X 120
#define MAP_SIZE_Y 30

#define MAP_OFFSET_X 1
#define MAP_OFFSET_Y 6

#define SNAKE_SLOTS ((MAP_SIZE_X - 2) * (MAP_SIZE_Y - 2))

enum ELEMENTS {
    EMPTY_SPACE_ID,
    TOP_RIGHT_CORNER_ID,
    BOTTOM_RIGHT_CORNER_ID,
    BOTTOM_LEFT_CORNER_ID,
    TOP_LEFT_CORNER_ID,
    HORIZONTAL_BORDER_ID,
    VERTICAL_BORDER_ID,
    SNAKE_HEAD_ID,
    SNAKE_HEAD_CLOSE_FOOD_ID,
    SNAKE_BODY_ID,
    FOOD_ID
};

enum DIRECTIONS {
    DIRECTION_UP,
    DIRECTION_DOWN,
    DIRECTION_LEFT,
    DIRECTION_RIGHT
};

typedef struct {
    int x;
    int y;
} Position;

typedef struct {
    int map[MAP_SIZE_Y][MAP_SIZE_X];

    Position snake[SNAKE_SLOTS];
    int snake_size;

    Position food;

    enum DIRECTIONS direction;

    int score;
    bool running;
    bool game_over;
} GameState;

long now_us(void)
{
    struct timespec ts;

    clock_gettime(CLOCK_MONOTONIC, &ts);

    return ts.tv_sec * 1000000L + ts.tv_nsec / 1000L;
}

int random_int(int min, int max)
{
    return (rand() % (max - min + 1)) + min;
}

bool samePosition(Position a, Position b)
{
    return a.x == b.x && a.y == b.y;
}

bool isWall(Position position)
{
    return position.x <= 0 ||
           position.y <= 0 ||
           position.x >= MAP_SIZE_X - 1 ||
           position.y >= MAP_SIZE_Y - 1;
}

bool isSnakePosition(GameState *game, Position position)
{
    for (int i = 0; i < game->snake_size; i++) {
        if (samePosition(game->snake[i], position)) {
            return true;
        }
    }

    return false;
}

bool isCloseToFood(GameState *game)
{
    Position head = game->snake[0];

    return (head.x == game->food.x && head.y == game->food.y - 1) ||
           (head.x == game->food.x && head.y == game->food.y + 1) ||
           (head.x == game->food.x - 1 && head.y == game->food.y) ||
           (head.x == game->food.x + 1 && head.y == game->food.y);
}

Position getNextHeadPosition(GameState *game)
{
    Position next = game->snake[0];

    if (game->direction == DIRECTION_UP) {
        next.y--;
    } else if (game->direction == DIRECTION_DOWN) {
        next.y++;
    } else if (game->direction == DIRECTION_LEFT) {
        next.x--;
    } else if (game->direction == DIRECTION_RIGHT) {
        next.x++;
    }

    return next;
}

bool wouldCollideWithBody(GameState *game, Position next_head, bool will_grow)
{
    /*
     * Se a cobra NÃO vai crescer, a última posição do corpo será liberada.
     * Então é permitido a cabeça entrar onde hoje está a cauda.
     */
    int limit = game->snake_size;

    if (!will_grow) {
        limit = game->snake_size - 1;
    }

    for (int i = 1; i < limit; i++) {
        if (samePosition(game->snake[i], next_head)) {
            return true;
        }
    }

    return false;
}

void spawnFood(GameState *game)
{
    while (true) {
        Position food = {
            .x = random_int(1, MAP_SIZE_X - 2),
            .y = random_int(1, MAP_SIZE_Y - 2),
        };

        if (!isSnakePosition(game, food)) {
            game->food = food;
            return;
        }
    }
}

void resetMap(GameState *game)
{
    for (int y = 0; y < MAP_SIZE_Y; y++) {
        for (int x = 0; x < MAP_SIZE_X; x++) {
            game->map[y][x] = EMPTY_SPACE_ID;
        }
    }
}

void generateWalls(GameState *game)
{
    for (int y = 0; y < MAP_SIZE_Y; y++) {
        for (int x = 0; x < MAP_SIZE_X; x++) {
            if (x == 0 && y == 0) {
                game->map[y][x] = TOP_LEFT_CORNER_ID;
            } else if (x == MAP_SIZE_X - 1 && y == 0) {
                game->map[y][x] = TOP_RIGHT_CORNER_ID;
            } else if (x == 0 && y == MAP_SIZE_Y - 1) {
                game->map[y][x] = BOTTOM_LEFT_CORNER_ID;
            } else if (x == MAP_SIZE_X - 1 && y == MAP_SIZE_Y - 1) {
                game->map[y][x] = BOTTOM_RIGHT_CORNER_ID;
            } else if (x == 0 || x == MAP_SIZE_X - 1) {
                game->map[y][x] = VERTICAL_BORDER_ID;
            } else if (y == 0 || y == MAP_SIZE_Y - 1) {
                game->map[y][x] = HORIZONTAL_BORDER_ID;
            }
        }
    }
}

void generateFood(GameState *game)
{
    game->map[game->food.y][game->food.x] = FOOD_ID;
}

void generateSnake(GameState *game)
{
    for (int i = 0; i < game->snake_size; i++) {
        Position position = game->snake[i];

        if (i == 0) {
            game->map[position.y][position.x] = isCloseToFood(game)
                ? SNAKE_HEAD_CLOSE_FOOD_ID
                : SNAKE_HEAD_ID;
        } else {
            game->map[position.y][position.x] = SNAKE_BODY_ID;
        }
    }
}

void buildMap(GameState *game)
{
    resetMap(game);
    generateWalls(game);
    generateFood(game);
    generateSnake(game);
}

void moveSnake(GameState *game)
{
    Position next_head = getNextHeadPosition(game);

    bool will_grow = samePosition(next_head, game->food);

    if (isWall(next_head) || wouldCollideWithBody(game, next_head, will_grow)) {
        game->game_over = true;
        game->running = false;
        return;
    }

    if (will_grow) {
        if (game->snake_size < SNAKE_SLOTS) {
            game->snake_size++;
        }

        game->score++;
    }

    for (int i = game->snake_size - 1; i > 0; i--) {
        game->snake[i] = game->snake[i - 1];
    }

    game->snake[0] = next_head;

    if (will_grow) {
        spawnFood(game);
    }
}

bool isOppositeDirection(enum DIRECTIONS current, enum DIRECTIONS next)
{
    return (current == DIRECTION_UP && next == DIRECTION_DOWN) ||
           (current == DIRECTION_DOWN && next == DIRECTION_UP) ||
           (current == DIRECTION_LEFT && next == DIRECTION_RIGHT) ||
           (current == DIRECTION_RIGHT && next == DIRECTION_LEFT);
}

void updateDirection(GameState *game, enum DIRECTIONS next)
{
    if (!isOppositeDirection(game->direction, next)) {
        game->direction = next;
    }
}

void handleInput(GameState *game)
{
    int ch = getch();

    if (ch == ERR) {
        return;
    }

    switch (ch) {
        case 'q':
            game->running = false;
            break;

        case 'w':
        case KEY_UP:
            updateDirection(game, DIRECTION_UP);
            break;

        case 's':
        case KEY_DOWN:
            updateDirection(game, DIRECTION_DOWN);
            break;

        case 'a':
        case KEY_LEFT:
            updateDirection(game, DIRECTION_LEFT);
            break;

        case 'd':
        case KEY_RIGHT:
            updateDirection(game, DIRECTION_RIGHT);
            break;

        default:
            break;
    }
}

void drawScreen(GameState *game)
{
    const char *elements[11] = {
        [EMPTY_SPACE_ID] = ".",
        [TOP_LEFT_CORNER_ID] = "╔",
        [TOP_RIGHT_CORNER_ID] = "╗",
        [BOTTOM_RIGHT_CORNER_ID] = "╝",
        [BOTTOM_LEFT_CORNER_ID] = "╚",
        [HORIZONTAL_BORDER_ID] = "═",
        [VERTICAL_BORDER_ID] = "║",
        [SNAKE_HEAD_ID] = "O",
        [SNAKE_HEAD_CLOSE_FOOD_ID] = "P",
        [SNAKE_BODY_ID] = "o",
        [FOOD_ID] = "Q",
    };

    for (int y = 0; y < MAP_SIZE_Y; y++) {
        for (int x = 0; x < MAP_SIZE_X; x++) {
            int element = game->map[y][x];

            if (element != EMPTY_SPACE_ID) {
                mvprintw(
                    y + MAP_OFFSET_Y,
                    x + MAP_OFFSET_X,
                    "%s",
                    elements[element]
                );
            }
        }
    }
}

void drawHud(GameState *game)
{
    mvprintw(0, 0, "FPS: %d", FPS);
    mvprintw(1, 0, "GAME TICK: %d", GAME_TICK);
    mvprintw(2, 0, "Score: %d", game->score);
    mvprintw(3, 0, "Snake size: %d", game->snake_size);
    mvprintw(4, 0, "Head: x=%d y=%d", game->snake[0].x, game->snake[0].y);
    mvprintw(5, 0, "Controls: W A S D / arrows | q to quit");
}

void initSnake(GameState *game)
{
    int snake_x = random_int(5, MAP_SIZE_X - 6);
    int snake_y = random_int(5, MAP_SIZE_Y - 6);

    game->snake_size = 3;
    game->direction = DIRECTION_LEFT;

    game->snake[0] = (Position){ .x = snake_x,     .y = snake_y };
    game->snake[1] = (Position){ .x = snake_x + 1, .y = snake_y };
    game->snake[2] = (Position){ .x = snake_x + 2, .y = snake_y };
}

void initGame(GameState *game)
{
    game->score = 0;
    game->running = true;
    game->game_over = false;

    initSnake(game);
    spawnFood(game);
    buildMap(game);
}

void initNcurses(void)
{
    setlocale(LC_ALL, "");

    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);
}

int main(void)
{
    srand((unsigned int) time(NULL));

    initNcurses();

    GameState game;
    initGame(&game);

    long last_frame = now_us();
    long last_game_tick = now_us();

    while (game.running) {
        handleInput(&game);

        long current = now_us();

        if (current - last_game_tick >= GAME_TIME_US) {
            last_game_tick = current;

            moveSnake(&game);
            buildMap(&game);
        }

        if (current - last_frame >= FRAME_TIME_US) {
            last_frame = current;

            erase();

            drawHud(&game);
            drawScreen(&game);

            refresh();
        }

        usleep(1000);
    }

    erase();

    if (game.game_over) {
        mvprintw(0, 0, "Game Over!");
        mvprintw(1, 0, "Score: %d", game.score);
        mvprintw(2, 0, "Press any key to exit...");

        nodelay(stdscr, FALSE);
        getch();
    }

    endwin();

    return 0;
}

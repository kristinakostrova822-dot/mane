#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <ctype.h>

#ifdef _WIN32
    #include <windows.h>
    #define CP_UTF8 65001
#else
    #define CP_UTF8 0
#endif

#define INVENTORY_SIZE 10
#define MAX_ITEMS 10
#define MAX_NAME_LEN 32
#define MAX_FARMER_NAME 64
#define MAX_LINE_LEN 256

#define STEP_STAMINA_COST    1   /* расход выносливости за один шаг */
#define FLOOR_STAMINA_COST   5   /* расход за переход на новый этаж */
#define BASE_FLOOR_REWARD    10  /* базовый множитель награды за этаж */
#define BUFF_CHANCE_PERCENT  5   /* шанс получить эффект за шаг */
#define BUFF_DURATION_STEPS  10  /* длительность эффекта в шагах */
#define POISON_HP_PER_STEP   2   /* урон от отравления за шаг */
#define START_MAX_HP         100
#define START_MAX_STAMINA    50
#define SLEEP_HOUR           8   /* пробуждение */
#define CURFEW_HOUR          2   /* час ночи, дальше которого нельзя ждать */

/* ID предметов */
#define ITEM_EMPTY   0
#define ITEM_WOOD    1
#define ITEM_STONE   2
#define ITEM_SEEDS   3
#define ITEM_IRON    4
#define ITEM_GOLD    5
#define ITEM_APPLE   6
#define ITEM_POTION  7
#define ITEM_ROPE    8
#define ITEM_TORCH   9

/* Символы карты */
#define CH_WALL '#'
#define CH_FLOOR '.'
#define CH_TREE 'T'
#define CH_ROCK 'R'
#define CH_ICE '~'
#define CH_HERO '@'
#define CH_STAIRS_DOWN 'D'
#define CH_STAIRS_UP 'U'

typedef enum {
    MENU_EXIT = 0, MENU_CREATE = 1, MENU_CARD = 2, MENU_TIME = 3,
    MENU_SHOW_INV = 4, MENU_PUT = 5, MENU_DROP = 6, MENU_CLEAR_ID = 7,
    MENU_FIND = 8, MENU_DIARY = 9, MENU_ADVENTURE = 10, MENU_LETTER = 11
} MenuCode;

typedef enum {
    TIME_BACK = 0, TIME_CLOCK = 1, TIME_WAIT = 2, TIME_SLEEP = 3
} TimeMenuCode;

typedef enum {
    DIR_NONE = 0, DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT
} Direction;

typedef enum {
    TILE_UNKNOWN = 0, TILE_WALL, TILE_FLOOR, TILE_TREE, TILE_ROCK,
    TILE_ICE, TILE_HERO, TILE_STAIRS_DOWN, TILE_STAIRS_UP
} TileType;

typedef enum {
    EFFECT_NONE = 0, EFFECT_SPRAIN, EFFECT_POISON, EFFECT_VIGOR
} StatusEffect;


typedef struct {
    StatusEffect type;      /* активный эффект */
    int remaining_steps;    /* сколько шагов он ещё действует */
} Buff;

typedef struct {
    char name[MAX_FARMER_NAME];
    int hp;
    int max_hp;
    int stamina;
    int max_stamina;
    int gold;
    int x;                  /* координата X на карте (столбец) */
    int y;                  /* координата Y на карте (строка) */
    int inventory[INVENTORY_SIZE];
    Buff buff;
} Player;

typedef struct {
    char** grid;            /* указатель на двумерный динамический массив */
    int rows;
    int cols;
    int hero_r, hero_c;
    int stairs_down_r, stairs_down_c;
    int stairs_up_r, stairs_up_c;
    int floor_level;        /* счётчик пройденных этажей */
} Map;

const char* default_names[MAX_ITEMS] = {
    "Пусто", "Дерево", "Камень", "Семена", "Железо",
    "Золото", "Яблоко", "Зелье", "Верёвка", "Факел"
};

/* Каталог названий предметов (заполняется в main из items.txt) */
static char g_item_names[MAX_ITEMS][MAX_NAME_LEN];

const char* effect_names[] = { "Нет", "Растяжение", "Отравление", "Бодрость" };

TileType char_to_tile(char c) {
    switch (c) {
        case CH_WALL:        return TILE_WALL;
        case CH_FLOOR:       return TILE_FLOOR;
        case CH_TREE:        return TILE_TREE;
        case CH_ROCK:        return TILE_ROCK;
        case CH_ICE:         return TILE_ICE;
        case CH_HERO:        return TILE_HERO;
        case CH_STAIRS_DOWN: return TILE_STAIRS_DOWN;
        case CH_STAIRS_UP:   return TILE_STAIRS_UP;
        default:             return TILE_UNKNOWN;
    }
}

const char* tile_description(char c) {
    switch (char_to_tile(c)) {
        case TILE_WALL:        return "стена";
        case TILE_TREE:        return "дерево";
        case TILE_ROCK:        return "камень";
        case TILE_ICE:         return "лёд";
        case TILE_STAIRS_DOWN: return "лестница вниз";
        case TILE_STAIRS_UP:   return "лестница вверх";
        case TILE_FLOOR:       return "пол";
        default:               return "неизвестность";
    }
}

void get_farmer_name(char* name) {
    printf("Введите имя фермера: ");
    if (!fgets(name, MAX_FARMER_NAME, stdin)) {
        name[0] = '\0';
        return;
    }
    size_t len = strlen(name);
    if (len > 0 && name[len - 1] == '\n') name[len - 1] = '\0';
    if (strlen(name) == 0) strcpy(name, "Фермер");
}

int read_int(const char* prompt, int* out) {
    printf("%s", prompt);
    int v;
    if (scanf("%d", &v) != 1) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {}
        return 0;
    }
    int c;
    while ((c = getchar()) != '\n' && c != EOF) {}
    *out = v;
    return 1;
}

int load_item_names(char item_names[MAX_ITEMS][MAX_NAME_LEN]) {
    FILE* f = fopen("items.txt", "r");
    if (!f) {
        printf("[Предупреждение] Файл items.txt не найден. Используем стандартные названия.\n");
        for (int i = 0; i < MAX_ITEMS; i++) {
            strncpy(item_names[i], default_names[i], MAX_NAME_LEN - 1);
            item_names[i][MAX_NAME_LEN - 1] = '\0';
        }
        return 0;
    }

    char line[MAX_NAME_LEN];
    int count = 0;
    while (count < MAX_ITEMS && fgets(line, sizeof(line), f)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        strncpy(item_names[count], line, MAX_NAME_LEN - 1);
        item_names[count][MAX_NAME_LEN - 1] = '\0';
        count++;
    }
    fclose(f);

    while (count < MAX_ITEMS) {
        strncpy(item_names[count], default_names[count], MAX_NAME_LEN - 1);
        item_names[count][MAX_NAME_LEN - 1] = '\0';
        count++;
    }
    return 1;
}

int find_id_by_name(const char item_names[MAX_ITEMS][MAX_NAME_LEN], const char* search_name) {
    for (int i = 0; i < MAX_ITEMS; i++) {
        if (strcmp(item_names[i], search_name) == 0) return i;
    }
    return -1;
}


Player* create_player(void) {
    Player* p = (Player*)malloc(sizeof(Player));
    if (!p) {
        printf("Не хватило памяти под персонажа!\n");
        return NULL;
    }

    get_farmer_name(p->name);
    p->max_hp = START_MAX_HP;
    p->hp = START_MAX_HP;
    p->max_stamina = START_MAX_STAMINA;
    p->stamina = START_MAX_STAMINA;
    p->gold = 0;
    p->x = 0;
    p->y = 0;
    p->buff.type = EFFECT_NONE;
    p->buff.remaining_steps = 0;

    for (int i = 0; i < INVENTORY_SIZE; i++) p->inventory[i] = ITEM_EMPTY;
    p->inventory[0] = ITEM_APPLE; /* стартовый запас: тег <ITEM> в письме покажет его */

    printf("Персонаж создан по адресу %p. Удачи, %s!\n", (void*)p, p->name);
    return p;
}

void destroy_player(Player** p) {
    if (p && *p) {
        printf("Освобождаю память персонажа по адресу %p...\n", (void*)*p);
        free(*p);
        *p = NULL;
    }
}

void print_card(const Player* p, int day, int hour) {
    printf("\n--- Карточка персонажа ---\n");
    printf("Имя:          %s\n", p->name);
    printf("Здоровье:     %d/%d\n", p->hp, p->max_hp);
    printf("Выносливость: %d/%d\n", p->stamina, p->max_stamina);
    printf("Золото:       %d\n", p->gold);
    printf("Активный эффект: %s", effect_names[p->buff.type]);
    if (p->buff.type != EFFECT_NONE) {
        printf(" (осталось шагов: %d)", p->buff.remaining_steps);
    }
    printf("\nТекущее время: День %d, %02d:00\n", day, hour);
    printf("Инвентарь:\n");
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        printf("  Слот %d: [%d] - %s\n", i, p->inventory[i], g_item_names[p->inventory[i]]);
    }
    printf("--------------------------\n");
}


void advance_hours(int* day, int* hour, int hours) {
    *hour += hours;
    while (*hour >= 24) {
        *hour -= 24;
        (*day)++;
    }
}


void write_diary(const Player* p, int day, int hour) {
    FILE* diary = fopen("diary.txt", "a");
    if (!diary) {
        printf("Не удалось открыть diary.txt для записи.\n");
        return;
    }

    fprintf(diary, "=== Дневник фермера ===\n");
    fprintf(diary, "Имя фермера: %s\n", p->name);
    fprintf(diary, "Дата: День %d, %02d:00\n", day, hour);
    fprintf(diary, "Здоровье: %d/%d, Выносливость: %d/%d, Золото: %d\n",
            p->hp, p->max_hp, p->stamina, p->max_stamina, p->gold);
    fprintf(diary, "Инвентарь:\n");
    for (int i = 0; i < INVENTORY_SIZE; i++) {
        fprintf(diary, "Слот %d: [%d] - %s\n", i, p->inventory[i], g_item_names[p->inventory[i]]);
    }
    fprintf(diary, "========================\n\n");
    fclose(diary);
}

void generate_letter(const Player* p, int current_day) {
    FILE* fin = fopen("input.txt", "r");
    if (!fin) {
        printf("[Ошибка] Файл input.txt не найден! Создайте его с текстом письма и тегами <NAME>, <DAY>, <ITEM>.\n");
        return;
    }

    FILE* fout = fopen("output.txt", "w");
    if (!fout) {
        printf("Не удалось создать output.txt.\n");
        fclose(fin);
        return;
    }

    char line[MAX_LINE_LEN];
    const char* item_in_slot0 = g_item_names[p->inventory[0]];

    while (fgets(line, sizeof(line), fin)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }

        char result[MAX_LINE_LEN * 2];
        int pos = 0;

        for (size_t i = 0; i <= len; i++) {
            if (i + 6 <= len && strncmp(&line[i], "<NAME>", 6) == 0) {
                pos += sprintf(&result[pos], "%s", p->name);
                i += 5;
                continue;
            }
            else if (i + 5 <= len && strncmp(&line[i], "<DAY>", 5) == 0) {
                pos += sprintf(&result[pos], "%d", current_day);
                i += 4;
                continue;
            }
            else if (i + 6 <= len && strncmp(&line[i], "<ITEM>", 6) == 0) {
                pos += sprintf(&result[pos], "%s", item_in_slot0);
                i += 5;
                continue;
            }

            result[pos++] = line[i];
            if (pos >= (int)(sizeof(result) - 1)) break;
        }
        result[pos] = '\0';

        fprintf(fout, "%s\n", result);
    }

    fclose(fin);
    fclose(fout);
    printf("Письмо сгенерировано в output.txt\n");
}

/* ===================== Карта подземелья ===================== */
Map create_dungeon_map(int rows, int cols, int floor_level) {
    Map m;
    m.rows = rows;
    m.cols = cols;
    m.floor_level = floor_level;
    m.grid = (char**)malloc(rows * sizeof(char*));
    for (int r = 0; r < rows; r++) {
        m.grid[r] = (char*)malloc(cols * sizeof(char));
    }

    /* Периметр — стены */
    for (int c = 0; c < cols; c++) {
        m.grid[0][c] = CH_WALL;
        m.grid[rows - 1][c] = CH_WALL;
    }
    for (int r = 1; r < rows - 1; r++) {
        m.grid[r][0] = CH_WALL;
        m.grid[r][cols - 1] = CH_WALL;
    }

    /* Внутренность: пол, препятствия, лёд */
    for (int r = 1; r < rows - 1; r++) {
        for (int c = 1; c < cols - 1; c++) {
            double rnd = (double)rand() / RAND_MAX;
            if (rnd < 0.10)      m.grid[r][c] = CH_TREE;
            else if (rnd < 0.20) m.grid[r][c] = CH_ROCK;
            else if (rnd < 0.35) m.grid[r][c] = CH_ICE;
            else                 m.grid[r][c] = CH_FLOOR;
        }
    }

    /* Точка старта: случайная свободная клетка */
    int sr, sc;
    do {
        sr = 1 + rand() % (rows - 2);
        sc = 1 + rand() % (cols - 2);
    } while (m.grid[sr][sc] != CH_FLOOR && m.grid[sr][sc] != CH_ICE);

    m.hero_r = sr;
    m.hero_c = sc;

    /* Лестница вниз D: в любой свободной клетке кроме старта */
    int dr, dc;
    do {
        dr = 1 + rand() % (rows - 2);
        dc = 1 + rand() % (cols - 2);
    } while ((dr == sr && dc == sc) ||
             char_to_tile(m.grid[dr][dc]) == TILE_WALL ||
             char_to_tile(m.grid[dr][dc]) == TILE_TREE ||
             char_to_tile(m.grid[dr][dc]) == TILE_ROCK);
    m.stairs_down_r = dr;
    m.stairs_down_c = dc;
    m.grid[dr][dc] = CH_STAIRS_DOWN;

    /* Лестница вверх U: рядом со стартом (одна из 4 соседних клеток) */
    int ur = -1, uc = -1;
    int candidates[4][2] = { {-1,0},{1,0},{0,-1},{0,1} };
    int tried = 0;
    while (tried < 4) {
        int idx = rand() % 4;
        ur = sr + candidates[idx][0];
        uc = sc + candidates[idx][1];
        if (ur >= 1 && ur <= rows - 2 && uc >= 1 && uc <= cols - 2) {
            TileType t = char_to_tile(m.grid[ur][uc]);
            if (t != TILE_WALL && t != TILE_TREE && t != TILE_ROCK) break;
        }
        tried++;
    }
    /* Если рядом не получилось — где-то в свободной клетке */
    if (ur == -1 || uc == -1) {
        do {
            ur = 1 + rand() % (rows - 2);
            uc = 1 + rand() % (cols - 2);
        } while ((ur == sr && uc == sc) || (ur == dr && uc == dc) ||
                 char_to_tile(m.grid[ur][uc]) == TILE_WALL ||
                 char_to_tile(m.grid[ur][uc]) == TILE_TREE ||
                 char_to_tile(m.grid[ur][uc]) == TILE_ROCK);
    }
    m.stairs_up_r = ur;
    m.stairs_up_c = uc;
    m.grid[ur][uc] = CH_STAIRS_UP;

    m.grid[m.hero_r][m.hero_c] = CH_HERO;

    return m;
}

void free_map(Map* m) {
    if (!m->grid) return;
    for (int r = 0; r < m->rows; r++) free(m->grid[r]);
    free(m->grid);
    m->grid = NULL;
    m->rows = m->cols = 0;
}

void print_map(const Map* m) {
    for (int r = 0; r < m->rows; r++) {
        for (int c = 0; c < m->cols; c++) putchar(m->grid[r][c]);
        putchar('\n');
    }
}

/* HUD: строка состояния над вводом хода */
void print_hud(const Player* p, const Map* m, int day, int hour) {
    printf("HP: %d/%d | Выносливость: %d/%d | Золото: %d | Этаж: %d | Время: День %d, %02d:00",
           p->hp, p->max_hp, p->stamina, p->max_stamina, p->gold,
           m->floor_level, day, hour);
    if (p->buff.type != EFFECT_NONE) {
        printf(" | Эффект: %s (%d)", effect_names[p->buff.type], p->buff.remaining_steps);
    }
    printf("\n");
}

/*
   Возвращает:
   0 — нельзя пойти (в out_blocked — символ клетки-препятствия, либо '\0' для границы)
   1 — обычный успешный ход (в out_steps — сколько клеток пройдено, включая лёд)
   2 — ход на лестницу вниз (переход на новый этаж)
   3 — ход на лестницу вверх
*/
int move_hero_dungeon(Map* m, Direction dir, int* out_steps, char* out_blocked) {
    *out_steps = 0;
    *out_blocked = '\0';

    int dr = 0, dc = 0;
    switch (dir) {
        case DIR_UP:    dr = -1; break;
        case DIR_DOWN:  dr =  1; break;
        case DIR_LEFT:  dc = -1; break;
        case DIR_RIGHT: dc =  1; break;
        default: return 0;
    }

    int nr = m->hero_r + dr;
    int nc = m->hero_c + dc;

    if (nr < 0 || nr >= m->rows || nc < 0 || nc >= m->cols) {
        *out_blocked = CH_WALL; /* граница карты ведёт себя как стена */
        return 0;
    }

    char target = m->grid[nr][nc];
    TileType t = char_to_tile(target);

    if (t == TILE_WALL || t == TILE_TREE || t == TILE_ROCK) {
        *out_blocked = target;
        return 0;
    }

    m->grid[m->hero_r][m->hero_c] = CH_FLOOR;

    int steps = 0;

    if (t == TILE_ICE) {
        /* Первый шаг на лёд */
        m->hero_r = nr;
        m->hero_c = nc;
        m->grid[m->hero_r][m->hero_c] = CH_HERO;
        steps++;

        /* Скольжение дальше в том же направлении */
        while (1) {
            int sr = m->hero_r + dr;
            int sc = m->hero_c + dc;

            if (sr < 0 || sr >= m->rows || sc < 0 || sc >= m->cols) {
                *out_steps = steps;
                return 1;
            }

            char next = m->grid[sr][sc];
            TileType nt = char_to_tile(next);
            if (nt == TILE_WALL || nt == TILE_TREE || nt == TILE_ROCK) {
                *out_steps = steps; /* врезались, герой остаётся перед препятствием */
                return 1;
            }

            m->grid[m->hero_r][m->hero_c] = CH_FLOOR;
            m->hero_r = sr;
            m->hero_c = sc;
            steps++;
            m->grid[m->hero_r][m->hero_c] = CH_HERO;

            if (nt != TILE_ICE) break;
        }
    } else {
        m->hero_r = nr;
        m->hero_c = nc;
        m->grid[m->hero_r][m->hero_c] = CH_HERO;
        steps = 1;
    }

    *out_steps = steps;

    if (m->hero_r == m->stairs_down_r && m->hero_c == m->stairs_down_c) return 2;
    if (m->hero_r == m->stairs_up_r && m->hero_c == m->stairs_up_c) return 3;

    return 1;
}

/* Синхронизация координат: строка карты = y, столбец = x */
void sync_player_coords(Player* p, const Map* m) {
    p->y = m->hero_r;
    p->x = m->hero_c;
}

/* 5% шанс получить случайный эффект за шаг */
void roll_buff(Player* p) {
    if (rand() % 100 < BUFF_CHANCE_PERCENT) {
        StatusEffect new_effect = (StatusEffect)(EFFECT_SPRAIN + rand() % 3);
        if (p->buff.type != EFFECT_NONE && p->buff.type != new_effect) {
            printf("Старый эффект (%s) сменился новым!\n", effect_names[p->buff.type]);
        }
        p->buff.type = new_effect;
        p->buff.remaining_steps = BUFF_DURATION_STEPS;
        printf("Случилось! Вы получили эффект: %s на %d шагов.\n",
               effect_names[p->buff.type], p->buff.remaining_steps);
    }
}

/* Тик активного эффекта после шага */
void tick_buff(Player* p) {
    if (p->buff.type == EFFECT_NONE) return;

    if (p->buff.type == EFFECT_POISON) {
        p->hp -= POISON_HP_PER_STEP;
        printf("Отравление: -%d HP. Здоровье: %d/%d\n", POISON_HP_PER_STEP, p->hp, p->max_hp);
    }

    p->buff.remaining_steps--;
    if (p->buff.remaining_steps <= 0) {
        printf("Эффект '%s' закончился.\n", effect_names[p->buff.type]);
        p->buff.type = EFFECT_NONE;
        p->buff.remaining_steps = 0;
    }
}

/* Стоимость одного шага с учётом активного эффекта */
int step_cost(const Player* p) {
    switch (p->buff.type) {
        case EFFECT_VIGOR:  return 0;
        case EFFECT_SPRAIN: return STEP_STAMINA_COST * 2;
        default:            return STEP_STAMINA_COST;
    }
}

/* Экстренная эвакуация: обморок, спасатели забирают половину золота */
void faint_and_evacuate(Player* p, Map* m, const char* reason) {
    printf("\n*** %s ***\n", reason);
    printf("%s теряет сознание! Спасательная служба доставляет героя домой.\n", p->name);
    int lost = p->gold / 2;
    p->gold -= lost;
    printf("За услуги спасателей забрано %d золота. Осталось: %d.\n", lost, p->gold);
    free_map(m);
}

void adventure_mode(Player* p, int* current_day, int* current_hour) {
    /* Допуск к походу */
    if (p->stamina <= 0) {
        printf("Выносливость на нуле — сначала отдохните (пункт 3, «Лечь спать»).\n");
        return;
    }
    if (*current_hour == CURFEW_HOUR) {
        printf("Уже %02d:00 ночи! Отправляться в поход нельзя — ложитесь спать.\n", CURFEW_HOUR);
        return;
    }

    int map_size = 5 + rand() % 26;
    Map current_map = create_dungeon_map(map_size, map_size, 1);
    int running = 1;
    int earned = 0;   /* золото, заработанное в этом походе */
    int final_floors = 1;

    printf("\n=== ПОДЗЕМЕЛЬЕ: %s спускается в темноту! ===\n", p->name);
    printf("W - вверх, A - влево, S - вниз, D - вправо\n");
    printf("Q - подняться по лестнице U и уйти домой (только стоя на U)\n");
    printf("Символы: @ - герой, # - стены, T - деревья, R - камни, ~ - лёд, D - вниз, U - вверх\n");

    while (running) {
        sync_player_coords(p, &current_map);
        print_map(&current_map);
        print_hud(p, &current_map, *current_day, *current_hour);
        printf("Выберите направление (WASDQ): ");

        char input[16];
        if (!fgets(input, sizeof(input), stdin)) break;
        char dir_char = (char)toupper((unsigned char)input[0]);

        Direction dir = DIR_NONE;
        switch (dir_char) {
            case 'W': dir = DIR_UP; break;
            case 'A': dir = DIR_LEFT; break;
            case 'S': dir = DIR_DOWN; break;
            case 'D': dir = DIR_RIGHT; break;
            case 'Q':
                if (current_map.hero_r == current_map.stairs_up_r &&
                    current_map.hero_c == current_map.stairs_up_c) {
                    /* Штатное возвращение: золото остаётся у героя */
                    printf("Вы поднялись по лестнице и вернулись домой!\n");
                    printf("Заработано за поход: %d золота. Итого: %d.\n", earned, p->gold);
                    running = 0;
                } else {
                    printf("Нельзя уйти домой, пока вы не на лестнице вверх (U).\n");
                }
                continue;
            default:
                printf("Неверное направление. Используйте W, A, S, D или Q.\n");
                continue;
        }

        int steps = 0;
        char blocked = '\0';
        int result = move_hero_dungeon(&current_map, dir, &steps, &blocked);

        if (result == 0) {
            printf("Нельзя туда пойти — там %s.\n", tile_description(blocked));
            continue;
        }

        /* Расход выносливости за каждый пройденный шаг (лёд = несколько клеток) */
        int cost = steps * step_cost(p);
        p->stamina -= cost;
        if (cost > 0) {
            printf("Пройдено шагов: %d, потрачено выносливости: %d. Осталось: %d\n",
                   steps, cost, p->stamina);
        } else {
            printf("Бодрость позволяет идти бесплатно!\n");
        }

        /* Тик эффекта: отравление бьёт, счётчик шагов уменьшается */
        tick_buff(p);

        /* Шанс получить новый эффект */
        roll_buff(p);

        /* Переход на новый этаж */
        if (result == 2) {
            final_floors = current_map.floor_level + 1;
            free_map(&current_map);
            map_size = 5 + rand() % 26;
            current_map = create_dungeon_map(map_size, map_size, final_floors);

            p->stamina -= FLOOR_STAMINA_COST;
            advance_hours(current_day, current_hour, 1);

            int reward = BASE_FLOOR_REWARD * final_floors;
            p->gold += reward;
            earned += reward;

            printf("\nВы спустились на этаж %d!\n", final_floors);
            printf("Награда за исследование: %d золота (итого за поход: %d).\n", reward, earned);
            printf("Переход стоит %d выносливости (осталось %d) и 1 час времени.\n",
                   FLOOR_STAMINA_COST, p->stamina);
        }
        else if (result == 3) {
            printf("Вы на лестнице вверх (U). Нажмите Q, чтобы подняться домой.\n");
        }

        /* Проверка на обморок: выносливость, время, здоровье */
        if (p->hp <= 0) {
            faint_and_evacuate(p, &current_map, "Здоровье на нуле!");
            running = 0;
        } else if (p->stamina <= 0) {
            faint_and_evacuate(p, &current_map, "Выносливость иссякла — герой падает без сил!");
            running = 0;
        } else if (*current_hour == CURFEW_HOUR) {
            faint_and_evacuate(p, &current_map, "Часы пробили 02:00 — слишком поздно!");
            running = 0;
        }
    }

    /* Карта могла быть уже освобождена при обмороке — не освобождаем дважды */
    if (current_map.grid) free_map(&current_map);

    printf("Приключение завершено. %s дома. Пройдено этажей: %d. Время: День %d, %02d:00\n",
           p->name, final_floors, *current_day, *current_hour);
}

void time_submenu(Player* p, int* day, int* hour) {
    int choice = -1;
    while (choice != TIME_BACK) {
        printf("\n=== Время и отдых (День %d, %02d:00) ===\n", *day, *hour);
        printf("[0] Назад\n");
        printf("[1] Посмотреть на часы\n");
        printf("[2] Пропустить время (Ждать)\n");
        printf("[3] Отдых (Лечь спать)\n");
        printf("Выберите пункт: ");

        if (!read_int("", &choice)) { choice = -1; continue; }

        switch ((TimeMenuCode)choice) {
            case TIME_BACK:
                break;

            case TIME_CLOCK:
                printf("Текущее время: День %d, %02d:00\n", *day, *hour);
                break;

            case TIME_WAIT: {
                if (!p) {
                    printf("Персонаж не создан. Сначала пункт [1].\n");
                    break;
                }
                if (*hour == CURFEW_HOUR) {
                    printf("Уже %02d:00 ночи — ждать нельзя, только спать!\n", CURFEW_HOUR);
                    break;
                }

                /* Сколько часов можно докрутить до ближайших 02:00 */
                int allowed;
                if (*hour < CURFEW_HOUR) allowed = CURFEW_HOUR - *hour;
                else allowed = (24 - *hour) + CURFEW_HOUR;

                int hours;
                if (!read_int("Сколько часов ждём? ", &hours)) {
                    printf("Ошибка ввода!\n");
                    break;
                }
                if (hours < 1) {
                    printf("Ждать можно минимум 1 час.\n");
                    break;
                }
                if (hours > allowed) {
                    printf("Ночью фермер должен спать! Через границу %02d:00 без сна нельзя (максимум %d ч).\n",
                           CURFEW_HOUR, allowed);
                    break;
                }

                advance_hours(day, hour, hours);
                printf("Прошло %d ч. Теперь: День %d, %02d:00\n", hours, *day, *hour);
                break;
            }

            case TIME_SLEEP: {
                if (!p) {
                    printf("Персонаж не создан. Сначала пункт [1].\n");
                    break;
                }
                *day += 1;
                *hour = SLEEP_HOUR;
                p->hp = p->max_hp;
                p->stamina = p->max_stamina;
                if (p->buff.type != EFFECT_NONE) {
                    printf("После сна эффект '%s' прошёл.\n", effect_names[p->buff.type]);
                    p->buff.type = EFFECT_NONE;
                    p->buff.remaining_steps = 0;
                }
                printf("Сладких снов! Наступило: День %d, %02d:00. HP и выносливость восстановлены.\n",
                       *day, *hour);
                break;
            }

            default:
                printf("Нет такого пункта.\n");
                break;
        }
    }
}

int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    srand((unsigned)time(NULL));

    /* Персонаж не создаётся автоматически — указатель пуст */
    Player* player = NULL;

    load_item_names(g_item_names);

    int current_day = 1;
    int current_hour = 8;

    int choice = -1;
    while (choice != MENU_EXIT) {
        printf("\n=== Меню ===\n");
        printf("[0] Выход\n");
        printf("[1] Создать / пересоздать персонажа\n");
        printf("[2] Карточка персонажа\n");
        printf("[3] Время и отдых\n");
        printf("[4] Посмотреть инвентарь\n");
        printf("[5] Положить предмет в слот\n");
        printf("[6] Выбросить предмет\n");
        printf("[7] Очистка от мусора (по ID)\n");
        printf("[8] Поиск предмета по названию\n");
        printf("[9] Записать состояние в дневник (diary.txt)\n");
        printf("[10] Отправиться в приключение\n");
        printf("[11] Сгенерировать письмо от старосты (input.txt -> output.txt)\n");
        printf("Выберите пункт: ");

        if (!read_int("", &choice)) {
            printf("Ошибка ввода! Введите число.\n");
            choice = -1;
            continue;
        }

        switch ((MenuCode)choice) {
            case 0:
             MENU_EXIT:
                if (player) destroy_player(&player);
                printf("Выход из игры. До свидания!\n");
                break;

            case 1: 
            MENU_CREATE: {
                if (player) {
                    char confirm[16];
                    printf("Персонаж '%s' уже существует (адрес %p). Точно удалить и создать нового? (y/n): ",
                           player->name, (void*)player);
                    if (!fgets(confirm, sizeof(confirm), stdin)) break;
                    if (confirm[0] != 'y' && confirm[0] != 'Y' &&
                        confirm[0] != 'д' && confirm[0] != 'Д') {
                        printf("Отмена. Старый персонаж сохранён.\n");
                        break;
                    }
                    destroy_player(&player);
                }
                player = create_player();
                break;
            }

            case 2:
            MENU_CARD:
                if (!player) { printf("Персонаж ещё не создан! Используйте пункт [1].\n"); break; }
                print_card(player, current_day, current_hour);
                break;

            case 3:
            MENU_TIME:
                time_submenu(player, &current_day, &current_hour);
                break;

            case 4:
            MENU_SHOW_INV: {
                if (!player) { printf("Персонаж ещё не создан! Используйте пункт [1].\n"); break; }
                for (int i = 0; i < INVENTORY_SIZE; i++) {
                    printf("Слот %d: [%d] - %s\n", i, player->inventory[i], g_item_names[player->inventory[i]]);
                }
                break;
            }

            case 5:
            MENU_PUT: {
                if (!player) { printf("Персонаж ещё не создан! Используйте пункт [1].\n"); break; }
                int slot, id;
                if (!read_int("Индекс слота (0-9): ", &slot)) { printf("Ошибка ввода!\n"); break; }
                if (slot < 0 || slot >= INVENTORY_SIZE) { printf("Индекс вне диапазона!\n"); break; }
                if (!read_int("ID предмета (0-9): ", &id)) { printf("Ошибка ввода!\n"); break; }
                if (id < 0 || id >= MAX_ITEMS) { printf("Недопустимый ID предмета!\n"); break; }
                player->inventory[slot] = id;
                printf("Предмет '%s' помещён в слот %d.\n", g_item_names[id], slot);
                break;
            }

            case 6:
            MENU_DROP: {
                if (!player) { printf("Персонаж ещё не создан! Используйте пункт [1].\n"); break; }
                int slot;
                if (!read_int("Индекс слота для удаления (0-9): ", &slot)) { printf("Ошибка ввода!\n"); break; }
                if (slot < 0 || slot >= INVENTORY_SIZE) { printf("Индекс вне диапазона!\n"); break; }
                player->inventory[slot] = ITEM_EMPTY;
                printf("Слот %d очищен.\n", slot);
                break;
            }

            case 7:
            MENU_CLEAR_ID: {
                if (!player) { printf("Персонаж ещё не создан! Используйте пункт [1].\n"); break; }
                int id;
                if (!read_int("Введите ID предмета для удаления: ", &id)) { printf("Ошибка ввода!\n"); break; }
                if (id < 0 || id >= MAX_ITEMS || id == ITEM_EMPTY) {
                    printf("Недопустимый ID или попытка удалить 'Пусто'.\n");
                    break;
                }
                int cleared = 0;
                for (int i = 0; i < INVENTORY_SIZE; i++) {
                    if (player->inventory[i] == id) {
                        player->inventory[i] = ITEM_EMPTY;
                        cleared++;
                    }
                }
                printf("Очищено слотов: %d\n", cleared);
                break;
            }

            case 8:
            MENU_FIND: {
                if (!player) { printf("Персонаж ещё не создан! Используйте пункт [1].\n"); break; }
                char search_name[MAX_NAME_LEN];
                printf("Введите название предмета для поиска: ");
                if (!fgets(search_name, sizeof(search_name), stdin)) {
                    printf("Ошибка чтения.\n");
                    break;
                }
                size_t len = strlen(search_name);
                if (len > 0 && search_name[len - 1] == '\n') search_name[len - 1] = '\0';

                int target_id = find_id_by_name(g_item_names, search_name);
                if (target_id == -1) {
                    printf("Такого предмета нет в каталоге.\n");
                    break;
                }

                int found = 0;
                for (int i = 0; i < INVENTORY_SIZE; i++) {
                    if (player->inventory[i] == target_id) {
                        printf("Найден в слоте %d\n", i);
                        found = 1;
                    }
                }
                if (!found) printf("Предмет '%s' не найден в инвентаре.\n", search_name);
                break;
            }

            case 9:
            MENU_DIARY:
                if (!player) { printf("Персонаж ещё не создан! Используйте пункт [1].\n"); break; }
                write_diary(player, current_day, current_hour);
                printf("Состояние сохранено в diary.txt\n");
                break;

            case 10:
            MENU_ADVENTURE:
                if (!player) { printf("Персонаж ещё не создан! Используйте пункт [1].\n"); break; }
                adventure_mode(player, &current_day, &current_hour);
                break;

            case 11:
            MENU_LETTER:
                if (!player) { printf("Персонаж ещё не создан! Используйте пункт [1].\n"); break; }
                generate_letter(player, current_day);
                break;

            default:
                printf("Нет такого пункта меню.\n");
                break;
        }
    }

    return 0;
}

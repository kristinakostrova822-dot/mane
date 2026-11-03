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

const char* default_names[MAX_ITEMS] = {
    "Пусто", "Дерево", "Камень", "Семена", "Железо",
    "Золото", "Яблоко", "Зелье", "Верёвка", "Факел"
};

void get_farmer_name(char* name) {
    printf("Введите имя фермера: ");
    if (!fgets(name, MAX_FARMER_NAME, stdin)) {
        name[0] = '\0';
        return;
    }
    size_t len = strlen(name);
    if (len > 0 && name[len - 1] == '\n') {
        name[len - 1] = '\0';
    }
    if (strlen(name) == 0) {
        strcpy(name, "Фермер");
    }
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
        if (strcmp(item_names[i], search_name) == 0) {
            return i;
        }
    }
    return -1;
}

void write_diary(const char* farmer_name, int day, int hour,
                 const int inventory[INVENTORY_SIZE],
                 const char item_names[MAX_ITEMS][MAX_NAME_LEN])
{
    FILE* diary = fopen("diary.txt", "a");
    if (!diary) {
        printf("Не удалось открыть diary.txt для записи.\n");
        return;
    }

    fprintf(diary, "=== Дневник фермера ===\n");
    fprintf(diary, "Имя фермера: %s\n", farmer_name);
    fprintf(diary, "Дата: День %d, %02d:00\n", day, hour);
    fprintf(diary, "Инвентарь:\n");

    for (int i = 0; i < INVENTORY_SIZE; i++) {
        fprintf(diary, "Слот %d: [%d] - %s\n", i, inventory[i], item_names[inventory[i]]);
    }

    fprintf(diary, "========================\n\n");
    fclose(diary);
}

void generate_letter(const char* farmer_name, int current_day,
                     const int inventory[INVENTORY_SIZE],
                     const char item_names[MAX_ITEMS][MAX_NAME_LEN])
{
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
    const char* item_in_slot0 = item_names[inventory[0]];

    while (fgets(line, sizeof(line), fin)) {
        size_t len = strlen(line);
        int has_newline = (len > 0 && line[len - 1] == '\n');
        if (has_newline) {
            line[len - 1] = '\0';
            len--;
        }

        char result[MAX_LINE_LEN * 2];
        int pos = 0;

        for (size_t i = 0; i <= len; i++) {
            if (i + 6 <= len && strncmp(&line[i], "<NAME>", 6) == 0) {
                pos += sprintf(&result[pos], "%s", farmer_name);
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

typedef struct {
    char** grid;
    int rows;
    int cols;
    int hero_r, hero_c;
    /* позиции лестниц */
    int stairs_down_r, stairs_down_c;
    int stairs_up_r, stairs_up_c;
} Map;

Map create_dungeon_map(int rows, int cols) {
    Map m;
    m.rows = rows;
    m.cols = cols;
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
            if (rnd < 0.10) {
                m.grid[r][c] = CH_TREE;
            } else if (rnd < 0.20) {
                m.grid[r][c] = CH_ROCK;
            } else if (rnd < 0.35) {
                m.grid[r][c] = CH_ICE;
            } else {
                m.grid[r][c] = CH_FLOOR;
            }
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
    } while ((dr == sr && dc == sc) || m.grid[dr][dc] == CH_WALL || m.grid[dr][dc] == CH_TREE || m.grid[dr][dc] == CH_ROCK);
    m.stairs_down_r = dr;
    m.stairs_down_c = dc;
    m.grid[dr][dc] = CH_STAIRS_DOWN;

    /* Лестница вверх U: рядом со стартом (одна из 4 соседей) */
    int ur = -1, uc = -1;
    int candidates[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    int tried = 0;
    while (tried < 4) {
        int idx = rand() % 4;
        ur = sr + candidates[idx][0];
        uc = sc + candidates[idx][1];
        if (ur >= 1 && ur <= rows-2 && uc >= 1 && uc <= cols-2) {
            if (m.grid[ur][uc] != CH_WALL && m.grid[ur][uc] != CH_TREE && m.grid[ur][uc] != CH_ROCK) {
                break; /* нашли подходящее место */
            }
        }
        tried++;
    }
    /* Если не получилось рядом — ставим где-то в свободной клетке */
    if (ur == -1 || uc == -1) {
        do {
            ur = 1 + rand() % (rows - 2);
            uc = 1 + rand() % (cols - 2);
        } while ((ur == sr && uc == sc) || (ur == dr && uc == dc) ||
                 m.grid[ur][uc] == CH_WALL || m.grid[ur][uc] == CH_TREE || m.grid[ur][uc] == CH_ROCK);
    }
    m.stairs_up_r = ur;
    m.stairs_up_c = uc;
    m.grid[ur][uc] = CH_STAIRS_UP;

    /* Ставим героя */
    m.grid[m.hero_r][m.hero_c] = CH_HERO;

    return m;
}

void free_map(Map* m) {
    if (!m->grid) return;
    for (int r = 0; r < m->rows; r++) {
        free(m->grid[r]);
    }
    free(m->grid);
    m->grid = NULL;
    m->rows = m->cols = 0;
}

void print_map(const Map* m) {
    for (int r = 0; r < m->rows; r++) {
        for (int c = 0; c < m->cols; c++) {
            putchar(m->grid[r][c]);
        }
        putchar('\n');
    }
}

/* Возвращает:
   0 — нельзя пойти
   1 — обычный успешный ход
   2 — ход на лестницу вниз (переход на новый этаж)
   3 — ход на лестницу вверх (выход домой)
*/
int move_hero_dungeon(Map* m, int dr, int dc, int* out_slide_steps) {
    *out_slide_steps = 0;

    int nr = m->hero_r + dr;
    int nc = m->hero_c + dc;

    /* Проверка границ: в подземелье выход за край невозможен, это ошибка логики */
    if (nr < 0 || nr >= m->rows || nc < 0 || nc >= m->cols) {
        return 0;
    }

    char target = m->grid[nr][nc];

    if (target == CH_WALL || target == CH_TREE || target == CH_ROCK) {
        return 0; /* нельзя пойти */
    }

    /* Очищаем старую клетку */
    m->grid[m->hero_r][m->hero_c] = CH_FLOOR; /* в подземелье нет лагеря, просто пол */

    /* Скольжение по льду */
    int slide_steps = 0;
    if (target == CH_ICE) {
        /* Сначала делаем первый шаг на лёд и ставим героя */
        m->hero_r = nr;
        m->hero_c = nc;
        m->grid[m->hero_r][m->hero_c] = CH_HERO;
        slide_steps++;

        /* Продолжаем скользить в том же направлении */
        while (1) {
            int sr = m->hero_r + dr;
            int sc = m->hero_c + dc;

            /* Если вышли за границы — невозможно, но на всякий случай */
            if (sr < 0 || sr >= m->rows || sc < 0 || sc >= m->cols) {
                *out_slide_steps = slide_steps;
                return 0;
            }

            char next = m->grid[sr][sc];
            if (next == CH_WALL || next == CH_TREE || next == CH_ROCK) {
                /* Врезались — останавливаемся, герой остаётся на последней клетке перед препятствием */
                *out_slide_steps = slide_steps;
                return 1;
            }

            /* Двигаем героя вперёд */
            m->grid[m->hero_r][m->hero_c] = CH_FLOOR;
            m->hero_r = sr;           
            m->hero_r = sr;
            m->hero_c = sc;
            slide_steps++;
            m->grid[m->hero_r][m->hero_c] = CH_HERO;

            /* Если следующая клетка — не лёд, значит здесь остановка */
            if (next != CH_ICE) break;
        }
    } else {
        /* Обычный шаг */
        m->hero_r = nr;
        m->hero_c = nc;
        m->grid[m->hero_r][m->hero_c] = CH_HERO;
    }

    *out_slide_steps = slide_steps;

    /* Проверяем, где остановился герой */
    if (m->hero_r == m->stairs_down_r && m->hero_c == m->stairs_down_c) {
        return 2; /* лестница вниз */
    }
    if (m->hero_r == m->stairs_up_r && m->hero_c == m->stairs_up_c) {
        return 3; /* лестница вверх */
    }

    return 1; /* обычный ход */
}

void adventure_mode(const char* farmer_name) {
    srand((unsigned)time(NULL));
    int depth = 0; /* счётчик глубины */
    int running = 1;

    Map current_map;
    int map_size = 5 + rand() % 26; /* 5..30 */
    current_map = create_dungeon_map(map_size, map_size);

    printf("\n=== ПОДЗЕМЕЛЬЕ: %s спускается в темноту! ===\n", farmer_name);
    printf("Глубина: %d этажей\n", depth);
    printf("W - вверх, A - влево, S - вниз, D - вправо\n");
    printf("Q - подняться по лестнице U и уйти домой (только стоя на U)\n");
    printf("Символы: @ - герой, # - стены, T - деревья, R - камни, ~ - лёд, D - лестница вниз, U - лестница вверх\n");

    while (running) {
        print_map(&current_map);
        printf("Глубина: %d | Герой: @ | Лёд: ~ | Деревья: T | Камни: R\n", depth);
        printf("Выберите направление (WASDQ): ");

        char input[16];
        if (!fgets(input, sizeof(input), stdin)) break;
        char dir = toupper(input[0]);

        int dr = 0, dc = 0;
        switch (dir) {
            case 'W': dr = -1; break;
            case 'A': dc = -1; break;
            case 'S': dr =  1; break;
            case 'D': dc =  1; break;
            case 'Q':
                if (current_map.hero_r == current_map.stairs_up_r &&
                    current_map.hero_c == current_map.stairs_up_c) {
                    printf("Вы поднялись по лестнице и вернулись домой!\n");
                    running = 0;
                    break;
                } else {
                    printf("Нельзя уйти домой, пока вы не на лестнице вверх (U).\n");
                    continue;
                }
            default:
                printf("Неверное направление. Используйте W, A, S, D или Q.\n");
                continue;
        }

        if (!running) break;

        int slide_steps = 0;
        int result = move_hero_dungeon(&current_map, dr, dc, &slide_steps);

        if (result == 0) {
            printf("Нельзя туда пойти — там стена или препятствие.\n");
        }
        else if (result == 1) {
            if (slide_steps > 0) {
                printf("Скользим по льду: %d клеток!\n", slide_steps);
            }
        }
        else if (result == 2) {
            /* Спуск на новый этаж */
            free_map(&current_map);
            depth++;
            map_size = 5 + rand() % 26;
            current_map = create_dungeon_map(map_size, map_size);
            printf("Вы спустились на новый этаж! Глубина: %d\n", depth);
            if (slide_steps > 0) {
                printf("(по льду вы проскользили %d клеток, прежде чем найти лестницу)\n", slide_steps);
            }
        }
        else if (result == 3) {
            /* Стоим на лестнице вверх — выход домой только через Q */
            if (slide_steps > 0) {
                printf("Скользим по льду: %d клеток — и выскочили на лестницу U!\n", slide_steps);
            }
            printf("Вы на лестнице вверх (U). Нажмите Q, чтобы подняться домой.\n");
        }
    }

    free_map(&current_map);
    printf("Приключение завершено. %s вернулся на поверхность. Всего спусков: %d.\n", farmer_name, depth);
}

int main(void) {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    char farmer_name[MAX_FARMER_NAME];
    get_farmer_name(farmer_name);

    char item_names[MAX_ITEMS][MAX_NAME_LEN];
    load_item_names(item_names);

    int current_day = 1;
    int current_hour = 8;

    int inventory[INVENTORY_SIZE] = {
        ITEM_EMPTY,  ITEM_WOOD, ITEM_STONE, ITEM_SEEDS,
        ITEM_IRON,  ITEM_GOLD, ITEM_APPLE, ITEM_POTION,
        ITEM_ROPE, ITEM_TORCH
    };

    int choice = -1;
    int c;

    while (choice != 0) {
        printf("\n=== Меню (%s) ===\n", farmer_name);
        printf("[0] Выход\n");
        printf("[1] Посмотреть на часы\n");
        printf("[2] Промотать время (Поработать)\n");
        printf("[3] Посмотреть инвентарь\n");
        printf("[4] Положить предмет в слот\n");
        printf("[5] Выбросить предмет\n");
        printf("[6] Очистка от мусора (по ID)\n");
        printf("[7] Поиск предмета по названию\n");
        printf("[8] Записать состояние в дневник (diary.txt)\n");
        printf("[9] Сгенерировать письмо от старосты (input.txt -> output.txt)\n");
        printf("[10] Отправиться в подземелье\n");
        printf("Выберите пункт: ");

        if (scanf("%d", &choice) != 1) {
            while ((c = getchar()) != '\n' && c != EOF) {}
            printf("Ошибка ввода! Введите число.\n");
            choice = -1;
            continue;
        }
        while ((c = getchar()) != '\n' && c != EOF) {}

        switch (choice) {
            case 0:
                printf("Выход из игры. До свидания, %s!\n", farmer_name);
                break;

            case 1:
                printf("Текущее время: День %d, %02d:00\n", current_day, current_hour);
                break;

            case 2: {
                int hours;
                printf("Сколько часов работаем? ");
                if (scanf("%d", &hours) != 1) {
                    while ((c = getchar()) != '\n' && c != EOF) {}
                    printf("Ошибка ввода!\n");
                    break;
                }
                while ((c = getchar()) != '\n' && c != EOF) {}
                if (hours < 0) {
                    printf("Часы не могут быть отрицательными.\n");
                    break;
                }
                current_hour += hours;
                while (current_hour >= 24) {
                    current_hour -= 24;
                    current_day++;
                }
                printf("Прошло %d ч. Теперь: День %d, %02d:00\n", hours, current_day, current_hour);
                break;
            }

            case 3:
                for (int i = 0; i < INVENTORY_SIZE; i++) {
                    printf("Слот %d: [%d] - %s\n", i, inventory[i], item_names[inventory[i]]);
                }
                break;

            case 4: {
                int slot, id;
                printf("Индекс слота (0-9): ");
                if (scanf("%d", &slot) != 1) {
                    while ((c = getchar()) != '\n' && c != EOF) {}
                    printf("Ошибка ввода!\n");
                    break;
                }
                while ((c = getchar()) != '\n' && c != EOF) {}
                if (slot < 0 || slot >= INVENTORY_SIZE) {
                    printf("Индекс вне диапазона!\n");
                    break;
                }

                printf("ID предмета (0-9): ");
                if (scanf("%d", &id) != 1) {
                    while ((c = getchar()) != '\n' && c != EOF) {}
                    printf("Ошибка ввода!\n");
                    break;
                }
                while ((c = getchar()) != '\n' && c != EOF) {}
                if (id < 0 || id >= MAX_ITEMS) {
                    printf("Недопустимый ID предмета!\n");
                    break;
                }

                inventory[slot] = id;
                printf("Предмет '%s' помещён в слот %d.\n", item_names[id], slot);
                break;
            }

            case 5: {
                int slot;
                printf("Индекс слота для удаления (0-9): ");
                if (scanf("%d", &slot) != 1) {
                    while ((c = getchar()) != '\n' && c != EOF) {}
                    printf("Ошибка ввода!\n");
                    break;
                }
                while ((c = getchar()) != '\n' && c != EOF) {}
                if (slot < 0 || slot >= INVENTORY_SIZE) {
                    printf("Индекс вне диапазона!\n");
                    break;
                }

                inventory[slot] = ITEM_EMPTY;
                printf("Слот %d очищен.\n", slot);
                break;
            }

            case 6: {
                int id;
                int cleared = 0;

                printf("Введите ID предмета для удаления: ");
                if (scanf("%d", &id) != 1) {
                    while ((c = getchar()) != '\n' && c != EOF) {}
                    printf("Ошибка ввода!\n");
                    break;
                }
                while ((c = getchar()) != '\n' && c != EOF) {}
                if (id < 0 || id >= MAX_ITEMS || id == ITEM_EMPTY) {
                    printf("Недопустимый ID или попытка удалить 'Пусто'.\n");
                    break;
                }

                for (int i = 0; i < INVENTORY_SIZE; i++) {
                    if (inventory[i] == id) {
                        inventory[i] = ITEM_EMPTY;
                        cleared++;
                    }
                }
                printf("Очищено слотов: %d\n", cleared);
                break;
            }

            case 7: {
                char search_name[MAX_NAME_LEN];
                printf("Введите название предмета для поиска: ");
                if (!fgets(search_name, sizeof(search_name), stdin)) {
                    printf("Ошибка чтения.\n");
                    break;
                }
                size_t len = strlen(search_name);
                if (len > 0 && search_name[len - 1] == '\n') search_name[len - 1] = '\0';

                int target_id = find_id_by_name(item_names, search_name);
                if (target_id == -1) {
                    printf("Такого предмета нет в каталоге.\n");
                    break;
                }

                int found = 0;
                for (int i = 0; i < INVENTORY_SIZE; i++) {
                    if (inventory[i] == target_id) {
                        printf("Найден в слоте %d\n", i);
                        found = 1;
                    }
                }
                if (!found) {
                    printf("Предмет '%s' не найден в инвентаре.\n", search_name);
                }
                break;
            }

            case 8:
                write_diary(farmer_name, current_day, current_hour, inventory, item_names);
                printf("Состояние сохранено в diary.txt\n");
                break;

            case 9:
                generate_letter(farmer_name, current_day, inventory, item_names);
                break;

            case 10:
                adventure_mode(farmer_name);
                break;

            default:
                printf("Нет такого пункта меню.\n");
                break;
        }
    }

    return 0;
}
#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#define INVENTORY_SIZE 10
#define MAX_ITEMS 10
#define MAX_NAME_LEN 64
#define MAX_FARMER_NAME 32
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

/* Стандартные названия по умолчанию */
const char* default_names[MAX_ITEMS] = {
    "Пусто",
    "Дерево",
    "Камень",
    "Семена",
    "Железо",
    "Золото",
    "Яблоко",
    "Зелье",
    "Верёвка",
    "Факел"
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

/* Генерация персонализированного письма от старосты */
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
        /* Убираем \n для удобной обработки, потом вернём */
        size_t len = strlen(line);
        int has_newline = (len > 0 && line[len - 1] == '\n');
        if (has_newline) {
            line[len - 1] = '\0';
            len--;
        }

        /* Проходим по строке и заменяем теги */
        char result[MAX_LINE_LEN * 2]; // буфер побольше
        int pos = 0;

        for (size_t i = 0; i <= len; i++) {
            /* Проверка на <NAME> */
            if (i + 6 <= len && strncmp(&line[i], "<NAME>", 6) == 0) {
                pos += sprintf(&result[pos], "%s", farmer_name);
                i += 5; // пропускаем 6 символов, цикл добавит +1
                continue;
            }
            /* Проверка на <DAY> */
            else if (i + 5 <= len && strncmp(&line[i], "<DAY>", 5) == 0) {
                pos += sprintf(&result[pos], "%d", current_day);
                i += 4;
                continue;
            }
            /* Проверка на <ITEM> */
            else if (i + 6 <= len && strncmp(&line[i], "<ITEM>", 6) == 0) {
                pos += sprintf(&result[pos], "%s", item_in_slot0);
                i += 5;
                continue;
            }

            result[pos++] = line[i];
            if (pos >= (int)(sizeof(result) - 1)) break; // защита от переполнения
        }
        result[pos] = '\0';

        fprintf(fout, "%s\n", result);
    }

    fclose(fin);
    fclose(fout);
    printf("Письмо сгенерировано в output.txt\n");
}

int main(void) {
    setlocale(LC_ALL, "");

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

            default:
                printf("Нет такого пункта меню.\n");
                break;
        }
    }

    return 0;
}
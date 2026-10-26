#include <gb/gb.h>
#include <rand.h>

#define MAP_WIDTH 10
#define MAP_HEIGHT 9

// Простая карта лабиринта (1 - это стена, 0 - пустая дорога)
// Координаты на экране: X от 0 до 9, Y от 0 до 8
const UBYTE MAZE[MAP_HEIGHT][MAP_WIDTH] = {
    {1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,1,0,0,0,0,1},
    {1,0,1,0,1,0,1,1,0,1},
    {1,0,1,0,0,0,0,1,0,1},
    {1,0,1,1,1,1,0,1,0,1},
    {1,0,0,0,0,1,0,0,0,1},
    {1,1,1,1,0,1,1,1,0,1},
    {1,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1}
};

// Структуры для объектов
struct Entity {
    UBYTE x;
    UBYTE y;
    UBYTE dir_x;
    UBYTE dir_y;
    UBYTE alive;
};

struct Entity player = {24, 24, 0, 1, 1}; // Старт игрока
struct Entity bullet = {0, 0, 0, 0, 0};   // Пуля изначально выключена

#define MAX_BOTS 3
struct Entity bots[MAX_BOTS] = {
    {120, 24, 1, 0, 1},
    {24, 104, 0, 1, 1},
    {120, 104, -1, 0, 1}
};

// Графика спрайта (обычный квадрат)
const unsigned char generic_sprite[] = {
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
    0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};

// Функция проверки: свободна ли клетка для движения?
UBYTE is_walkable(UBYTE new_x, UBYTE new_y) {
    // Переводим пиксельные координаты в индексы сетки (плитки 16x16)
    // Учитываем смещение экрана Game Boy (спрайты смещены на X+8, Y+16)
    UBYTE tile_x = (new_x - 8) / 16;
    UBYTE tile_y = (new_y - 16) / 16;
    
    if (tile_x >= MAP_WIDTH || tile_y >= MAP_HEIGHT) return 0;
    return (MAZE[tile_y][tile_x] == 0);
}

void main() {
    UBYTE i;
    UBYTE remaining_bots = MAX_BOTS;
    
    // Инициализация генератора случайных чисел
    initrand(DIV_REG);

    // Загружаем графику
    set_sprite_data(0, 1, generic_sprite);
    
    // Привязываем индексы спрайтов
    set_sprite_tile(0, 0); // Игрок
    set_sprite_tile(1, 0); // Пуля
    for(i = 0; i < MAX_BOTS; i++) {
        set_sprite_tile(2 + i, 0); // Боты
    }

    SHOW_SPRITES;

    while(1) {
        // Если все боты убиты — игра останавливается (Победа!)
        if (remaining_bots == 0) {
            // Просто фиксируем экран. Спрайты пули и игрока скрываем за экраном.
            move_sprite(0, 0, 0);
            move_sprite(1, 0, 0);
            continue;
        }

        // --- УПРАВЛЕНИЕ ИГРОКОМ ---
        if (joypad() & J_LEFT) {
            player.dir_x = -1; player.dir_y = 0;
            if (is_walkable(player.x - 1, player.y)) player.x--;
        }
        else if (joypad() & J_RIGHT) {
            player.dir_x = 1; player.dir_y = 0;
            if (is_walkable(player.x + 1, player.y)) player.x++;
        }
        else if (joypad() & J_UP) {
            player.dir_x = 0; player.dir_y = -1;
            if (is_walkable(player.x, player.y - 1)) player.y--;
        }
        else if (joypad() & J_DOWN) {
            player.dir_x = 0; player.dir_y = 1;
            if (is_walkable(player.x, player.y + 1)) player.y++;
        }

        // --- ВЫСТРЕЛ (Кнопка А) ---
        if ((joypad() & J_A) && !bullet.alive) {
            bullet.x = player.x;
            bullet.y = player.y;
            bullet.dir_x = player.dir_x;
            bullet.dir_y = player.dir_y;
            bullet.alive = 1;
        }

        // --- ЛОГИКА ПУЛИ ---
        if (bullet.alive) {
            bullet.x += bullet.dir_x * 2;
            bullet.y += bullet.dir_y * 2;

            // Если пуля врезалась в стену — уничтожаем её
            if (!is_walkable(bullet.x, bullet.y)) {
                bullet.alive = 0;
            }
        }

        // --- ЛОГИКА БОТОВ ---
        for(i = 0; i < MAX_BOTS; i++) {
            if (!bots[i].alive) continue;

            // Пробуем сдвинуть бота в его текущем направлении
            UBYTE next_x = bots[i].x + bots[i].dir_x;
            UBYTE next_y = bots[i].y + bots[i].dir_y;

            // Если впереди стена или просто случайно (1 шанс из 40), меняем направление
            if (!is_walkable(next_x, next_y) || (rand() % 40 == 0)) {
                UBYTE rand_dir = rand() % 4;
                if (rand_dir == 0) { bots[i].dir_x = 1; bots[i].dir_y = 0; }
                else if (rand_dir == 1) { bots[i].dir_x = -1; bots[i].dir_y = 0; }
                else if (rand_dir == 2) { bots[i].dir_x = 0; bots[i].dir_y = 1; }
                else { bots[i].dir_x = 0; bots[i].dir_y = -1; }
            } else {
                bots[i].x = next_x;
                bots[i].y = next_y;
            }

            // ПРОВЕРКА КОЛЛИЗИИ: Пуля попала в бота?
            if (bullet.alive && 
                bullet.x >= bots[i].x - 6 && bullet.x <= bots[i].x + 6 &&
                bullet.y >= bots[i].y - 6 && bullet.y <= bots[i].y + 6) {
                
                bots[i].alive = 0;   // Убиваем бота
                bullet.alive = 0;    // Прячем пулю
                remaining_bots--;    // Уменьшаем счетчик живых врагов
                move_sprite(2 + i, 0, 0); // Убираем спрайт бота с экрана
            }
        }

        // --- ОТРИСОВКА ВСЕХ ОБЪЕКТОВ ---
        move_sprite(0, player.x, player.y);
        
        if (bullet.alive) {
            move_sprite(1, bullet.x, bullet.y);
        } else {
            move_sprite(1, 0, 0); // Прячем за экран
        }

        for(i = 0; i < MAX_BOTS; i++) {
            if (bots[i].alive) {
                move_sprite(2 + i, bots[i].x, bots[i].y);
            }
        }

        wait_vbl_done();
    }
}

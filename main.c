#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <math.h>

#define WIDTH 80
#define HEIGHT 20
#define MAX_BULLETS 10
#define MAX_ENEMIES 15  // Уменьшено количество врагов
#define MAX_ENEMY_BULLETS 100
#define MIN_DISTANCE_FROM_PLAYER 8
#define SAFE_DISTANCE 5
#define max(a, b) ((a) > (b) ? (a) : (b))
#define min(a, b) ((a) < (b) ? (a) : (b))
#define BOSS_LIVES 50
#define BOSS_WIDTH 11
#define BOSS_HEIGHT 7
#define BOSS_MIN_DISTANCE 10
#define PI 3.14159265

struct termios orig_termios;

typedef struct {
    int x, y;
    int active;
    float dx, dy;
    int timer;
    int type;
} Bullet;

typedef struct {
    int x, y;
    int spawn_x, spawn_y;
    int alive;
    int type; // 0 - зеленый, 1 - фиолетовый, 2 - танк
    int direction;
    int shoot_timer;
    int min_x, max_x;
    int min_y, max_y;
    int moving_to_position;
    int cooldown; // Задержка перед первым выстрелом
} Enemy;

typedef struct {
    int x, y;
    int lives;
    int hit_timer;
    int invulnerable;
    int max_lives; // Для подсчета сохраненных HP
} Player;

typedef struct {
    int x, y;
    int lives;
    int active;
    int shoot_timer;
    int move_timer;
    int attack_timer;
    int invulnerable;
    int phase;
    int direction;
    int vertical_direction;
    int attack_pattern;
    int rage_mode;
} Boss;

typedef struct {
    int x, y;
    int size;
    int active;
} Meteor;

typedef struct {
    int x, y;
    int active;
    int timer;
    int moving_left;
    int homing;
} Medkit;

Player player;
Bullet bullets[MAX_BULLETS];
Bullet enemy_bullets[MAX_ENEMY_BULLETS];
Enemy enemies[MAX_ENEMIES];
Boss boss;
Meteor meteors[3];
Medkit medkits[2];

int score = 0;
int wave = 1;
int game_over = 0;
int boss_defeated = 0;
int wave_display_timer = 0;
int player_spawn_timer = 0;

// Новая статистика
int enemies_killed = 0;
int tanks_killed = 0;
int boss_killed = 0;
int medkits_collected = 0;
int shots_fired = 0;
time_t start_time;
time_t end_time;



// Функция сброса статистики
void reset_stats() {
    enemies_killed = 0;
    tanks_killed = 0;
    boss_killed = 0;
    medkits_collected = 0;
    shots_fired = 0;
    score = 0;
    wave = 1;
    game_over = 0;
    boss_defeated = 0;
    start_time = time(NULL);
}

int kbhit(void) {
    struct timeval tv = { 0L, 0L };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(0, &fds);
    return select(1, &fds, NULL, NULL, &tv);
}

void shoot(void) {
    shots_fired++;
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (!bullets[i].active) {
            bullets[i].x = player.x + 1;
            bullets[i].y = player.y;
            bullets[i].active = 1;
            bullets[i].dx = 2.0;
            bullets[i].dy = 0;
            bullets[i].timer = 0;
            bullets[i].type = 0;
            break;
        }
    }
}

void boss_shoot(int type) {
    switch (type) {
        case 0:
            for (int i = 0; i < 7; i++) {
                for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                    if (!enemy_bullets[j].active) {
                        enemy_bullets[j].x = boss.x;
                        enemy_bullets[j].y = boss.y + BOSS_HEIGHT/2;
                        enemy_bullets[j].dx = -2.0f;
                        enemy_bullets[j].dy = (i-3) * 0.5f;
                        enemy_bullets[j].active = 1;
                        enemy_bullets[j].type = 0;
                        enemy_bullets[j].timer = 0;
                        break;
                    }
                }
            }
            break;
            
        case 1:
            for (int k = -1; k <= 1; k++) {
                for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                    if (!enemy_bullets[j].active) {
                        enemy_bullets[j].x = boss.x;
                        enemy_bullets[j].y = boss.y + BOSS_HEIGHT/2;
                        float dx = (player.x + k*20) - enemy_bullets[j].x;
                        float dy = (player.y + k*10) - enemy_bullets[j].y;
                        float length = sqrt(dx*dx + dy*dy);
                        if (length > 0) {
                            enemy_bullets[j].dx = dx/length * 2.5f;
                            enemy_bullets[j].dy = dy/length * 2.5f;
                        }
                        enemy_bullets[j].active = 1;
                        enemy_bullets[j].type = 1;
                        enemy_bullets[j].timer = 0;
                        break;
                    }
                }
            }
            break;
            
        case 2:
            for (int i = 0; i < 24; i++) {
                for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                    if (!enemy_bullets[j].active) {
                        enemy_bullets[j].x = boss.x;
                        enemy_bullets[j].y = boss.y + BOSS_HEIGHT/2;
                        float angle = PI/12 * i;
                        enemy_bullets[j].dx = cos(angle) * 2.0f;
                        enemy_bullets[j].dy = sin(angle) * 2.0f;
                        enemy_bullets[j].active = 1;
                        enemy_bullets[j].type = 2;
                        enemy_bullets[j].timer = 0;
                        break;
                    }
                }
            }
            break;
            
        case 3:
            for (int layer = 0; layer < 2; layer++) {
                for (int i = 0; i < 16; i++) {
                    for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                        if (!enemy_bullets[j].active) {
                            enemy_bullets[j].x = boss.x;
                            enemy_bullets[j].y = boss.y + BOSS_HEIGHT/2;
                            float angle = PI/8 * i + (boss.attack_timer % 30) * 0.2f - PI/2;
                            float speed = 1.5f + layer * 0.5f;
                            enemy_bullets[j].dx = cos(angle) * speed;
                            enemy_bullets[j].dy = sin(angle) * speed;
                            enemy_bullets[j].active = 1;
                            enemy_bullets[j].type = 3;
                            enemy_bullets[j].timer = 0;
                            break;
                        }
                    }
                }
            }
            break;
    }
}

void spawn_meteor() {
    for (int i = 0; i < 3; i++) {
        if (!meteors[i].active && rand() % 200 == 0) {
            meteors[i].x = WIDTH - 2;
            meteors[i].y = 2 + rand() % (HEIGHT - 4);
            meteors[i].size = 1 + rand() % 3;
            meteors[i].active = 1;
            break;
        }
    }
}

void move_meteors() {
    for (int i = 0; i < 3; i++) {
        if (meteors[i].active) {
            meteors[i].x -= 1;
            
            if (player.x >= meteors[i].x && player.x < meteors[i].x + meteors[i].size &&
                player.y >= meteors[i].y && player.y < meteors[i].y + meteors[i].size &&
                player.invulnerable <= 0) {
                player.lives--;
                player.hit_timer = 10;
                player.invulnerable = 30;
                meteors[i].active = 0;
                printf("\a");
                fflush(stdout);
            }
            
            if (meteors[i].x + meteors[i].size <= 0) {
                meteors[i].active = 0;
            }
        }
    }
}

void spawn_medkit(int x, int y) {
    for (int i = 0; i < 2; i++) {
        if (!medkits[i].active) {
            medkits[i].x = x;
            medkits[i].y = y;
            medkits[i].active = 1;
            medkits[i].timer = 100;
            break;
        }
    }
}

void move_medkits() {
    for (int i = 0; i < 2; i++) {
        if (medkits[i].active) {
            medkits[i].x -= 1;
            medkits[i].timer--;
            
            if (player.x >= medkits[i].x - 1 && player.x <= medkits[i].x + 1 &&
                player.y == medkits[i].y) {
                player.lives = min(3, player.lives + 1);
                medkits[i].active = 0;
                // Очки за аптечки больше не начисляются
            }
            
            if (medkits[i].x <= 0 || medkits[i].timer <= 0) {
                medkits[i].active = 0;
            }
        }
    }
}

void handle_input() {
    while (kbhit()) {
        int ch = getchar();
        switch (ch) {
            case 'w': player.y = max(2, player.y - 1); break;
            case 's': player.y = min(HEIGHT - 1, player.y + 1); break;
            case ' ': shoot(); break;
            case 'q': game_over = 1; break;
        }
    }
}

void reset_terminal_mode() {
    tcsetattr(0, TCSANOW, &orig_termios);
}

void set_conio_terminal_mode() {
    struct termios new_termios;
    tcgetattr(0, &orig_termios);
    memcpy(&new_termios, &orig_termios, sizeof(new_termios));
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VTIME] = 0;
    new_termios.c_cc[VMIN] = 0;
    tcsetattr(0, TCSANOW, &new_termios);
    atexit(reset_terminal_mode);
}

void clear_screen() {
    printf("\033[2J\033[1;1H");
}

void move_cursor(int x, int y) {
    printf("\033[%d;%dH", y, x);
}

void hide_cursor() {
    printf("\033[?25l");
}

void show_cursor() {
    printf("\033[?25h");
}

void draw_borders() {
    for (int x = 0; x <= WIDTH; x++) {
        move_cursor(x, 0);
        printf("\033[36m═\033[0m");
        move_cursor(x, HEIGHT);
        printf("\033[36m═\033[0m");
    }
    for (int y = 0; y <= HEIGHT; y++) {
        move_cursor(0, y);
        printf("\033[36m║\033[0m");
        move_cursor(WIDTH, y);
        printf("\033[36m║\033[0m");
    }
    move_cursor(0, 0);
    printf("\033[36m╔\033[0m");
    move_cursor(WIDTH, 0);
    printf("\033[36m╗\033[0m");
    move_cursor(0, HEIGHT);
    printf("\033[36m╚\033[0m");
    move_cursor(WIDTH, HEIGHT);
    printf("\033[36m╝\033[0m");
}

void draw_stars() {
    static int initialized = 0;
    static int stars[WIDTH][HEIGHT];
    
    if (!initialized) {
        for (int x = 1; x < WIDTH; x++) {
            for (int y = 1; y < HEIGHT; y++) {
                if (rand() % 15 == 0) {
                    stars[x][y] = 1;
                } else {
                    stars[x][y] = 0;
                }
            }
        }
        initialized = 1;
    }
    
    static float offset = 0.0f;
    offset += 0.1f;
    
    for (int x = 1; x < WIDTH; x++) {
        for (int y = 1; y < HEIGHT; y++) {
            if (stars[x][y]) {
                float wave = sin(offset + x*0.2f + y*0.1f);
                if (wave > 0.8f) {
                    move_cursor(x, y);
                    printf("\033[36m~\033[0m");
                }
            }
        }
    }
}

void draw_player_health() {
    move_cursor(2, HEIGHT + 3);
    printf("\033[31mHP: [");
    for (int i = 0; i < 3; i++) {
        if (i < player.lives) {
            printf("♥");
        } else {
            printf(" ");
        }
    }
    printf("]\033[0m");
}

void draw_controls() {
    move_cursor(2, HEIGHT + 4);
    printf("\033[36mControls: W/S - Move | SPACE - Shoot | Q - Quit\033[0m");
}

void draw() {
    clear_screen();
    draw_borders();
    draw_stars();

    // Метеориты
    for (int i = 0; i < 3; i++) {
        if (meteors[i].active) {
            for (int dx = 0; dx < meteors[i].size; dx++) {
                for (int dy = 0; dy < meteors[i].size; dy++) {
                    move_cursor(meteors[i].x + dx, meteors[i].y + dy);
                    printf("\033[37m#\033[0m");
                }
            }
        }
    }

    // Аптечки
    for (int i = 0; i < 2; i++) {
        if (medkits[i].active) {
            move_cursor(medkits[i].x, medkits[i].y);
            printf("\033[32m[HP]\033[0m");
        }
    }

    // Player
    move_cursor(player.x, player.y);
    if (player.hit_timer > 0) {
        printf("\033[41m>\033[0m");
    }
    else if (player.invulnerable > 0 && (player.invulnerable % 4 < 2)) {
        printf("\033[43m>\033[0m");
    }
    else {
        printf("\033[22m>\033[0m");
    }

    // Player bullets
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            move_cursor((int)bullets[i].x, (int)bullets[i].y);
            printf("\033[33m-\033[0m");
        }
    }

    // Enemy bullets
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemy_bullets[i].active) {
            move_cursor((int)enemy_bullets[i].x, (int)enemy_bullets[i].y);
            switch(enemy_bullets[i].type) {
                case 1: printf("\033[31m*\033[0m"); break;
                case 2: printf("\033[35m*\033[0m"); break;
                case 3: printf("\033[33m*\033[0m"); break;
                default: printf("\033[31m*\033[0m");
            }
        }
    }

    // Enemies
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].alive) {
            if (enemies[i].type == 0) {
                move_cursor(enemies[i].x-1, enemies[i].y);
                printf("\033[32m(\033[0m");
                move_cursor(enemies[i].x, enemies[i].y);
                printf("\033[32mO\033[0m");
                move_cursor(enemies[i].x+1, enemies[i].y);
                printf("\033[32m)\033[0m");
            } else if (enemies[i].type == 1) {
                move_cursor(enemies[i].x-1, enemies[i].y);
                printf("\033[35m[\033[0m");
                move_cursor(enemies[i].x, enemies[i].y);
                printf("\033[35m@\033[0m");
                move_cursor(enemies[i].x+1, enemies[i].y);
                printf("\033[35m]\033[0m");
            } else if (enemies[i].type == 2) {
                move_cursor(enemies[i].x-1, enemies[i].y);
                printf("\033[33m[\033[0m");
                move_cursor(enemies[i].x, enemies[i].y);
                printf("\033[33mT\033[0m");
                move_cursor(enemies[i].x+1, enemies[i].y);
                printf("\033[33m]\033[0m");
            }
        }
    }

    // Boss
    if (boss.active) {
        const char* boss_art[7] = {
            "  /-----\\ ",
            " /  • •  \\",
            "|  ====  |",
            "| ~~~~~~ |",
            " \\ ~~~~ / ",
            "  \\____/  ",
            "   /  \\   "
        };

        for (int dy = 0; dy < 7; dy++) {
            move_cursor(boss.x, boss.y + dy);
            if (boss.invulnerable > 0) {
                if (boss.rage_mode) {
                    printf("\033[45m%s\033[0m", boss_art[dy]);
                } else {
                    printf("\033[46m%s\033[0m", boss_art[dy]);
                }
            } else if (boss.rage_mode) {
                printf("\033[41m%s\033[0m", boss_art[dy]);
            } else {
                printf("\033[44m%s\033[0m", boss_art[dy]);
            }
        }
    }

    // HUD
     // HUD с таймером
     time_t current_time = time(NULL);
     double play_time = difftime(current_time, start_time);
     int minutes = (int)play_time / 60;
     int seconds = (int)play_time % 60;
     
     move_cursor(2, HEIGHT + 2);
     printf("\033[32mScore: %d\033[0m | \033[31mLives: %d\033[0m | \033[36mWave: %d\033[0m | \033[33mTime: %02d:%02d\033[0m", 
            score, player.lives, wave, minutes, seconds);
    move_cursor(2, HEIGHT + 2);
    printf("\033[32mScore: %d\033[0m | \033[31mLives: %d\033[0m | \033[36mWave: %d\033[0m", score, player.lives, wave);
    
    draw_player_health();
    draw_controls();
    
    if (boss.active) {
        move_cursor(WIDTH - 25, HEIGHT + 2);
        printf("Boss: \033[31m%d%%\033[0m [", (boss.lives * 100) / BOSS_LIVES);
        int bars = (boss.lives * 20) / BOSS_LIVES;
        for (int i = 0; i < 20; i++) {
            if (i < bars) {
                if (boss.rage_mode) printf("\033[41m \033[0m");
                else if (boss.phase == 2) printf("\033[43m \033[0m");
                else printf("\033[42m \033[0m");
            } else {
                printf(" ");
            }
        }
        printf("]");
    }

    if (wave_display_timer < 30) {
        move_cursor(WIDTH/2 - 5, HEIGHT/2);
        printf("\033[33mWAVE %d\033[0m", wave);
    }

    fflush(stdout);
}

void spawn_enemies() {
    int spawned = 0;
    int attempts;
    
    int sections_x = 4;
    int sections_y = 3;
    int per_section = MAX_ENEMIES / (sections_x * sections_y);
    
    for (int sx = 0; sx < sections_x; sx++) {
        for (int sy = 0; sy < sections_y; sy++) {
            int section_min_x = 4 + (WIDTH - 8) * sx / sections_x;
            int section_max_x = 4 + (WIDTH - 8) * (sx + 1) / sections_x;
            int section_min_y = 3 + (HEIGHT - 5) * sy / sections_y;
            int section_max_y = 3 + (HEIGHT - 5) * (sy + 1) / sections_y;
            
            for (int i = 0; i < per_section && spawned < MAX_ENEMIES; i++) {
                int idx = spawned++;
                attempts = 0;
                
                do {
                    enemies[idx].spawn_x = section_min_x + rand() % (section_max_x - section_min_x);
                    enemies[idx].spawn_y = section_min_y + rand() % (section_max_y - section_min_y);
                    attempts++;
                    if (attempts > 50) break;
                } while (abs(enemies[idx].spawn_x - player.x) < MIN_DISTANCE_FROM_PLAYER || 
                        abs(enemies[idx].spawn_y - player.y) < MIN_DISTANCE_FROM_PLAYER);
                
                enemies[idx].x = WIDTH + rand() % 20;
                enemies[idx].y = enemies[idx].spawn_y;
                enemies[idx].alive = 1;
                enemies[idx].moving_to_position = 1;
                enemies[idx].cooldown = rand() % 60 + 30; // Задержка перед стрельбой
                
                int r = rand() % 100;
                
                if (wave > 1 && wave % 5 != 0 && r < 10) {
                    enemies[idx].type = 2;
                    enemies[idx].min_x = max(2, enemies[idx].spawn_x - 3);
                    enemies[idx].max_x = min(WIDTH - 4, enemies[idx].spawn_x + 3);
                    enemies[idx].min_y = max(2, enemies[idx].spawn_y - 3);
                    enemies[idx].max_y = min(HEIGHT - 2, enemies[idx].spawn_y + 3);
                } else {
                    enemies[idx].type = (r < 70) ? 1 : 0;
                    if (enemies[idx].type == 0) {
                        enemies[idx].min_x = max(2, enemies[idx].spawn_x - 3);
                        enemies[idx].max_x = min(WIDTH - 4, enemies[idx].spawn_x + 3);
                    } else {
                        enemies[idx].min_y = max(2, enemies[idx].spawn_y - 3);
                        enemies[idx].max_y = min(HEIGHT - 2, enemies[idx].spawn_y + 3);
                    }
                }
                
                enemies[idx].direction = (rand() % 2) ? 1 : -1;
                enemies[idx].shoot_timer = rand() % 80 + 40; // Увеличен интервал между выстрелами
            }
        }
    }
    
    // Оставшиеся враги
    while (spawned < MAX_ENEMIES) {
        int idx = spawned++;
        attempts = 0;
        
        do {
            enemies[idx].spawn_x = 4 + rand() % (WIDTH - 8);
            enemies[idx].spawn_y = 3 + rand() % (HEIGHT - 5);
            attempts++;
            if (attempts > 50) break;
        } while (abs(enemies[idx].spawn_x - player.x) < MIN_DISTANCE_FROM_PLAYER || 
                abs(enemies[idx].spawn_y - player.y) < MIN_DISTANCE_FROM_PLAYER);
        
        enemies[idx].x = WIDTH + rand() % 20;
        enemies[idx].y = enemies[idx].spawn_y;
        enemies[idx].alive = 1;
        enemies[idx].moving_to_position = 1;
        
        int r = rand() % 100;
        
        if (wave > 1 && wave % 5 != 0 && r < 10) {
            enemies[idx].type = 2;
            enemies[idx].min_x = max(2, enemies[idx].spawn_x - 3);
            enemies[idx].max_x = min(WIDTH - 4, enemies[idx].spawn_x + 3);
            enemies[idx].min_y = max(2, enemies[idx].spawn_y - 3);
            enemies[idx].max_y = min(HEIGHT - 2, enemies[idx].spawn_y + 3);
        } else {
            enemies[idx].type = (r < 70) ? 1 : 0;
            if (enemies[idx].type == 0) {
                enemies[idx].min_x = max(2, enemies[idx].spawn_x - 3);
                enemies[idx].max_x = min(WIDTH - 4, enemies[idx].spawn_x + 3);
            } else {
                enemies[idx].min_y = max(2, enemies[idx].spawn_y - 3);
                enemies[idx].max_y = min(HEIGHT - 2, enemies[idx].spawn_y + 3);
            }
        }
        
        enemies[idx].direction = (rand() % 2) ? 1 : -1;
        enemies[idx].shoot_timer = rand() % 40 + 10;
    }
}


void move_enemies() {
    static int move_timer = 0;
    move_timer++;
    
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (!enemies[i].alive) continue;
        
        if (enemies[i].moving_to_position) {
            if (move_timer % 3 == 0) { // Еще медленнее к позиции
                if (enemies[i].x > enemies[i].spawn_x) {
                    enemies[i].x--;
                } else {
                    enemies[i].moving_to_position = 0;
                }
            }
            continue;
        }
        
        // Проверяем дистанцию до игрока
        int dist_to_player = abs(enemies[i].x - player.x) + abs(enemies[i].y - player.y);
        if (dist_to_player < SAFE_DISTANCE) {
            // Отдаляемся от игрока
            if (enemies[i].type == 0) {
                if (enemies[i].x < player.x) enemies[i].direction = -1;
                else enemies[i].direction = 1;
            } else {
                if (enemies[i].y < player.y) enemies[i].direction = -1;
                else enemies[i].direction = 1;
            }
        }
        
        if (enemies[i].type == 2) {
            if (rand() % 50 == 0) {
                enemies[i].x += (rand() % 3) - 1;
                enemies[i].y += (rand() % 3) - 1;
                
                enemies[i].x = max(enemies[i].min_x, min(enemies[i].max_x, enemies[i].x));
                enemies[i].y = max(enemies[i].min_y, min(enemies[i].max_y, enemies[i].y));
            }
        } 
        else if (enemies[i].type == 0) {
            if (move_timer % 4 == 0) { // Чуть медленнее
                enemies[i].x += enemies[i].direction;
                
                if (enemies[i].x <= enemies[i].min_x || enemies[i].x >= enemies[i].max_x) {
                    enemies[i].direction *= -1;
                    enemies[i].x = max(enemies[i].min_x, min(enemies[i].max_x, enemies[i].x));
                }
            }
        } else {
            if (move_timer % 4 == 0) { // Чуть медленнее
                enemies[i].y += enemies[i].direction;
                
                if (enemies[i].y <= enemies[i].min_y || enemies[i].y >= enemies[i].max_y) {
                    enemies[i].direction *= -1;
                    enemies[i].y = max(enemies[i].min_y, min(enemies[i].max_y, enemies[i].y));
                }
            }
        }

        // Стрельба только после задержки и реже
        if (enemies[i].cooldown > 0) {
            enemies[i].cooldown--;
        } else if (--enemies[i].shoot_timer <= 0) {
            for (int j = 0; j < MAX_ENEMY_BULLETS; j++) {
                if (!enemy_bullets[j].active) {
                    enemy_bullets[j].x = enemies[i].x;
                    enemy_bullets[j].y = enemies[i].y;
                    enemy_bullets[j].dx = -1.2f; // Медленнее пули
                    enemy_bullets[j].dy = 0;
                    enemy_bullets[j].active = 1;
                    enemy_bullets[j].type = 0;
                    enemy_bullets[j].timer = 0;
                    enemies[i].shoot_timer = rand() % 100 + 50; // Еще реже стрельба
                    break;
                }
            }
        }
    }
}

void move_bullets() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            bullets[i].x += bullets[i].dx;
            bullets[i].y += bullets[i].dy;
            bullets[i].timer++;
            
            if (bullets[i].x >= WIDTH || bullets[i].x <= 0 || 
                bullets[i].y >= HEIGHT || bullets[i].y <= 0 ||
                bullets[i].timer > 100) {
                bullets[i].active = 0;
            }
        }
    }

    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
        if (enemy_bullets[i].active) {
            enemy_bullets[i].x += enemy_bullets[i].dx;
            enemy_bullets[i].y += enemy_bullets[i].dy;
            enemy_bullets[i].timer++;
            
            if (enemy_bullets[i].x >= WIDTH || enemy_bullets[i].x <= 0 || 
                enemy_bullets[i].y >= HEIGHT || enemy_bullets[i].y <= 0 ||
                enemy_bullets[i].timer > 150) {
                enemy_bullets[i].active = 0;
            }
        }
    }
}

void move_boss() {
    if (!boss.active) return;

    boss.move_timer++;
    boss.shoot_timer--;
    boss.attack_timer++;
    
    if (boss.invulnerable > 0) boss.invulnerable--;

    if (boss.move_timer % 5 == 0) {
        boss.x += boss.direction;
        
        if (boss.phase == 2) {
            boss.y += boss.vertical_direction;
        }
        
        if (boss.x <= 1 || boss.x >= WIDTH - BOSS_WIDTH - 1) {
            boss.direction *= -1;
            if (boss.phase == 2 && rand() % 3 == 0) {
                boss.vertical_direction = (rand() % 3) - 1;
            }
        }
        
        if (boss.phase == 2 && (boss.y <= 1 || boss.y >= HEIGHT - BOSS_HEIGHT - 1)) {
            boss.vertical_direction *= -1;
            if (rand() % 3 == 0) {
                boss.direction = (rand() % 3) - 1;
            }
        }
        
        if (rand() % (boss.phase == 1 ? 40 : 25) == 0) {
            boss.direction = (rand() % 3) - 1;
            if (boss.phase == 2) {
                boss.vertical_direction = (rand() % 3) - 1;
            }
        }
    }

    if (boss.shoot_timer <= 0) {
        if (boss.phase == 1) {
            if (boss.attack_pattern == 0) {
                boss_shoot(0);
                boss.attack_pattern = 1;
                boss.shoot_timer = 25;
            } else {
                boss_shoot(1);
                boss.attack_pattern = 0;
                boss.shoot_timer = 35;
            }
        } else if (boss.phase == 2) {
            if (boss.attack_pattern == 0) {
                boss_shoot(2);
                boss.attack_pattern = 1;
                boss.shoot_timer = 20;
            } else if (boss.attack_pattern == 1) {
                for (int i = 0; i < 5; i++) boss_shoot(1);
                boss.attack_pattern = 2;
                boss.shoot_timer = 30;
            } else {
                boss_shoot(3);
                boss.attack_pattern = 0;
                boss.shoot_timer = 45;
            }
        }
        
        if (boss.lives < BOSS_LIVES / 4 && !boss.rage_mode) {
            boss.rage_mode = 1;
            boss.shoot_timer = 8;
        }
    }
    
    if (boss.rage_mode && boss.shoot_timer % 8 == 0) {
        boss_shoot(rand() % 4);
    }
}

int all_enemies_dead() {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        if (enemies[i].alive) return 0;
    }
    return 1;
}

void check_collisions() {
    for (int i = 0; i < MAX_BULLETS; i++) {
        if (bullets[i].active) {
            for (int j = 0; j < MAX_ENEMIES; j++) {
                if (enemies[j].alive && 
                    bullets[i].x >= enemies[j].x - 1 && bullets[i].x <= enemies[j].x + 1 &&
                    bullets[i].y == enemies[j].y) {
                    
                    if (enemies[j].type == 2) { // Танк
                        static int tank_hits[MAX_ENEMIES] = {0};
                        tank_hits[j]++;
                        
                        if (tank_hits[j] >= 5) {
                            enemies[j].alive = 0;
                            score += 8;
                            tanks_killed++;
                            spawn_medkit(enemies[j].x, enemies[j].y);
                            tank_hits[j] = 0;
                        }
                    } else {
                        enemies[j].alive = 0;
                        score += 10;
                        enemies_killed++;
                    }
                    
                    bullets[i].active = 0;
                    break;
                }
            }
            
            if (boss.active && boss.invulnerable <= 0 &&
                bullets[i].x >= boss.x && bullets[i].x <= boss.x + BOSS_WIDTH &&
                bullets[i].y >= boss.y && bullets[i].y <= boss.y + BOSS_HEIGHT) {
                
                bullets[i].active = 0;
                boss.lives--;
                
                if (boss.lives > 0) {
                    boss.invulnerable = 10;
                    
                    if (boss.lives <= BOSS_LIVES/2 && boss.phase == 1) {
                        boss.phase = 2;
                        boss.invulnerable = 30;
                    }
                } else {
                    boss.active = 0;
                    boss_defeated = 1;
                    score += 80;
                    boss_killed++;
                    
                    // Бонус за сохраненные HP (5 очков за каждое HP)
                    int hp_bonus = player.lives * 5;
                    score += hp_bonus;
                    
                    // Бонус за быстрое прохождение (если меньше 90 секунд)
                    time_t current_time = time(NULL);
                    double play_time = difftime(current_time, start_time);
                    if (play_time < 90) {
                        int time_bonus = (int)((90 - play_time) * 2);
                        score += time_bonus;
                    }
                }
            }
        }


    }

    if (player.invulnerable <= 0) {
        for (int i = 0; i < MAX_ENEMY_BULLETS; i++) {
            if (enemy_bullets[i].active && 
                enemy_bullets[i].x >= player.x - 1 && enemy_bullets[i].x <= player.x + 1 &&
                enemy_bullets[i].y == player.y) {
                player.lives--;
                player.hit_timer = 10;
                player.invulnerable = 30;
                enemy_bullets[i].active = 0;
                printf("\a");
                fflush(stdout);
                break;
            }
        }
    }
    
    for (int i = 0; i < 2; i++) {
        if (medkits[i].active && 
            player.x >= medkits[i].x - 1 && player.x <= medkits[i].x + 1 &&
            player.y == medkits[i].y) {
            player.lives = min(3, player.lives + 1);
            medkits[i].active = 0;
            medkits_collected++; // Теперь точно увеличивается счетчик
        }
    }
    
    if (player.invulnerable > 0) player.invulnerable--;
    if (player.hit_timer > 0) player.hit_timer--;
}

void reset_game() {
    // Полный сброс игры
    enemies_killed = 0;
    tanks_killed = 0;
    boss_killed = 0;
    shots_fired = 0;
    score = 0;
    wave = 1;
    game_over = 0;
    boss_defeated = 0;
    start_time = time(NULL);
    
    // Правильный спавн игрока - плавное появление слева
    player.x = 2;
    player.y = HEIGHT / 2;
    player.lives = 3;
    player.max_lives = 3;
    player.hit_timer = 0;
    player.invulnerable = 0;
    player_spawn_timer = 0;
    
    for (int i = 0; i < 3; i++) meteors[i].active = 0;
    for (int i = 0; i < 2; i++) medkits[i].active = 0;
    for (int i = 0; i < MAX_BULLETS; i++) bullets[i].active = 0;
    for (int i = 0; i < MAX_ENEMY_BULLETS; i++) enemy_bullets[i].active = 0;
    for (int i = 0; i < MAX_ENEMIES; i++) enemies[i].alive = 0;
    boss.active = 0;
    
    spawn_enemies();
}


void draw_help_screen() {
    clear_screen();
    move_cursor(WIDTH / 2 - 10, 2);
    printf("\033[36m=== ИНФОРМАЦИЯ ОБ ИГРЕ ===\033[0m");
    
    move_cursor(5, 4);
    printf("\033[32mЗеленые враги (O)\033[0m - 10 очков | Двигаются горизонтально");
    move_cursor(5, 5);
    printf("\033[35mФиолетовые враги (@)\033[0m - 10 очков | Двигаются вертикально");
    move_cursor(5, 6);
    printf("\033[33mТанки (T)\033[0m - 8 очков | Требуют 5 попаданий");
    move_cursor(5, 7);
    printf("\033[34mБосс\033[0m - 80 очков | Меняет атаки при потере HP");
    
    move_cursor(5, 9);
    printf("\033[32m[HP]\033[0m - Аптечка | Восстанавливает 1 HP (макс. 3)");
    move_cursor(5, 10);
    printf("\033[37m#\033[0m - Метеорит | Наносит урон при столкновении");
    
    move_cursor(5, 12);
    printf("\033[33mДополнительные очки:\033[0m");
    move_cursor(5, 13);
    printf("+5 очков за каждое сохраненное HP после победы над боссом");
    move_cursor(5, 14);
    printf("Бонус за скорость: +2 очка за каждую секунду меньше 90 сек");
    move_cursor(5, 15);
    printf("\033[33mУправление:\033[0m");
    move_cursor(5, 16);
    printf("W - вверх S - вниз Space - стрелять Q - выйти ");
    move_cursor(5, 17);
    move_cursor(WIDTH / 2 - 10, HEIGHT - 2);
    printf("\033[36mНажмите 1 чтобы начать игру\033[0m");
    fflush(stdout);
}

void help_screen() {
    draw_help_screen();

    while (1) {
        if (kbhit()) {
            char ch = getchar();
            if (ch == '1') {
                return;
            }
        }
        usleep(10000);
    }
}

void draw_game_over_screen() {
    clear_screen();
    time_t current_time = time(NULL);
    double play_time = difftime(current_time, start_time);
    int minutes = (int)play_time / 60;
    int seconds = (int)play_time % 60;
    
    move_cursor(WIDTH / 2 - 6, HEIGHT / 2 - 4);
    printf("\033[31m=== GAME OVER ===\033[0m");
    
    move_cursor(WIDTH / 2 - 15, HEIGHT / 2 - 2);
    printf("Убито врагов: \033[33m%d\033[0m (+%d очков)", enemies_killed, enemies_killed * 10);
    
    if (boss_killed > 0) {
        move_cursor(WIDTH / 2 - 15, HEIGHT / 2);
        printf("Убито боссов: \033[33m%d\033[0m (+%d очков)", boss_killed, boss_killed * 80);
        
        move_cursor(WIDTH / 2 - 15, HEIGHT / 2 + 1);
        printf("Сохранено HP: \033[33m%d\033[0m (+%d очков)", player.lives, player.lives * 5);
        
        move_cursor(WIDTH / 2 - 15, HEIGHT / 2 + 2);
        printf("Время игры: \033[33m%02d:%02d\033[0m", minutes, seconds);
        
        if (play_time < 90) {
            int time_bonus = (int)((90 - play_time) * 2);
            move_cursor(WIDTH / 2 - 15, HEIGHT / 2 + 3);
            printf("Бонус за скорость: \033[33m+%d очков\033[0m", time_bonus);
        }
    }
    
    move_cursor(WIDTH / 2 - 15, HEIGHT / 2 + (boss_killed > 0 ? 5 : 3));
    printf("Всего очков: \033[33m%d\033[0m", score);
    
    move_cursor(WIDTH / 2 - 15, HEIGHT / 2 + (boss_killed > 0 ? 6 : 4));
    printf("Сделано выстрелов: \033[33m%d\033[0m", shots_fired);
    
    move_cursor(WIDTH / 2 - 10, HEIGHT / 2 + (boss_killed > 0 ? 8 : 6));
    printf("1 — Restart    2 — Exit");
    fflush(stdout);
}

void draw_victory_screen() {
    clear_screen();
    time_t current_time = time(NULL);
    double play_time = difftime(current_time, start_time);
    int minutes = (int)play_time / 60;
    int seconds = (int)play_time % 60;
    
    move_cursor(WIDTH / 2 - 8, HEIGHT / 2 - 4);
    printf("\033[32m=== VICTORY! ===\033[0m");
    
    move_cursor(WIDTH / 2 - 15, HEIGHT / 2 - 2);
    printf("Убито врагов: \033[33m%d\033[0m (+%d очков)", enemies_killed, enemies_killed * 10);
    
    move_cursor(WIDTH / 2 - 15, HEIGHT / 2);
    printf("Убито боссов: \033[33m%d\033[0m (+%d очков)", boss_killed, boss_killed * 80);
    
    move_cursor(WIDTH / 2 - 15, HEIGHT / 2 + 1);
    printf("Сохранено HP: \033[33m%d\033[0m (+%d очков)", player.lives, player.lives * 5);
    
    move_cursor(WIDTH / 2 - 15, HEIGHT / 2 + 2);
    printf("Время игры: \033[33m%02d:%02d\033[0m", minutes, seconds);
    
    if (play_time < 90) {
        int time_bonus = (int)((90 - play_time) * 2);
        move_cursor(WIDTH / 2 - 15, HEIGHT / 2 + 3);
        printf("Бонус за скорость: \033[33m+%d очков\033[0m", time_bonus);
    }
    
    move_cursor(WIDTH / 2 - 15, HEIGHT / 2 + 5);
    printf("Всего очков: \033[33m%d\033[0m", score);
    
    move_cursor(WIDTH / 2 - 15, HEIGHT / 2 + 6);
    printf("Сделано выстрелов: \033[33m%d\033[0m", shots_fired);
    
    move_cursor(WIDTH / 2 - 10, HEIGHT / 2 + 8);
    printf("1 — Restart    2 — Exit");
    fflush(stdout);
}

void game_over_screen() {
    draw_game_over_screen();

    while (1) {
        if (kbhit()) {
            char ch = getchar();
            if (ch == '1') {
                reset_game();
                return;
            } else if (ch == '2') {
                show_cursor();
                clear_screen();
                exit(0);
            }
        }
        usleep(10000);
    }
}

void victory_screen() {
    draw_victory_screen();

    while (1) {
        if (kbhit()) {
            char ch = getchar();
            if (ch == '1') {
                reset_game();
                return;
            } else if (ch == '2') {
                show_cursor();
                clear_screen();
                exit(0);
            }
        }
        usleep(10000);
    }
}

void boss_spawn() {
    boss.x = WIDTH / 2 - BOSS_WIDTH/2;
    boss.y = 3;
    boss.lives = BOSS_LIVES;
    boss.active = 1;
    boss.shoot_timer = 30;
    boss.move_timer = 0;
    boss.attack_timer = 0;
    boss.attack_pattern = 0;
    boss.invulnerable = 0;
    boss.phase = 1;
    boss.direction = 1;
    boss.vertical_direction = 0;
    boss.rage_mode = 0;
}

void draw_intro_screen() {
    clear_screen();
    move_cursor(WIDTH / 2 - 8, HEIGHT / 2 - 2);
    printf("\033[36mSPACE SHOOTER\033[0m");
    move_cursor(WIDTH / 2 - 10, HEIGHT / 2);
    printf("\033[33mUse W/S to move\033[0m");
    move_cursor(WIDTH / 2 - 10, HEIGHT / 2 + 1);
    printf("\033[33mSPACE to shoot\033[0m");
    move_cursor(WIDTH / 2 - 10, HEIGHT / 2 + 3);
    printf("\033[36mPwrt\033[0m");
    fflush(stdout);
}

void intro_screen() {
    draw_intro_screen();

    while (1) {
        if (kbhit()) {
            char ch = getchar();
            if (ch == '1') {
                return;
            }
        }
        usleep(10000);
    }
}

int main() {
    srand(time(NULL));
    set_conio_terminal_mode();
    hide_cursor();

    help_screen();
    reset_game();

    while (!game_over) {
        handle_input();
        
        if (player_spawn_timer < 30) {
            player_spawn_timer++;
            if (player.x < 3) player.x++;
        }
        
        move_bullets();
        move_enemies();
        move_boss();
        move_meteors();
        move_medkits();
        
        spawn_meteor();
        
        check_collisions();
        draw();

        if (wave_display_timer < 30) {
            wave_display_timer++;
        }

        if (!boss.active && all_enemies_dead()) {
            wave++;
            wave_display_timer = 0;
            if (wave % 5 == 0) {
                boss_spawn();
            } else {
                spawn_enemies();
            }
        }

        if (player.lives <= 0) {
            game_over_screen();
        }
        
        if (boss_defeated) {
            victory_screen();
        }

        usleep(30000);
    }

    show_cursor();
    return 0;
}
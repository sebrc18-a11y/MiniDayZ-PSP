#include <SDL.h>
#include <SDL_image.h>
#include <pspkernel.h>
#include <math.h>

/* libSDL2main.a ya define su propio PSP_MODULE_INFO internamente;
 * declararlo aquí también causaría símbolos duplicados al enlazar. */

#define SCREEN_WIDTH   480
#define SCREEN_HEIGHT  272
#define MOVE_SPEED     150.0f
#define STICK_DEADZONE 8000

/* player.png: hoja de 128x128, cuadrícula 4x4 (16 fotogramas de 32x32).
 * Layout real (confirmado jugando):
 *   fila 0 = las 4 poses ESTÁTICAS, una por dirección (no un ciclo de caminar):
 *            columna 0 = quieto abajo, columna 1 = quieto arriba,
 *            columna 2 = quieto derecha, columna 3 = quieto izquierda
 *   fila 1 = ciclo de caminar hacia abajo
 *   fila 2 = ciclo de caminar hacia arriba
 *   fila 3 = ciclo de caminar hacia la derecha (de perfil)
 * No hay fila propia de "caminar izquierda": se reutiliza la fila 3
 * volteada horizontalmente. Para quieto-izquierda sí existe pose propia
 * (columna 3 de la fila 0), así que ahí no se voltea nada. */
#define FRAME_WIDTH     32
#define FRAME_HEIGHT    32
#define FRAME_COUNT     4
#define ANIM_FRAME_TIME 0.12f

#define ROW_IDLE 0
#define ROW_DOWN 1
#define ROW_UP   2
#define ROW_SIDE 3

#define IDLE_COL_DOWN  0
#define IDLE_COL_UP    1
#define IDLE_COL_RIGHT 2
#define IDLE_COL_LEFT  3

/* Mapa de prueba: todavía no es el mapa real de Mini DayZ (ese vive
 * empaquetado dentro de data.js y no es extraíble, como vimos antes).
 * Esto solo existe para probar que la cámara y el desplazamiento
 * funcionan -- son tiles de color sólido generados en código,
 * no arte del juego. */
#define TILE_SIZE    32
#define MAP_COLS     30
#define MAP_ROWS     20
#define WORLD_WIDTH  (MAP_COLS * TILE_SIZE)
#define WORLD_HEIGHT (MAP_ROWS * TILE_SIZE)

#define TILE_GRASS 0
#define TILE_DIRT  1
#define TILE_WALL  2

/* Objetos recogibles: por ahora todos del mismo tipo (lata de atún),
 * solo para probar que la recolección funciona. */
#define ITEM_SIZE  15
#define ITEM_COUNT 5

typedef struct {
    float x, y;
    int active;
} Item;

typedef enum { DIR_DOWN, DIR_UP, DIR_LEFT, DIR_RIGHT } Direction;

/* El zombi comparte el mismo layout de hoja de sprites que el jugador
 * (fila 0 = poses estáticas, filas 1-3 = ciclos de caminar), confirmado
 * viendo zombie.png directamente. */
#define ZOMBIE_SPEED         100.0f
#define ZOMBIE_DETECT_RADIUS 150.0f
#define ZOMBIE_DAMAGE           10.0f
#define ZOMBIE_DAMAGE_COOLDOWN  0.8f
#define PLAYER_MAX_HEALTH      100.0f

typedef struct {
    float x, y;
    Direction facing;
    int isMoving;
    int animFrame;
    float animTimer;
} Zombie;

static int exit_callback(int arg1, int arg2, void *common) {
    sceKernelExitGame();
    return 0;
}

static int CallbackThread(SceSize args, void *argp) {
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

static int SetupCallbacks(void) {
    int thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }
    return thid;
}

static int worldMap[MAP_ROWS][MAP_COLS];

static void BuildTestMap(void) {
    int row, col;
    for (row = 0; row < MAP_ROWS; row++) {
        for (col = 0; col < MAP_COLS; col++) {
            worldMap[row][col] = ((row + col) % 5 == 0) ? TILE_DIRT : TILE_GRASS;
        }
    }

    /* Un bloque de paredes de prueba, para tener contra qué chocar.
     * Lejos del centro del mapa a propósito: el jugador aparece en el
     * centro exacto, y la versión anterior de este bloque (filas 6-10,
     * columnas 12-18) caía justo encima de ese punto de aparición. */
    for (row = 2; row <= 4; row++) {
        for (col = 3; col <= 7; col++) {
            worldMap[row][col] = TILE_WALL;
        }
    }
}

/* Fuera del mapa cuenta como bloqueado, igual que una pared. */
static int IsTileBlocking(int row, int col) {
    if (row < 0 || row >= MAP_ROWS || col < 0 || col >= MAP_COLS) {
        return 1;
    }
    return worldMap[row][col] == TILE_WALL;
}

/* Revisa las 4 esquinas del sprite en la posición propuesta. */
static int CanMoveTo(float newX, float newY) {
    int leftCol   = (int)(newX / TILE_SIZE);
    int rightCol  = (int)((newX + FRAME_WIDTH - 1) / TILE_SIZE);
    int topRow    = (int)(newY / TILE_SIZE);
    int bottomRow = (int)((newY + FRAME_HEIGHT - 1) / TILE_SIZE);

    if (IsTileBlocking(topRow, leftCol))     return 0;
    if (IsTileBlocking(topRow, rightCol))    return 0;
    if (IsTileBlocking(bottomRow, leftCol))  return 0;
    if (IsTileBlocking(bottomRow, rightCol)) return 0;
    return 1;
}

static void PlaceTestItems(Item items[ITEM_COUNT]) {
    /* Posiciones fijas, repartidas por el mapa y lejos del bloque de
     * paredes (filas 2-4, columnas 3-7) y del punto de aparición
     * (centro del mapa). Cada item se centra dentro de su tile. */
    static const int rows[ITEM_COUNT] = { 2, 15, 17, 5, 10 };
    static const int cols[ITEM_COUNT] = { 20, 5, 22, 25, 2 };
    int i;

    for (i = 0; i < ITEM_COUNT; i++) {
        items[i].x = cols[i] * TILE_SIZE + (TILE_SIZE - ITEM_SIZE) / 2.0f;
        items[i].y = rows[i] * TILE_SIZE + (TILE_SIZE - ITEM_SIZE) / 2.0f;
        items[i].active = 1;
    }
}

/* Colisión simple de rectángulos (AABB) entre el jugador y un item. */
static int Overlaps(float ax, float ay, float aw, float ah,
                     float bx, float by, float bw, float bh) {
    return (ax < bx + bw) && (ax + aw > bx) &&
           (ay < by + bh) && (ay + ah > by);
}

/* Elige fila/columna dentro de la hoja de sprites según la dirección
 * y si la entidad se está moviendo o no. La usan tanto el jugador
 * como el zombi, porque comparten el mismo layout de hoja. */
static void GetSpriteFrame(Direction facing, int isMoving, int animFrame,
                            int *outRow, int *outCol, SDL_RendererFlip *outFlip) {
    *outFlip = SDL_FLIP_NONE;

    if (isMoving) {
        *outCol = animFrame;
        switch (facing) {
            case DIR_UP:    *outRow = ROW_UP;   break;
            case DIR_LEFT:  *outRow = ROW_SIDE; *outFlip = SDL_FLIP_HORIZONTAL; break;
            case DIR_RIGHT: *outRow = ROW_SIDE; break;
            case DIR_DOWN:
            default:        *outRow = ROW_DOWN; break;
        }
    } else {
        *outRow = ROW_IDLE;
        switch (facing) {
            case DIR_UP:    *outCol = IDLE_COL_UP;    break;
            case DIR_LEFT:  *outCol = IDLE_COL_LEFT;  break;
            case DIR_RIGHT: *outCol = IDLE_COL_RIGHT; break;
            case DIR_DOWN:
            default:        *outCol = IDLE_COL_DOWN;  break;
        }
    }
}

/* IA del zombi: si el jugador está dentro del radio de detección, se
 * mueve hacia él en línea recta; si no, se queda quieto. Respeta las
 * mismas paredes que el jugador (reutiliza CanMoveTo). */
static void UpdateZombie(Zombie *z, float playerX, float playerY, float deltaTime) {
    float dx = playerX - z->x;
    float dy = playerY - z->y;
    float dist = sqrtf(dx * dx + dy * dy);

    float moveX = 0.0f;
    float moveY = 0.0f;

    if (dist < ZOMBIE_DETECT_RADIUS && dist > 0.001f) {
        moveX = dx / dist;
        moveY = dy / dist;
    }

    z->isMoving = (moveX != 0.0f || moveY != 0.0f);

    if (z->isMoving) {
        if (fabsf(moveX) > fabsf(moveY)) {
            z->facing = (moveX < 0) ? DIR_LEFT : DIR_RIGHT;
        } else {
            z->facing = (moveY < 0) ? DIR_UP : DIR_DOWN;
        }

        z->animTimer += deltaTime;
        if (z->animTimer >= ANIM_FRAME_TIME) {
            z->animTimer -= ANIM_FRAME_TIME;
            z->animFrame = (z->animFrame + 1) % FRAME_COUNT;
        }
    } else {
        z->animFrame = 0;
        z->animTimer = 0.0f;
    }

    float newX = z->x + moveX * ZOMBIE_SPEED * deltaTime;
    float newY = z->y + moveY * ZOMBIE_SPEED * deltaTime;

    if (CanMoveTo(newX, z->y)) {
        z->x = newX;
    }
    if (CanMoveTo(z->x, newY)) {
        z->y = newY;
    }
}

int main(int argc, char *argv[]) {
    SetupCallbacks();

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window *window = SDL_CreateWindow(
        "MiniDayZ-PSP",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_Texture *playerTexture = IMG_LoadTexture(renderer, "player.png");
    SDL_Texture *itemTexture = IMG_LoadTexture(renderer, "item.png");
    SDL_Texture *zombieTexture = IMG_LoadTexture(renderer, "zombie.png");

    BuildTestMap();

    Item items[ITEM_COUNT];
    PlaceTestItems(items);
    int pickedUpCount = 0;

    /* Zombi de prueba: fila 10, columna 20 -- lejos del bloque de
     * paredes (filas 2-4, columnas 3-7) y a una distancia inicial
     * mayor que ZOMBIE_DETECT_RADIUS desde el punto de aparición del
     * jugador, para poder ver el cambio de quieto a persiguiendo. */
    Zombie zombie;
    zombie.x = 20 * TILE_SIZE;
    zombie.y = 10 * TILE_SIZE;
    zombie.facing = DIR_DOWN;
    zombie.isMoving = 0;
    zombie.animFrame = 0;
    zombie.animTimer = 0.0f;

    SDL_GameController *pad = NULL;

    /* x,y ahora son coordenadas dentro del MUNDO, no de la pantalla. */
    float x = (WORLD_WIDTH - FRAME_WIDTH) / 2.0f;
    float y = (WORLD_HEIGHT - FRAME_HEIGHT) / 2.0f;

    int dpad_up = 0, dpad_down = 0, dpad_left = 0, dpad_right = 0;

    Direction facing = DIR_DOWN;
    int isMoving = 0;
    int animFrame = 0;
    float animTimer = 0.0f;

    float playerHealth = PLAYER_MAX_HEALTH;
    float damageCooldown = 0.0f;
    int isDead = 0;

    int running = 1;
    SDL_Event event;
    Uint32 lastTicks = SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_QUIT:
                    running = 0;
                    break;

                case SDL_CONTROLLERDEVICEADDED:
                    pad = SDL_GameControllerOpen(event.cdevice.which);
                    break;

                case SDL_CONTROLLERBUTTONDOWN:
                case SDL_CONTROLLERBUTTONUP: {
                    int pressed = (event.type == SDL_CONTROLLERBUTTONDOWN);
                    switch (event.cbutton.button) {
                        case SDL_CONTROLLER_BUTTON_DPAD_UP:    dpad_up = pressed;    break;
                        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:  dpad_down = pressed;  break;
                        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:  dpad_left = pressed;  break;
                        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: dpad_right = pressed; break;
                        case SDL_CONTROLLER_BUTTON_START:
                            if (pressed) running = 0;
                            break;
                        default:
                            break;
                    }
                    break;
                }

                default:
                    break;
            }
        }

        Uint32 nowTicks = SDL_GetTicks();
        float deltaTime = (nowTicks - lastTicks) / 1000.0f;
        lastTicks = nowTicks;

        if (!isDead) {
            float moveX = 0.0f;
            float moveY = 0.0f;

            if (dpad_left)  moveX -= 1.0f;
            if (dpad_right) moveX += 1.0f;
            if (dpad_up)    moveY -= 1.0f;
            if (dpad_down)  moveY += 1.0f;

            if (pad) {
                Sint16 axisX = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTX);
                Sint16 axisY = SDL_GameControllerGetAxis(pad, SDL_CONTROLLER_AXIS_LEFTY);

                if (axisX > STICK_DEADZONE || axisX < -STICK_DEADZONE) {
                    moveX = axisX / 32768.0f;
                }
                if (axisY > STICK_DEADZONE || axisY < -STICK_DEADZONE) {
                    moveY = axisY / 32768.0f;
                }
            }

            isMoving = (moveX != 0.0f || moveY != 0.0f);

            if (isMoving) {
                /* La dirección dominante decide la animación, incluso en diagonal.
                 * "facing" conserva su último valor mientras no hay movimiento,
                 * para que la pose estática sepa hacia dónde debe mirar. */
                if (fabsf(moveX) > fabsf(moveY)) {
                    facing = (moveX < 0) ? DIR_LEFT : DIR_RIGHT;
                } else {
                    facing = (moveY < 0) ? DIR_UP : DIR_DOWN;
                }

                animTimer += deltaTime;
                if (animTimer >= ANIM_FRAME_TIME) {
                    animTimer -= ANIM_FRAME_TIME;
                    animFrame = (animFrame + 1) % FRAME_COUNT;
                }
            } else {
                animFrame = 0;
                animTimer = 0.0f;
            }

            {
                float newX = x + moveX * MOVE_SPEED * deltaTime;
                float newY = y + moveY * MOVE_SPEED * deltaTime;

                /* X e Y se comprueban por separado: si una pared te frena en
                 * X, igual puedes seguir moviéndote en Y (deslizar contra
                 * la pared en vez de quedar pegado). */
                if (CanMoveTo(newX, y)) {
                    x = newX;
                }
                if (CanMoveTo(x, newY)) {
                    y = newY;
                }
            }

            /* Los límites del MUNDO siguen aplicando además de las paredes. */
            if (x < 0) x = 0;
            if (x > WORLD_WIDTH - FRAME_WIDTH) x = WORLD_WIDTH - FRAME_WIDTH;
            if (y < 0) y = 0;
            if (y > WORLD_HEIGHT - FRAME_HEIGHT) y = WORLD_HEIGHT - FRAME_HEIGHT;

            {
                int i;
                for (i = 0; i < ITEM_COUNT; i++) {
                    if (!items[i].active) {
                        continue;
                    }
                    if (Overlaps(x, y, FRAME_WIDTH, FRAME_HEIGHT,
                                 items[i].x, items[i].y, ITEM_SIZE, ITEM_SIZE)) {
                        items[i].active = 0;
                        pickedUpCount++;
                    }
                }
            }

            UpdateZombie(&zombie, x, y, deltaTime);

            /* Daño de contacto: si el zombi te toca, pierdes vida cada
             * ZOMBIE_DAMAGE_COOLDOWN segundos mientras sigas en contacto,
             * no en cada frame (si no, morirías instantáneamente). */
            if (damageCooldown > 0.0f) {
                damageCooldown -= deltaTime;
            }

            if (damageCooldown <= 0.0f &&
                Overlaps(x, y, FRAME_WIDTH, FRAME_HEIGHT,
                         zombie.x, zombie.y, FRAME_WIDTH, FRAME_HEIGHT)) {
                playerHealth -= ZOMBIE_DAMAGE;
                damageCooldown = ZOMBIE_DAMAGE_COOLDOWN;

                if (playerHealth <= 0.0f) {
                    playerHealth = 0.0f;
                    isDead = 1;
                }
            }
        }

        /* La cámara se centra en el jugador, sin salirse de los bordes del mundo. */
        float cameraX = x + FRAME_WIDTH / 2.0f - SCREEN_WIDTH / 2.0f;
        float cameraY = y + FRAME_HEIGHT / 2.0f - SCREEN_HEIGHT / 2.0f;

        if (cameraX < 0) cameraX = 0;
        if (cameraX > WORLD_WIDTH - SCREEN_WIDTH) cameraX = WORLD_WIDTH - SCREEN_WIDTH;
        if (cameraY < 0) cameraY = 0;
        if (cameraY > WORLD_HEIGHT - SCREEN_HEIGHT) cameraY = WORLD_HEIGHT - SCREEN_HEIGHT;

        SDL_SetRenderDrawColor(renderer, 25, 25, 35, 255);
        SDL_RenderClear(renderer);

        /* Dibujar solo los tiles visibles en pantalla, no el mapa completo. */
        {
            int firstCol = (int)(cameraX / TILE_SIZE);
            int firstRow = (int)(cameraY / TILE_SIZE);
            int lastCol = (int)((cameraX + SCREEN_WIDTH) / TILE_SIZE) + 1;
            int lastRow = (int)((cameraY + SCREEN_HEIGHT) / TILE_SIZE) + 1;
            int row, col;

            if (firstCol < 0) firstCol = 0;
            if (firstRow < 0) firstRow = 0;
            if (lastCol > MAP_COLS) lastCol = MAP_COLS;
            if (lastRow > MAP_ROWS) lastRow = MAP_ROWS;

            for (row = firstRow; row < lastRow; row++) {
                for (col = firstCol; col < lastCol; col++) {
                    SDL_Rect tileRect = {
                        (int)(col * TILE_SIZE - cameraX),
                        (int)(row * TILE_SIZE - cameraY),
                        TILE_SIZE,
                        TILE_SIZE
                    };

                    if (worldMap[row][col] == TILE_WALL) {
                        SDL_SetRenderDrawColor(renderer, 90, 90, 95, 255);
                    } else if (worldMap[row][col] == TILE_DIRT) {
                        SDL_SetRenderDrawColor(renderer, 120, 90, 60, 255);
                    } else {
                        SDL_SetRenderDrawColor(renderer, 60, 120, 60, 255);
                    }
                    SDL_RenderFillRect(renderer, &tileRect);
                }
            }
        }

        if (itemTexture) {
            int i;
            for (i = 0; i < ITEM_COUNT; i++) {
                if (!items[i].active) {
                    continue;
                }
                SDL_Rect dst = {
                    (int)(items[i].x - cameraX),
                    (int)(items[i].y - cameraY),
                    ITEM_SIZE,
                    ITEM_SIZE
                };
                SDL_RenderCopy(renderer, itemTexture, NULL, &dst);
            }
        }

        if (zombieTexture) {
            int row, col;
            SDL_RendererFlip flip;
            GetSpriteFrame(zombie.facing, zombie.isMoving, zombie.animFrame, &row, &col, &flip);

            SDL_Rect src = { col * FRAME_WIDTH, row * FRAME_HEIGHT, FRAME_WIDTH, FRAME_HEIGHT };
            SDL_Rect dst = { (int)(zombie.x - cameraX), (int)(zombie.y - cameraY), FRAME_WIDTH, FRAME_HEIGHT };
            SDL_RenderCopyEx(renderer, zombieTexture, &src, &dst, 0.0, NULL, flip);
        }

        if (playerTexture) {
            int row, col;
            SDL_RendererFlip flip;
            GetSpriteFrame(facing, isMoving, animFrame, &row, &col, &flip);

            SDL_Rect src = { col * FRAME_WIDTH, row * FRAME_HEIGHT, FRAME_WIDTH, FRAME_HEIGHT };
            SDL_Rect dst = { (int)(x - cameraX), (int)(y - cameraY), FRAME_WIDTH, FRAME_HEIGHT };
            SDL_RenderCopyEx(renderer, playerTexture, &src, &dst, 0.0, NULL, flip);
        }

        /* Contador de items recogidos: fijo en pantalla, no se mueve con
         * la cámara. Un cuadrito por item; relleno = ya recogido. */
        {
            int i;
            const int pipSize = 12;
            const int pipGap = 4;
            for (i = 0; i < ITEM_COUNT; i++) {
                SDL_Rect pip = { 8 + i * (pipSize + pipGap), 8, pipSize, pipSize };
                if (i < pickedUpCount) {
                    SDL_SetRenderDrawColor(renderer, 230, 190, 60, 255);
                    SDL_RenderFillRect(renderer, &pip);
                } else {
                    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
                    SDL_RenderDrawRect(renderer, &pip);
                }
            }
        }

        /* Barra de vida: fondo oscuro fijo + relleno verde proporcional
         * a la vida actual. Debajo del contador de items. */
        {
            const int barX = 8;
            const int barY = 28;
            const int barWidth = 100;
            const int barHeight = 10;
            int fillWidth = (int)(barWidth * (playerHealth / PLAYER_MAX_HEALTH));

            SDL_Rect barBg = { barX, barY, barWidth, barHeight };
            SDL_SetRenderDrawColor(renderer, 60, 20, 20, 255);
            SDL_RenderFillRect(renderer, &barBg);

            if (fillWidth > 0) {
                SDL_Rect barFill = { barX, barY, fillWidth, barHeight };
                SDL_SetRenderDrawColor(renderer, 90, 200, 90, 255);
                SDL_RenderFillRect(renderer, &barFill);
            }
        }

        /* Al morir, un tinte rojo semitransparente sobre toda la
         * pantalla -- el mundo se queda congelado (isDead detiene el
         * bloque de arriba), pero visualmente queda claro qué pasó. */
        if (isDead) {
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
            SDL_SetRenderDrawColor(renderer, 150, 0, 0, 90);
            SDL_Rect fullScreen = { 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT };
            SDL_RenderFillRect(renderer, &fullScreen);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
        }

        SDL_RenderPresent(renderer);
    }

    if (playerTexture) {
        SDL_DestroyTexture(playerTexture);
    }
    if (itemTexture) {
        SDL_DestroyTexture(itemTexture);
    }
    if (zombieTexture) {
        SDL_DestroyTexture(zombieTexture);
    }
    if (pad) {
        SDL_GameControllerClose(pad);
    }
    IMG_Quit();
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
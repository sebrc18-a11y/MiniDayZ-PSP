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

typedef enum { DIR_DOWN, DIR_UP, DIR_LEFT, DIR_RIGHT } Direction;

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

    SDL_GameController *pad = NULL;

    float x = (SCREEN_WIDTH - FRAME_WIDTH) / 2.0f;
    float y = (SCREEN_HEIGHT - FRAME_HEIGHT) / 2.0f;

    int dpad_up = 0, dpad_down = 0, dpad_left = 0, dpad_right = 0;

    Direction facing = DIR_DOWN;
    int isMoving = 0;
    int animFrame = 0;
    float animTimer = 0.0f;

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

        x += moveX * MOVE_SPEED * deltaTime;
        y += moveY * MOVE_SPEED * deltaTime;

        if (x < 0) x = 0;
        if (x > SCREEN_WIDTH - FRAME_WIDTH) x = SCREEN_WIDTH - FRAME_WIDTH;
        if (y < 0) y = 0;
        if (y > SCREEN_HEIGHT - FRAME_HEIGHT) y = SCREEN_HEIGHT - FRAME_HEIGHT;

        SDL_SetRenderDrawColor(renderer, 25, 25, 35, 255);
        SDL_RenderClear(renderer);

        if (playerTexture) {
            int row;
            int col;
            SDL_RendererFlip flip = SDL_FLIP_NONE;

            if (isMoving) {
                col = animFrame;
                switch (facing) {
                    case DIR_UP:    row = ROW_UP;   break;
                    case DIR_LEFT:  row = ROW_SIDE; flip = SDL_FLIP_HORIZONTAL; break;
                    case DIR_RIGHT: row = ROW_SIDE; break;
                    case DIR_DOWN:
                    default:        row = ROW_DOWN; break;
                }
            } else {
                row = ROW_IDLE;
                switch (facing) {
                    case DIR_UP:    col = IDLE_COL_UP;    break;
                    case DIR_LEFT:  col = IDLE_COL_LEFT;  break;
                    case DIR_RIGHT: col = IDLE_COL_RIGHT; break;
                    case DIR_DOWN:
                    default:        col = IDLE_COL_DOWN;  break;
                }
            }

            SDL_Rect src = { col * FRAME_WIDTH, row * FRAME_HEIGHT, FRAME_WIDTH, FRAME_HEIGHT };
            SDL_Rect dst = { (int)x, (int)y, FRAME_WIDTH, FRAME_HEIGHT };
            SDL_RenderCopyEx(renderer, playerTexture, &src, &dst, 0.0, NULL, flip);
        }

        SDL_RenderPresent(renderer);
    }

    if (playerTexture) {
        SDL_DestroyTexture(playerTexture);
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
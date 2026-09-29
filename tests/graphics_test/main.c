#include <SDL.h>
#include <pspkernel.h>

/* libSDL2main.a ya define su propio PSP_MODULE_INFO internamente;
 * declararlo aquí también causaría símbolos duplicados al enlazar. */

#define SCREEN_WIDTH   480
#define SCREEN_HEIGHT  272
#define SQUARE_SIZE    20
#define MOVE_SPEED     3.0f
#define STICK_DEADZONE 8000

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

    SDL_Window *window = SDL_CreateWindow(
        "MiniDayZ-PSP",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        0
    );

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

    SDL_GameController *pad = NULL;

    float x = (SCREEN_WIDTH - SQUARE_SIZE) / 2.0f;
    float y = (SCREEN_HEIGHT - SQUARE_SIZE) / 2.0f;

    int dpad_up = 0, dpad_down = 0, dpad_left = 0, dpad_right = 0;

    int running = 1;
    SDL_Event event;

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

        x += moveX * MOVE_SPEED;
        y += moveY * MOVE_SPEED;

        if (x < 0) x = 0;
        if (x > SCREEN_WIDTH - SQUARE_SIZE) x = SCREEN_WIDTH - SQUARE_SIZE;
        if (y < 0) y = 0;
        if (y > SCREEN_HEIGHT - SQUARE_SIZE) y = SCREEN_HEIGHT - SQUARE_SIZE;

        SDL_SetRenderDrawColor(renderer, 25, 25, 35, 255);
        SDL_RenderClear(renderer);

        SDL_Rect square = { (int)x, (int)y, SQUARE_SIZE, SQUARE_SIZE };
        SDL_SetRenderDrawColor(renderer, 90, 210, 130, 255);
        SDL_RenderFillRect(renderer, &square);

        SDL_RenderPresent(renderer);
    }

    if (pad) {
        SDL_GameControllerClose(pad);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
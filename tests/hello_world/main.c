#include <pspkernel.h>
#include <pspdebug.h>

PSP_MODULE_INFO("MiniDayZPSP_HelloWorld", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

#define printf pspDebugScreenPrintf

int exit_callback(int arg1, int arg2, void *common) {
    sceKernelExitGame();
    return 0;
}

int CallbackThread(SceSize args, void *argp) {
    int cbid;
    cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

int SetupCallbacks(void) {
    int thid = sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, 0, 0);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, 0);
    }
    return thid;
}

int main(void) {
    pspDebugScreenInit();
    SetupCallbacks();

    printf("MiniDayZ-PSP\n");
    printf("Hello World - el toolchain funciona.\n");

    sceKernelSleepThread();
    return 0;
}
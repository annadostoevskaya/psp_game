/**
 * File: psp_main.cpp
 * Author: github.com/annadostoevskaya
 * Date: 08/29/2023 21:38:27
 * Last Modified Date: 09/07/2023 01:38:40
 */

#include <pspkernel.h>
#include <pspmoduleinfo.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <psprtc.h>
#include <stdio.h>
#include <stdlib.h>

PSP_MODULE_INFO("TINYRENDERER", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define TRUE  1
#define FALSE 0

const int pspScreenWidth = 480; // 480*272*sizeof(int) =  510KB, 
const int pspScreenHeight = 272; // ~ x 2 ~ 1MB for double buffering
const int pspLineSize = 512;

SceFloat32 psp_calcDeltaTime(u64 curTick, u64 lastTick)
{
    SceFloat32 tickres = (SceFloat32)(sceRtcGetTickResolution() / 1000);
    u32 dtick = curTick - lastTick;
    return (SceFloat32)dtick / tickres; // NOTE: tick / tick/ms -> ms
}

enum SCREEN_BUFFERS
{
    SCREEN_BUFFER_FIRST = 0,
    SCREEN_BUFFER_SECOND,
    SCREEN_BUFFER_COUNT
};

struct Screen
{
    u32 *buffer;
    u32 size;
    u16 width;
    u16 height;
};

void gtick(Screen *screen, float dt)
{
    (void)screen; 
    (void)(dt);

    // game
}

#include "tinyrend.cpp"

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    
    // init ticks
    u64 curTick = 0;
    u64 lastTick = 0;
    SceFloat32 frameTime = 1000.0f / sceDisplayGetFramePerSec();
    SceFloat32 deltaTime = frameTime;

    // init screen
    u32 *vram = (u32*)(0x40000000 | (u32)sceGeEdramGetAddr());
    const size_t screenBufferSize = pspLineSize * pspScreenHeight;
    sceDisplaySetMode(0, pspScreenWidth, pspScreenHeight);
    u32 *screenBuffers[SCREEN_BUFFER_COUNT];
    screenBuffers[SCREEN_BUFFER_FIRST] = vram;
    screenBuffers[SCREEN_BUFFER_SECOND] = vram + screenBufferSize;

    Screen screen;
    screen.buffer = screenBuffers[SCREEN_BUFFER_FIRST];
    screen.size = screenBufferSize;
    screen.width = pspLineSize; // pspScreenWidth;
    screen.height = pspScreenHeight;
    /*
    SceUID astFileHandler = sceIoOpen("./ASSETS.AST", PSP_O_RDONLY, 0777);
    size_t storageSize = 4 * 1024 * 1024;
    u8 *storage = (u8*)malloc(storageSize);
    if (astFileHandler > 0)
    {
        sceIoRead(astFileHandler, storage, storageSize);
    }
    else
    {
        // TODO(annad): Error handling!
        return -1;
    }
    */
    while (TRUE)
    {
        // rendering, switch buffer
        sceDisplaySetFrameBuf((void*)screen.buffer, 
            pspLineSize, 
            PSP_DISPLAY_PIXEL_FORMAT_8888, 
            PSP_DISPLAY_SETBUF_IMMEDIATE);

        screen.buffer  = (screen.buffer != screenBuffers[SCREEN_BUFFER_FIRST])
            ? screenBuffers[SCREEN_BUFFER_FIRST] 
            : screenBuffers[SCREEN_BUFFER_SECOND];

        for (u32 i = 0; i < screen.size; i += 1)
        {
            screen.buffer[i] = 0x0;
        }

        gtick(&screen, 1.0f/60.0f);
        tiny_renderer_test(&screen);
        
        // tick
        sceRtcGetCurrentTick(&curTick);
        deltaTime = psp_calcDeltaTime(curTick, lastTick);
        s32 delay = (s32)(1000.0f * (frameTime - deltaTime));
        if (delay > 0)
        {
            sceKernelDelayThread(delay);
        }

#if DEBUG_BUILD
        pspDebugScreenInitEx(screen.buffer, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
        pspDebugScreenPrintf("Hello\n");
        SceFloat32 realFps = 1000.0f / deltaTime;
        pspDebugScreenPrintf("Real FPS: %f\n", realFps);
        pspDebugScreenPrintf("frameTime: %f ms\n", frameTime);
        pspDebugScreenPrintf("deltaTime: %f ms\n", deltaTime);
        pspDebugScreenPrintf("delay: %fms\n", (SceFloat32)delay / 1000.0f);
#endif

        sceRtcGetCurrentTick(&curTick);
        deltaTime = psp_calcDeltaTime(curTick, lastTick);
        lastTick = curTick;
    }

	return 0;
}


/**
 * File: main.c
 * Author: github.com/annadostoevskaya
 * Date: 08/29/2023 21:38:27
 * Last Modified Date: 09/06/2023 15:18:11
 */

#include <pspkernel.h>
#include <pspmoduleinfo.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <psprtc.h>
#include <stdio.h>

PSP_MODULE_INFO("TINYRENDERER", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

typedef float f32;
// typedef unsigned int u32;
typedef int i32;
// typedef b32 u32;
typedef char byte;

#define TRUE  1
#define FALSE 0

const int pspScreenWidth = 480; // 480*272*sizeof(int) =  510KB, 
const int pspScreenHeight = 272; // ~ x 2 ~ 1MB for double buffering
const int pspLineSize = 512;

void gtick(byte *screen, f32 dt)
{
    (void)screen; 
    (void)(dt);
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    
    // init ticks
    u32 tickRes = sceRtcGetTickResolution() / 1000; // ticks per ms
    u64 curTick = 0;
    u64 lastTick = 0;
    u32 dtick = 0;
    f32 fps = sceDisplayGetFramePerSec();
    f32 frameTime = 1000.0f / fps;
    f32 deltaTime = frameTime;

    // init screen
    u32 *vram = (u32*)(0x40000000 | (u32)sceGeEdramGetAddr());
    const size_t screenBufferSize = pspLineSize * pspScreenHeight * (4 * sizeof(byte));
    sceDisplaySetMode(0, pspScreenWidth, pspScreenHeight);
    byte *screenBuffers[2];
    screenBuffers[0] =(byte*)(vram);
    screenBuffers[1] = screenBuffers[0] + screenBufferSize;
    byte *currentBuffer = screenBuffers[0];
    
    while (TRUE)
    {
        // rendering, switch buffer
        sceDisplaySetFrameBuf((void*)currentBuffer, 
            pspLineSize, 
            PSP_DISPLAY_PIXEL_FORMAT_8888, 
            PSP_DISPLAY_SETBUF_IMMEDIATE);
        currentBuffer = currentBuffer != screenBuffers[0] 
            ? screenBuffers[0] 
            : screenBuffers[1];

        pspDebugScreenInitEx(currentBuffer, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
        pspDebugScreenPrintf("Hello\n");
        f32 realFps = 1000.0f / deltaTime;
        pspDebugScreenPrintf("FPS: %f\n", fps);
        pspDebugScreenPrintf("Real FPS: %f\n", realFps);
        pspDebugScreenPrintf("frameTime: %f ms\n", frameTime);
        pspDebugScreenPrintf("deltaTime: %f ms\n", deltaTime);
        
        gtick(currentBuffer, deltaTime);
        
        // tick
        sceRtcGetCurrentTick(&lastTick);
        dtick = (u32)(lastTick - curTick);
        deltaTime = (f32)dtick / (f32)tickRes; // tick / tick/ms -> ms
        i32 delay = (i32)(1000.0f * (frameTime - deltaTime));
        pspDebugScreenPrintf("delay: %fms\n", (float)delay / 1000.0f);
        if (delay > 0)
        {
            sceKernelDelayThread(delay);
        }

        sceRtcGetCurrentTick(&lastTick);
        dtick = lastTick - curTick;
        deltaTime = (f32)dtick / (f32)tickRes; // tick / tick/ms -> ms

        curTick = lastTick;
    }

	return 0;
}

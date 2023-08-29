/**
 * File: main.c
 * Author: github.com/annadostoevskaya
 * Date: 08/29/2023 21:38:27
 * Last Modified Date: 08/30/2023 00:23:37
 */

#include <pspkernel.h>
#include <pspmoduleinfo.h>
#include <pspdisplay.h>
#include <psprtc.h>
#include <stdio.h>

PSP_MODULE_INFO("TINYRENDERER", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

typedef float f32;
// typedef unsigned int u32;
typedef int i32;
// typedef b32 u32;

#define TRUE  1
#define FALSE 0

const int pspScreenWidth = 480;
const int pspScreenHeight = 272;

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    u32 tickRes = sceRtcGetTickResolution() / 1000; // ticks per ms
    u64 curTick = 0;
    u64 lastTick = 0;
    u32 dtick = 0;
    f32 fps = sceDisplayGetFramePerSec();
    f32 frameTime = 1000.0f / fps;
    f32 deltaTime = 0.0f;

    
    if (sceRtcGetCurrentTick(&curTick) != 0)
    {
        // TODO(annad): Error handling!
        return -1;
    }

    while (TRUE)
    {
        // rendering

        if (sceRtcGetCurrentTick(&lastTick) != 0)
        {
            // TODO(annad): Error handling!
            return -1;
        }

        dtick = lastTick - curTick;
        deltaTime = (f32)dtick / (f32)tickRes; // tick / tick/ms -> ms
        // f32 realFps = 1000.0f / deltaTime;
        // pspDebugScreenPrintf("FPS: %f\n", 1000.0f / deltaTime);
        // pspDebugScreenPrintf("CURTICK:  %lld ticks\n", curTick / tickRes);
        // pspDebugScreenPrintf("LASTTICK: %lld ticks\n", lastTick / tickRes);
        // pspDebugScreenPrintf("frameTime:  %f ms\n", frameTime);
        // pspDebugScreenPrintf("deltaTime: %f ms\n", deltaTime);

        if (deltaTime < frameTime)
        {
            SceUInt delay = (SceUInt)(frameTime - deltaTime);
            sceKernelDelayThread(delay);
            deltaTime += (f32)delay;
        }
        
        curTick = lastTick;
    }

	return 0;
}

/**
 * File: psp_main.cpp
 * Author: github.com/annadostoevskaya
 * Date: 08/29/2023 21:38:27
 * Last Modified Date: 09/17/2023 02:29:33
 */

#include <pspkernel.h>
#include <pspsysmem.h>
#include <pspmoduleinfo.h>
#include <pspmodulemgr.h>
#include <pspdisplay.h>
#include <pspge.h>
#include <psprtc.h>
#include <stdio.h>
#include <stdlib.h>

#define BYTE(x) (x)
#define KB(x)   (BYTE(x) * 1024L)
#define MB(x)   (KB(x) * 1024L)

PSP_MODULE_INFO("TINYRENDERER", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(MB(24) / 1024L);

const int pspScreenWidth = 480; // 480*272*sizeof(int) =  510KB, 
const int pspScreenHeight = 272; // ~ x 2 ~ 1MB for double buffering
const int pspLineSize = 512;

void memory_zeroing(u32* memory, size_t size)
{
    // TODO(annad): SSE 4.2? LOL
    while(size--) memory[size] = 0x0;
}

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

struct Arena
{
    u8 *memory;
    size_t size;
    size_t offset;
};

u8 *arena_alloc(Arena *arena, size_t requested)
{
    if (requested % 2 == 0)
    {
        size_t mask = requested - 1;
        arena->offset = (arena->offset + mask) & ~mask;
    }

    if (arena->offset + requested >= arena->size)
    {
        return NULL;
    }
    
    size_t current_offset = arena->offset;
    arena->offset += requested;
    return &arena->memory[current_offset];
}

void arena_reset(Arena *arena)
{
    arena->offset = 0;
}

void write_str(char *dst_str, size_t dst_str_sz, const char *src_str)
{
    while (dst_str_sz-- && *src_str != '\0')
        *dst_str++ = *src_str++;
}

#include "platform_asset.cpp"

struct Game
{
    enum 
    {
        STATE_INIT = 0,
        STATE_UPLOAD_RES,
        STATE_MAIN,

        STATE_COUNT
    } state;

    Asset *asset;
};

void gtick(Game *game, Screen *screen, Arena *arena, float dt)
{
    (void)screen; 
    (void)(dt);

    Asset *asset = game->asset;

    static Resource resources[2];
    static int curres_idx = 0;

    switch (game->state)
    {
        case Game::STATE_INIT:
        {
            // TODO(annad): Custom allocator for user space!
            resources[0].path = (char*)arena_alloc(arena, asset_path_str_size);
            write_str(resources[0].path, asset_path_str_size, "./OBJ/AFRICAN_HEAD.OBJ");
            resources[0].state = Resource::STATE_INACTIVE;

            resources[1].path = (char*)arena_alloc(arena, asset_path_str_size);
            write_str(resources[1].path, asset_path_str_size, "./OBJ/AFRICAN_HEAD_DIFFUSE.BMP");
            resources[1].state = Resource::STATE_INACTIVE;

            game->state = Game::STATE_UPLOAD_RES;
        } break;

        case Game::STATE_UPLOAD_RES:
        {
            Resource *curres = &resources[curres_idx];
            int resource_count = sizeof(resources) / sizeof(resources[0]);

            asset_processing(asset, curres, arena);

            // NOTE(annad): Maybe callback?..
            if (asset->state == Asset::STATE_UPLOADING)
            {
                float percent = 100.0f * (float)asset->uploaded / (float)asset->size;
                printf("Uploading resource %s: %.2f%%\r", asset->path, percent);
                fflush(stdout);
                break;
            }

            if (curres->state == Resource::STATE_COMPLETED 
                && curres_idx < resource_count)
            {
                printf("Uploading resource %s: [COMPLETE]\n", curres->path);
                curres_idx += 1;
            }

            if (curres_idx == (sizeof(resources) / sizeof(resources[0])))
                game->state = Game::STATE_MAIN;
        } break;

        case Game::STATE_MAIN:
        {
            printf("resource:%s\n", resources[0].path);
            printf("size:%d\n", resources[0].size);
            game->state = Game::STATE_COUNT; // NOTE(annad): Blah-blah-blah...
        }

        default:
        {

        } break;
    }

    // game
}

#include "tinyrend.cpp"
#include "psp_asset.cpp"

#define PROFILING_START(name) \
        u64 profiler_StartTick ## name = 0; \
        sceRtcGetCurrentTick(&profiler_StartTick ## name)

#define PROFILING_END(name) \
        u64 profiler_EndTick ## name = 0; \
        sceRtcGetCurrentTick(&profiler_EndTick ## name); \
        SceFloat32 profiler_Delta ## name = \
            (SceFloat32)(profiler_EndTick ## name - profiler_StartTick ## name) \
            / (SceFloat32)(sceRtcGetTickResolution() / 1000); \
        (void)(profiler_Delta ## name)

#define PROFILING_PRINT(name) \
        pspDebugScreenPrintf(#name ": %.4fms\n", profiler_Delta ## name)

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    
    Arena arena = {};
    arena.size = MB(16);
    SceUID block_id = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, "GLOBAL_BLOCK", PSP_SMEM_Low, arena.size, NULL);
    if (block_id < 0) return -1; // TODO(annad): Handling error! 
    arena.memory = (u8*)sceKernelGetBlockHeadAddr(block_id);
    if (arena.memory == NULL) return -1; // TODO(annad): Handling error! 
    memory_zeroing((u32*)arena.memory, arena.size / 4);

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

    Screen screen = {};
    screen.buffer = screenBuffers[SCREEN_BUFFER_FIRST];
    screen.size = screenBufferSize;
    screen.width = pspLineSize; // pspScreenWidth;
    screen.height = pspScreenHeight;

    Asset asset = {};
    asset.ctx = arena_alloc(&arena, sizeof(SceUID));
    asset.data = NULL;
    asset.state = Asset::STATE_INACTIVE;
    asset.path = asset_path;

    Game game = {};
    game.state = Game::STATE_INIT;
    game.asset = &asset;

    PROFILING_START(DebugScreenInit);
    pspDebugScreenInitEx(screen.buffer, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
    PROFILING_END(DebugScreenInit);

    for (;;)
    {
        PROFILING_START(GameLoop);
        // rendering, switch buffer
        sceDisplaySetFrameBuf((void*)screen.buffer, 
            pspLineSize, 
            PSP_DISPLAY_PIXEL_FORMAT_8888, 
            PSP_DISPLAY_SETBUF_IMMEDIATE);

        screen.buffer  = (screen.buffer != screenBuffers[SCREEN_BUFFER_FIRST])
            ? screenBuffers[SCREEN_BUFFER_FIRST] 
            : screenBuffers[SCREEN_BUFFER_SECOND];

        PROFILING_START(ZeroingScreen);
        memory_zeroing(screen.buffer, screen.size);
        PROFILING_END(ZeroingScreen);
        pspDebugScreenSetBase(screen.buffer);
        pspDebugScreenSetXY(0, 0);


#if DEBUG_BUILD
        // pspDebugScreenPrintf("Hello\n");
        SceFloat32 FPS = 1000.0f / deltaTime;
        pspDebugScreenPrintf("FPS: %.4f\n", FPS);
        // pspDebugScreenPrintf("frameTime: %f ms\n", frameTime);
        pspDebugScreenPrintf("dt: %.4f ms\n", deltaTime);
        // pspDebugScreenPrintf("delay: %fms\n", (SceFloat32)delay / 1000.0f);
#endif
        gtick(&game, &screen, &arena, 1.0f/60.0f);

        // psp asset processing
        psp_asset_processing(&asset);

        PROFILING_END(GameLoop);
        PROFILING_PRINT(GameLoop);
        PROFILING_PRINT(ZeroingScreen);
        PROFILING_PRINT(DebugScreenInit);

        // tick
        sceRtcGetCurrentTick(&curTick);
        deltaTime = psp_calcDeltaTime(curTick, lastTick);
        // FIXME(annad): Difficult bug in this part. FPU Exception!
        SceFloat32 delay = (1000.0f * (frameTime - deltaTime));
        if (delay > 0.0f)
        {
            sceKernelDelayThread((u32)delay);
        }

        sceRtcGetCurrentTick(&curTick);
        deltaTime = psp_calcDeltaTime(curTick, lastTick);
        lastTick = curTick;
    }
    
    arena_reset(&arena);
    sceKernelFreePartitionMemory(block_id);

	return 0;
}


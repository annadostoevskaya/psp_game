/**
 * File: psp_main.cpp
 * Author: github.com/annadostoevskaya
 * Date: 08/29/2023 21:38:27
 * Last Modified Date: 09/12/2023 21:16:33
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

PSP_MODULE_INFO("TINYRENDERER", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

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

enum AssetState
{
    ASSET_STATE_INACTIVE = 0,
    ASSET_STATE_REQUESTED,
    ASSET_STATE_RESOLVED,
    ASSET_STATE_UPLOADING,
    ASSET_STATE_UPLOADED,
    ASSET_STATE_COMPLETED,
    ASSET_STATE_RELEASED,

    ASSET_STATE_COUNT,
    ASSET_STATE_UNDEFINED
};

// TODO(annad): Is it make sense?..
const int asset_path_str_size = 64;
char asset_path[asset_path_str_size];

struct Asset
{
    void *ctx;
    char *path;
    char *data;

    int size;
    int uploaded;

    AssetState state;
};

void write_str(char *dst_str, size_t dst_str_sz, const char *src_str)
{
    while (dst_str_sz-- && *src_str != '\0')
        *dst_str++ = *src_str++;
}

void asset_request(Asset *asset, const char *respath)
{
    write_str(asset->path, asset_path_str_size, respath);
    asset->data = NULL;
    asset->uploaded = 0;
    asset->size = 0;
    asset->state = ASSET_STATE_REQUESTED;
}

void asset_upload(Asset *asset, void *dst)
{
    asset->data = (char*)dst;
    asset->state = ASSET_STATE_UPLOADING;
}

void asset_complete(Asset *asset)
{
    asset->data = NULL;
    asset->uploaded = 0;
    asset->size = 0;
    asset->state = ASSET_STATE_COMPLETED;
}

enum ResourceState
{
    RESOURCE_STATE_INACTIVE,
    RESOURCE_STATE_COMPLETED,

    RESOURCE_STATE_COUNT,
    RESOURCE_STATE_UNDEFINED
};

struct Resource
{
    char *path;
    char *data;
    int size;

    ResourceState state;
};

void asset_processing(Asset *asset, Resource *res)
{
    switch (asset->state)
    {
        case ASSET_STATE_REQUESTED:
        case ASSET_STATE_UPLOADING:
        case ASSET_STATE_COMPLETED:
        {
            // ...
        } break;

        case ASSET_STATE_INACTIVE:
        {
            if (res->state == RESOURCE_STATE_INACTIVE)
            {
                asset_request(asset, res->path);
            }
        } break;

        case ASSET_STATE_RESOLVED:
        {
            void *data = malloc(asset->size); // TODO(annad): alloc memory
            asset_upload(asset, data);
        } break;

        case ASSET_STATE_UPLOADED:
        {
            res->data = asset->data; // pick up data
            res->size = asset->size;
            res->state = RESOURCE_STATE_COMPLETED;
            asset_complete(asset);
        } break;

        case ASSET_STATE_RELEASED:
        {
            asset->state = ASSET_STATE_INACTIVE;
        } break;

        default:
        {
            asset->state = ASSET_STATE_UNDEFINED;
        } break;
    }
}

void gtick(Screen *screen, Asset *asset, float dt)
{
    (void)screen; 
    (void)asset;
    (void)(dt);

    static int game_state = 0;
    static Resource resources[2];
    static int curres_idx = 0;

    switch (game_state)
    {
        case 0:
        {
            resources[0].path = (char*)malloc(asset_path_str_size);
            write_str(resources[0].path, asset_path_str_size, "./OBJ/AFRICAN_HEAD.OBJ");
            resources[0].state = RESOURCE_STATE_INACTIVE;

            resources[1].path = (char*)malloc(asset_path_str_size);
            write_str(resources[1].path, asset_path_str_size, "./OBJ/AFRICAN_HEAD_DIFFUSE.BMP");
            resources[1].state = RESOURCE_STATE_INACTIVE;

            game_state = 1;
        } break;

        case 1:
        {
            Resource *curres = &resources[curres_idx];
            int resource_count = sizeof(resources) / sizeof(resources[0]);

            asset_processing(asset, curres);

            // NOTE(annad): Maybe callback?..
            if (asset->state == ASSET_STATE_UPLOADING)
            {
                float percent = 100.0f * (float)asset->uploaded / (float)asset->size;
                printf("Uploading resource %s: %.2f\r", asset->path, percent);
                fflush(stdout);
                break;
            }

            if (curres->state == RESOURCE_STATE_COMPLETED 
                && curres_idx < resource_count)
            {
                printf("Uploading resource %s: [COMPLETE]\n", curres->path);
                curres_idx += 1;
            }

            if (curres_idx == (sizeof(resources) / sizeof(resources[0])))
                game_state = 2;
        } break;

        case 2:
        {
            printf("resource:%s\n", resources[0].path);
            printf("size:%d\n", resources[0].size);
            game_state = 3;
        }

        default:
        {

        } break;
    }

    // game
}

#include "tinyrend.cpp"

void psp_asset_processing(Asset *asset)
{
    switch (asset->state)
    {
        case ASSET_STATE_INACTIVE:
        case ASSET_STATE_RESOLVED:
        case ASSET_STATE_UPLOADED:
        case ASSET_STATE_RELEASED:
        {
            // ... 
        } break;

        case ASSET_STATE_REQUESTED:
        {
            if (asset->ctx != NULL) 
                break;

            SceIoStat filestat;
            if (sceIoGetstat(asset->path, &filestat) < 0)
            {
                asset->state = ASSET_STATE_UNDEFINED;
                break;
            }

            asset->size = filestat.st_size;
            asset->ctx = malloc(sizeof(SceUID));
            SceUID *fhandler = (SceUID*)asset->ctx;
            *fhandler = sceIoOpen(asset->path, PSP_O_RDONLY, 0777);

            asset->state = ASSET_STATE_RESOLVED;
        } break;

        case ASSET_STATE_UPLOADING:
        {
            SceUID fhandler = *((SceUID*)asset->ctx);
            if (asset->data == NULL)
            {
                asset->state = ASSET_STATE_UNDEFINED;
                break;
            }

            void *cursor = (void*)(asset->data + asset->uploaded);
            int chunk_size = 1024; // TODO(annad): Calc optimal variant!
            asset->uploaded += sceIoRead(fhandler, cursor, chunk_size);

            if (asset->size == asset->uploaded)
                asset->state = ASSET_STATE_UPLOADED;
        } break;

        case ASSET_STATE_COMPLETED:
        {
            SceUID fhandler = *((SceUID*)asset->ctx);
            if (sceIoClose(fhandler) < 0)
                asset->state = ASSET_STATE_UNDEFINED;

            free(asset->ctx);
            asset->ctx = NULL;
            asset->state = ASSET_STATE_RELEASED;
        } break;

        default:
        {
            asset->state = ASSET_STATE_UNDEFINED;
        } break;
    }
}

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

    Asset asset;
    asset.ctx = NULL;
    asset.data = NULL;
    asset.state = ASSET_STATE_INACTIVE;
    asset.path = asset_path;

    SceSize storage_size = 1024 * 1024 * 1024;
    storage_size *= 16;
    SceUID blockid = sceKernelAllocPartitionMemory(PSP_MEMORY_PARTITION_USER, "GLOBAL_BLOCK", PSP_SMEM_Low, storage_size, NULL);
    char *storage = (char*)sceKernelGetBlockHeadAddr(blockid);
    for (SceSize i = 0; i < storage_size; i += 1)
    {
        storage[i] = 0x0;
    }

    sceKernelFreePartitionMemory(blockid);

    for (;;)
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

#if DEBUG_BUILD
        pspDebugScreenInitEx(screen.buffer, PSP_DISPLAY_PIXEL_FORMAT_8888, 0);
        // pspDebugScreenPrintf("Hello\n");
        SceFloat32 FPS = 1000.0f / deltaTime;
        pspDebugScreenPrintf("FPS: %f\n", FPS);
        // pspDebugScreenPrintf("frameTime: %f ms\n", frameTime);
        pspDebugScreenPrintf("dt: %f ms\n", deltaTime);
        // pspDebugScreenPrintf("delay: %fms\n", (SceFloat32)delay / 1000.0f);
#endif

        gtick(&screen, &asset, 1.0f/60.0f);
        // psp asset processing
        psp_asset_processing(&asset);
        
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

	return 0;
}


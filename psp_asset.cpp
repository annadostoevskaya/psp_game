/**
 * File: psp_asset.cpp
 * Author: github.com/annadostoevskaya
 * Date: 09/14/2023 23:55:00
 * Last Modified Date: 09/17/2023 02:11:07
 */

#include "platform_asset.h"

void psp_asset_processing(Asset *asset)
{
    switch (asset->state)
    {
        case Asset::STATE_INACTIVE:
        case Asset::STATE_RESOLVED:
        case Asset::STATE_UPLOADED:
        case Asset::STATE_RELEASED:
        {
            // ... 
        } break;

        case Asset::STATE_REQUESTED:
        {
            if (asset->ctx == NULL) 
            {
                asset->state = Asset::STATE_UNDEFINED;
                break;
            }

            SceIoStat filestat;
            if (sceIoGetstat(asset->path, &filestat) < 0)
            {
                asset->state = Asset::STATE_UNDEFINED;
                break;
            }

            asset->size = filestat.st_size;
            SceUID *fhandler = (SceUID*)asset->ctx;
            *fhandler = sceIoOpen(asset->path, PSP_O_RDONLY, 0777);

            asset->state = Asset::STATE_RESOLVED;
        } break;

        case Asset::STATE_UPLOADING:
        {
            SceUID *fhandler = (SceUID*)asset->ctx;
            if (asset->data == NULL)
            {
                asset->state = Asset::STATE_UNDEFINED;
                break;
            }

            void *cursor = (void*)(asset->data + asset->uploaded);
            size_t chunk_size = KB(512); // TODO(annad): Calc optimal variant!
            asset->uploaded += sceIoRead(*fhandler, cursor, chunk_size);

            if (asset->size == asset->uploaded)
                asset->state = Asset::STATE_UPLOADED;
        } break;

        case Asset::STATE_COMPLETED:
        {
            SceUID *fhandler = (SceUID*)asset->ctx;
            if (sceIoClose(*fhandler) < 0)
                asset->state = Asset::STATE_UNDEFINED;

            *fhandler = -1;
            asset->state = Asset::STATE_RELEASED;
        } break;

        default:
        {
            asset->state = Asset::STATE_UNDEFINED;
        } break;
    }
}


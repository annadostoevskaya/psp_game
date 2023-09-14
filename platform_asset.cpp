/**
 * File: platform_asset.cpp
 * Author: github.com/annadostoevskaya
 * Date: 09/14/2023 23:49:20
 * Last Modified Date: 09/15/2023 00:40:23
 */

#include "platform_asset.h"

// TODO(annad): Is it make sense?..
const int asset_path_str_size = 64;
char asset_path[asset_path_str_size];

void asset_request(Asset *asset, const char *respath)
{
    write_str(asset->path, asset_path_str_size, respath);
    asset->data = NULL;
    asset->uploaded = 0;
    asset->size = 0;
    asset->state = Asset::STATE_REQUESTED;
}

void asset_upload(Asset *asset, void *dst)
{
    asset->data = (char*)dst;
    asset->state = Asset::STATE_UPLOADING;
}

void asset_complete(Asset *asset)
{
    asset->data = NULL;
    asset->uploaded = 0;
    asset->size = 0;
    asset->state = Asset::STATE_COMPLETED;
}

void asset_processing(Asset *asset, Resource *res, Arena *arena)
{
    switch (asset->state)
    {
        case Asset::STATE_REQUESTED:
        case Asset::STATE_UPLOADING:
        case Asset::STATE_COMPLETED:
        {
            // ...
        } break;

        case Asset::STATE_INACTIVE:
        {
            if (res->state == Resource::STATE_INACTIVE)
            {
                asset_request(asset, res->path);
            }
        } break;

        case Asset::STATE_RESOLVED:
        {
            void *data = arena_alloc(arena, asset->size);
            if (data == NULL)
            {
                asset->state = Asset::STATE_UNDEFINED;
                break;
            }

            asset_upload(asset, data);
        } break;

        case Asset::STATE_UPLOADED:
        {
            res->data = asset->data; // pick up data
            res->size = asset->size;
            res->state = Resource::STATE_COMPLETED;
            asset_complete(asset);
        } break;

        case Asset::STATE_RELEASED:
        {
            asset->state = Asset::STATE_INACTIVE;
        } break;

        default:
        {
            asset->state = Asset::STATE_UNDEFINED;
        } break;
    }
}


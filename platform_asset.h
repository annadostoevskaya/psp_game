/**
 * File: platform_asset.h
 * Author: github.com/annadostoevskaya
 * Date: 09/14/2023 23:50:10
 * Last Modified Date: 09/15/2023 00:19:58
 */

#pragma once

struct Resource
{
    char *path;
    char *data;
    size_t size;

    enum 
    {
        STATE_INACTIVE = 0,
        STATE_COMPLETED,

        STATE_COUNT,
        STATE_UNDEFINED
    } state;
};

struct Asset
{
    void *ctx;
    char *path;
    char *data;

    size_t size;
    size_t uploaded;

   enum 
   {
        STATE_INACTIVE = 0,
        STATE_REQUESTED,
        STATE_RESOLVED,
        STATE_UPLOADING,
        STATE_UPLOADED,
        STATE_COMPLETED,
        STATE_RELEASED,

        STATE_COUNT,
        STATE_UNDEFINED
    } state;
};

void asset_request(Asset *asset, const char *respath);
void asset_upload(Asset *asset, void *dst);
void asset_complete(Asset *asset);
void asset_processing(Asset *asset, Resource *res, Arena *arena);


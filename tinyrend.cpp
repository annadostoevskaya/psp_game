/**
 * File: tinyrend.cpp
 * Author: github.com/annadostoevskaya
 * Date: 09/06/2023 22:19:00
 * Last Modified Date: 09/07/2023 01:57:13
 */

#include <algorithm>

template <typename T>
void custom_swap(T& x, T& y)
{
    x ^= y;
    y ^= x;
}

template <typename T>
T custom_abs(T x)
{
    if (x < 0) 
        return -x;
    return x;
}

void screen_set_color(Screen *screen, int x, int y, int color)
{
    if (y * screen->width + x < (int)screen->size)
        screen->buffer[(screen->height - y) * screen->width + x] = color;
}

void line(Screen *screen, int x0, int y0, int x1, int y1, int color)
{
    bool steep = false;
    if (std::abs(x0 - x1) < std::abs(y0 - y1))
    {
        std::swap(x0, y0);
        std::swap(x1, y1);
        steep = true;
    }

    if (x0 > x1)
    {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }

    for (int x = x0; x < x1; x += 1)
    {
        float t = (x - x0) / (float)(x1 - x0);
        int y = y0 * (1.0f - t) + y1 * t;
        if (steep)
            screen_set_color(screen, y, x, color);
        else 
            screen_set_color(screen, x, y, color);
    }
}

#include "tinyrend_model.cpp"

void tiny_renderer_test(Screen *screen)
{
    Model *model = new Model("./OBJ/AFRICAN_HEAD.OBJ");
    for (int i = 0; i < model->nfaces(); i += 1) 
    {
        std::vector<int> face = model->face(i);
        for (int j = 0; j < 3; j += 1) 
        {
            Vec3f v0 = model->vert(face[j]);
            Vec3f v1 = model->vert(face[(j + 1) % 3]);
            int x0 = (v0.x + 1.0f) * screen->width / 2.0f;
            int y0 = (v0.y + 1.0f) * screen->height / 2.0f;
            int x1 = (v1.x + 1.0f) * screen->width / 2.0f;
            int y1 = (v1.y + 1.0f) * screen->height / 2.0f;
            line(screen, x0, y0, x1, y1, 0xffffff);
        }
    }

    delete model;
}

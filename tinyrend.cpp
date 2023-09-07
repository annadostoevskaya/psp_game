/**
 * File: tinyrend.cpp
 * Author: github.com/annadostoevskaya
 * Date: 09/06/2023 22:19:00
 * Last Modified Date: 09/09/2023 00:01:06
 */

#include "tinyrend_geometry.h"

void screen_set_color(Screen *screen, int x, int y, int color)
{
    if (y * screen->width + x < (int)screen->size)
        screen->buffer[(screen->height - y) * screen->width + x] = color;
}

void tr_line(Screen *screen, Vec2i v1, Vec2i v2, int color)
{
    int x0 = v1.x;
    int y0 = v1.y;
    int x1 = v2.x;
    int y1 = v2.y;

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

    int dx = x1 - x0;
    int dy = y1 - y0;
    int derror2 = std::abs(dy) * 2;
    int error2 = 0;
    int y = y0;
    for (int x = x0; x < x1; x += 1)
    {
        if (steep)
            screen_set_color(screen, y, x, color);
        else 
            screen_set_color(screen, x, y, color);

        error2 += derror2;
        if (error2 > dx)
        {
            y += (y1 > y0 ? 1 : -1);
            error2 -= dx * 2;
        }
    }
}

Vec3f tr_barycentric(Vec2i *pts, Vec2i P)
{
    Vec3f u = Vec3f(pts[2][0] - pts[0][0], pts[1][0] - pts[0][0], pts[0][0] - P[0])
            ^ Vec3f(pts[2][1] - pts[0][1], pts[1][1] - pts[0][1], pts[0][1] - P[1]);
    if (std::abs(u.z) < 1) return Vec3f(-1, 1, 1);
    return Vec3f(1.0f - (u.x + u.y) / u.z, u.y / u.z, u.x / u.z);
}

void tr_triangle(Screen *screen, Vec2i *pts, int color)
{
    Vec2i bboxmin(screen->width - 1, screen->height - 1);
    Vec2i bboxmax(0, 0);
    Vec2i clamp(screen->width - 1, screen->height - 1);

    for (int i = 0; i < 3; i += 1)
    {
        bboxmin.x = std::max(0, std::min(bboxmin.x, pts[i].x));
        bboxmin.y = std::max(0, std::min(bboxmin.y, pts[i].y));

        bboxmax.x = std::min(clamp.x, std::max(bboxmax.x, pts[i].x));
        bboxmax.y = std::min(clamp.y, std::max(bboxmax.y, pts[i].y));
    }

    Vec2i P;
    for(P.x = bboxmin.x; P.x <= bboxmax.x; P.x += 1)
    {
        for (P.y = bboxmin.y; P.y <= bboxmax.y; P.y += 1)
        {
            Vec3f bcScreen = tr_barycentric(pts, P);
            if (bcScreen.x < 0 || bcScreen.y < 0 || bcScreen.z < 0)
                continue;

            screen_set_color(screen, P.x, P.y, color);
        }
    }
}

#include "tinyrend_model.cpp"

void tiny_renderer_test(Screen *screen)
{
    Model *model = new Model("./OBJ/AFRICAN_HEAD.OBJ");
    Vec3f light(0, 0, -1);
    for (int i = 0; i < model->nfaces(); i += 1)
    {
        std::vector face = model->face(i);
        Vec2i screen_coords[3];
        Vec3f world_coords[3];
        for (int j = 0; j < 3; j += 1)
        {
            Vec3f v = model->vert(face[j]);
            screen_coords[j] = Vec2i(
                (v.x + 1.0f) * screen->width / 2.0f,
                (v.y + 1.0f) * screen->height / 2.0f
            );
            world_coords[j] = v;
        }
        
        Vec3f n = (world_coords[2] - world_coords[0]) ^ (world_coords[1] - world_coords[0]);
        n.normalize();
        float intensity = n * light;
        if (intensity > 0)
        {
            tr_triangle(screen, screen_coords, (int)(intensity * 255.0f)
                                            | (int)(intensity * 255.0f) << 8 
                                            | (int)(intensity * 255.0f) << 16);
        }
    }

    delete model;
}

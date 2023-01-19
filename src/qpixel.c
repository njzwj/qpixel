#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "qpixel.h"

#define EPS 1e-6

// ================================
// MATH
// ================================

// =====================================================
// RENDER
// =====================================================

typedef struct
{
    vec4_t pndc;        // point ndc (normalized)
    vec2_t ps;          // point screen
    float  w;           // 1/z
    float  *vary;       // varying
    size_t vary_size;
} vertex_t;

typedef struct
{
    vertex_t *tl, *tr;
    vertex_t *bl, *br;
} trapezoid_t;

typedef struct
{
    float l, r, y;
    vertex_t *p;
    vertex_t *step;
} scanline_t;


// free a vertex
void vertex_destroy(vertex_t *v)
{
    assert(v != NULL);
    assert(v->vary != NULL);
    free(v->vary);
    free(v);
}

// copy v to new vertex. if v == NULL, return a new one.
vertex_t *vertex_split(vertex_t *v)
{
    vertex_t *r = (vertex_t *)malloc(sizeof(vertex_t));
    r->vary = NULL;
    r->vary_size = 0;
    if (v == NULL) return r;

    r->pndc = v->pndc;
    r->ps = v->ps;
    r->w = v->w;
    r->vary_size = v->vary_size;
    r->vary = (float *)malloc(v->vary_size * sizeof(float));
    memcpy(r->vary, v->vary, sizeof(float) * v->vary_size);
    return r;
}

void vertex_copy(vertex_t *dst, vertex_t *src)
{
    assert(dst->vary_size == src->vary_size);
    assert(dst->vary != NULL);
    assert(src->vary != NULL);
    float *vary = dst->vary;
    *dst = *src;
    dst->vary = vary;
    memcpy(dst->vary, src->vary, sizeof(float) * src->vary_size);
}

// returns a new vertex
vertex_t *vertex_new(vec4_t pndc,
                     vec2_t ps,
                     float  w,
                     float  *vary,
                     size_t vary_size
                     )
{
    vertex_t *res = vertex_split(NULL);
    res->pndc = pndc;
    res->ps = ps;
    res->w = w;
    res->vary = (float*)malloc(sizeof(float) * vary_size);
    res->vary_size = vary_size;
    memcpy(res->vary, vary, sizeof(float) * vary_size);
    return res;
}

// a += b
void vertex_add(vertex_t *a, vertex_t *b)
{
    a->pndc.x += b->pndc.x;
    a->pndc.y += b->pndc.y;
    a->pndc.z += b->pndc.z;
    a->ps.x += b->ps.x;
    a->ps.y += b->ps.y;
    a->w += b->w;
    for (int i = 0; i < a->vary_size; i++)
    {
        a->vary[i] += b->vary[i];
    }
}

// a -= b
void vertex_sub(vertex_t *a, vertex_t *b)
{
    a->pndc.x -= b->pndc.x;
    a->pndc.y -= b->pndc.y;
    a->pndc.z -= b->pndc.z;
    a->ps.x -= b->ps.x;
    a->ps.y -= b->ps.y;
    a->w -= b->w;
    for (int i = 0; i < a->vary_size; i++)
    {
        a->vary[i] -= b->vary[i];
    }
}

// a *= b
void vertex_mul(vertex_t *a, float b)
{
    a->pndc.x *= b;
    a->pndc.y *= b;
    a->pndc.z *= b;
    a->ps.x *= b;
    a->ps.y *= b;
    a->w *= b;
    for (int i = 0; i < a->vary_size; i++)
    {
        a->vary[i] *= b;
    }
}

// a /= b
void vertex_div(vertex_t *a, float b)
{
    vertex_mul(a, 1.0f / b);
}

// interpolate vertex
vertex_t *vertex_lerp(vertex_t *a, vertex_t *b, float r)
{
    assert(a != NULL);
    assert(b != NULL);
    vertex_t *res = vertex_split(a);
    vertex_t *tmp = vertex_split(b);
    vertex_mul(res, 1.0f - r);
    vertex_mul(tmp, r);
    vertex_add(res, tmp);
    vertex_destroy(tmp);
    return res;
}

void swap_ptrs(void **p1, void **p2)
{
    void *t = *p1;
    *p1 = *p2;
    *p2 = t;
}

unsigned char float_to_int(float x)
{
    x = clip_float(x, 0.0f, 1.0f);
    return (unsigned char)roundf(x * 255);
}

void fill_buffer(device_t *device, int x, int y, color3_t * color, float depth)
{
    y = device->height - y - 1;
    unsigned char * ptr = device->colorBuffer + x * 4 + y * 4 * device->width;
    *(ptr++) = float_to_int(color->b);
    *(ptr++) = float_to_int(color->g);
    *(ptr++) = float_to_int(color->r);
    device->depthBuffer[x + y * device->width] = depth;
}

/**
 * @brief Given 3 vertices, caculate the cross prod of a->b and a->c
 * 
 * @param v     Vertices (3)
 * @return int  Whether the prod is > 0
 */
int face_side(vec2_t *v)
{
    vec3_t x = (vec3_t){v[1].x - v[0].x, v[1].y - v[0].y, 0.0f};
    vec3_t y = (vec3_t){v[2].x - v[0].x, v[2].y - v[0].y, 0.0f};
    vec3_t c = vec3_cross(x, y);
    return c.z < 0.0f;
}

int depth_test(device_t *device, int x, int y, float depth)
{
    y = device->height - y - 1;
    float buffer_depth = device->depthBuffer[x + y * device->width];
    return depth > buffer_depth;
}

void rasterize_scanline(device_t *device, scanline_t *scanline)
{
    float y = scanline->y;
    float x = scanline->l;
    int ix = (int)ceilf(x), iy = y, ir = (int)floorf(scanline->r);
    vertex_t *v = vertex_split(scanline->p);
    for (; ix <= ir; ix ++)
    {
        color3_t color;
        // Test boundary
        int inBoundary = \
            iy >= 0 && iy < device->height && ix >= 0 && ix < device->width;
        if (inBoundary)
        {
            vertex_copy(v, scanline->p);
            float z = 1.0f / v->w;
            for (int i = 0; i < v->vary_size; i++)
            {
                v->vary[i] *= z;
            }
            // depth test
            // assert(v->w >= 0.0f && v->w <= 1.0f);
            if (depth_test(device, ix, iy, v->w))
            {
                device->fs(device, device->unif, v->vary, v->w, &color);
                fill_buffer(device, ix, iy, &color, v->w);
            }
        }

        vertex_add(scanline->p, scanline->step);
    }
    vertex_destroy(v);
}

void rasterize_trapezoid(device_t *device, trapezoid_t *trap)
{
    float top = trap->tl->ps.y;
    float bottom = trap->bl->ps.y;
    float y = ceilf(top);

    vertex_t *left = vertex_lerp(
        trap->tl, trap->bl, (y - top) / (bottom - top));
    vertex_t *right = vertex_lerp(
        trap->tr, trap->br, (y - top) / (bottom - top));
    vertex_t *left_step = vertex_split(trap->bl);
    vertex_t *right_step = vertex_split(trap->br);
    vertex_sub(left_step, trap->tl);
    vertex_div(left_step, bottom - top);
    vertex_sub(right_step, trap->tr);
    vertex_div(right_step, bottom - top);
    for (; y <= bottom; y += 1.0f)
    {
        float l = ceilf(left->ps.x);
        vertex_t *begin = vertex_lerp(
            left, right, (l - left->ps.x) / (right->ps.x - left->ps.x));
        vertex_t *step = vertex_split(right);
        vertex_sub(step, left);
        vertex_div(step, right->ps.x - left->ps.x);
        scanline_t scanline = (scanline_t){left->ps.x, right->ps.x, y, begin, step};
        
        rasterize_scanline(device, &scanline);

        vertex_add(left, left_step);
        vertex_add(right, right_step);
        vertex_destroy(step);
        vertex_destroy(begin);
    }

    vertex_destroy(left);
    vertex_destroy(right);
    vertex_destroy(left_step);
    vertex_destroy(right_step);
}

// split triangle to trapezoids and draw
void split_and_rasterize_triangle(device_t *device,
                                  vec4_t *vndc,
                                  vec2_t *vs,
                                  float *z
                                  )
{
    // before interpolating, multiply varyings with 1/z
    //   because *1/z space is linear
    float w[3];
    float *_vary = device->vary;
    for (int i = 0; i < 3; i++)
    {
        w[i] = 1.0f / z[i];
        for (int j = 0; j < device->vary_size; j++)
        {
            *(_vary ++) *= w[i];
        }
    }

    vertex_t *a = vertex_new(vndc[0], vs[0], w[0], 
        device->vary, device->vary_size);
    vertex_t *b = vertex_new(vndc[1], vs[1], w[1], 
        device->vary + device->vary_size, device->vary_size);
    vertex_t *c = vertex_new(vndc[2], vs[2], w[2], 
        device->vary + device->vary_size * 2, device->vary_size);
    // y: a -> b -> c
    if (a->ps.y > b->ps.y) swap_ptrs((void **)&a, (void **)&b);
    if (a->ps.y > c->ps.y) swap_ptrs((void **)&a, (void **)&c);
    if (b->ps.y > c->ps.y) swap_ptrs((void **)&b, (void **)&c);
    // find the other point for trapezoid
    vertex_t *d = vertex_lerp(a, c, (b->ps.y - a->ps.y) / (c->ps.y - a->ps.y));
    if (b->ps.x > d->ps.x) swap_ptrs((void **)&b, (void **)&d);

    trapezoid_t trap1 = (trapezoid_t){a, a, b, d};
    trapezoid_t trap2 = (trapezoid_t){b, d, c, c};

    rasterize_trapezoid(device, &trap1);
    rasterize_trapezoid(device, &trap2);

    vertex_destroy(a);
    vertex_destroy(b);
    vertex_destroy(c);
    vertex_destroy(d);
}

void draw_triangle(device_t *device)
{
    vec3_t *v = device->vertex;
    vec4_t vc[3];       // camera
    vec4_t vndc[3];     // ndc
    vec2_t vs[3];       // 2d
    float  z[3];

    // world -> camera
    for (int i = 0; i < 3; i++)
    {
        vc[i] = vec4_mat_mul(get_vec4(v[i]), &device->m_world);
    }

    // camera -> orth
    for (int i = 0; i < 3; i++)
    {
        vndc[i] = vec4_mat_mul(vc[i], &device->m_project);
        z[i] = vndc[i].w;
        vndc[i] = vec4_normalize(vndc[i]);
        vs[i].x = clip_float(vndc[i].x * 0.5f + 0.5f, 0, 1) * device->width;
        vs[i].y = clip_float(vndc[i].y * 0.5f + 0.5f, 0, 1) * device->height;
    }

    // clip, culling
    for (int i = 0; i < 3; i++)
    {
        if (z[i] < 0.0f) return;
    }
    // back face culling
    if (face_side(vs))
    {
        return;
    }

    // vertex shader
    for (int i = 0; i < 3; i++)
    {
        device->vs(device,
                   device->unif,
                   device->attr + i * device->attr_size,
                   device->vary + i * device->vary_size
                   );
    }

    // fragment shader
    split_and_rasterize_triangle(device, vndc, vs, z);
}

void setup_device(device_t *device, 
                  uint32_t width,
                  uint32_t height,
                  uint8_t *screen_buffer)
{
    device->width = width;
    device->height = height;
    device->colorBuffer = screen_buffer;
    device->depthBuffer = calloc(width * height, sizeof(float));
}

void clear_buffer(device_t *device)
{
    uint32_t width = device->width;
    uint32_t height = device->height;

    uint8_t *line = device->colorBuffer;
    float *depth_ptr = device->depthBuffer;
    for (int i = 0; i < height; i ++)
    {
        uint8_t *p = line;
        line += width * 4;
        for (int j = 0; j < width; j ++)
        {
            *(p ++) = 127;
            *(p ++) = 127;
            *(p ++) = 127;
            *(p ++) = 255;
            *(depth_ptr ++) = 0.0f;
        }
    }
}

void draw_mesh(device_t *device, mesh_t *mesh)
{
    device->drawer(device, mesh);
}
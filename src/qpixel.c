#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include "qpixel.h"

#define EPS 1e-6

// ================================
// MATH
// ================================

typedef enum
{
    CVV_LEFT = 1,
    CVV_RIGHT = 2,
    CVV_TOP = 4,
    CVV_BOTTOM = 8,
    CVV_FRONT = 16,
    CVV_REAR = 32
} cvv_type_t;

/**
 * @brief Judges if an AABB (Axis Aligned Bounding Box) is inside a half-plane
 *      represented by a homogenenous normal. Such plane in Homogeneous space
 *      always passes through (0, 0, 0, 0).
 * 
 * @param normal_homogenenous   Normal of the 4-dimensional plane
 * @param aabb                  The aabb
 * @return int                  The result. 1 if all inside. Otherwise 0.
 */
int homogeneous_half_plane_aabb(vec4_t normal_homogenenous, aabb_t aabb)
{
    vec4_t *n = &normal_homogenenous;
    vec3_t *b = (vec3_t *)&aabb;
    // project 8 corners to normal
    for (int i = 0; i < 8; i ++)
    {
        float x = b[i & 1].x;
        float y = b[(i >> 1) & 1].y;
        float z = b[(i >> 2) & 1].z;
        float dot = x * n->x + y * n->y + z * n->z + n->w;
        if (dot < 0) return 0;
    }
    return 1;
}

/**
 * @brief Get the homogeneous normal from projection matrix.
 *      The direction of the normal is outside the Frustum
 * 
 * @param m_project     The projection matrix
 * @param type          CVV type
 * @return vec4_t       The expected normal
 */
vec4_t get_homogeneous_normal_from_projection(
    mat4_t *m_project, cvv_type_t type)
{
    mat4_t *m = m_project;
    vec4_t a;
    vec4_t b = (vec4_t){ m->m[3][0], m->m[3][1], m->m[3][2], m->m[3][3] };
    switch (type)
    {
    case CVV_LEFT:
        a = (vec4_t){ -m->m[0][0], -m->m[0][1], -m->m[0][2], -m->m[0][3] };
        break;
    case CVV_RIGHT:
        a = (vec4_t){ m->m[0][0], m->m[0][1], m->m[0][2], m->m[0][3] };
        break;
    case CVV_TOP:
        a = (vec4_t){ -m->m[1][0], -m->m[1][1], -m->m[1][2], -m->m[1][3] };
        break;
    case CVV_BOTTOM:
        a = (vec4_t){ m->m[1][0], m->m[1][1], m->m[1][2], m->m[1][3] };
        break;
    case CVV_FRONT:
        a = (vec4_t){ -m->m[2][0], -m->m[2][1], -m->m[2][2], -m->m[2][3] };
        break;
    case CVV_REAR:
        a = (vec4_t){ m->m[2][0], m->m[2][1], m->m[2][2], m->m[2][3] };
        break;
    default: break;
    }
    return (vec4_t){ a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w };
}

// =====================================================
// RENDER
// =====================================================

typedef struct
{
    vec4_t pndc;        // point homogeneous / ndc coord
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
    a->pndc.w += b->pndc.w;
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
    a->pndc.w -= b->pndc.w;
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
    a->pndc.w *= b;
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


int homogeneous_clip_test(vec4_t v, cvv_type_t cvv_type)
{
    int res = 0;
    float w = v.w;
    switch (cvv_type)
    {
        case CVV_LEFT:
            res = v.x < -w;
            break;
        case CVV_RIGHT:
            res = v.x > w;
            break;
        case CVV_TOP:
            res = v.y < -w;
            break;
        case CVV_BOTTOM:
            res = v.y > w;
            break;
        case CVV_FRONT:
            res = v.z < -w;
            break;
        case CVV_REAR:
            res = v.z > w;
            break;
        default: break;
    }
    return res;
}

/**
 * @brief Calculates the intersection of CVV plane and polygon edge.
 * 
 * @param a     Starting point
 * @param b     Ending point
 * @param cvv_type  CVV plane. cvv_type_t
 * @return vertex_t*    The intersection
 */
vertex_t *homogeneous_clip_intersect(vertex_t *a, vertex_t *b, cvv_type_t cvv_type)
{
    vertex_t *c;
    vec4_t u = a->pndc;
    vec4_t v = b->pndc;
    float l;
    switch (cvv_type)
    {
    case CVV_LEFT:
        l = (u.x + u.w) / ((u.x + u.w) - (v.x + v.w));
        break;
    case CVV_RIGHT:
        l = (u.x - u.w) / ((u.x - u.w) - (v.x - v.w));
        break;
    case CVV_TOP:
        l = (u.y + u.w) / ((u.y + u.w) - (v.y + v.w));
        break;
    case CVV_BOTTOM:
        l = (u.y - u.w) / ((u.y - u.w) - (v.y - v.w));
        break;
    case CVV_FRONT:
        l = (u.z + u.w) / ((u.z + u.w) - (v.z + v.w));
        break;
    case CVV_REAR:
        l = (u.z - u.w) / ((u.z - u.w) - (v.z - v.w));
        break;
    defaut: break;
    }
    c = vertex_lerp(a, b, l);
    return c;
}


/**
 * @brief Perform polygon CVV culling with one plane
 * 
 * @param v     The input vertices representing a polygon.
 * @param n     Number of vertices.
 * @param cvv_type      The plane of CVV you want to use to cut the polygon.
 * @return int     Number of new polygon.
 */
int homogeneous_clip(vertex_t *v, int n, cvv_type_t cvv_type)
{
    int m = 0;
    vertex_t v_buffer[9];
    int clip_results[9];
    int sum = 0;
    for (int i = 0; i < n; i ++)
    {
        clip_results[i] = homogeneous_clip_test(v[i].pndc, cvv_type);
        sum += clip_results[i];
    }
    if (sum == 0) return n;

    for (int i = 0; i < n; i ++)
    {
        vertex_t *a = v + i;
        vertex_t *b = v + ((i + 1) % n);
        vertex_t *c;
        int cvv_a = clip_results[i], cvv_b = clip_results[(i + 1) % n];

        if (!cvv_a && i == 0)
        {
            v_buffer[m ++] = *a;
        }

        if (cvv_a != cvv_b)
        {
            c = homogeneous_clip_intersect(a, b, cvv_type);
            v_buffer[m ++] = *c;
            free(c);
        }

        if (!cvv_b && i < n - 1)
        {
            v_buffer[m ++] = *b;
        }
    }

    // clear old varying resources
    for (int i = 0; i < n; i ++)
    {
        // free the varyings that the vertex is outside CVV
        //   since these points are removed from polygon and will never be used.
        if (clip_results[i] != 0)
        {
            free(v[i].vary);
        }
    }

    memcpy(v, v_buffer, sizeof(vertex_t) * m);
    return m;
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
                device->texel_count ++;
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
                                  vertex_t *_a,
                                  vertex_t *_b,
                                  vertex_t *_c
                                  )
{
    device->triangle_count ++;

    vertex_t *a = vertex_split(_a);
    vertex_t *b = vertex_split(_b);
    vertex_t *c = vertex_split(_c);
    // y: a -> b -> c
    if (a->ps.y > b->ps.y) swap_ptrs((void **)&a, (void **)&b);
    if (a->ps.y > c->ps.y) swap_ptrs((void **)&a, (void **)&c);
    if (b->ps.y > c->ps.y) swap_ptrs((void **)&b, (void **)&c);

    // find the other point for trapezoid
    vertex_t *d = vertex_lerp(a, c, (b->ps.y - a->ps.y) / (c->ps.y - a->ps.y));
    if (b->ps.x > d->ps.x)
    {
        swap_ptrs((void **)&b, (void **)&d);
    }
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
    float  w[3];

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
        w[i] = 1.0f / z[i];
        vs[i].x = clip_float(vndc[i].x * 0.5f + 0.5f, 0, 1) * device->width;
        vs[i].y = clip_float(vndc[i].y * 0.5f + 0.5f, 0, 1) * device->height;
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

    // w = 0 plane clipping

    // homogeneous clipping
    int n_vertices = 3;
    vertex_t vertices[9], *v_temp;
    for (int i = 0; i < n_vertices; i ++)
    {
        float *_vary;
        // NOTICE: only vs & z are placeholders and 
        //    will be updated after clipping.
        v_temp = vertex_new(vndc[i], vs[i], z[i],
            device->vary + i * device->vary_size,
            device->vary_size);
        vertices[i] = *v_temp;
        free(v_temp);
    }
    n_vertices = homogeneous_clip(vertices, n_vertices, CVV_LEFT);
    n_vertices = homogeneous_clip(vertices, n_vertices, CVV_RIGHT);
    n_vertices = homogeneous_clip(vertices, n_vertices, CVV_TOP);
    n_vertices = homogeneous_clip(vertices, n_vertices, CVV_BOTTOM);
    n_vertices = homogeneous_clip(vertices, n_vertices, CVV_FRONT);
    n_vertices = homogeneous_clip(vertices, n_vertices, CVV_REAR);

    for (int i = 0; i < n_vertices; i ++)
    {
        float *_vary = vertices[i].vary;
        vertex_t *v_temp = &vertices[i];
        v_temp->w = v_temp->pndc.w;
        v_temp->w = 1.0f / v_temp->w;
        for (int j = 0; j < v_temp->vary_size; j ++)
        {
            *(_vary ++) *= v_temp->w;
        }
        v_temp->pndc = vec4_normalize(v_temp->pndc);
        v_temp->ps.x = \
            clip_float(v_temp->pndc.x * 0.5f + 0.5f, 0, 1) * device->width;
        v_temp->ps.y = \
            clip_float(v_temp->pndc.y * 0.5f + 0.5f, 0, 1) * device->height;
    }

    for (int i = 1; i < n_vertices - 1; i ++)
    {
        vs[0] = vertices[0].ps;
        vs[1] = vertices[i].ps;
        vs[2] = vertices[i + 1].ps;
        // back face culling
        if (!face_side(vs))
        {
            // fragment shader
            split_and_rasterize_triangle(device,
                &vertices[0], &vertices[i], &vertices[i + 1]);
        }
    }
    for (int i = 0; i < n_vertices; i ++)
    {
        free(vertices[i].vary);
    }
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
    device->triangle_count = 0;
    device->texel_count = 0;
}

void draw_mesh(device_t *device, mesh_t *mesh)
{
    device->drawer(device, mesh);
}

void draw_scene(device_t *device, scene_t *scene)
{
    for (int i = 0; i < scene->n_objects; i ++)
    {
        object3d_t * obj = scene->objects + i;
        device->m_world = device->m_camera;
        mat4_mul(&device->m_world, &obj->m_world);
        draw_mesh(device, obj->mesh);
    }
}

void object_update_m_world(object3d_t * obj)
{
    get_world_mat(&obj->m_world, obj->position, obj->rotation, obj->scale);
}

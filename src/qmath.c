#include <math.h>
#include <string.h>
#include "qmath.h"

void get_identity_mat(mat4_t * m)
{
    memset(m->m, 0, sizeof(int) * 16);
    for (int i = 0; i < 4; i++) m->m[i][i] = 1.0f;
}

void get_projection_mat(mat4_t * m, float fov, float a, float n, float f)
{
    float t = tanf(fov * PI / 360.0f);
    memset(m->m, 0, sizeof(float) * 16);
    m->m[0][0] = 1.0f / t / a;
    m->m[1][1] = 1.0f / t;
    m->m[2][2] = - (f + n) / (f - n);
    m->m[2][3] = - 2.0f * f * n / (f - n);
    m->m[3][2] = -1.0f;
}

void get_lookat_mat(mat4_t * m, vec3_t eye, vec3_t center, vec3_t up)
{
    up = vec3_normalize(up);
    vec3_t forward = vec3_normalize(vec3_sub(center, eye));
    vec3_t U = vec3_normalize(vec3_cross(forward, up));
    vec3_t V = vec3_normalize(vec3_cross(U, forward));
    vec3_t W = (vec3_t){- forward.x, - forward.y, - forward.z};

    m->m[0][0] = U.x; m->m[0][1] = U.y; m->m[0][2] = U.z;
    m->m[0][3] = - vec3_dot(eye, U);
    m->m[1][0] = V.x; m->m[1][1] = V.y; m->m[1][2] = V.z;
    m->m[1][3] = - vec3_dot(eye, V);
    m->m[2][0] = W.x; m->m[2][1] = W.y; m->m[2][2] = W.z;
    m->m[2][3] = - vec3_dot(eye, W);
    m->m[3][0] = m->m[3][1] = m->m[3][2] = 0.0f;
    m->m[3][3] = 1.0f;
}

// Returns Determinant of MINOR
float calc_det3(mat4_t * m, int r, int c)
{
    float n[3][3], det = 0.0f;
    for (int i = 0; i < 4; i ++)
    {
        int nr = i < r ? i : i - 1;
        if (i == r) continue;
        for (int j = 0; j < 4; j ++) {
            int nc = j < c ? j : j - 1;
            if (j == c) continue;
            n[nr][nc] = m->m[i][j];
        }
    }
    det += n[0][0] * n[1][1] * n[2][2];
    det += n[0][1] * n[1][2] * n[2][0];
    det += n[0][2] * n[1][0] * n[2][1];
    det -= n[2][0] * n[1][1] * n[0][2];
    det -= n[2][1] * n[1][2] * n[0][0];
    det -= n[2][2] * n[1][0] * n[0][1];
    return det;
}

// Outputs Adjugate Matrix, returns Determinant of m.
float calc_adjugate_mat(mat4_t * m, mat4_t * out)
{
    float det = 0.0f;
    for (int i = 0; i < 4; i++)
    {
        float f = (i & 1) ? -1.0f : 1.0f;
        for (int j = 0; j < 4; j++)
        {
            float det3 = f * calc_det3(m, i, j);
            out->m[j][i] = det3;
            f = -f;
        }
    }
    for (int i = 0; i < 4; i++) {
        det += out->m[i][0] * m->m[0][i];
    }
    return det;
}

// A' = A* / det(A)
void calc_inv_mat(mat4_t * m, mat4_t * out)
{
    float det = calc_adjugate_mat(m, out);
    float inv_det = 1.0f / det;
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            out->m[i][j] *= inv_det;
        }
    }
}

// clip float
float clip_float(float x, float a, float b)
{
    if (x > b) return b;
    if (x < a) return a;
    return x;
}

float lerp(float a, float b, float x)
{
    return (1.0f - x) * a + x * b;
}

vec4_t vec4_mul(vec4_t v, float x)
{
    return (vec4_t){v.x * x, v.y * x, v.z * x, v.w * x};
}

vec4_t vec4_div(vec4_t v, float x)
{
    return vec4_mul(v, 1.0f / x);
}

vec4_t get_vec4(vec3_t v)
{
    return (vec4_t){v.x, v.y, v.z, 1.0f};
}

vec4_t vec4_normalize(vec4_t v)
{
    return vec4_div(v, v.w);
}

vec4_t vec4_mat_mul(vec4_t v, mat4_t * m)
{
    vec4_t res;
    res.x = v.x * m->m[0][0] + v.y * m->m[0][1] + v.z * m->m[0][2] + v.w * m->m[0][3];
    res.y = v.x * m->m[1][0] + v.y * m->m[1][1] + v.z * m->m[1][2] + v.w * m->m[1][3];
    res.z = v.x * m->m[2][0] + v.y * m->m[2][1] + v.z * m->m[2][2] + v.w * m->m[2][3];
    res.w = v.x * m->m[3][0] + v.y * m->m[3][1] + v.z * m->m[3][2] + v.w * m->m[3][3];
    return res;
}

vec3_t vec3_add(vec3_t a, vec3_t b)
{
    return (vec3_t){a.x + b.x, a.y + b.y, a.z + b.z};
}

vec3_t vec3_sub(vec3_t a, vec3_t b)
{
    return (vec3_t){a.x - b.x, a.y - b.y, a.z - b.z};
}

vec3_t vec3_mul(vec3_t a, float b)
{
    return (vec3_t){a.x * b, a.y * b, a.z * b};
}

vec3_t vec3_div(vec3_t a, float b)
{
    float c = 1.0f / b;
    return (vec3_t){a.x * c, a.y * c, a.z * c};
}

vec3_t vec3_cross(vec3_t a, vec3_t b)
{
    vec3_t r;
    r.x = a.y * b.z - a.z * b.y;
    r.y = a.z * b.x - a.x * b.z;
    r.z = a.x * b.y - a.y * b.x;
    return r;
}

float  vec3_dot(vec3_t a, vec3_t b)
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

vec3_t vec3_normalize(vec3_t v)
{
    float norm = v.x * v.x + v.y * v.y + v.z * v.z;
    norm = 1.0f / sqrtf(norm);
    return (vec3_t){v.x * norm, v.y * norm, v.z * norm};
}

vec3_t vec3_mat_mul(vec3_t v, mat4_t * m)
{
    vec3_t res;
    res.x = v.x * m->m[0][0] + v.y * m->m[0][1] + v.z * m->m[0][2];
    res.y = v.x * m->m[1][0] + v.y * m->m[1][1] + v.z * m->m[1][2];
    res.z = v.x * m->m[2][0] + v.y * m->m[2][1] + v.z * m->m[2][2];
    return res;
}

vec3_t vec3_clip(vec3_t v, float a, float b)
{
    vec3_t r;
    r.x = clip_float(v.x, a, b);
    r.y = clip_float(v.y, a, b);
    r.z = clip_float(v.z, a, b);
    return r;
}

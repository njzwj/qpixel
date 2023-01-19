#pragma once

#define PI 3.1415926

typedef struct { float x, y, z, w; } vec4_t;
typedef struct { float x, y, z; } vec3_t;
typedef struct { float x, y; } vec2_t;
typedef struct { float m[4][4]; } mat4_t;

/**
 * @brief Writes an identity matrix to m
 * 
 * @param m The matrix
 */
void get_identity_mat(mat4_t * m);

/**
 * @brief Get the projection matrix
 * 
 * @param m The matrix
 * @param fov  Field of view
 * @param a  Aspect ratio, width / height
 * @param n  Near distance
 * @param f  Far distance
 */
void get_projection_mat(mat4_t * m, float fov, float a, float n, float f);

/**
 * @brief Get the lookat matrix
 * 
 * @param m  The matrix
 * @param eye  Camera position
 * @param center  The target you are looking at
 * @param up  Up direction
 */
void get_lookat_mat(mat4_t * m, vec3_t eye, vec3_t center, vec3_t up);

/**
 * @brief Calculate the residual det 
 * 
 * @param m  The matrix
 * @param r  Row
 * @param c  Row
 * @return float Determinant 
 */
float calc_det3(mat4_t * m, int r, int c);

float calc_adjugate_mat(mat4_t * m, mat4_t * out);

void calc_inv_mat(mat4_t * m, mat4_t * out);

float clip_float(float x, float a, float b);

float lerp(float a, float b, float x);

vec4_t vec4_mul(vec4_t v, float x);

vec4_t vec4_div(vec4_t v, float x);

vec4_t get_vec4(vec3_t v);

vec4_t vec4_normalize(vec4_t v);

vec4_t vec4_mat_mul(vec4_t v, mat4_t * m);

vec3_t vec3_add(vec3_t a, vec3_t b);

vec3_t vec3_sub(vec3_t a, vec3_t b);

vec3_t vec3_mul(vec3_t a, float b);

vec3_t vec3_div(vec3_t a, float b);

vec3_t vec3_cross(vec3_t a, vec3_t b);

float  vec3_dot(vec3_t a, vec3_t b);

vec3_t vec3_normalize(vec3_t v);

vec3_t vec3_mat_mul(vec3_t, mat4_t *);

vec3_t vec3_clip(vec3_t v, float a, float b);

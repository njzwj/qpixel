#pragma once

#define PI 3.1415926

typedef struct { float x, y, z, w; } vec4_t;
typedef struct { float x, y, z; } vec3_t;
typedef struct { float x, y; } vec2_t;
typedef struct { float m[4][4]; } mat4_t;
typedef struct { vec3_t v1, v2; } aabb_t;
typedef struct { float w, x, y, z; } quat_t;

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
 * @brief Get the world matrix
 * 
 * @param m     Output matrix
 * @param translation       Translation
 * @param rotation          Rotation (quaternion)
 * @param scale             Scale (in porpotion)
 */
void get_world_mat(mat4_t * m, vec3_t translation, quat_t rotation,
    vec3_t scale);

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

/**
 * @brief Mat4 multiply, a <- a * b
 * 
 * @param a     Mat a
 * @param b     Mat b
 */
void mat4_mul(mat4_t * a, mat4_t * b);

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

/**
 * @brief Calculates new aabb after the translation
 * 
 * @param aabb 
 * @param m 
 * @return aabb_t 
 */
aabb_t aabb_mat_mul(aabb_t aabb, mat4_t * m);

/**
 * @brief Converts axis-angle rotation to quaternion
 * 
 * @param u         The axis (normalized)
 * @param theta     The rotation
 * @return quat_t   Quaternion
 */
quat_t quat_from_axis_angle(vec3_t u, float theta);

/**
 * @brief Get rotation matrix from normalized quaternion
 * 
 * @param m     Output matrix
 * @param q     Quaterion (normalized)
 */
void mat4_from_quat(mat4_t * m, quat_t q);

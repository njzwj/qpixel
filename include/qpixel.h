#pragma once

#include <windows.h>
#include <math.h>
#include "qmath.h"
#include "qmesh.h"

typedef unsigned char * color_buffer_t;
typedef float *         depth_buffer_t;

typedef struct { float b, g, r; } color3_t;

typedef struct device_t device_t;

typedef void (*drawer_t)(device_t *device, mesh_t *mesh);
typedef void (*vertex_shader_t)(device_t *device, float *unif, float *attr, float *vary);
typedef void (*fragment_shader_t)(device_t *device, float *unif, float *vary, float w, color3_t * out);

typedef struct device_t
{
    int             width;
    int             height;
    color_buffer_t  colorBuffer;
    depth_buffer_t  depthBuffer;
    mat4_t          m_project;
    mat4_t          m_camera;
    mat4_t          m_world;
    mat4_t          m_world_inv;

    vec3_t          vertex[3];      // vertex

    float           *unif;          // uniform
    size_t          unif_size;
    float           *attr;          // attribute
    size_t          attr_size;
    float           *vary;          // varying
    size_t          vary_size;

    drawer_t            drawer;
    vertex_shader_t     vs;
    fragment_shader_t   fs;

    float debug[256];
    uint32_t triangle_count;
    uint32_t texel_count;
} device_t;

typedef struct {
    HDC             dc;             /* compatible DC */
    HBITMAP         bmp;            /* device independent bitmap */
    unsigned char   *buffer;        /* buffer */
    int             width;
    int             height;
    int             pitch;
} screen_t;

typedef struct
{
    vec3_t eye;
    vec3_t target;
    vec3_t up;
    float  fov;
    float  aspect;
} camera_t;

typedef struct
{
    mesh_t *mesh;
    vec3_t position;
    quat_t rotation;
    vec3_t scale;
    mat4_t m_world;
} object3d_t;

typedef struct
{
    int n_objects;
    object3d_t *objects;
} scene_t;

/**
 * @brief This will use device->drawer to assemble uniforms, varyings, etc.
 * 
 * @param device  Device Handle
 * @param mesh  Mesh
 */
void draw_mesh(device_t *device, mesh_t *mesh);


/**
 * @brief Draw triangle specified by vertices, unifroms and attributes.
 * 
 * @param device Device handle
 */
void draw_triangle(device_t *device);


/**
 * @brief Draw scene
 * 
 * @param device    Device handle
 * @param scene     Scene
 */
void draw_scene(device_t *device, scene_t *scene);


/**
 * @brief Setup device, initialize the depth buffer and color buffer according
 *      the width, height and given screen_buffer reference.
 * 
 * @param device Device handle
 * @param width  Width
 * @param height Height
 * @param screen_buffer Screen buffer reference 
 */
void setup_device(device_t *device, 
                  uint32_t width,
                  uint32_t height,
                  uint8_t *screen_buffer);


/**
 * @brief Reset color buffer and depth buffer for new frame
 * 
 * @param device Device handle
 */
void clear_buffer(device_t *device);


/**
 * @brief Update world matrix
 * 
 * @param obj Object
 */
void object_update_m_world(object3d_t * obj);

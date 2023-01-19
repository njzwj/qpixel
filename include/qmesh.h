#pragma once

#include <stdlib.h>
#include <stdint.h>
#include "qmath.h"

typedef enum
{
    T_TEXCOORD = 1,
    T_NORMAL = 2
} mesh_type_t;

typedef struct
{
    vec3_t *vertices;
    vec3_t *normals;
    vec2_t *texcoords;
    uint32_t *vertex_idx;
    uint32_t *texcoord_idx;
    uint32_t *normal_idx;

    uint32_t n_vertices;
    uint32_t n_texcoords;
    uint32_t n_normals;
    uint32_t n_faces;

    mesh_type_t mesh_type;
} mesh_t;

/**
 * @brief Release the mesh resource
 * 
 * @param mesh  The mesh you wan't to release
 */
void destroy_mesh(mesh_t *mesh);

/**
 * @brief Read obj and build mesh from it
 * 
 * @param fn  The filename
 * @return mesh_t*  The mesh. NULL if any error.
 */
mesh_t *load_mesh(const char *fn);


/**
 * @brief Returns the center point of mesh
 * 
 * @param mesh 
 * @return vec3_t 
 */
vec3_t mesh_center(mesh_t *mesh);

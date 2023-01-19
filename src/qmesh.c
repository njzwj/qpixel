#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "qmesh.h"

#define MAX_LINE_LEN 256

const char *get_extension(const char *fn)
{
    size_t len = strlen(fn);
    while (len > 0)
    {
        if (fn[len - 1] == '.') break;
        -- len;
    }
    return &fn[len];
}

int strequ(const char *str0, const char *str1)
{
    return strcmp(str0, str1) == 0;
}

typedef enum
{
    TYPE_UINT32,
    TYPE_FLOAT,
} type_t;

int read_line(char *line, void *buffer, const int dim, const type_t format)
{
    char *line_cpy;
    char *token = strtok_s(line, " ", &line_cpy);
    for (int i = 0; i < dim; ++ i)
    {
        token = strtok_s(NULL, " ", &line_cpy);
        token[strcspn(token, "\n")] = '\0';
        switch (format)
        {
            case TYPE_FLOAT:
                ((float*)buffer)[i] = atof(token);
                break;
            case TYPE_UINT32:
                ((uint32_t*)buffer)[i] = atoi(token);
                break;
            default: break;
        }
    }
    return 0;
}

mesh_t *load_obj(const char *fn);

mesh_t *get_mesh()
{
    mesh_t *mesh = (mesh_t *)malloc(sizeof(mesh_t));
    mesh->vertices = NULL;
    mesh->normals = NULL;
    mesh->texcoords = NULL;
    
    mesh->vertex_idx = NULL;
    mesh->texcoord_idx = NULL;
    mesh->normal_idx = NULL;

    mesh->n_vertices = 0;
    mesh->n_normals = 0;
    mesh->n_texcoords = 0;
    mesh->n_faces = 0;

    mesh->mesh_type = 0;

    return mesh;
}

void destroy_mesh(mesh_t *mesh)
{
    free(mesh->vertices);
    free(mesh->normals);
    free(mesh->texcoords);

    free(mesh->vertex_idx);
    free(mesh->texcoord_idx);
    free(mesh->normal_idx);
}

mesh_t *load_mesh(const char *fn)
{
    const char *ext = get_extension(fn);
    if (strcmp(ext, "obj") == 0)
    {
        return load_obj(fn);
    }
    
    // Unidentified file extension
    printf("Undefined extension %s. Mesh load failed.\n", ext);
    return NULL;
}

int load_obj_dimensions(mesh_t *mesh, FILE *file, char* err_msg)
{
    char buffer[MAX_LINE_LEN] = {0};
    char *buffer_cpy;
    uint32_t line_no = 0;

    uint32_t n_vertices = 0;
    uint32_t n_normals = 0;
    uint32_t n_texcoords = 0;
    uint32_t n_faces = 0;
    
    while (fgets(buffer, sizeof(buffer), file))
    {
        ++ line_no;
        const char *token;

        token = strtok_s(buffer, " ", &buffer_cpy);
        if (strequ(token, "v"))     // Vertex
        {
            ++ n_vertices;
        }
        else if (strequ(token, "vn"))   // Normal
        {
            ++ n_normals;
        }
        else if (strequ(token, "vt"))   // Texture coords
        {
            ++ n_texcoords;
        }
        else if (strequ(token, "f"))    // Face
        {
            ++ n_faces;
        }
        else if (strequ(token, "o"))    // Name
        {
            // TODO:
        }
    }

    mesh->n_vertices = n_vertices;
    mesh->n_normals = n_normals;
    mesh->n_texcoords = n_texcoords;
    mesh->n_faces = n_faces;

    if (n_texcoords > 0)
    {
        mesh->mesh_type |= T_TEXCOORD;
    }
    if (n_normals > 0)
    {
        mesh->mesh_type |= T_NORMAL;
    }

    return 0;
}

mesh_t *load_obj(const char *fn)
{
    char err_msg[256];
    char line_buffer[MAX_LINE_LEN];
    char *line_buffer_cpy;
    uint32_t vi = 0, ti = 0, ni = 0, fi = 0;
    mesh_t *mesh;

    FILE *file;
    fopen_s(&file, fn, "r");
    if (!file)
    {
        // FILE NOT FOUND
        printf("%s not found.\n", fn);
        return NULL;
    }

    // Get object dimensions
    mesh = get_mesh();
    if (load_obj_dimensions(mesh, file, err_msg) != 0)
    {
        destroy_mesh(mesh);
        return NULL;
    }

    // Allocation
    mesh->vertices = calloc(mesh->n_vertices, sizeof(vec3_t));
    mesh->normals = calloc(mesh->n_normals, sizeof(vec3_t));
    mesh->texcoords = calloc(mesh->n_texcoords, sizeof(vec2_t));
    mesh->vertex_idx = calloc(mesh->n_faces * 3, sizeof(uint32_t));
    mesh->texcoord_idx = calloc(mesh->n_faces * 3, sizeof(uint32_t));
    mesh->normal_idx = calloc(mesh->n_faces * 3, sizeof(uint32_t));

    // Scan for data
    fseek(file, 0, SEEK_SET);
    while (fgets(line_buffer, sizeof(line_buffer), file))
    {
        const char *token = strtok_s(line_buffer, " ", &line_buffer_cpy);
        void *buffer = NULL;
        int dim;
        type_t format;

        if (strequ(token, "v"))
        {
            buffer = (void*)(&mesh->vertices[vi]);
            dim = 3;
            format = TYPE_FLOAT;
            ++ vi;
        }
        else if (strequ(token, "vn"))
        {
            buffer = (void*)(&mesh->normals[ni]);
            dim = 3;
            format = TYPE_FLOAT;
            ++ ni;
        }
        else if (strequ(token, "vt"))
        {
            buffer = (void*)(&mesh->texcoords[ti]);
            dim = 2;
            format = TYPE_FLOAT;
            ++ ti;
        }
        else if (strequ(token, "f"))
        {
            for (int i = 0; i < 3; i++)
            {
                token = strtok_s(NULL, "/ ", &line_buffer_cpy);
                mesh->vertex_idx[fi * 3 + i] = atoi(token);
                if ((mesh->mesh_type & T_TEXCOORD)
                 || (mesh->mesh_type & T_NORMAL))
                {
                    token = strtok_s(NULL, "/ ", &line_buffer_cpy);
                    if (mesh->mesh_type & T_TEXCOORD)
                    {
                        mesh->texcoord_idx[fi * 3 + i] = atoi(token);
                    }
                    token = strtok_s(NULL, " ", &line_buffer_cpy);
                    if (mesh->mesh_type & T_NORMAL)
                    {
                        mesh->normal_idx[fi * 3 + i] = atoi(token);
                    }
                }
            }
            ++ fi;
            continue;
        }

        line_buffer[strcspn(token, "\0")] = ' ';
        if (buffer != NULL) read_line(line_buffer, buffer, dim, format);
    }

    fclose(file);
    return mesh;
}

vec3_t mesh_center(mesh_t *mesh)
{
    vec3_t mi = (vec3_t){ 1e7f, 1e7f, 1e7f };
    vec3_t mx = (vec3_t){ -1e7f, -1e7f, -1e7f };
    for (uint32_t i = 0; i < mesh->n_vertices; i ++)
    {
        vec3_t v = mesh->vertices[i];
        mi.x = mi.x > v.x ? v.x : mi.x;
        mi.y = mi.y > v.y ? v.y : mi.y;
        mi.z = mi.z > v.z ? v.z : mi.z;
        mx.x = mx.x < v.x ? v.x : mx.x;
        mx.y = mx.y < v.y ? v.y : mx.y;
        mx.z = mx.z < v.z ? v.z : mx.z;
    }
    vec3_t res = (vec3_t){
        0.5f * (mi.x + mx.x),
        0.5f * (mi.y + mx.y),
        0.5f * (mi.z + mx.z)
    };
    return res;
}

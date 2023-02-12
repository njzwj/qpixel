#include <stdio.h>
#include "qmesh.h"
#include "qtga.h"

#define MESH_PATH "./models/helmet.obj"

mesh_t *mesh = NULL;

int main()
{

    /*
    mesh = load_mesh(MESH_PATH);

    if (!mesh)
    {
        printf("Load %s failed.\n", MESH_PATH);
        return 0;
    }
    printf("Loading %s successful.\n", MESH_PATH);
    
    if (mesh->mesh_type & T_TEXCOORD)
    {
        printf("> file contains texcoord\n");
    }
    if (mesh->mesh_type & T_NORMAL)
    {
        printf("> file contains normal\n");
    }
    
    printf("> Here is the first face:\n");
    for (int i = 0; i < 3; i++)
    {
        printf("%d", mesh->vertex_idx[i]);
        if ((mesh->mesh_type & T_TEXCOORD)
         || (mesh->mesh_type & T_NORMAL))
        {
            printf("/");
            if (mesh->mesh_type & T_TEXCOORD)
            {
                printf("%d", mesh->texcoord_idx[i]);
            }
            printf("/");
            if (mesh->mesh_type & T_NORMAL)
            {
                printf("%d", mesh->normal_idx[i]);
            }
        }
        printf(" ");
    }
    puts("");
    */

    tga_t *tga_image;
    
    tga_image = read_tga("./models/helmet_basecolor.tga");

    brief_tga_header(&tga_image->header);

    return 0;
}

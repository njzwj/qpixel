#pragma once

// http://www.paulbourke.net/dataformats/tga/

typedef struct
{
    char  idlength;         /* Identity char length */
    char  colourmaptype;    /* 0 if no colormap, 1 otherwise */
    char  datatypecode;     /* Data type specificatiosn */
    
    short colourmaporigin;  /* Colormaps */
    short colourmaplength;
    char  colourmapdepth;

    short x_origin;
    short y_origin;
    unsigned short width;
    unsigned short height;
    char  bitsperpixel;
    char  imagedescriptor;
} tga_header_t;

typedef enum
{
    TGA_BGR,
    TGA_BGRA
} tga_color_t;

typedef struct
{
    tga_header_t header;
    char *id;               /* identification */
    int width;
    int height;
    int bytes_per_pixel;
    tga_color_t color_type;

    unsigned char *buffer;
} tga_t;

/**
 * @brief Reads tga header
 * 
 * @param f 
 * @param header 
 * @return int  0 if success.
 */
int read_tga_header(FILE *f, tga_header_t *header);

/**
 * @brief Prints TGA header
 * 
 * @param header 
 */
void brief_tga_header(tga_header_t *header);

/**
 * @brief Reads TGA file
 * 
 * @param fn 
 * @return tga_t*  NULL if failed.
 */
tga_t *read_tga(const char *fn);

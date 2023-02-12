#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "qtga.h"

typedef enum
{
    NODATA = 0,
    
    UNCOMP_COLORMAPPED = 1,
    UNCOMP_RGB,
    UNCOMP_BW,

    RLE_COLORMAPPED = 9,
    RLE_RGB,
    
    COMP_BW,
    COMP_COLORMAPPED = 32,
    COMP_COLORMAPPED_QUAD,
} tga_datatype;

int fgetshort(FILE *f)
{
    int low = fgetc(f);
    int high = fgetc(f);
    return low | (high << 8);
}

int read_tga_header(FILE *f, tga_header_t *header)
{
    header->idlength = fgetc(f);
    header->colourmaptype = fgetc(f);
    header->datatypecode = fgetc(f);
    header->colourmaporigin = fgetshort(f);
    header->colourmaplength = fgetshort(f);
    header->colourmapdepth = fgetc(f);
    header->x_origin = fgetshort(f);
    header->y_origin = fgetshort(f);
    header->width = fgetshort(f);
    header->height = fgetshort(f);
    header->bitsperpixel = fgetc(f);
    header->imagedescriptor = fgetc(f);
    return 0;
}

void brief_tga_header(tga_header_t *header)
{
    printf("TGA FILE HEADER");
    printf("\nID Length        %u", header->idlength);
    printf("\nColormap Type    %u", header->colourmaptype);
    printf("\nDatatype Code    %u", header->datatypecode);
    printf("\nColormap Origin  %d", header->colourmaporigin);
    printf("\nColormap Length  %d", header->colourmaplength);
    printf("\nColormap Depth   %u", header->colourmapdepth);
    printf("\nX Origin         %d", header->x_origin);
    printf("\nY Origin         %d", header->y_origin);
    printf("\nWidth            %u", header->width);
    printf("\nHeight           %u", header->height);
    printf("\nBits per Pixel   %u", header->bitsperpixel);
    printf("\nImage Descriptor %u", header->imagedescriptor);
    puts("");
}

int read_rle_buffer(FILE *f, unsigned char *buffer, unsigned char bitsperpixel,
    int width, int height);

tga_t *read_tga(const char *fn)
{
    tga_t *tga;
    FILE *f;
    tga_header_t header;
    tga_datatype tgatype;
    char *id;
    unsigned char *buffer;
    int ret;

    fopen_s(&f, fn, "rb");
    if (f == NULL)
    {
        fclose(f);
        return NULL;
    }
    
    ret = read_tga_header(f, &header);
    if (ret)
    {
        fclose(f);
        return NULL;
    }
    tgatype = (tga_datatype)header.datatypecode;
    assert(header.bitsperpixel == 24 || header.bitsperpixel == 32);
    assert(header.colourmaptype == 0);
    assert(tgatype == RLE_RGB);
    
    switch (tgatype)
    {
        case NODATA:
        case UNCOMP_COLORMAPPED:
        case UNCOMP_RGB:
        case UNCOMP_BW:
        case RLE_COLORMAPPED:
            fclose(f);
            return NULL;
            break;
        case RLE_RGB:
            break;
        default:
            fclose(f);
            return NULL;
            break;
    }

    tga = (tga_t *)malloc(sizeof(tga_t));
    if (header.bitsperpixel == 24)
    {
        tga->color_type = TGA_BGR;
    }
    else if (header.bitsperpixel == 32)
    {
        tga->color_type = TGA_BGRA;
    }
    tga->id = id;
    tga->width = header.width;
    tga->height = header.height;
    tga->bytes_per_pixel = header.bitsperpixel / 8;
    memcpy_s(&tga->header, sizeof(tga_header_t), &header, sizeof(tga_header_t));

    /* Read Identity String */
    id = (char *)calloc(header.idlength + 1, sizeof(char));
    fread_s(id, header.idlength + 1, 1, header.idlength, f);

    /* Read RLE buffer */
    buffer = (unsigned char*)calloc((size_t)header.width * header.height, header.bitsperpixel / 8);
    ret = read_rle_buffer(f, buffer, header.bitsperpixel,
        header.width, header.height);
    assert(ret == (int)header.width * header.height);
    
    /* Close file */
    fclose(f);

    tga->buffer = buffer;

    return tga;
}

int read_rle_buffer(FILE *f, unsigned char *buffer, unsigned char bitsperpixel,
    int width, int height)
{
    int count = 0, full_length = width * height;
    int color_bytes = bitsperpixel / 8;
    int c;
    while (count < full_length && (c = fgetc(f)) != EOF)
    {
        int pak_type = c >> 7;
        int pak = (c & 0x7f) + 1;

        fread_s(buffer, color_bytes, color_bytes, 1, f);
        if (pak_type == 1)  /* Repeated color */
        {
            for (int i = 1; i < pak; i ++)
            {
                memcpy_s(buffer + i * color_bytes,
                    color_bytes, buffer, color_bytes);
            }
        }
        else                /* Color packet */
        {
            fread_s(buffer + color_bytes, color_bytes * (pak - 1), color_bytes, pak - 1, f);
        }
        buffer += color_bytes * pak;
        count += pak;
    }
    return count;
}

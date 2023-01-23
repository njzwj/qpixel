#ifndef UNICODE
#define UNICODE
#endif 

#include <assert.h>
#include <windows.h>
#include <stdio.h>
#include "common.h"
#include "qpixel.h"

#define N_OBJ_MAX 1024
#define MAP_ROW 5
#define MAP_COL 5

typedef struct
{
    int mouseX, mouseY;     // mouse position
    
    float distance;         // camera distance to target
    float fov;              // camera fov
    float n;
    float f;                // camera near and far
    vec3_t eye;             // eye position
    vec3_t target;          // looking to target
    vec3_t up;              // camera up direction

    screen_t screen;
    device_t device;
    scene_t  scene;
    mesh_t   *cube;
} demo_t;

demo_t demo;
clock_t last_tick;


/**
 * @brief Set demo info
 */
void init_demo();


/**
 * @brief Initialize Device Independent Bitmap and buffer, screen object
 * 
 * @param hwnd      Window handle
 */
int init_dib(HWND hwnd);


/**
 * @brief Initialize the scene. Setup all objects.
 * 
 */
void init_scene();


/**
 * @brief Set camera, animation, and render
 * 
 */
void draw_demo_scene();


/**
 * @brief The entrance of drawing functionality
 * 
 * @param hwnd      Window handle
 */
void on_draw(HWND hwnd);

// ==================
// Setup GDI window & dib, events, etc.
// ==================

LRESULT CALLBACK WindowProc(HWND hwnd,
                            UINT uMsg,
                            WPARAM wParam,
                            LPARAM lParam)
{

    switch (uMsg)
    {
    case WM_CREATE:
        {
            return 0;
        }
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0);
        return 0;

    case WM_PAINT:
        on_draw(hwnd);
        return 0;

    case WM_MOUSEMOVE:
        demo.mouseX = LOWORD(lParam);
        demo.mouseY = HIWORD(lParam);
        demo.mouseX = demo.mouseX >= 0 ? (demo.mouseX < USER_WIDTH ? demo.mouseX : USER_WIDTH - 1) : 0;
        demo.mouseY = demo.mouseY >= 0 ? (demo.mouseY < USER_HEIGHT ? demo.mouseY : USER_HEIGHT - 1) : 0;
        return 0;
    
    case WM_KEYUP:
        switch (wParam)
        {
        defaut: break;
        }
        return 0;

    case WM_TIMER:
        // Fix window size & redraw
        // SetWindowPos(hwnd, NULL, 0, 0, USER_WIDTH, USER_HEIGHT, SWP_NOMOVE);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


int WINAPI wWinMain(HINSTANCE hInstance,
                    HINSTANCE hPrevInstance,
                    PWSTR pCmdLine,
                    int nCmdShow
                    )
{
    // Register the window class.
    const wchar_t CLASS_NAME[]  = L"Sample Window Class";
    
    WNDCLASS wc = { };

    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.style         = CS_HREDRAW | CS_VREDRAW;
    wc.hCursor       = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    // Create the window.

    HWND hwnd = CreateWindowEx(
        0,                              // Optional window styles.
        CLASS_NAME,                     // Window class
        L"Q PIXEL",                     // Window text
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,            // Window style

        // Size and position
        CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,

        NULL,       // Parent window    
        NULL,       // Menu
        hInstance,  // Instance handle
        NULL        // Additional application data
        );

    if (hwnd == NULL)
    {
        return 0;
    }

    // Set Window Size

    RECT rect = { 0, 0, USER_WIDTH, USER_HEIGHT };
    AdjustWindowRect(&rect, GetWindowLong(hwnd, GWL_STYLE), 0);
    SetWindowPos(hwnd, NULL, 0, 0, rect.right - rect.left, rect.bottom - rect.top,
        SWP_NOMOVE);


    // Set DIB
    if (!init_dib(hwnd))
    {
        return 0;
    }
    init_demo();

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Run the message loop.
    SetTimer(hwnd, 1, MIN_RESET, NULL);

    MSG msg = { };
    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}


// ===================
// Shader
// ===================
typedef struct
{
    vec3_t c_glow;          // Glow color
    vec3_t c_diffuse;       // Diffuse color
    vec3_t c_ambient;       // Ambient color
    vec3_t c_light;         // Light color
    vec3_t dir_light;       // Light direction
} material_t;

typedef struct
{
    vec3_t c_glow;          // Glow color
    vec3_t c_diffuse;       // Diffuse color
    vec3_t c_ambient;       // Ambient color
    vec3_t c_light;         // Light color
    vec3_t dir_light;       // Light direction
} uniform_t;

typedef struct
{
    vec3_t normal;          // Vertex normal
    vec2_t texcoord;        // Texture coordinates
} attribute_t;

typedef struct
{
    vec3_t normal;
    vec2_t texcoord;
} varying_t;


void drawer(device_t *device, mesh_t *mesh, void *material)
{
    vec3_t *v = device->vertex;
    material_t *mtl = (material_t *)material;
    uniform_t *uniforms = (uniform_t *)device->unif;
    attribute_t *attributes = (attribute_t *)device->attr;
    uint32_t n_faces = mesh->n_faces;
    uint32_t fi = 0;

    // device->debug[0] = n_faces;
    memcpy(uniforms, mtl, sizeof(uniform_t));
    for (int i = 0; i < n_faces; i ++)
    {
        uint32_t *vidx = &mesh->vertex_idx[fi];
        uint32_t *nidx = &mesh->normal_idx[fi];
        uint32_t *tidx = &mesh->texcoord_idx[fi];
        for (int j = 0; j < 3; j ++)
        {
            attributes[j].normal = vec3_normalize(vec3_mat_mul(
                mesh->normals[nidx[j] - 1], &device->m_world
            ));
            attributes[j].texcoord = mesh->texcoords[tidx[j] - 1];
            v[j] = mesh->vertices[vidx[j] - 1];
        }
        draw_triangle(device);
        fi += 3;
    }
}


void vs(device_t *device, float *unif, float *attr, float *vary)
{
    attribute_t *attributes = (attribute_t *)attr;
    varying_t *varyings = (varying_t *)vary;

    varyings->normal = attributes->normal;
    varyings->texcoord = attributes->texcoord;
}


void fs(device_t *device, float *unif, float *vary, float w, color3_t *out)
{
    size_t unif_l = sizeof(uniform_t);
    size_t attr_l = sizeof(attribute_t);
    size_t vary_l = sizeof(varying_t);
    
    uniform_t *uniforms = (uniform_t *)unif;
    varying_t *varyings = (varying_t *)vary;
    
    vec3_t diffuse = uniforms->c_diffuse;

    float intensity = - vec3_dot(varyings->normal, uniforms->dir_light);
    intensity = clip_float(intensity, 0.0f, 1.0f);
    vec3_t color = vec3_mul(uniforms->c_light, intensity);
    color = vec3_add(color, uniforms->c_ambient);
    color = vec3_add(color, diffuse);
    color = vec3_add(color, uniforms->c_glow);
    color = vec3_clip(color, 0.0f, 1.0f);
    
    memcpy(out, &color, sizeof(float) * 3);
}


void init_demo()
{
    screen_t *screen = &demo.screen;
    device_t *device = &demo.device;
    scene_t  *scene  = &demo.scene;

    // Load cube mesh
    demo.cube = load_mesh("./models/cube.obj");

    // Setup scene
    init_scene();

    // Setup camera info
    demo.distance = 20.0f;
    demo.fov = 30.0f;
    demo.n = 1.0f;
    demo.f = 100.0f;
    demo.target = (vec3_t){ 0.0f, 0.0f, 0.0f };
    demo.up = (vec3_t){ 0.0f, 0.0f, 1.0f };

    // Setup device buffer -> screen buffer / DIB
    setup_device(device, screen->width, screen->height, screen->buffer);
    clear_buffer(device);
    
    // Setup shader
    device->unif_size = sizeof(uniform_t) / sizeof(float);
    device->unif = calloc(1, sizeof(uniform_t));
    device->attr_size = sizeof(attribute_t) / sizeof(float);
    device->attr = calloc(3, sizeof(attribute_t));
    device->vary_size = sizeof(varying_t) / sizeof(float);
    device->vary = calloc(3, sizeof(varying_t));

    device->drawer = &drawer;
    device->vs = &vs;
    device->fs = &fs;
}


int init_dib(HWND hwnd)
{
    int w = USER_WIDTH;
    int h = USER_HEIGHT;
    screen_t * screen = &demo.screen;
    screen->width = w;
    screen->height = h;
    screen->pitch = 4;
    LPVOID ptr;
    BITMAPINFO bi = { { sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB, 
        w * h * 4, 0, 0, 0, 0 }  };
    HDC hdc = GetDC(hwnd);
    screen->dc = CreateCompatibleDC(hdc);
    ReleaseDC(hwnd, hdc);
    screen->bmp = CreateDIBSection(screen->dc, &bi, DIB_RGB_COLORS, &ptr, 0, 0);
    if (screen->bmp == NULL)
    {
        return 0;
    }
    SelectObject(screen->dc, screen->bmp);
    screen->buffer = (unsigned char*)ptr;
    memset(screen->buffer, 0, w * h * 4);
    return 1;
}


void on_draw(HWND hwnd)
{
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rect;

    // Render
    draw_demo_scene();
    BitBlt(hdc, 0, 0, USER_WIDTH, USER_HEIGHT, demo.screen.dc, 0, 0, SRCCOPY);
    
    // Draw debug info
    WCHAR debugInfo[256];
    clock_t current = clock();
    device_t *dev = &demo.device;
    GetClientRect(hwnd, &rect);
    float ms = (float)(current - last_tick) / CLOCKS_PER_MS;
    last_tick = current;
    swprintf(debugInfo, 256, TEXT("%.2f fps\n%u triangles\n%u texels\n%u objects\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n"),
        1000.0f / ms, demo.device.triangle_count, demo.device.texel_count,
        demo.device.object_count,
        dev->debug[0], dev->debug[1], dev->debug[2], dev->debug[3],
        dev->debug[4], dev->debug[5], dev->debug[6], dev->debug[7],
        dev->debug[8], dev->debug[9], dev->debug[10], dev->debug[11],
        dev->debug[12], dev->debug[13], dev->debug[14], dev->debug[15]);
    DrawText(hdc, debugInfo, -1, &rect,
                DT_LEFT | DT_TOP );

    EndPaint(hwnd, &ps);
}

// ===============
// Scene objects
// ===============
object3d_t object_pool[N_OBJ_MAX];
material_t material_pool[N_OBJ_MAX];
float      glow_pool[N_OBJ_MAX];

void init_scene()
{
    device_t *device = &demo.device;
    scene_t *scene = &demo.scene;

    // init object3d
    scene->n_objects = 0;
    scene->objects = calloc(N_OBJ_MAX, sizeof(object3d_t *));
    memset(scene->objects, 0, sizeof(object3d_t *) * N_OBJ_MAX);

    // init material pool
    memset(object_pool, 0, sizeof(material_t) * N_OBJ_MAX);
    memset(material_pool, 0, sizeof(material_t) * N_OBJ_MAX);

    int n = 0;
    float y = - ((MAP_ROW - 1.0f) / 2.0f), x;
    for (int i = 0; i < MAP_ROW; i ++)
    {
        x = - ((MAP_COL - 1.0f) / 2.0f);
        for (int j = 0; j < MAP_COL; j ++)
        {
            object3d_t *obj = &object_pool[n];
            obj->mesh = demo.cube;
            obj->material = &material_pool[n];
            obj->position = (vec3_t){ x, y, 0.0f };
            obj->rotation = quat_from_axis_angle(
                (vec3_t){ 1.0f, 0.0f, 0.0f }, 0.0f);
            obj->scale = (vec3_t){ 0.4f, 0.4f, 0.1f };
            object_update_m_world(obj);

            // memcpy(device->debug, &obj->m_world, 16 * sizeof(float));

            material_t *mtl = &material_pool[n];
            mtl->c_glow = (vec3_t){ 0.0f, 0.0f, 0.0f };
            mtl->c_diffuse = (vec3_t){ 0.2f, 0.2f, 0.2f };
            mtl->c_light = (vec3_t){ 0.2f, 0.2f, 0.2f };
            mtl->c_ambient = (vec3_t){ 0.1f, 0.1f, 0.1f };
            mtl->dir_light = vec3_normalize((vec3_t){ -1.0f, -1.0f, -1.0f });

            scene->objects[n] = obj;
            scene->n_objects ++;
            n ++;
            x += 1.0f;
        }
        y += 1.0f;
    }
}


void draw_demo_scene()
{
    device_t *device = &demo.device;
    scene_t *scene = &demo.scene;
    int w = device->width;
    int h = device->height;

    // projection matrix
    get_projection_mat(&device->m_project,
        demo.fov, (float)w / h, demo.n, demo.f);

    // camera matrix
    float r = demo.distance;
    float u = (1.5f * demo.mouseX / w - 0.75f) * PI;
    float v = (0.25f + 0.5 * (1.0f * demo.mouseY / h - 0.5f)) * PI;
    float s = sinf(u), c = cosf(u);
    float s2 = sinf(v), c2 = cosf(v);
    vec3_t eye = (vec3_t){ c * r * c2, -s * r * c2, s2 * r };
    demo.eye = eye;
    get_lookat_mat(&device->m_camera, demo.eye, demo.target, demo.up);

    // memcpy(device->debug, &device->m_camera, 16 * sizeof(float));
    // Random Glow
    // srand((unsigned int)time(NULL));
    for (int i = 0; i < scene->n_objects; i ++)
    {
        glow_pool[i] -= 0.1f;
        glow_pool[i] = glow_pool[i] > 0 ? glow_pool[i] : 0.0f;
    }
    int lit = rand() % scene->n_objects;
    glow_pool[lit] = 1.0f;
    for (int i = 0; i < scene->n_objects; i ++)
    {
        material_t *mtl = scene->objects[i]->material;
        mtl->c_glow = vec3_mul((vec3_t){ 0.0f, 1.0f, 1.0f }, glow_pool[i]);
    }

    clear_buffer(device);
    draw_scene(device, scene);
}

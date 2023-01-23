#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <stdio.h>
#include "common.h"
#include "qpixel.h"

/* ========= GLOBAL INFO =========== */
// #define MESH_FILE_NAME "./models/helmet.obj"
#define MESH_FILE_NAME "./models/cube.obj"
#define N_OBJECT_MAX 256

float sample_vary[9];

int ready = 0;

screen_t screen;
device_t device;
scene_t scene;
int mouseX = 100, mouseY = 100;
float distance = 20.0f;

mesh_t *mesh;

clock_t last_tick = 0;

void setup_render_info(device_t * device);

void setup_scene(device_t * device);

void render(device_t * device);

void fill_ramp(int);

void on_draw(HWND);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

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

    int w = USER_WIDTH;
    int h = USER_HEIGHT;
    screen.width = w;
    screen.height = h;
    screen.pitch = 4;
    LPVOID ptr;
	BITMAPINFO bi = { { sizeof(BITMAPINFOHEADER), w, -h, 1, 32, BI_RGB, 
		w * h * 4, 0, 0, 0, 0 }  };
    HDC hdc = GetDC(hwnd);
    screen.dc = CreateCompatibleDC(hdc);
    ReleaseDC(hwnd, hdc);
    screen.bmp = CreateDIBSection(screen.dc, &bi, DIB_RGB_COLORS, &ptr, 0, 0);
    if (screen.bmp == NULL)
    {
        return 0;
    }
    SelectObject(screen.dc, screen.bmp);
    screen.buffer = (unsigned char*)ptr;
    memset(screen.buffer, 0, w * h * 4);
    // Set Device
    setup_device(&device, screen.width, screen.height, screen.buffer);
    clear_buffer(&device);
    setup_render_info(&device);
    setup_scene(&device);

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
        mouseX = LOWORD(lParam);
        mouseY = HIWORD(lParam);
        mouseX = mouseX >= 0 ? (mouseX < USER_WIDTH ? mouseX : USER_WIDTH - 1) : 0;
        mouseY = mouseY >= 0 ? (mouseY < USER_HEIGHT ? mouseY : USER_HEIGHT - 1) : 0;
        return 0;
    
    case WM_KEYUP:
        switch (wParam)
        {
        case VK_UP:
            distance /= 1.2f;
            break;
        case VK_DOWN:
            distance *= 1.2f;
            break;
        defaut: break;
        }
        distance = distance < 0.5f ? 0.5f : distance;
        return 0;

    case WM_TIMER:
        // Fix window size & redraw
        // SetWindowPos(hwnd, NULL, 0, 0, USER_WIDTH, USER_HEIGHT, SWP_NOMOVE);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;

    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void fill_ramp(int t)
{
    int i, j;
    float r, g, b;
    color_buffer_t buf = device.colorBuffer;
    color_buffer_t line = buf;
    for (i = 0; i < device.height; i++) {
        line = buf + (i * device.width * 4);
        float ratio = (float)((i + t * 10) % device.height) / device.height;
        for (j = 0; j < device.width; j++) {
            r = 0.5;
            g = ratio * (-0.5) + 1.0;
            b = ratio * 0.5 + 0.5;
            *(line ++) = (unsigned char)roundf(b * 255.0);
            *(line ++) = (unsigned char)roundf(g * 255.0);
            *(line ++) = (unsigned char)roundf(r * 255.0);
            line ++;
        }
    }
}

void on_draw(HWND hwnd)
{
    static int t = 0;
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rect;

    // All painting occurs here, between BeginPaint and EndPaint.

    // fill_ramp(t);

    render(&device);

    BitBlt(hdc, 0, 0, USER_WIDTH, USER_HEIGHT, screen.dc, 0, 0, SRCCOPY);
    
    // Debug info

    WCHAR debugInfo[256];
    // FillRect(hdc, &ps.rcPaint, (HBRUSH) (COLOR_WINDOW+1));
    GetClientRect(hwnd, &rect);
    float dpi = GetDpiForSystem();

    // calculate tick
    clock_t current = clock();
    float ms = (float)(current - last_tick) / CLOCKS_PER_MS;
    last_tick = current;
    
    // swprintf(debugInfo, 256, TEXT("%ld, %ld, %ld, %ld\n%ld, %ld\n%f\n"),
    //     rect.left, rect.top, rect.right, rect.bottom,
    //     mouseX, mouseY,
    //     device.debug);
    // mat4_t *m = &device.m_world;
    // float *d = device.debug;
    // swprintf(debugInfo, 256, TEXT("%f %f %f\n%f"),
    //     d[0], d[1], d[2], d[3]);
    swprintf(debugInfo, 256, TEXT("%.2f fps\n%u triangles\n%u texels\n"),
        1000.0f / ms, device.triangle_count, device.texel_count);
    
    DrawText(hdc, debugInfo, -1, &rect,
                DT_LEFT | DT_TOP );
    EndPaint(hwnd, &ps);

    t++;
}


// ========================= TEST =======================
typedef struct
{
    mat4_t m_project;
    mat4_t m_world;
    mat4_t m_world_inv;

    vec3_t dir_light;
    vec3_t c_ambient;
    vec3_t c_light;
} uniform_t;

typedef struct
{
    vec3_t normal;
    // vec2_t texcoord;
} attribute_t;

typedef struct
{
    vec3_t normal;
    // vec2_t texcoord;
} varying_t;

void drawer_build_attribute(mesh_t *mesh, uint32_t fi, uniform_t *unif, attribute_t *attr)
{
    uint32_t *vidx = &mesh->vertex_idx[fi];
    uint32_t *nidx = &mesh->normal_idx[fi];
    uint32_t *tidx = &mesh->texcoord_idx[fi];

    // Calc light
    // float intensity = vec3_dot(normal, unif->dir_light);
    // device.debug[3] = intensity;
    // intensity = clip_float(intensity, 0.0f, 1.0f);

    // vec3_t color = vec3_mul(unif->c_light, intensity);
    // color = vec3_add(color, unif->c_ambient);
    // color = vec3_clip(color, 0.0f, 1.0f);
    // memcpy(device.debug, &color, 3 * sizeof(float));
    
    for (int i = 0; i < 3; i++)
    {
        attr[i].normal = vec3_normalize(
            vec3_mat_mul(mesh->normals[nidx[i] - 1], &unif->m_world)
        );
        // attr[i].texcoord = mesh->texcoords[tidx[i] - 1];
    }
}

void drawer(device_t *device, mesh_t *mesh)
{
    uniform_t *uniforms = (uniform_t *)device->unif;
    attribute_t *attributes = (attribute_t *)device->attr;
    varying_t *varyings = (varying_t *)device->vary;
    vec3_t *v = device->vertex;
    uint32_t n_faces = mesh->n_faces;
    uint32_t fi = 0;

    uniforms->m_world = device->m_world;
    uniforms->m_project = device->m_project;
    uniforms->m_world_inv = device->m_world_inv;
    uniforms->c_ambient = (vec3_t){ 0.1f, 0.1f, 0.1f };
    uniforms->c_light = (vec3_t){ 0.5f, 0.5f, 0.5f };
    uniforms->dir_light = vec3_normalize((vec3_t){ -1.0f, -1.0f, -1.0f });

    for (int i = 0; i < n_faces; i ++)
    {
        drawer_build_attribute(mesh, fi, (uniform_t *)device->unif, (attribute_t *)device->attr);
        v[0] = mesh->vertices[mesh->vertex_idx[fi++] - 1];
        v[1] = mesh->vertices[mesh->vertex_idx[fi++] - 1];
        v[2] = mesh->vertices[mesh->vertex_idx[fi++] - 1];
        draw_triangle(device);
    }
}

void vs(device_t *device, float *unif, float *attr, float *vary)
{
    size_t unif_l = sizeof(float) * device->unif_size;
    size_t attr_l = sizeof(float) * device->attr_size;
    size_t vary_l = sizeof(float) * device->vary_size;

    uniform_t *uniforms = (uniform_t *)unif;
    attribute_t *attributes = (attribute_t *)attr;
    varying_t *varyings = (varying_t *)vary;

    varyings->normal = attributes->normal;
    // varyings->texcoord = attributes->texcoord;
}

int sample_chessboard(vec2_t texcoord, int w, int h)
{
    int u = (int)roundf(texcoord.x * w);
    int v = (int)roundf(texcoord.y * h);
    return (u + v) & 1;
}

void fs(device_t *device, float *unif, float *vary, float w, color3_t *out)
{
    size_t unif_l = sizeof(uniform_t);
    size_t attr_l = sizeof(attribute_t);
    size_t vary_l = sizeof(varying_t);
    
    uniform_t *uniforms = (uniform_t *)unif;
    varying_t *varyings = (varying_t *)vary;
    
    // vec2_t t = varyings->texcoord;
    // vec3_t diffuse = (vec3_t){ 0.0f, 0.0f, 0.0f };
    // if (sample_chessboard(t, 32, 32))
    // {
    //     diffuse = (vec3_t){ 0.5f, 0.5f, 0.5f };
    // }
    vec3_t diffuse = (vec3_t){ 0.5f, 0.5f, 0.5f };

    float intensity = - vec3_dot(varyings->normal, uniforms->dir_light);
    intensity = clip_float(intensity, 0.0f, 1.0f);
    vec3_t color = vec3_mul(uniforms->c_light, intensity);
    color = vec3_add(color, uniforms->c_ambient);
    color = vec3_add(color, diffuse);
    color = vec3_clip(color, 0.0f, 1.0f);
    
    memcpy(out, &color, sizeof(float) * 3);
}

void setup_render_info(device_t * device)
{
    device->unif_size = sizeof(uniform_t) / sizeof(float);
    device->attr_size = sizeof(attribute_t) / sizeof(float);
    device->vary_size = sizeof(varying_t) / sizeof(float);
    device->unif = (float *)malloc(sizeof(uniform_t));
    device->attr = (float *)malloc(sizeof(attribute_t) * 3);
    device->vary = (float *)malloc(sizeof(varying_t) * 3);

    device->drawer = &drawer;
    device->vs = &vs;
    device->fs = &fs;

    get_projection_mat(&device->m_project, 45.0,
        (float)USER_WIDTH / USER_HEIGHT,
        1.0f, 100.0f);
    get_lookat_mat(&device->m_world, (vec3_t){ 0.0f, 0.0f, 3.0f }, (vec3_t){0.0f, 0.0f, 0.0f}, (vec3_t){0.0f, 1.0f, 0.0f});
    // calc_inv_mat(&device->m_world, &device->m_world_inv);
}

/**
 * @brief Get random float between a and b
 * 
 * @return float  Random number
 */
inline float rfloat(float a, float b)
{
    return ((float)rand() / RAND_MAX) * (b - a) + a;
}

void setup_scene(device_t * device)
{
    mesh = load_mesh(MESH_FILE_NAME);

    scene.n_objects = N_OBJECT_MAX;
    scene.objects = calloc(N_OBJECT_MAX, sizeof(object3d_t));

    srand((unsigned int)time(NULL));
    for (int i = 0; i < N_OBJECT_MAX; i ++)
    {
        object3d_t * obj = scene.objects + i;
        obj->mesh = mesh;
        obj->position = \
            (vec3_t){ rfloat(-5, 5), rfloat(-5, 5), rfloat(-5, 5) };
        obj->scale = \
            (vec3_t){ rfloat(0.1, 0.2), rfloat(0.1, 0.2), rfloat(0.1, 0.2) };
        obj->rotation = \
            quat_from_axis_angle(vec3_normalize(
                (vec3_t){ rfloat(-1, 1), rfloat(-1, 1), rfloat(-1, 1) }
            ), rfloat(0, PI));
        object_update_m_world(obj);
    }
    
    ready = 1;
}

void render(device_t * device)
{
    float r = distance;
    float z = (1.5f * mouseX / device->width - 0.75f) * PI;
    float u = (1.0f * mouseY / device->height - 0.5f) * PI;
    float ss = sinf(u);
    float cc = cosf(u);
    float s = sinf(z) * cc * r;
    float c = cosf(z) * cc * r;
    float ya = 1.5f;

    vec3_t eye = (vec3_t){ - s, ss * r, c };
    vec3_t tar = (vec3_t){ 0.0, 0.0, 0.0 };
    eye = vec3_add(eye, tar);
    get_lookat_mat(&device->m_camera, eye, tar, (vec3_t){0.0f, 1.0f, 0.0f});

    clear_buffer(device);
    draw_scene(device, &scene);
}

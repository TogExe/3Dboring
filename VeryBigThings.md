# First logged version (to lazy to know wich one)
``main.cpp``
```c++
#include <windows.h>
#include <gdiplus.h>
#include <cmath>
#include <vector>
#include <chrono>
#include <stdio.h>

#define TRUE 1
#define FALSE 0
#define DEBUG TRUE
#define HEIGHT 400
#define WIDTH 400
#define FOV 90
#define PI 3.14159265

int update = true;

float player_x_offset = 0.0f;
float player_y_offset = 0.0f;
float player_z_offset = 7.0f;

float camera_offset_x = 0.0f;
float camera_offset_y = 0.0f;
float camera_offset_z = 0.0f;
float camera_rotation[3] = {0., 0., 0.};

int s_l = 0;

typedef struct Point {
    float pos_x, pos_y, pos_z;
} Point;

typedef struct Vertex {
    Point point;
    struct Vertex* next;
    enum { HEAD, BASIC } type;
} Vertex;
Vertex pt_u[3];
typedef struct Triangle {
    Vertex vertex[3];
} Triangle;

typedef struct Environment {
    int size;
    Triangle* triangles;
} Environment;

void setupEnvironment(Environment* env, int size_) {
    env->size = size_;
    env->triangles = (Triangle*)malloc(size_ * sizeof(Triangle));
    if (env->triangles == NULL) {
        fprintf(stderr, "Erreur d'allocation de mémoire\n");
        exit(1);
    }
}

void addTriangle(Environment* env, Vertex* v1, Vertex* v2, Vertex* v3) {
    env->size += 1;
    env->triangles = (Triangle*)realloc(env->triangles, env->size * sizeof(Triangle));
    if (env->triangles == NULL) {
        fprintf(stderr, "Erreur de réallocation de mémoire\n");
        exit(1);
    }
    env->triangles[env->size - 1] = (Triangle){ *v1, *v2, *v3 };
}

Triangle makeTriangle(Point point[3]) {
    Vertex vertex[3];
    Triangle triangle;

    vertex[0].point = point[0];
    vertex[1].point = point[1];
    vertex[2].point = point[2];

    for (int i = 0; i < 3; i++) {
        vertex[i].next = &vertex[(i + 1) % 3];
        triangle.vertex[i] = vertex[i];
    }
    return triangle;
}

void forwardMove(float d) {
    float pi = PI;
    float forward_z = d * cos(pi * camera_rotation[1]) * cos(pi * camera_rotation[0]);
    float forward_y = d * sin(pi * camera_rotation[0]);
    float forward_x = d * sin(pi * camera_rotation[1]) * cos(pi * camera_rotation[0]);

    camera_offset_x += forward_x;
    camera_offset_y += forward_y;
    camera_offset_z += forward_z;
}

void DrawTriangle(HDC hdc, Triangle* triangle) {
    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 255));
    for (int i = 0; i < 3; i++) {
        Gdiplus::PointF pt1(
            triangle->vertex[i].point.pos_x,
            triangle->vertex[i].point.pos_y);
        Gdiplus::PointF pt2(
            triangle->vertex[(i + 1) % 3].point.pos_x,
            triangle->vertex[(i + 1) % 3].point.pos_y);
        graphics.DrawLine(&pen, pt1, pt2);
    }
}

float ProjectionMatrix[4][4] = {0};

void setProjectionMatrix(const float& angleOfView, const float& near_, const float& far_) {
    float scale = 1 / tan(angleOfView * 0.5 * M_PI / 180);
    ProjectionMatrix[0][0] = scale;
    ProjectionMatrix[1][1] = scale;
    ProjectionMatrix[2][2] = -far_ / (far_ - near_);
    ProjectionMatrix[3][2] = -far_ * near_ / (far_ - near_);
    ProjectionMatrix[2][3] = -1;
    ProjectionMatrix[3][3] = 0;
    if (DEBUG) {
        for (int i = 0; i < 4; i++) {
            printf("\n");
            for (int j = 0; j < 4; j++) {
                printf("%.2f ", ProjectionMatrix[i][j]);
            }
        }
    }
}

typedef struct ScreenPoint {
    float x, y;
} ScreenPoint;

ScreenPoint ProjectPoint(const Point& point) {
    float x = point.pos_x * ProjectionMatrix[0][0];
    float y = point.pos_y * ProjectionMatrix[1][1];
    float z = point.pos_z * ProjectionMatrix[2][2];
    float w = point.pos_z * ProjectionMatrix[2][3];
    if (w == 0) w = 1; // Avoid division by zero
    x /= w;
    y /= w;
    ScreenPoint screenPoint;
    screenPoint.x = x * WIDTH / 2 + WIDTH / 2;
    screenPoint.y = -y * HEIGHT / 2 + HEIGHT / 2;
    return screenPoint;
}

Point rotatePoint(const Point& point) {
    float pi = PI;
    Point rotatedPoint = point;
    float rp[3] = {
        point.pos_x + camera_offset_x,
        (point.pos_y + camera_offset_y) * cos(pi * camera_rotation[0]) - (point.pos_z + camera_offset_z) * sin(pi * camera_rotation[0]),
        (point.pos_y + camera_offset_y) * sin(camera_rotation[0] * pi) + (point.pos_z + camera_offset_z) * cos(camera_rotation[0] * pi)
    };
    float rp_[3] = {
        rp[0] * cos(camera_rotation[2] * pi) - rp[1] * sin(camera_rotation[2] * pi),
        rp[0] * sin(camera_rotation[2] * pi) + rp[1] * cos(camera_rotation[2] * pi),
        rp[2],
    };
    float rp__[3] = {
        rp_[0] * cos(camera_rotation[1] * pi) - rp_[2] * sin(camera_rotation[1] * pi),
        rp_[1],
        rp_[0] * sin(camera_rotation[1] * pi) + rp_[2] * cos(camera_rotation[1] * pi)
    };
    rotatedPoint.pos_x = rp__[0];
    rotatedPoint.pos_y = rp__[1];
    rotatedPoint.pos_z = rp__[2];
    return rotatedPoint;
}
Point rotatePointAO(const Point& point,float*off) {
    float pi = PI;
    Point rotatedPoint = point;
    float rp[3] = {
        point.pos_x + camera_offset_x+off[0],
        (point.pos_y + camera_offset_y+off[1]) * cos(pi * camera_rotation[0]) - (point.pos_z + camera_offset_z+off[2]) * sin(pi * camera_rotation[0]),
        (point.pos_y + camera_offset_y+off[1]) * sin(camera_rotation[0] * pi) + (point.pos_z + camera_offset_z+off[2]) * cos(camera_rotation[0] * pi)
    };
    float rp_[3] = {
        rp[0] * cos(camera_rotation[2] * pi) - rp[1] * sin(camera_rotation[2] * pi),
        rp[0] * sin(camera_rotation[2] * pi) + rp[1] * cos(camera_rotation[2] * pi),
        rp[2],
    };
    float rp__[3] = {
        rp_[0] * cos(camera_rotation[1] * pi) - rp_[2] * sin(camera_rotation[1] * pi),
        rp_[1],
        rp_[0] * sin(camera_rotation[1] * pi) + rp_[2] * cos(camera_rotation[1] * pi)
    };
    rotatedPoint.pos_x = rp__[0];
    rotatedPoint.pos_y = rp__[1];
    rotatedPoint.pos_z = rp__[2];
    return rotatedPoint;
}

bool IsTriangleVisible(const Triangle& triangle) {
    return rotatePoint(triangle.vertex[0].point).pos_z <= 0 ||
           rotatePoint(triangle.vertex[1].point).pos_z <= 0 ||
           rotatePoint(triangle.vertex[2].point).pos_z <= 0;
}

void DrawTriangle3D(HDC hdc, Triangle* triangle) {
    if (IsTriangleVisible(*triangle)) {
        return;
    }

    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(100, 70, 120, 174));

    ScreenPoint points_s[3] = {
        ProjectPoint(rotatePoint(triangle->vertex[0].point)),
        ProjectPoint(rotatePoint(triangle->vertex[1].point)),
        ProjectPoint(rotatePoint(triangle->vertex[2].point))
    };

    Gdiplus::PointF points[] = {
        Gdiplus::PointF(points_s[0].x, points_s[0].y),
        Gdiplus::PointF(points_s[1].x, points_s[1].y),
        Gdiplus::PointF(points_s[2].x, points_s[2].y)
    };

    graphics.DrawLines(&pen, points, 3);
}
void DrawTriangle3DWithOffset(HDC hdc, Triangle* triangle,float*off) {
    if (IsTriangleVisible(*triangle)) {
        return;
    }

    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(100, 70, 120, 174));

    ScreenPoint points_s[3] = {
        ProjectPoint(rotatePointAO(triangle->vertex[0].point,off)),
        ProjectPoint(rotatePointAO(triangle->vertex[1].point,off)),
        ProjectPoint(rotatePointAO(triangle->vertex[2].point,off))
    };

    Gdiplus::PointF points[] = {
        Gdiplus::PointF(points_s[0].x, points_s[0].y),
        Gdiplus::PointF(points_s[1].x, points_s[1].y),
        Gdiplus::PointF(points_s[2].x, points_s[2].y)
    };

    graphics.DrawLines(&pen, points, 3);
}

typedef struct Model {
    std::vector<Triangle> triangles;
} Model;

Model makeCube(float size, float x_offset, float y_offset, float z_offset) {
    Model cube;

    Point vertices[8] = {
        { -size + x_offset, -size + y_offset, -size + z_offset },
        { size + x_offset, -size + y_offset, -size + z_offset },
        { size + x_offset, size + y_offset, -size + z_offset },
        { -size + x_offset, size + y_offset, -size + z_offset },
        { -size + x_offset, -size + y_offset, size + z_offset },
        { size + x_offset, -size + y_offset, size + z_offset },
        { size + x_offset, size + y_offset, size + z_offset },
        { -size + x_offset, size + y_offset, size + z_offset }
    };

    int indices[12][3] = {
        {0, 1, 2}, {0, 2, 3}, // Front face
        {4, 5, 6}, {4, 6, 7}, // Back face
        {0, 1, 5}, {0, 5, 4}, // Bottom face
        {3, 2, 6}, {3, 6, 7}, // Top face
        {0, 3, 7}, {0, 7, 4}, // Left face
        {1, 2, 6}, {1, 6, 5}  // Right face
    };

    for (int i = 0; i < 12; i++) {
        Point triangle_vertices[3] = {
            vertices[indices[i][0]],
            vertices[indices[i][1]],
            vertices[indices[i][2]]
        };
        cube.triangles.push_back(makeTriangle(triangle_vertices));
    }

    return cube;
}
void DrawModel3D(HDC hdc, Model* model) {
    for (Triangle& triangle : model->triangles) {
        DrawTriangle3D(hdc, &triangle);
    }
}
void DrawModelWithOffsetIn3d(HDC hdc, Model* model,float * off) {
    for (Triangle& triangle : model->triangles) {
        DrawTriangle3DWithOffset(hdc, &triangle,off);
    }
}

typedef struct Renderable {
    Model* model;
    float offsets[3];//x y z position
    int rotations[3];// every rotation
    struct Renderable* next;
} Renderable;
void drawRenderable(HDC hdc, Renderable* renderable) {
    DrawModelWithOffsetIn3d(hdc,renderable->model,renderable->offsets);
}
void addRenderable(Renderable** head, Renderable* newRenderable) {
    if (*head == NULL) {
        *head = newRenderable;
    } else {
        Renderable* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newRenderable;
    }
    newRenderable->next = NULL;
}
void drawRenderables(HDC hdc,Renderable** head) {
    Renderable* current = *head;
    while (current->next != NULL) {
        drawRenderable(hdc,current);
        current = current->next;
    }
}

Model createPyramid(float size, float x_offset, float y_offset, float z_offset) {
    Model pyramid;

    Point vertices[5] = {
        { -size + x_offset, -size + y_offset, -size + z_offset }, // Base - bas gauche
        { size + x_offset, -size + y_offset, -size + z_offset },  // Base - bas droite
        { size + x_offset, -size + y_offset, size + z_offset },   // Base - haut droite
        { -size + x_offset, -size + y_offset, size + z_offset },  // Base - haut gauche
        { x_offset, size + y_offset, z_offset } // Sommet
    };

    int indices[6][3] = {
        {0, 1, 2}, {0, 2, 3}, // Base
        {0, 1, 4}, // Face avant
        {1, 2, 4}, // Face droite
        {2, 3, 4}, // Face arrière
        {3, 0, 4}  // Face gauche
    };

    for (int _ = 0; _ < 6; _++) {
        Point triangle_vertices[3] = {
            vertices[indices[_][0]],
            vertices[indices[_][1]],
            vertices[indices[_][2]]
        };
        pyramid.triangles.push_back(makeTriangle(triangle_vertices));
    }

    return pyramid;
}

Environment env;

#include <stdbool.h>

// Tableau pour suivre l'état des touches
bool keys[256] = { false };

// Fonction pour gérer l'entrée des touches
void HandleInput(WPARAM wParam, bool isPressed) {
    keys[wParam] = isPressed;
}

bool helpSpaceKey = true;
Model cube = makeCube(1.0f, player_x_offset+1, player_y_offset-1, player_z_offset);
Renderable world ={&cube,NULL};
// Fonction pour mettre à jour les mouvements en fonction de l'état des touches
void UpdateMovement() {
    Renderable* current = &world;
    if (keys[VK_UP]) {
        forwardMove(-0.1f);
        update = true;
    }
    if (keys[VK_DOWN]) {
        forwardMove(0.1f);
        update = true;
    }
    if (keys[VK_LEFT]) {
        camera_rotation[1] += 0.01f;
        update = true;
    }
    if (keys[VK_RIGHT]) {
        camera_rotation[1] -= 0.01f;
        update = true;
    }
    if (keys['Q']) {
        camera_rotation[1] -= 0.05f;
        update = true;
    }
    if (keys['D']) {
        camera_rotation[1] += 0.05f;
        update = true;
    }
    if (keys['S']) {
        camera_rotation[0] -= 0.05f;
        update = true;
    }
    if (keys['Z']) {
        camera_rotation[0] += 0.05f;
        update = true;
    }
    if (keys[VK_SPACE]) {
        if(helpSpaceKey) {
            //addRenderable(&current, &cube);
            helpSpaceKey = false;
        }
    }else {
        helpSpaceKey = true;
    }
}

// Dans votre boucle principale ou fonction de mise à jour
void MainLoop() {
    // Mettre à jour les mouvements en fonction de l'état des touches
    UpdateMovement();

    // Autres mises à jour et rendu
    // ...
}
//Model player_model = makeCube(1.0f, player_x_offset, player_y_offset, player_z_offset);
Model pyr = createPyramid(2.0f, player_x_offset+10, player_y_offset-3, player_z_offset+1);

Renderable * head =NULL;

void RenderScene(HWND hwnd, HDC hdc) {
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
    HGDIOBJ hOldBitmap = SelectObject(hdcMem, hbmMem);
    //drawRenderables(hdc,&head);
    Gdiplus::Graphics graphics(hdcMem);
    graphics.Clear(Gdiplus::Color(100, 0, 0, 0));


    //DrawModel3D(hdcMem, &player_model);
    DrawModel3D(hdcMem, &pyr);
    drawRenderables(hdcMem,&head);
    //drawEnv(hdcMem, &env);

    BitBlt(hdc, 0, 0, WIDTH, HEIGHT, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_KEYDOWN:
            HandleInput(wParam, true);
            if (update) {
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        case WM_KEYUP:
            HandleInput(wParam, false);
            if (update) {
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RenderScene(hwnd, hdc);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    setProjectionMatrix(FOV, 0.5f, 1000.0f);
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Renderable * newRenderable = (Renderable*)malloc(sizeof(Renderable));;
    newRenderable->model = &cube/* initialize model */;
    newRenderable->offsets[0] = 0.0f; // Set x offset
    newRenderable->offsets[1] = 0.0f; // Set y offset
    newRenderable->offsets[2] = 0.0f; // Set z offset
    newRenderable->rotations[0] = 0;  // Set x rotation
    newRenderable->rotations[1] = 0;  // Set y rotation
    newRenderable->rotations[2] = 0;  // Set z rotation
    addRenderable(&head, newRenderable);
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      "GDI+ Player Model Window", NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, "Flight Simulator 2001",
                             WS_OVERLAPPEDWINDOW, 0, 0, WIDTH, HEIGHT,
                             NULL, NULL, wc.hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    const double targetFrameTime = 1.0 / 120.0;

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - lastFrameTime;
        if (elapsed.count() < targetFrameTime) {
            Sleep((targetFrameTime - elapsed.count()) * 30);
        }
        lastFrameTime = std::chrono::high_resolution_clock::now();
        MainLoop();
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return (int)msg.wParam;
}
```


# VERSION 3.1.1
___
we now separate the program into different files to make the code more readable. 
And make finding parts easier. (cancelled)

old ver : 

``main.cpp``
```c++
#include <windows.h>
#include <gdiplus.h>
#include <cmath>
#include <vector>
#include <chrono>
#include <stdio.h>

#define TRUE 1
#define FALSE 0
#define DEBUG TRUE
#define HEIGHT 400
#define WIDTH 400
#define FOV 90
#define PI 3.14159265

int update = true;

float player_x_offset = 0.0f;
float player_y_offset = 0.0f;
float player_z_offset = 7.0f;

float camera_offset_x = 0.0f;
float camera_offset_y = 0.0f;
float camera_offset_z = 0.0f;
float camera_rotation[3] = {0., 0., 0.};

typedef struct Point {
    float pos_x, pos_y, pos_z;
} Point;

typedef struct Vertex {
    Point point;
    struct Vertex* next;
    enum { HEAD, BASIC } type;
} Vertex;

typedef struct Triangle {
    Vertex vertex[3];
} Triangle;

typedef struct Environment {
    int size;
    Triangle* triangles;
} Environment;

typedef struct Model {
    std::vector<Triangle> triangles;
} Model;

typedef struct Renderable {
    Model* model;
    float offsets[3]; // x, y, z position
    int rotations[3]; // rotations around x, y, z
    struct Renderable* next;
} Renderable;

Renderable* head = NULL;

void setupEnvironment(Environment* env, int size_) {
    env->size = size_;
    env->triangles = (Triangle*)malloc(size_ * sizeof(Triangle));
    if (env->triangles == NULL) {
        fprintf(stderr, "Erreur d'allocation de mémoire\n");
        exit(1);
    }
}

void addTriangle(Environment* env, Vertex* v1, Vertex* v2, Vertex* v3) {
    env->size += 1;
    env->triangles = (Triangle*)realloc(env->triangles, env->size * sizeof(Triangle));
    if (env->triangles == NULL) {
        fprintf(stderr, "Erreur de réallocation de mémoire\n");
        exit(1);
    }
    env->triangles[env->size - 1] = (Triangle){ *v1, *v2, *v3 };
}

Triangle makeTriangle(Point point[3]) {
    Vertex vertex[3];
    Triangle triangle;

    vertex[0].point = point[0];
    vertex[1].point = point[1];
    vertex[2].point = point[2];

    for (int i = 0; i < 3; i++) {
        vertex[i].next = &vertex[(i + 1) % 3];
        triangle.vertex[i] = vertex[i];
    }
    return triangle;
}

void forwardMove(float d) {
    float pi = PI;
    float forward_z = d * cos(pi * camera_rotation[1]) * cos(pi * camera_rotation[0]);
    float forward_y = d * sin(pi * camera_rotation[0]);
    float forward_x = d * sin(pi * camera_rotation[1]) * cos(pi * camera_rotation[0]);

    camera_offset_x += forward_x;
    camera_offset_y += forward_y;
    camera_offset_z += forward_z;
}

void DrawTriangle(HDC hdc, Triangle* triangle) {
    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 255));
    for (int i = 0; i < 3; i++) {
        Gdiplus::PointF pt1(
            triangle->vertex[i].point.pos_x,
            triangle->vertex[i].point.pos_y);
        Gdiplus::PointF pt2(
            triangle->vertex[(i + 1) % 3].point.pos_x,
            triangle->vertex[(i + 1) % 3].point.pos_y);
        graphics.DrawLine(&pen, pt1, pt2);
    }
}

float ProjectionMatrix[4][4] = {0};

void setProjectionMatrix(const float& angleOfView, const float& near_, const float& far_) {
    float scale = 1 / tan(angleOfView * 0.5 * M_PI / 180);
    ProjectionMatrix[0][0] = scale;
    ProjectionMatrix[1][1] = scale;
    ProjectionMatrix[2][2] = -far_ / (far_ - near_);
    ProjectionMatrix[3][2] = -far_ * near_ / (far_ - near_);
    ProjectionMatrix[2][3] = -1;
    ProjectionMatrix[3][3] = 0;
    if (DEBUG) {
        for (int i = 0; i < 4; i++) {
            printf("\n");
            for (int j = 0; j < 4; j++) {
                printf("%.2f ", ProjectionMatrix[i][j]);
            }
        }
    }
}

typedef struct ScreenPoint {
    float x, y;
} ScreenPoint;

ScreenPoint ProjectPoint(const Point& point) {
    float x = point.pos_x * ProjectionMatrix[0][0];
    float y = point.pos_y * ProjectionMatrix[1][1];
    float z = point.pos_z * ProjectionMatrix[2][2];
    float w = point.pos_z * ProjectionMatrix[2][3];
    if (w == 0) w = 1; // Avoid division by zero
    x /= w;
    y /= w;
    ScreenPoint screenPoint;
    screenPoint.x = x * WIDTH / 2 + WIDTH / 2;
    screenPoint.y = -y * HEIGHT / 2 + HEIGHT / 2;
    return screenPoint;
}

Point rotatePoint(const Point& point) {
    float pi = PI;
    Point rotatedPoint = point;
    float rp[3] = {
        point.pos_x + camera_offset_x,
        (point.pos_y + camera_offset_y) * cos(pi * camera_rotation[0]) - (point.pos_z + camera_offset_z) * sin(pi * camera_rotation[0]),
        (point.pos_y + camera_offset_y) * sin(camera_rotation[0] * pi) + (point.pos_z + camera_offset_z) * cos(camera_rotation[0] * pi)
    };
    float rp_[3] = {
        rp[0] * cos(camera_rotation[2] * pi) - rp[1] * sin(camera_rotation[2] * pi),
        rp[0] * sin(camera_rotation[2] * pi) + rp[1] * cos(camera_rotation[2] * pi),
        rp[2],
    };
    float rp__[3] = {
        rp_[0] * cos(camera_rotation[1] * pi) - rp_[2] * sin(camera_rotation[1] * pi),
        rp_[1],
        rp_[0] * sin(camera_rotation[1] * pi) + rp_[2] * cos(camera_rotation[1] * pi)
    };
    rotatedPoint.pos_x = rp__[0];
    rotatedPoint.pos_y = rp__[1];
    rotatedPoint.pos_z = rp__[2];
    return rotatedPoint;
}

Point rotatePointAO(const Point& point, float* off) {
    float pi = PI;
    Point rotatedPoint = point;
    float rp[3] = {
        point.pos_x + camera_offset_x + off[0],
        (point.pos_y + camera_offset_y + off[1]) * cos(pi * camera_rotation[0]) - (point.pos_z + camera_offset_z + off[2]) * sin(pi * camera_rotation[0]),
        (point.pos_y + camera_offset_y + off[1]) * sin(camera_rotation[0] * pi) + (point.pos_z + camera_offset_z + off[2]) * cos(camera_rotation[0] * pi)
    };
    float rp_[3] = {
        rp[0] * cos(camera_rotation[2] * pi) - rp[1] * sin(camera_rotation[2] * pi),
        rp[0] * sin(camera_rotation[2] * pi) + rp[1] * cos(camera_rotation[2] * pi),
        rp[2],
    };
    float rp__[3] = {
        rp_[0] * cos(camera_rotation[1] * pi) - rp_[2] * sin(camera_rotation[1] * pi),
        rp_[1],
        rp_[0] * sin(camera_rotation[1] * pi) + rp_[2] * cos(camera_rotation[1] * pi)
    };
    rotatedPoint.pos_x = rp__[0];
    rotatedPoint.pos_y = rp__[1];
    rotatedPoint.pos_z = rp__[2];
    return rotatedPoint;
}

bool IsTriangleVisible(const Triangle& triangle) {
    return rotatePoint(triangle.vertex[0].point).pos_z <= 0 ||
           rotatePoint(triangle.vertex[1].point).pos_z <= 0 ||
           rotatePoint(triangle.vertex[2].point).pos_z <= 0;
}

void DrawTriangle3D(HDC hdc, Triangle* triangle) {
    if (IsTriangleVisible(*triangle)) {
        return;
    }

    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(100, 70, 120, 174));

    ScreenPoint points_s[3] = {
        ProjectPoint(rotatePoint(triangle->vertex[0].point)),
        ProjectPoint(rotatePoint(triangle->vertex[1].point)),
        ProjectPoint(rotatePoint(triangle->vertex[2].point))
    };

    Gdiplus::PointF points[] = {
        Gdiplus::PointF(points_s[0].x, points_s[0].y),
        Gdiplus::PointF(points_s[1].x, points_s[1].y),
        Gdiplus::PointF(points_s[2].x, points_s[2].y)
    };

    graphics.DrawLines(&pen, points, 3);
}

void DrawTriangle3DWithOffset(HDC hdc, Triangle* triangle, float* off) {
    if (IsTriangleVisible(*triangle)) {
        return;
    }

    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(100, 70, 120, 174));

    ScreenPoint points_s[3] = {
        ProjectPoint(rotatePointAO(triangle->vertex[0].point, off)),
        ProjectPoint(rotatePointAO(triangle->vertex[1].point, off)),
        ProjectPoint(rotatePointAO(triangle->vertex[2].point, off))
    };

    Gdiplus::PointF points[] = {
        Gdiplus::PointF(points_s[0].x, points_s[0].y),
        Gdiplus::PointF(points_s[1].x, points_s[1].y),
        Gdiplus::PointF(points_s[2].x, points_s[2].y)
    };

    graphics.DrawLines(&pen, points, 3);
}

Model makeCube(float size, float x_offset, float y_offset, float z_offset) {
    Model cube;

    Point vertices[8] = {
        { -size + x_offset, -size + y_offset, -size + z_offset },
        { size + x_offset, -size + y_offset, -size + z_offset },
        { size + x_offset, size + y_offset, -size + z_offset },
        { -size + x_offset, size + y_offset, -size + z_offset },
        { -size + x_offset, -size + y_offset, size + z_offset },
        { size + x_offset, -size + y_offset, size + z_offset },
        { size + x_offset, size + y_offset, size + z_offset },
        { -size + x_offset, size + y_offset, size + z_offset }
    };

    int indices[12][3] = {
        {0, 1, 2}, {0, 2, 3}, // Front face
        {4, 5, 6}, {4, 6, 7}, // Back face
        {0, 1, 5}, {0, 5, 4}, // Bottom face
        {3, 2, 6}, {3, 6, 7}, // Top face
        {0, 3, 7}, {0, 7, 4}, // Left face
        {1, 2, 6}, {1, 6, 5}  // Right face
    };

    for (int i = 0; i < 12; i++) {
        Point triangle_vertices[3] = {
            vertices[indices[i][0]],
            vertices[indices[i][1]],
            vertices[indices[i][2]]
        };
        cube.triangles.push_back(makeTriangle(triangle_vertices));
    }

    return cube;
}

void DrawModel3D(HDC hdc, Model* model) {
    for (Triangle& triangle : model->triangles) {
        DrawTriangle3D(hdc, &triangle);
    }
}

void DrawModelWithOffsetIn3d(HDC hdc, Model* model, float* off) {
    for (Triangle& triangle : model->triangles) {
        DrawTriangle3DWithOffset(hdc, &triangle, off);
    }
}

void drawRenderable(HDC hdc, Renderable* renderable) {
    DrawModelWithOffsetIn3d(hdc, renderable->model, renderable->offsets);
}

void addRenderable(Renderable** head, Renderable* newRenderable) {
    if (*head == NULL) {
        *head = newRenderable;
    } else {
        Renderable* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newRenderable;
    }
    newRenderable->next = NULL;
}

void drawRenderables(HDC hdc, Renderable** head) {
    Renderable* current = *head;
    while (current != NULL) {
        drawRenderable(hdc, current);
        current = current->next;
    }
}

Model createPyramid(float size, float x_offset, float y_offset, float z_offset) {
    Model pyramid;

    Point vertices[5] = {
        { -size + x_offset, -size + y_offset, -size + z_offset }, // Base - bas gauche
        { size + x_offset, -size + y_offset, -size + z_offset },  // Base - bas droite
        { size + x_offset, -size + y_offset, size + z_offset },   // Base - haut droite
        { -size + x_offset, -size + y_offset, size + z_offset },  // Base - haut gauche
        { x_offset, size + y_offset, z_offset } // Sommet
    };

    int indices[6][3] = {
        {0, 1, 2}, {0, 2, 3}, // Base
        {0, 1, 4}, // Face avant
        {1, 2, 4}, // Face droite
        {2, 3, 4}, // Face arrière
        {3, 0, 4}  // Face gauche
    };

    for (int _ = 0; _ < 6; _++) {
        Point triangle_vertices[3] = {
            vertices[indices[_][0]],
            vertices[indices[_][1]],
            vertices[indices[_][2]]
        };
        pyramid.triangles.push_back(makeTriangle(triangle_vertices));
    }

    return pyramid;
}

#include <stdbool.h>

bool keys[256] = { false };

void HandleInput(WPARAM wParam, bool isPressed) {
    keys[wParam] = isPressed;
}

bool helpSpaceKey = true;
Model cube = makeCube(1.0f, 0, 0, 0);
Model pyr = createPyramid(2.0f, player_x_offset + 10, player_y_offset - 3, player_z_offset + 1);

void UpdateMovement() {
    if (keys[VK_UP]) {
        forwardMove(-0.1f);
        update = true;
    }
    if (keys[VK_DOWN]) {
        forwardMove(0.1f);
        update = true;
    }
    if (keys[VK_LEFT]) {
        camera_rotation[1] += 0.01f;
        update = true;
    }
    if (keys[VK_RIGHT]) {
        camera_rotation[1] -= 0.01f;
        update = true;
    }
    if (keys['Q']) {
        camera_rotation[1] -= 0.05f;
        update = true;
    }
    if (keys['D']) {
        camera_rotation[1] += 0.05f;
        update = true;
    }
    if (keys['S']) {
        camera_rotation[0] -= 0.05f;
        update = true;
    }
    if (keys['Z']) {
        camera_rotation[0] += 0.05f;
        update = true;
    }
    if (keys[VK_SPACE]) {
        if (helpSpaceKey) {
            Renderable* newRenderable = (Renderable*)malloc(sizeof(Renderable));
            newRenderable->model = &cube;
            newRenderable->offsets[0] = -camera_offset_x; // Set x offset
            newRenderable->offsets[1] = camera_offset_y; // Set y offset
            newRenderable->offsets[2] = -camera_offset_z; // Set z offset
            newRenderable->rotations[0] = 0;  // Set x rotation
            newRenderable->rotations[1] = 0;  // Set y rotation
            newRenderable->rotations[2] = 0;  // Set z rotation
            addRenderable(&head, newRenderable);
            helpSpaceKey = false;
        }
    } else {
        helpSpaceKey = true;
    }
}

void MainLoop() {
    UpdateMovement();
}

void RenderScene(HWND hwnd, HDC hdc) {
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
    HGDIOBJ hOldBitmap = SelectObject(hdcMem, hbmMem);

    Gdiplus::Graphics graphics(hdcMem);
    graphics.Clear(Gdiplus::Color(100, 0, 0, 0));

    DrawModel3D(hdcMem, &pyr);
    drawRenderables(hdcMem, &head);

    BitBlt(hdc, 0, 0, WIDTH, HEIGHT, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_KEYDOWN:
            HandleInput(wParam, true);
            if (update) {
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        case WM_KEYUP:
            HandleInput(wParam, false);
            if (update) {
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RenderScene(hwnd, hdc);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    setProjectionMatrix(FOV, 0.5f, 1000.0f);
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      "GDI+ Player Model Window", NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, "Flight Simulator 2001",
                             WS_OVERLAPPEDWINDOW, 0, 0, WIDTH, HEIGHT,
                             NULL, NULL, wc.hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    const double targetFrameTime = 1.0 / 120.0;

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - lastFrameTime;
        if (elapsed.count() < targetFrameTime) {
            Sleep((targetFrameTime - elapsed.count()) * 30);
        }
        lastFrameTime = std::chrono::high_resolution_clock::now();
        MainLoop();
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return (int)msg.wParam;
}
```
## Patch 3.2.1
___
``main.cpp``
```c++
#include <windows.h>
#include <gdiplus.h>
#include <cmath>
#include <vector>
#include <chrono>
#include <stdio.h>

#define TRUE 1
#define FALSE 0
#define DEBUG TRUE
#define HEIGHT 400
#define WIDTH 400
#define FOV 90
#define PI 3.14159265

int update = true;

float player_x_offset = 0.0f;
float player_y_offset = 0.0f;
float player_z_offset = 7.0f;

float camera_offset_x = 0.0f;
float camera_offset_y = 0.0f;
float camera_offset_z = 0.0f;
float camera_rotation[3] = {0., 0., 0.};

typedef struct Point {
    float pos_x, pos_y, pos_z;
} Point;

typedef struct Vertex {
    Point point;
    struct Vertex* next;
    enum { HEAD, BASIC } type;
} Vertex;

typedef struct Triangle {
    Vertex vertex[3];
} Triangle;

typedef struct Environment {
    int size;
    Triangle* triangles;
} Environment;

typedef struct Model {
    std::vector<Triangle> triangles;
} Model;

typedef struct Renderable {
    Model* model;
    float offsets[3]; // x, y, z position
    int rotations[3]; // rotations around x, y, z
    struct Renderable* next;
} Renderable;

Renderable* head = NULL;

void setupEnvironment(Environment* env, int size_) {
    env->size = size_;
    env->triangles = (Triangle*)malloc(size_ * sizeof(Triangle));
    if (env->triangles == NULL) {
        fprintf(stderr, "Erreur d'allocation de mémoire\n");
        exit(1);
    }
}

void addTriangle(Environment* env, Vertex* v1, Vertex* v2, Vertex* v3) {
    env->size += 1;
    env->triangles = (Triangle*)realloc(env->triangles, env->size * sizeof(Triangle));
    if (env->triangles == NULL) {
        fprintf(stderr, "Erreur de réallocation de mémoire\n");
        exit(1);
    }
    env->triangles[env->size - 1] = (Triangle){ *v1, *v2, *v3 };
}

Triangle makeTriangle(Point point[3]) {
    Vertex vertex[3];
    Triangle triangle;

    vertex[0].point = point[0];
    vertex[1].point = point[1];
    vertex[2].point = point[2];

    for (int i = 0; i < 3; i++) {
        vertex[i].next = &vertex[(i + 1) % 3];
        triangle.vertex[i] = vertex[i];
    }
    return triangle;
}

void forwardMove(float d) {
    float pi = PI;
    float forward_z = d * cos(pi * camera_rotation[1]) * cos(pi * camera_rotation[0]);
    float forward_y = d * sin(pi * camera_rotation[0]);
    float forward_x = d * sin(pi * camera_rotation[1]) * cos(pi * camera_rotation[0]);

    camera_offset_x += forward_x;
    camera_offset_y += forward_y;
    camera_offset_z += forward_z;
}

void DrawTriangle(HDC hdc, Triangle* triangle) {
    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 255));
    for (int i = 0; i < 3; i++) {
        Gdiplus::PointF pt1(
            triangle->vertex[i].point.pos_x,
            triangle->vertex[i].point.pos_y);
        Gdiplus::PointF pt2(
            triangle->vertex[(i + 1) % 3].point.pos_x,
            triangle->vertex[(i + 1) % 3].point.pos_y);
        graphics.DrawLine(&pen, pt1, pt2);
    }
}

float ProjectionMatrix[4][4] = {0};

void setProjectionMatrix(const float& angleOfView, const float& near_, const float& far_) {
    float scale = 1 / tan(angleOfView * 0.5 * M_PI / 180);
    ProjectionMatrix[0][0] = scale;
    ProjectionMatrix[1][1] = scale;
    ProjectionMatrix[2][2] = -far_ / (far_ - near_);
    ProjectionMatrix[3][2] = -far_ * near_ / (far_ - near_);
    ProjectionMatrix[2][3] = -1;
    ProjectionMatrix[3][3] = 0;
    if (DEBUG) {
        for (int i = 0; i < 4; i++) {
            printf("\n");
            for (int j = 0; j < 4; j++) {
                printf("%.2f ", ProjectionMatrix[i][j]);
            }
        }
    }
}

typedef struct ScreenPoint {
    float x, y;
} ScreenPoint;

ScreenPoint ProjectPoint(const Point& point) {
    float x = point.pos_x * ProjectionMatrix[0][0];
    float y = point.pos_y * ProjectionMatrix[1][1];
    float z = point.pos_z * ProjectionMatrix[2][2];
    float w = point.pos_z * ProjectionMatrix[2][3];
    if (w == 0) w = 1; // Avoid division by zero
    x /= w;
    y /= w;
    ScreenPoint screenPoint;
    screenPoint.x = x * WIDTH / 2 + WIDTH / 2;
    screenPoint.y = -y * HEIGHT / 2 + HEIGHT / 2;
    return screenPoint;
}

Point rotatePoint(const Point& point) {
    float pi = PI;
    Point rotatedPoint = point;
    float rp[3] = {
        point.pos_x + camera_offset_x,
        (point.pos_y + camera_offset_y) * cos(pi * camera_rotation[0]) - (point.pos_z + camera_offset_z) * sin(pi * camera_rotation[0]),
        (point.pos_y + camera_offset_y) * sin(camera_rotation[0] * pi) + (point.pos_z + camera_offset_z) * cos(camera_rotation[0] * pi)
    };
    float rp_[3] = {
        rp[0] * cos(camera_rotation[2] * pi) - rp[1] * sin(camera_rotation[2] * pi),
        rp[0] * sin(camera_rotation[2] * pi) + rp[1] * cos(camera_rotation[2] * pi),
        rp[2],
    };
    float rp__[3] = {
        rp_[0] * cos(camera_rotation[1] * pi) - rp_[2] * sin(camera_rotation[1] * pi),
        rp_[1],
        rp_[0] * sin(camera_rotation[1] * pi) + rp_[2] * cos(camera_rotation[1] * pi)
    };
    rotatedPoint.pos_x = rp__[0];
    rotatedPoint.pos_y = rp__[1];
    rotatedPoint.pos_z = rp__[2];
    return rotatedPoint;
}

Point rotatePointAO(const Point& point, float* off) {
    float pi = PI;
    Point rotatedPoint = point;
    float rp[3] = {
        point.pos_x + camera_offset_x + off[0],
        (point.pos_y + camera_offset_y + off[1]) * cos(pi * camera_rotation[0]) - (point.pos_z + camera_offset_z - off[2]) * sin(pi * camera_rotation[0]),
        (point.pos_y + camera_offset_y + off[1]) * sin(camera_rotation[0] * pi) + (point.pos_z + camera_offset_z - off[2]) * cos(camera_rotation[0] * pi)
    };
    float rp_[3] = {
        rp[0] * cos(camera_rotation[2] * pi) - rp[1] * sin(camera_rotation[2] * pi),
        rp[0] * sin(camera_rotation[2] * pi) + rp[1] * cos(camera_rotation[2] * pi),
        rp[2],
    };
    float rp__[3] = {
        rp_[0] * cos(camera_rotation[1] * pi) - rp_[2] * sin(camera_rotation[1] * pi),
        rp_[1],
        rp_[0] * sin(camera_rotation[1] * pi) + rp_[2] * cos(camera_rotation[1] * pi)
    };
    rotatedPoint.pos_x = rp__[0];
    rotatedPoint.pos_y = rp__[1];
    rotatedPoint.pos_z = rp__[2];
    return rotatedPoint;
}

bool IsTriangleVisible(const Triangle& triangle) {
    return rotatePoint(triangle.vertex[0].point).pos_z <= 0 ||
           rotatePoint(triangle.vertex[1].point).pos_z <= 0 ||
           rotatePoint(triangle.vertex[2].point).pos_z <= 0;
}

bool IsTriangleVisibleWithOffset(const Triangle& triangle,float*off){
    return rotatePointAO(triangle.vertex[0].point,off).pos_z <= 0 ||
       rotatePointAO(triangle.vertex[1].point,off).pos_z <= 0 ||
       rotatePointAO(triangle.vertex[2].point,off).pos_z <= 0;
}

void DrawTriangle3D(HDC hdc, Triangle* triangle) {
    if (IsTriangleVisible(*triangle)) {
        return;
    }

    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(100, 70, 120, 174));

    ScreenPoint points_s[3] = {
        ProjectPoint(rotatePoint(triangle->vertex[0].point)),
        ProjectPoint(rotatePoint(triangle->vertex[1].point)),
        ProjectPoint(rotatePoint(triangle->vertex[2].point))
    };

    Gdiplus::PointF points[] = {
        Gdiplus::PointF(points_s[0].x, points_s[0].y),
        Gdiplus::PointF(points_s[1].x, points_s[1].y),
        Gdiplus::PointF(points_s[2].x, points_s[2].y)
    };

    graphics.DrawLines(&pen, points, 3);
}

void DrawTriangle3DWithOffset(HDC hdc, Triangle* triangle, float* off) {
    if (IsTriangleVisibleWithOffset(*triangle,off)) {
        return;
    }

    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(100, 70, 120, 174));

    ScreenPoint points_s[3] = {
        ProjectPoint(rotatePointAO(triangle->vertex[0].point, off)),
        ProjectPoint(rotatePointAO(triangle->vertex[1].point, off)),
        ProjectPoint(rotatePointAO(triangle->vertex[2].point, off))
    };

    Gdiplus::PointF points[] = {
        Gdiplus::PointF(points_s[0].x, points_s[0].y),
        Gdiplus::PointF(points_s[1].x, points_s[1].y),
        Gdiplus::PointF(points_s[2].x, points_s[2].y)
    };

    graphics.DrawLines(&pen, points, 3);
}

Model makeCube(float size, float x_offset, float y_offset, float z_offset) {
    Model cube;

    Point vertices[8] = {
        { -size + x_offset, -size + y_offset, -size + z_offset },
        { size + x_offset, -size + y_offset, -size + z_offset },
        { size + x_offset, size + y_offset, -size + z_offset },
        { -size + x_offset, size + y_offset, -size + z_offset },
        { -size + x_offset, -size + y_offset, size + z_offset },
        { size + x_offset, -size + y_offset, size + z_offset },
        { size + x_offset, size + y_offset, size + z_offset },
        { -size + x_offset, size + y_offset, size + z_offset }
    };

    int indices[12][3] = {
        {0, 1, 2}, {0, 2, 3}, // Front face
        {4, 5, 6}, {4, 6, 7}, // Back face
        {0, 1, 5}, {0, 5, 4}, // Bottom face
        {3, 2, 6}, {3, 6, 7}, // Top face
        {0, 3, 7}, {0, 7, 4}, // Left face
        {1, 2, 6}, {1, 6, 5}  // Right face
    };

    for (int i = 0; i < 12; i++) {
        Point triangle_vertices[3] = {
            vertices[indices[i][0]],
            vertices[indices[i][1]],
            vertices[indices[i][2]]
        };
        cube.triangles.push_back(makeTriangle(triangle_vertices));
    }

    return cube;
}

void DrawModel3D(HDC hdc, Model* model) {
    for (Triangle& triangle : model->triangles) {
        DrawTriangle3D(hdc, &triangle);
    }
}

void DrawModelWithOffsetIn3d(HDC hdc, Model* model, float* off) {
    for (Triangle& triangle : model->triangles) {
        DrawTriangle3DWithOffset(hdc, &triangle, off);
    }
}

void drawRenderable(HDC hdc, Renderable* renderable) {
    DrawModelWithOffsetIn3d(hdc, renderable->model, renderable->offsets);
}

void addRenderable(Renderable** head, Renderable* newRenderable) {
    if (*head == NULL) {
        *head = newRenderable;
    } else {
        Renderable* current = *head;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = newRenderable;
    }
    newRenderable->next = NULL;
}

void drawRenderables(HDC hdc, Renderable** head) {
    Renderable* current = *head;
    while (current != NULL) {
        drawRenderable(hdc, current);
        current = current->next;
    }
}

Model createPyramid(float size, float x_offset, float y_offset, float z_offset) {
    Model pyramid;

    Point vertices[5] = {
        { -size + x_offset, -size + y_offset, -size + z_offset }, // Base - bas gauche
        { size + x_offset, -size + y_offset, -size + z_offset },  // Base - bas droite
        { size + x_offset, -size + y_offset, size + z_offset },   // Base - haut droite
        { -size + x_offset, -size + y_offset, size + z_offset },  // Base - haut gauche
        { x_offset, size + y_offset, z_offset } // Sommet
    };

    int indices[6][3] = {
        {0, 1, 2}, {0, 2, 3}, // Base
        {0, 1, 4}, // Face avant
        {1, 2, 4}, // Face droite
        {2, 3, 4}, // Face arrière
        {3, 0, 4}  // Face gauche
    };

    for (int _ = 0; _ < 6; _++) {
        Point triangle_vertices[3] = {
            vertices[indices[_][0]],
            vertices[indices[_][1]],
            vertices[indices[_][2]]
        };
        pyramid.triangles.push_back(makeTriangle(triangle_vertices));
    }

    return pyramid;
}

#include <stdbool.h>

bool keys[256] = { false };

void HandleInput(WPARAM wParam, bool isPressed) {
    keys[wParam] = isPressed;
}

bool helpSpaceKey = true;
Model cube = makeCube(1.0f, 0, 0, 0);
Model pyr = createPyramid(2.0f, player_x_offset + 10, player_y_offset - 3, player_z_offset + 1);

void UpdateMovement() {
    if (keys[VK_UP]) {
        forwardMove(-0.1f);
        update = true;
    }
    if (keys[VK_DOWN]) {
        forwardMove(0.1f);
        update = true;
    }
    if (keys[VK_LEFT]) {
        camera_rotation[1] += 0.01f;
        update = true;
    }
    if (keys[VK_RIGHT]) {
        camera_rotation[1] -= 0.01f;
        update = true;
    }
    if (keys['Q']) {
        camera_rotation[1] -= 0.05f;
        update = true;
    }
    if (keys['D']) {
        camera_rotation[1] += 0.05f;
        update = true;
    }
    if (keys['S']) {
        camera_rotation[0] -= 0.05f;
        update = true;
    }
    if (keys['Z']) {
        camera_rotation[0] += 0.05f;
        update = true;
    }
    if (keys[VK_SPACE]) {
        if (helpSpaceKey) {
            Renderable* newRenderable = (Renderable*)malloc(sizeof(Renderable));
            newRenderable->model = &cube;
            newRenderable->offsets[0] = -camera_offset_x; // Set x offset
            newRenderable->offsets[1] = -camera_offset_y; // Set y offset
            newRenderable->offsets[2] = camera_offset_z; // Set z offset
            newRenderable->rotations[0] = 0;  // Set x rotation
            newRenderable->rotations[1] = 0;  // Set y rotation
            newRenderable->rotations[2] = 0;  // Set z rotation
            addRenderable(&head, newRenderable);
            helpSpaceKey = false;
        }
    } else {
        helpSpaceKey = true;
    }
}

void MainLoop() {
    UpdateMovement();
}

void RenderScene(HWND hwnd, HDC hdc) {
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
    HGDIOBJ hOldBitmap = SelectObject(hdcMem, hbmMem);

    Gdiplus::Graphics graphics(hdcMem);
    graphics.Clear(Gdiplus::Color(100, 0, 0, 0));

    DrawModel3D(hdcMem, &pyr);
    drawRenderables(hdcMem, &head);

    BitBlt(hdc, 0, 0, WIDTH, HEIGHT, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_KEYDOWN:
            HandleInput(wParam, true);
            if (update) {
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        case WM_KEYUP:
            HandleInput(wParam, false);
            if (update) {
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RenderScene(hwnd, hdc);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void freeRenderables(Renderable** head) {
    Renderable* current = *head;
    while (current != NULL) {
        Renderable* next = current->next;
        free(current);
        current = next;
    }
    *head = NULL;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    setProjectionMatrix(FOV, 0.01f, 1000.0f);
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      "GDI+ Player Model Window", NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, "Flight Simulator 2001",
                             WS_OVERLAPPEDWINDOW, 0, 0, WIDTH, HEIGHT,
                             NULL, NULL, wc.hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    const double targetFrameTime = 1.0 / 120.0;

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - lastFrameTime;
        if (elapsed.count() < targetFrameTime) {
            Sleep((targetFrameTime - elapsed.count()) * 30);
        }
        lastFrameTime = std::chrono::high_resolution_clock::now();
        MainLoop();
    }

    freeRenderables(&head);
    Gdiplus::GdiplusShutdown(gdiplusToken);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return (int)msg.wParam;
}
```
## Version 3.2.2
___
- slight performance improvement. By improving existing systems.
- officially implementing a world generation for testing.
- still has some issues with pitch and yaw rotation.

``main.cpp``
```c++
#include <windows.h>
#include <gdiplus.h>
#include <cmath>
#include <vector>
#include <chrono>
#include <stdio.h>
#include <execution>
#include <algorithm>
#include "donjon.h"

#define TRUE 1
#define FALSE 0
#define DEBUG TRUE
#define HEIGHT 720
#define WIDTH 720
#define FOV 45
constexpr float PI = 3.14159265f;

int update = true;

float player_x_offset = 0.0f;
float player_y_offset = 0.0f;
float player_z_offset = 7.0f;

float camera_offset_x = 0.0f;
float camera_offset_y = 0.0f;
float camera_offset_z = 0.0f;
float camera_rotation[3] = {0.f, 0.f, 0.f};

struct Point {
    float pos_x, pos_y, pos_z;
};

struct Vertex {
    Point point;
    Vertex* next;
    enum { HEAD, BASIC } type;
};

struct Triangle {
    Vertex vertex[3];
    Point*dvec;
};

struct Environment {
    std::vector<Triangle> triangles;
};

struct Model {
    std::vector<Triangle> triangles;
};

struct Renderable {
    Model* model;
    float offsets[3]; // x, y, z position
    int rotations[3]; // rotations around x, y, z
    Renderable* next;
};

Renderable* head = nullptr;

void setupEnvironment(Environment* env, int size_) {
    env->triangles.reserve(size_);
}

void addTriangle(Environment* env, Vertex* v1, Vertex* v2, Vertex* v3) {
    env->triangles.emplace_back(Triangle{ *v1, *v2, *v3 });
}

Triangle makeTriangle(Point point[3]) {
    Vertex vertex[3];
    Triangle triangle;

    for (int i = 0; i < 3; i++) {
        vertex[i].point = point[i];
        vertex[i].next = &vertex[(i + 1) % 3];
        triangle.vertex[i] = vertex[i];
    }
    return triangle;
}

void forwardMove(float d) {
    float forward_z = d * cos(PI * camera_rotation[1]) * cos(PI * camera_rotation[0]);
    float forward_y = d * sin(PI * camera_rotation[0]);
    float forward_x = d * sin(PI * camera_rotation[1]) * cos(PI * camera_rotation[0]);

    camera_offset_x += forward_x;
    camera_offset_y += forward_y;
    camera_offset_z += forward_z;
}

void DrawTriangle(HDC hdc, Triangle* triangle) {
    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 255));
    for (int i = 0; i < 3; i++) {
        Gdiplus::PointF pt1(triangle->vertex[i].point.pos_x, triangle->vertex[i].point.pos_y);
        Gdiplus::PointF pt2(triangle->vertex[(i + 1) % 3].point.pos_x, triangle->vertex[(i + 1) % 3].point.pos_y);
        graphics.DrawLine(&pen, pt1, pt2);
    }
}

float ProjectionMatrix[4][4] = {0};

void setProjectionMatrix(const float& angleOfView, const float& near_, const float& far_) {
    float scale = 1 / tan(angleOfView * 0.5f * PI / 180.0f);
    ProjectionMatrix[0][0] = scale;
    ProjectionMatrix[1][1] = scale;
    ProjectionMatrix[2][2] = -far_ / (far_ - near_);
    ProjectionMatrix[3][2] = -far_ * near_ / (far_ - near_);
    ProjectionMatrix[2][3] = -1;
    ProjectionMatrix[3][3] = 0;
    if (DEBUG) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                printf("%.2f ", ProjectionMatrix[i][j]);
            }
            printf("\n");
        }
    }
}

struct ScreenPoint {
    float x, y;
};

ScreenPoint ProjectPoint(const Point& point) {
    float x = point.pos_x * ProjectionMatrix[0][0];
    float y = point.pos_y * ProjectionMatrix[1][1];
    float z = point.pos_z * ProjectionMatrix[2][2];
    float w = point.pos_z * ProjectionMatrix[2][3];
    if (w == 0) w = 1; // Avoid division by zero
    x /= w;
    y /= w;
    ScreenPoint screenPoint;
    screenPoint.x = x * WIDTH / 2 + WIDTH / 2;
    screenPoint.y = -y * HEIGHT / 2 + HEIGHT / 2;
    return screenPoint;
}

Point rotatePoint(const Point& point) {
    Point rotatedPoint = point;
    float rp[3] = {
        point.pos_x + camera_offset_x,
        (point.pos_y + camera_offset_y) * cos(PI * camera_rotation[0]) - (point.pos_z + camera_offset_z) * sin(PI * camera_rotation[0]),
        (point.pos_y + camera_offset_y) * sin(PI * camera_rotation[0]) + (point.pos_z + camera_offset_z) * cos(PI * camera_rotation[0])
    };
    float rp_[3] = {
        rp[0] * cos(PI * camera_rotation[2]) - rp[1] * sin(PI * camera_rotation[2]),
        rp[0] * sin(PI * camera_rotation[2]) + rp[1] * cos(PI * camera_rotation[2]),
        rp[2],
    };
    float rp__[3] = {
        rp_[0] * cos(PI * camera_rotation[1]) - rp_[2] * sin(PI * camera_rotation[1]),
        rp_[1],
        rp_[0] * sin(PI * camera_rotation[1]) + rp_[2] * cos(PI * camera_rotation[1])
    };
    rotatedPoint.pos_x = rp__[0];
    rotatedPoint.pos_y = rp__[1];
    rotatedPoint.pos_z = rp__[2];
    return rotatedPoint;
}
Point rotateVec(const Point& point) {
    Point rotatedPoint = point;
    float rp[3] = {
        point.pos_x,
        (point.pos_y) * cos(PI * camera_rotation[0]) - (point.pos_z) * sin(PI * camera_rotation[0]),
        (point.pos_y) * sin(PI * camera_rotation[0]) + (point.pos_z) * cos(PI * camera_rotation[0])
    };
    float rp_[3] = {
        rp[0] * cos(PI * camera_rotation[2]) - rp[1] * sin(PI * camera_rotation[2]),
        rp[0] * sin(PI * camera_rotation[2]) + rp[1] * cos(PI * camera_rotation[2]),
        rp[2],
    };
    float rp__[3] = {
        rp_[0] * cos(PI * camera_rotation[1]) - rp_[2] * sin(PI * camera_rotation[1]),
        rp_[1],
        rp_[0] * sin(PI * camera_rotation[1]) + rp_[2] * cos(PI * camera_rotation[1])
    };
    rotatedPoint.pos_x = rp__[0];
    rotatedPoint.pos_y = rp__[1];
    rotatedPoint.pos_z = rp__[2];
    return rotatedPoint;
}

Point rotatePointAO(const Point& point, float* off) {
    Point rotatedPoint = point;
    float rp[3] = {
        point.pos_x + camera_offset_x + off[0],
        (point.pos_y + camera_offset_y + off[1]) * cos(PI * camera_rotation[0]) - (point.pos_z + camera_offset_z - off[2]) * sin(PI * camera_rotation[0]),
        (point.pos_y + camera_offset_y + off[1]) * sin(PI * camera_rotation[0]) + (point.pos_z + camera_offset_z - off[2]) * cos(PI * camera_rotation[0])
    };
    float rp_[3] = {
        rp[0] * cos(PI * camera_rotation[2]) - rp[1] * sin(PI * camera_rotation[2]),
        rp[0] * sin(PI * camera_rotation[2]) + rp[1] * cos(PI * camera_rotation[2]),
        rp[2],
    };
    float rp__[3] = {
        rp_[0] * cos(PI * camera_rotation[1]) - rp_[2] * sin(PI * camera_rotation[1]),
        rp_[1],
        rp_[0] * sin(PI * camera_rotation[1]) + rp_[2] * cos(PI * camera_rotation[1])
    };
    rotatedPoint.pos_x = rp__[0];
    rotatedPoint.pos_y = rp__[1];
    rotatedPoint.pos_z = rp__[2];
    return rotatedPoint;
}

bool IsTriangleVisible(const Triangle& triangle) {
    return
        rotatePoint(triangle.vertex[0].point).pos_z <= 0 ||
            rotatePoint(triangle.vertex[1].point).pos_z <= 0 ||
                rotatePoint(triangle.vertex[2].point).pos_z <= 0;//rotateVec(*triangle.dvec).pos_z <=0;
}

bool IsTriangleVisibleWithOffset(const Triangle& triangle, float* off,Point * a) {
    a[0] = rotatePointAO(triangle.vertex[0].point, off);
    a[1] = rotatePointAO(triangle.vertex[1].point, off);
    a[2] = rotatePointAO(triangle.vertex[2].point, off);

    if (a[0].pos_z <= 0 ||
           a[1].pos_z <= 0 ||
           a[2].pos_z <= 0) {
        return false;
    }else{return true;}
}

void DrawTriangle3D(HDC hdc, Triangle* triangle) {
    if (IsTriangleVisible(*triangle)) {
        return;
    }

    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(100, 70, 120, 174));

    ScreenPoint points_s[3] = {
        ProjectPoint(rotatePoint(triangle->vertex[0].point)),
        ProjectPoint(rotatePoint(triangle->vertex[1].point)),
        ProjectPoint(rotatePoint(triangle->vertex[2].point))
    };

    Gdiplus::PointF points[] = {
        Gdiplus::PointF(points_s[0].x, points_s[0].y),
        Gdiplus::PointF(points_s[1].x, points_s[1].y),
        Gdiplus::PointF(points_s[2].x, points_s[2].y)
    };

    graphics.DrawLines(&pen, points, 3);
}

void DrawTriangle3DWithOffset(HDC hdc, Triangle* triangle, float* off) {
    Point a[3];
    if (IsTriangleVisibleWithOffset(*triangle,off,a) == false) {return;}
    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(255, 50, 100, 100));
    Gdiplus::SolidBrush brush(Gdiplus::Color(200, 40, 90, 120));
    ScreenPoint points_s[3] = {
        ProjectPoint(a[0]),
        ProjectPoint(a[1]),
        ProjectPoint(a[2])
    };

    Gdiplus::PointF points[] = {
        Gdiplus::PointF(points_s[0].x, points_s[0].y),
        Gdiplus::PointF(points_s[1].x, points_s[1].y),
        Gdiplus::PointF(points_s[2].x, points_s[2].y)
    };
    //graphics.FillPolygon(&brush,points,3);
    graphics.DrawLines(&pen, points, 3);
}

Model makeCube(float size, float x_offset, float y_offset, float z_offset) {
    Model cube;

    Point vertices[8] = {
        { -size + x_offset, -size + y_offset, -size + z_offset },
        { size + x_offset, -size + y_offset, -size + z_offset },
        { size + x_offset, size + y_offset, -size + z_offset },
        { -size + x_offset, size + y_offset, -size + z_offset },
        { -size + x_offset, -size + y_offset, size + z_offset },
        { size + x_offset, -size + y_offset, size + z_offset },
        { size + x_offset, size + y_offset, size + z_offset },
        { -size + x_offset, size + y_offset, size + z_offset }
    };

    int indices[12][3] = {
        {0, 1, 2}, {0, 2, 3}, // Front face
        {4, 5, 6}, {4, 6, 7}, // Back face
        {0, 1, 5}, {0, 5, 4}, // Bottom face
        {3, 2, 6}, {3, 6, 7}, // Top face
        {0, 3, 7}, {0, 7, 4}, // Left face
        {1, 2, 6}, {1, 6, 5}  // Right face
    };

    for (int i = 0; i < 12; i++) {
        Point triangle_vertices[3] = {
            vertices[indices[i][0]],
            vertices[indices[i][1]],
            vertices[indices[i][2]]
        };
        cube.triangles.emplace_back(makeTriangle(triangle_vertices));
    }

    return cube;
}

void DrawModel3D(HDC hdc, Model* model) {
    for (const Triangle& triangle : model->triangles) {
        DrawTriangle3D(hdc, const_cast<Triangle*>(&triangle));
    }
}

void DrawModelWithOffsetIn3d(HDC hdc, Model* model, float* off) {
    for (const Triangle& triangle : model->triangles) {
        DrawTriangle3DWithOffset(hdc, const_cast<Triangle*>(&triangle), off);
    }
}

void drawRenderable(HDC hdc, Renderable* renderable) {
    DrawModelWithOffsetIn3d(hdc, renderable->model, renderable->offsets);
}

void addRenderable(Renderable** head, Renderable* newRenderable) {
    if (*head == nullptr) {
        *head = newRenderable;
    } else {
        Renderable* current = *head;
        while (current->next != nullptr) {
            current = current->next;
        }
        current->next = newRenderable;
    }
    newRenderable->next = nullptr;
}

void drawRenderables(HDC hdc, Renderable** head) {
    std::vector<Renderable*> renderables;
    Renderable* current = *head;
    while (current != nullptr) {
        renderables.push_back(current);
        current = current->next;
    }

    // Trier les objets par distance par rapport à la caméra
    std::sort(renderables.begin(), renderables.end(), [](Renderable* a, Renderable* b) {
        float distA = sqrt(a->offsets[0] * a->offsets[0] + a->offsets[1] * a->offsets[1] + a->offsets[2] * a->offsets[2]);
        float distB = sqrt(b->offsets[0] * b->offsets[0] + b->offsets[1] * b->offsets[1] + b->offsets[2] * b->offsets[2]);
        return distA > distB; // Dessiner du plus éloigné au plus proche
    });

    // Dessiner les objets triés
    for (Renderable* renderable : renderables) {
        drawRenderable(hdc, renderable);
    }
}

Model createPyramid(float size, float x_offset, float y_offset, float z_offset) {
    Model pyramid;

    Point vertices[5] = {
        { -size + x_offset, -size + y_offset, -size + z_offset }, // Base - bas gauche
        { size + x_offset, -size + y_offset, -size + z_offset },  // Base - bas droite
        { size + x_offset, -size + y_offset, size + z_offset },   // Base - haut droite
        { -size + x_offset, -size + y_offset, size + z_offset },  // Base - haut gauche
        { x_offset, size + y_offset, z_offset } // Sommet
    };

    int indices[6][3] = {
        {0, 1, 2}, {0, 2, 3}, // Base
        {0, 1, 4}, // Face avant
        {1, 2, 4}, // Face droite
        {2, 3, 4}, // Face arrière
        {3, 0, 4}  // Face gauche
    };

    for (int _ = 0; _ < 6; _++) {
        Point triangle_vertices[3] = {
            vertices[indices[_][0]],
            vertices[indices[_][1]],
            vertices[indices[_][2]]
        };
        pyramid.triangles.emplace_back(makeTriangle(triangle_vertices));
    }

    return pyramid;
}

#include <stdbool.h>

bool keys[256] = { false };

void HandleInput(WPARAM wParam, bool isPressed) {
    keys[wParam] = isPressed;
}

bool helpSpaceKey = true;
Model cube = makeCube(0.5f, 0, 0, 0);
Model pyr = createPyramid(0.5f, player_x_offset + 10, player_y_offset - 3, player_z_offset + 1);
Point facing ={camera_rotation[0], camera_rotation[1], camera_rotation[2]};
void UpdateMovement() {
    if (keys[VK_UP]) {
        forwardMove(-0.1f);
        update = true;
    }
    if (keys[VK_DOWN]) {
        forwardMove(0.1f);
        update = true;
    }
    if (keys[VK_LEFT]) {
        camera_rotation[1] += 0.01f;
        update = true;
    }
    if (keys[VK_RIGHT]) {
        camera_rotation[1] -= 0.01f;
        update = true;
    }
    if (keys['Q']) {
        camera_rotation[1] -= 0.05f;
        update = true;
    }
    if (keys['D']) {
        camera_rotation[1] += 0.05f;
        update = true;
    }
    if (keys['S']) {
        camera_rotation[2] +=0.03*cosf(camera_rotation[1]);
        camera_rotation[0] += 0.03*sinf(camera_rotation[1]);
        update = true;
    }
    if (keys['Z']) {
        camera_rotation[2] -=0.03*cosf(camera_rotation[1]);
        camera_rotation[0] -= 0.03*sinf(camera_rotation[1]);
        update = true;
    }
    if (keys[VK_SPACE]) {
        if (helpSpaceKey) {
            Renderable* newRenderable = new Renderable();
            newRenderable->model = &cube;
            newRenderable->offsets[0] = -camera_offset_x; // Set x offset
            newRenderable->offsets[1] = -camera_offset_y; // Set y offset
            newRenderable->offsets[2] = camera_offset_z; // Set z offset
            newRenderable->rotations[0] = 0;  // Set x rotation
            newRenderable->rotations[1] = 0;  // Set y rotation
            newRenderable->rotations[2] = 0;  // Set z rotation
            addRenderable(&head, newRenderable);
            helpSpaceKey = false;
        }
    } else {
        helpSpaceKey = true;
    }
}

void MainLoop() {
    UpdateMovement();
}

void RenderScene(HWND hwnd, HDC hdc) {
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
    HGDIOBJ hOldBitmap = SelectObject(hdcMem, hbmMem);

    Gdiplus::Graphics graphics(hdcMem);
    graphics.Clear(Gdiplus::Color(100, 0, 0, 0));

    DrawModel3D(hdcMem, &pyr);
    drawRenderables(hdcMem, &head);

    BitBlt(hdc, 0, 0, WIDTH, HEIGHT, hdcMem, 0, 0, SRCCOPY);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_KEYDOWN:
            HandleInput(wParam, true);
            if (update) {
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            break;
        case WM_KEYUP:
            HandleInput(wParam, false);
            if (update) {
                InvalidateRect(hwnd, nullptr, TRUE);
            }
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RenderScene(hwnd, hdc);
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void freeRenderables(Renderable** head) {
    Renderable* current = *head;
    while (current != nullptr) {
        Renderable* next = current->next;
        delete current;
        current = next;
    }
    *head = nullptr;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    setProjectionMatrix(FOV, 0.01f, 1000.0f);
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, nullptr);
    static char donjon[DONJON_HAUTEUR][DONJON_LARGEUR] = {0};
    generer_donjon(donjon);

    for (int y = 0; y < DONJON_HAUTEUR; y++) {
        for (int x = 0; x < DONJON_LARGEUR; x++) {
            if (donjon[y][x] == 1) {
                Renderable* newRenderable = new Renderable();
                newRenderable->model = &cube;
                newRenderable->offsets[0] = static_cast<float>(y) - 50; // Set x offset
                newRenderable->offsets[1] = 0; // Set y offset
                newRenderable->offsets[2] = static_cast<float>(x) - 50; // Set z offset
                newRenderable->rotations[0] = 0;  // Set x rotation
                newRenderable->rotations[1] = 0;  // Set y rotation
                newRenderable->rotations[2] = 0;  // Set z rotation
                addRenderable(&head, newRenderable);
            }
        }
    }

    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L,
                      GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr,
                      "GDI+ Player Model Window", nullptr };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, "Flight Simulator 2001",
                             WS_OVERLAPPEDWINDOW, 0, 0, WIDTH, HEIGHT,
                             nullptr, nullptr, wc.hInstance, nullptr);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    const double targetFrameTime = 1.0 / 120.0;

    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - lastFrameTime;
        if (elapsed.count() < targetFrameTime) {
            Sleep((targetFrameTime - elapsed.count()) * 30);
        }
        lastFrameTime = std::chrono::high_resolution_clock::now();
        MainLoop();
    }

    freeRenderables(&head);
    Gdiplus::GdiplusShutdown(gdiplusToken);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return (int)msg.wParam;
}
```
``dungeon.h``
```h
//
// Created by TogExe on 03/03/2025.
//
// donjon.h

#ifndef DONJON_H
#define DONJON_H

#define DONJON_LARGEUR 100
#define DONJON_HAUTEUR 100

typedef struct Salle{
    int x, y, largeur, hauteur;
} Salle;

Salle generer_salle();
int salles_se_chevauchent(Salle salle1, Salle salle2);
void deplacer_ver_centre(Salle * salles, int nb);
Salle* generer_salles(int nombre_salles);
void creer_couloir_horizontal(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR], int x1, int y, int x2);
void creer_couloir_vertical(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR], int x, int y1, int y2);
void creer_couloir(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR], Salle salle1, Salle salle2);
void generer_donjon(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR]);

#endif // DONJON_H

```
``dungeon.cpp``
```c++
//
// Created by TogExe on 03/03/2025.
//
// donjon.c

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "donjon.h"

// Définir les dimensions du donjon
#define DONJON_LARGEUR 100
#define DONJON_HAUTEUR 70

// Définir les tailles minimales et maximales des salles
#define TAILLE_MIN_SALLE 5
#define TAILLE_MAX_SALLE 8

// Définir le nombre de salles
#define NOMBRE_SALLES 7

// Structure pour représenter une salle (inutile déja dans le donjon.h)

// Fonction pour générer une salle aléatoire
Salle generer_salle() {
    Salle salle;
    salle.largeur = TAILLE_MIN_SALLE + rand() % (TAILLE_MAX_SALLE - TAILLE_MIN_SALLE + 1);
    salle.hauteur = TAILLE_MIN_SALLE + rand() % (TAILLE_MAX_SALLE - TAILLE_MIN_SALLE + 1);
    salle.x = 1 + (rand() % (DONJON_LARGEUR - salle.largeur));
    salle.y = 1 + (rand() % (DONJON_HAUTEUR - salle.hauteur));
    return salle;
}

// Fonction pour vérifier si deux salles se chevauchent
int salles_se_chevauchent(Salle salle1, Salle salle2) {
    return !(salle1.x + salle1.largeur < salle2.x || salle1.x > salle2.x + salle2.largeur ||
             salle1.y + salle1.hauteur < salle2.y || salle1.y > salle2.y + salle2.hauteur);
}

void deplacer_ver_centre(Salle * salles, int nb) {
    for(int i = 0; i < nb; i++) {
        if(salles[i].x > 50) {
            salles[i].x = salles[i].x - 1;
            int a = 0;
            for(int j = 0; j < nb; j++) {
                if(!a) {
                    if(salles_se_chevauchent(salles[i], salles[j]) && j != i) {
                        a = 1;
                    }
                }
            }
            if(a) {
                salles[i].x = salles[i].x + 1;
            }
        } else if(salles[i].x < 50) {
            salles[i].x++;
            int a = 0;
            for(int j = 0; j < nb; j++) {
                if(!a) {
                    if(salles_se_chevauchent(salles[i], salles[j]) && j != i) {
                        a = 1;
                    }
                }
            }
            if(a) {
                salles[i].x--;
            }
        }
    }
}

// Fonction pour générer un tableau de salles non chevauchantes
Salle* generer_salles(int nombre_salles) {
    Salle* salles = (Salle*)malloc(nombre_salles * sizeof(Salle));
    for (int i = 0; i < nombre_salles; i++) {
        Salle nouvelle_salle = generer_salle();
        while (1) {
            int chevauchement = 0;
            for (int j = 0; j < i; j++) {
                if (salles_se_chevauchent(nouvelle_salle, salles[j])) {
                    chevauchement = 1;
                    break;
                }
            }
            if (!chevauchement) {
                break;
            }
            nouvelle_salle = generer_salle();
        }
        salles[i] = nouvelle_salle;
        //deplacer_ver_centre(salles, NOMBRE_SALLES);
    }
    return salles;
}

// Fonction pour créer un couloir horizontal
void creer_couloir_horizontal(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR], int x1, int y, int x2) {
    for (int x = x1; x <= x2; x++) {
        donjon[y][x] = 1;
    }
}

// Fonction pour créer un couloir vertical
void creer_couloir_vertical(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR], int x, int y1, int y2) {
    for (int y = y1; y <= y2; y++) {
        donjon[y][x] = 1;
    }
}

// Fonction pour créer un couloir entre deux salles
void creer_couloir(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR], Salle salle1, Salle salle2) {
    int cx1 = salle1.x;
    int cy1 = salle1.y;
    int cx2 = salle2.x;
    int cy2 = salle2.y;

    if (rand() % 2) {
        creer_couloir_horizontal(donjon, cx1, cy1, cx2);
        creer_couloir_vertical(donjon, cx2, cy1, cy2);
    } else {
        creer_couloir_vertical(donjon, cx1, cy1, cy2);
        creer_couloir_horizontal(donjon, cx1, cy2, cx2);
    }
}

// Fonction pour générer le donjon
void generer_donjon(char donjon[DONJON_HAUTEUR][DONJON_LARGEUR]) {
    Salle* salles = generer_salles(NOMBRE_SALLES);
    for (int i = 0; i < NOMBRE_SALLES; i++) {
        Salle salle = salles[i];
        for (int y = salle.y; y < salle.y + salle.hauteur; y++) {
            for (int x = salle.x; x < salle.x + salle.largeur; x++) {
                donjon[y][x] = 1;
            }
        }
    }

    for (int i = 0; i < NOMBRE_SALLES - 1; i++) {
        creer_couloir(donjon, salles[i], salles[i + 1]);
    }

    free(salles);
}
```

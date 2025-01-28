#include <stdio.h>
#include <windows.h>
#include <gdiplus.h>
#include <tchar.h>
#include <math.h>
#include <chrono>

#define TRUE 1
#define FALSE 0
#define DEBUG TRUE
#define HEIGHT 600
#define WIDTH 600
#define FOV 90

int update = true;

float player_x_offset = 0.0f;
float player_y_offset = 0.0f;
float player_z_offset = 7.0f;


typedef struct Point {
    float pos_x, pos_y, pos_z;
    void setup(float x, float y, float z) {
        pos_x = x;
        pos_y = y;
        pos_z = z;
    }
} Point;

typedef struct Vertex {
    Point point;
    struct Vertex* next;
    enum { HEAD, BASIC } type;
} Vertex;

typedef struct Triangle {
    Vertex vertex[3];
} Triangle;


Triangle make_triangle(Point point[3]) {
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

void DrawTriangle(HDC hdc, Triangle* triangle) {
    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 255));
    //graphics.Clear(Gdiplus::Color(100, 255, 255, 255));
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

#include <cmath>

// Define the projection matrix array globally if not already defined
float ProjectionMatrix[4][4] = {0};

void setProjectionMatrix(const float& angleOfView, const float& near_, const float& far_) {
    float scale = 1 / tan(angleOfView * 0.5 * M_PI / 180);
    ProjectionMatrix[0][0] = scale;
    ProjectionMatrix[1][1] = scale;
    ProjectionMatrix[2][2] = -far_ / (far_ - near_);
    ProjectionMatrix[3][2] = -far_ * near_ / (far_ - near_);
    ProjectionMatrix[2][3] = -1;
    ProjectionMatrix[3][3] = 0;
    if(DEBUG) {
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
    float x = (point.pos_x+player_x_offset) * ProjectionMatrix[0][0];
    float y = (point.pos_y+player_y_offset) * ProjectionMatrix[1][1];
    float z = (point.pos_z+ player_z_offset) * ProjectionMatrix[2][2] + ProjectionMatrix[3][2];
    float w = (point.pos_z+player_z_offset) * ProjectionMatrix[2][3];

    if (w == 0) w = 1; // Avoid division by zero
    x /= w;
    y /= w;

    ScreenPoint screenPoint;
    screenPoint.x = x * WIDTH / 2 + WIDTH / 2;
    screenPoint.y = -y * HEIGHT / 2 + HEIGHT / 2; // Flip Y-axis for screen space
    return screenPoint;
}


void DrawTriangle3D(HDC hdc, Triangle* triangle) {
    Gdiplus::Graphics graphics(hdc);
    Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 255));
    Gdiplus::SolidBrush my_solid_brush(Gdiplus::Color(255, 0, 0, 255));
    /*
    EPIC FAILS
    Gdiplus::PointF pt1(
        triangle->vertex[0].point.pos_x,
        triangle->vertex[0].point.pos_y);
    Gdiplus::PointF pt2(
        triangle->vertex[1].point.pos_x,
        triangle->vertex[1].point.pos_y);
    Gdiplus::PointF pt3(
        triangle->vertex[2].point.pos_x,
        triangle->vertex[2].point.pos_y);

    ScreenPoint pt1 = ProjectPoint(triangle->vertex[0].point);
    ScreenPoint pt2 = ProjectPoint(triangle->vertex[2].point);
    ScreenPoint pt3 = ProjectPoint(triangle->vertex[3].point);
    */

    Gdiplus::PointF points[] = {
        Gdiplus::PointF(ProjectPoint(triangle->vertex[0].point).x, ProjectPoint(triangle->vertex[0].point).y),
        Gdiplus::PointF(ProjectPoint(triangle->vertex[1].point).x, ProjectPoint(triangle->vertex[1].point).y),
        Gdiplus::PointF(ProjectPoint(triangle->vertex[2].point).x, ProjectPoint(triangle->vertex[2].point).y)
    };

    graphics.FillPolygon(&my_solid_brush, points, 3);

}
#include <vector>


typedef struct Model {
    std::vector<Triangle> triangles;
} Model;

// Function to create a cube
Model make_cube(float size, float x_offset, float y_offset, float z_offset) {
    Model cube;

    // Define the 8 vertices of the cube
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

    // Define the 12 triangles (6 faces, 2 triangles per face)
    int indices[12][3] = {
        {0, 1, 2}, {0, 2, 3}, // Front face
        {4, 5, 6}, {4, 6, 7}, // Back face
        {0, 1, 5}, {0, 5, 4}, // Bottom face
        {3, 2, 6}, {3, 6, 7}, // Top face
        {0, 3, 7}, {0, 7, 4}, // Left face
        {1, 2, 6}, {1, 6, 5}  // Right face
    };

    // Create triangles from the indices
    for (int i = 0; i < 12; i++) {
        Point triangle_vertices[3] = {
            vertices[indices[i][0]],
            vertices[indices[i][1]],
            vertices[indices[i][2]]
        };
        cube.triangles.push_back(make_triangle(triangle_vertices));
    }

    return cube;
}

// Function to draw a 3D model
/*
void DrawModel3D(HDC hdc, Model* model) {
    for (Triangle& triangle : model->triangles) {
        DrawTriangle3D(hdc, &triangle);
    }
}*/
void DrawModel3D(HDC hdc, Model* model, float x_offset, float y_offset, float z_offset) {
    for (Triangle& triangle : model->triangles) {
        /*
        for (int i = 0; i < 3; i++) {
            triangle.vertex[i].point.pos_x += x_offset;
            triangle.vertex[i].point.pos_y += y_offset;
            triangle.vertex[i].point.pos_z += z_offset;
        }*/
        DrawTriangle3D(hdc, &triangle);

        /* Pretty useless if we do it within the 3D
         for (int i = 0; i < 3; i++) {  // Reset the offset to prevent permanent changes
            triangle.vertex[i].point.pos_x -= x_offset;
            triangle.vertex[i].point.pos_y -= y_offset;
            triangle.vertex[i].point.pos_z -= z_offset;
        }*/
    }
}


void HandleInput(WPARAM wParam) {
    switch (wParam) {
        case VK_UP:    // Move forward
            player_y_offset -= 0.2f;
            update=true;
            break;
        case VK_DOWN:  // Move backward
            player_y_offset += 0.2f;
            update=true;
            break;
        case VK_LEFT:  // Move left
            player_x_offset += 0.2f;
            update=true;
            break;
        case VK_RIGHT: // Move right
            player_x_offset -= 0.2f;
            update=true;
            break;
        case 'S':      // Move up
            player_z_offset += 0.2f;
            update=true;
            break;
        case 'Z':      // Move down
            player_z_offset -= 0.2f;
            update=true;
            break;
    }
}

void RenderScene(HWND hwnd, HDC hdc) {
    // Double buffering setup
    HDC hdcMem = CreateCompatibleDC(hdc);
    HBITMAP hbmMem = CreateCompatibleBitmap(hdc, WIDTH, HEIGHT);
    HGDIOBJ hOldBitmap = SelectObject(hdcMem, hbmMem);

    Gdiplus::Graphics graphics(hdcMem);

    // Clear the offscreen buffer
    graphics.Clear(Gdiplus::Color(255, 0, 0, 0));

    // Create a player model (cube)
    Model player_model = make_cube(3.0f, player_x_offset, player_y_offset, player_z_offset);

    // Draw the player model to the offscreen buffer
    DrawModel3D(hdcMem, &player_model, player_x_offset, player_y_offset, player_z_offset);

    // Copy the buffer to the main HDC
    BitBlt(hdc, 0, 0, WIDTH, HEIGHT, hdcMem, 0, 0, SRCCOPY);

    // Cleanup
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hbmMem);
    DeleteDC(hdcMem);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_KEYDOWN:
            HandleInput(wParam);
            if(update) {
                InvalidateRect(hwnd, NULL, TRUE);
                update = false;
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

WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L,
                  GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                  _T("GDI+ Player Model Window"), NULL };




// Updated WinMain function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Set up the projection matrix
    setProjectionMatrix(FOV, 0.1f, 100.0f); // Adjust near and far planes appropriately
    if (DEBUG) {
        printf("Running 'WinMain()'\n");
    }

    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Register and create the window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WindowProc, 0L, 0L,
                      GetModuleHandle(NULL), NULL, NULL, NULL, NULL,
                      _T("GDI+ Player Model Window"), NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(wc.lpszClassName, _T("3D Player Model"),
                             WS_OVERLAPPEDWINDOW, WIDTH, HEIGHT, WIDTH, HEIGHT,
                             NULL, NULL, wc.hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Main message loop
    MSG msg;
    auto lastFrameTime = std::chrono::high_resolution_clock::now();
    const double targetFrameTime = 1.0 / 120.0; // 60 FPS

    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        // Frame limiting
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> elapsed = now - lastFrameTime;
        if (elapsed.count() < targetFrameTime) {
            Sleep((targetFrameTime - elapsed.count()) * 1000); // Sleep to cap frame rate
        }
        lastFrameTime = std::chrono::high_resolution_clock::now();
        printf("%u\n", msg.message);
    }


    // Clean up
    Gdiplus::GdiplusShutdown(gdiplusToken);
    UnregisterClass(wc.lpszClassName, wc.hInstance);
    return (int)msg.wParam;
}


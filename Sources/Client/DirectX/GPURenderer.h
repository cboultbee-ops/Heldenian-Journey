// GPURenderer.h: GPU-accelerated sprite renderer using OpenGL 3.3
// Uses WGL to create OpenGL context on Win32 HWND (no separate GLFW window)
//
//////////////////////////////////////////////////////////////////////

#ifndef GPU_RENDERER_H
#define GPU_RENDERER_H

#include <windows.h>
#include <vector>
#include "..\include\GL\glew.h"
// Note: GLFW no longer used for windowing - using WGL directly

// Forward declarations
class CShaderManager;

// Blend modes for GPU rendering
enum GPUBlendMode {
    BLEND_COLORKEY = 0,     // Simple color key transparency
    BLEND_ALPHA = 1,        // Alpha blending with configurable alpha
    BLEND_ADDITIVE = 2,     // Additive blending (for RGB effects)
    BLEND_SHADOW = 3,       // Shadow/darken blend
    BLEND_FADE = 4,         // Fade/subtract blend (mask-only darken to ~25%)
    BLEND_AVERAGE = 5,      // Average blend (Trans2)
    BLEND_TEXT = 6,          // Text rendering (tint color + texture alpha)
    BLEND_OPAQUE = 7,       // Opaque draw (map background - force alpha 1.0)
    BLEND_DARKEN = 8,       // Subtractive darken (reverse-subtract blend)
    BLEND_TINTED = 9,       // Replacement draw with color tint (standard alpha blend)
    BLEND_SUBTRACTIVE = 10  // Subtractive blend: dst - src (PutRevTransSprite)
};

// Vertex structure for sprite batching
struct SpriteVertex {
    float x, y;             // Position
    float u, v;             // Texture coordinates
    float r, g, b, a;       // Color/tint and alpha
};

// Character metrics for font atlas
struct CharMetrics {
    int x, y;               // Position in atlas texture
    int width, height;      // Character dimensions in pixels
};

// Sprite draw command for batching
struct SpriteDrawCmd {
    GLuint textureID;
    int dstX, dstY;
    int srcX, srcY;
    int width, height;
    int texWidth, texHeight;
    GPUBlendMode blendMode;
    float alpha;
    float colorR, colorG, colorB;
};

// Render configuration for resolution scaling
struct RenderConfig {
    int virtualWidth;       // Game logic resolution (default 640)
    int virtualHeight;      // Game logic resolution (default 480)
    int nativeWidth;        // Actual window/screen width
    int nativeHeight;       // Actual window/screen height
    float scaleX;           // Scale factor X (kept for compat)
    float scaleY;           // Scale factor Y (kept for compat)
    // Letterbox viewport (aspect-ratio-preserving)
    int viewportX;          // Viewport X offset (black bar left)
    int viewportY;          // Viewport Y offset (black bar top)
    int viewportW;          // Viewport width
    int viewportH;          // Viewport height
    float uniformScale;     // min(scaleX, scaleY) — the actual scaling factor used
};

class CGPURenderer {
public:
    void* operator new(size_t size) {
        return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    }

    void operator delete(void* mem) {
        HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, mem);
    }

    CGPURenderer();
    ~CGPURenderer();

    // Initialization - uses WGL to create OpenGL context on Win32 window
    bool Init(HWND hWnd, int virtualWidth = 640, int virtualHeight = 480);
    void Shutdown();
    bool IsInitialized() const { return m_bInitialized; }

    // Buffer swapping
    void SwapBuffers();

    // Frame management
    void BeginFrame();
    void EndFrame();

    // Sprite rendering - queue for batching
    // spriteScale: 1 for original sprites, 2 for 2x upscaled (UV coords scaled, vertex size unchanged)
    void QueueSprite(GLuint textureID, int dstX, int dstY,
                     int srcX, int srcY, int width, int height,
                     int texWidth, int texHeight,
                     int spriteScale = 1,
                     GPUBlendMode blendMode = BLEND_COLORKEY,
                     float alpha = 1.0f,
                     float colorR = 0.0f, float colorG = 0.0f, float colorB = 0.0f);

    // Flush batched sprites to GPU
    void Flush();

    // Pixel drawing (1x1 solid color, used for minimap dots/borders)
    void DrawPixel(int x, int y, float r, float g, float b);

    // Filled rectangle (uses white texture + shadow blend for dark panels)
    void DrawFilledRect(int x, int y, int w, int h, float r, float g, float b, float a);

    // Line drawing (additive blend, used for lightning/thunder effects)
    void QueueLine(int x0, int y0, int x1, int y1, float r, float g, float b);
    void FlushLines();

    // Texture creation from RGBA data
    GLuint CreateTexture(const uint8_t* rgbaData, int width, int height);
    void DeleteTexture(GLuint textureID);

    // Resolution management
    void SetNativeResolution(int width, int height);
    void OnResize(int newWidth, int newHeight);
    const RenderConfig& GetRenderConfig() const { return m_renderConfig; }

    // Coordinate transformation
    void VirtualToNative(int vx, int vy, int& nx, int& ny);
    void NativeToVirtual(int nx, int ny, int& vx, int& vy);

    // Clear the render target
    void Clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);

    // Font atlas and text rendering
    bool InitFontAtlas(HFONT hFont);
    void RenderText(int x, int y, const char* text, COLORREF color);
    void RenderTextRect(RECT* pRect, const char* text, COLORREF color);
    int GetTextWidth(const char* text, int len = -1);

    // Map background texture
    void UpdateMapTexture(const uint8_t* surfaceData, int pitch, int width, int height,
                          int bitDepth, DWORD dwRMask, DWORD dwGMask, DWORD dwBMask);
    void RenderMapQuad(int srcX, int srcY, int srcW, int srcH);

private:
    bool CreateShaders();
    bool CreateBuffers();
    void UpdateProjectionMatrix();
    void RenderBatch();

    // OpenGL objects
    GLuint m_vao;               // Vertex array object
    GLuint m_vbo;               // Vertex buffer object
    GLuint m_ebo;               // Element buffer object (for indexed drawing)
    GLuint m_shaderProgram;     // Compiled shader program

    // Shader uniform locations
    GLint m_uProjection;        // Projection matrix uniform
    GLint m_uTexture;           // Texture sampler uniform
    GLint m_uBlendMode;         // Blend mode uniform
    GLint m_uAlpha;             // Alpha value uniform
    GLint m_uColorTint;         // Color tint uniform

    // Batching
    std::vector<SpriteVertex> m_vertices;
    std::vector<GLuint> m_indices;
    std::vector<SpriteDrawCmd> m_drawCmds;
    static const size_t MAX_SPRITES_PER_BATCH = 4096;
    static const size_t VERTICES_PER_SPRITE = 4;
    static const size_t INDICES_PER_SPRITE = 6;

    // Projection matrix (column-major for OpenGL)
    float m_projectionMatrix[16];

    // Configuration
    RenderConfig m_renderConfig;
    HWND m_hWnd;          // Win32 window handle
    HDC m_hDC;            // Device context for OpenGL
    HGLRC m_hGLRC;        // OpenGL rendering context
    bool m_bInitialized;
    bool m_bInFrame;

    // Current batch state
    GLuint m_currentTexture;

    // Line drawing
    std::vector<SpriteVertex> m_lineVertices;
    GLuint m_glWhiteTexture;

    // Font atlas
    GLuint m_fontAtlasTexture;
    int m_fontAtlasWidth;
    int m_fontAtlasHeight;
    int m_fontCharHeight;
    CharMetrics m_charMetrics[256];
    bool m_bFontAtlasReady;

    // Map background texture
    GLuint m_mapTexture;
    int m_mapTexWidth;
    int m_mapTexHeight;
    uint8_t* m_pMapRGBABuffer;
};

// Helper function for 16-bit to RGBA conversion
void ConvertRGB565ToRGBA(const uint16_t* src, uint8_t* dst,
                          int width, int height, uint16_t colorKey);

void ConvertRGB555ToRGBA(const uint16_t* src, uint8_t* dst,
                          int width, int height, uint16_t colorKey);

#endif // GPU_RENDERER_H

// GPURenderer.cpp: GPU-accelerated sprite renderer implementation
//
//////////////////////////////////////////////////////////////////////

#include "GPURenderer.h"
#include "..\include\GL\wglew.h"
#include <cstring>
#include <cstdio>

// Embedded shader sources
static const char* s_vertexShaderSource = R"glsl(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aColor;

out vec2 TexCoord;
out vec4 Color;

uniform mat4 uProjection;

void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
    Color = aColor;
}
)glsl";

static const char* s_fragmentShaderSource = R"glsl(
#version 330 core
in vec2 TexCoord;
in vec4 Color;
out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uBlendMode;       // 0=colorkey, 1=alpha, 2=additive, 3=shadow, 4=fade, 5=average, 7=opaque, 8=darken, 9=tinted, 10=subtractive
uniform float uAlpha;         // For alpha blend modes
uniform vec3 uColorTint;      // For RGB tinting

void main() {
    vec4 texColor = texture(uTexture, TexCoord);

    // Opaque mode (map background) - skip alpha discard, force full opacity
    if (uBlendMode == 7) {
        FragColor = vec4(texColor.rgb, 1.0);
        return;
    }

    // Discard transparent pixels (alpha from RGBA conversion)
    if (texColor.a < 0.01) discard;
    // Alpha blend: discard near-black or near-white pixels that may have wrong alpha
    if (uBlendMode == 1) {
        float lum = length(texColor.rgb);
        if (lum < 0.08) discard;
        if (lum > 0.92 && min(texColor.r, min(texColor.g, texColor.b)) > 0.85) discard;
    }

    if (uBlendMode == 0) {
        // Color key - direct copy
        FragColor = texColor * Color;
    }
    else if (uBlendMode == 1) {
        // Alpha blend
        FragColor = vec4(texColor.rgb, texColor.a * uAlpha) * Color;
    }
    else if (uBlendMode == 2) {
        // Additive: sprite RGB + tint, added to destination.
        // In DD additive blending, a pixel's brightness IS its contribution
        // strength — dark pixels add almost nothing, bright pixels add a lot.
        // We replicate this by using max(RGB) as alpha so dark edge pixels
        // smoothly fade to invisible instead of rendering as a black outline.
        vec3 added = clamp(texColor.rgb + uColorTint, 0.0, 1.0);
        float brightness = max(added.r, max(added.g, added.b));
        FragColor = vec4(added, brightness * uAlpha);
    }
    else if (uBlendMode == 3) {
        // Shadow (darken destination - simulated via dark semi-transparent)
        FragColor = vec4(0.0, 0.0, 0.0, texColor.a * 0.5);
    }
    else if (uBlendMode == 4) {
        // Fade: darken destination to ~25% using sprite as mask only
        // Original DD: pDst = (pDst & mask) >> 2 (divide by 4)
        FragColor = vec4(0.0, 0.0, 0.0, texColor.a * 0.75);
    }
    else if (uBlendMode == 5) {
        // Average blend (50% blend with destination - use alpha 0.5)
        FragColor = vec4(texColor.rgb, texColor.a * 0.5) * Color;
    }
    else if (uBlendMode == 6) {
        // Text rendering: use tint color as text color, texture alpha as opacity
        FragColor = vec4(uColorTint, texColor.a);
    }
    else if (uBlendMode == 8) {
        // Darken: subtract tint amount from destination, masked by sprite alpha
        // Used with GL_FUNC_REVERSE_SUBTRACT: result = dst - src * srcAlpha
        FragColor = vec4(uColorTint, texColor.a);
    }
    else if (uBlendMode == 9) {
        // Tinted replacement: add tint to texture RGB, draw with standard alpha blend
        // Used for PutColouredSprite (replaces destination with tinted source)
        vec3 tinted = clamp(texColor.rgb + uColorTint, 0.0, 1.0);
        FragColor = vec4(tinted, texColor.a) * Color;
    }
    else if (uBlendMode == 10) {
        // Subtractive (PutRevTransSprite): result = dst - src.rgb * src.a
        // DD formula: result = max(dst - src, 0)  per channel.
        // Dark source pixels subtract almost nothing (invisible).
        // Bright source pixels subtract a lot (visible darkening).
        // GL_FUNC_REVERSE_SUBTRACT handles the dst-src and clamping.
        FragColor = vec4(texColor.rgb, texColor.a * uAlpha);
    }
    else {
        FragColor = texColor * Color;
    }
}
)glsl";

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGPURenderer::CGPURenderer()
{
    m_vao = 0;
    m_vbo = 0;
    m_ebo = 0;
    m_shaderProgram = 0;
    m_uProjection = -1;
    m_uTexture = -1;
    m_uBlendMode = -1;
    m_uAlpha = -1;
    m_uColorTint = -1;
    m_hWnd = NULL;
    m_hDC = NULL;
    m_hGLRC = NULL;
    m_bInitialized = false;
    m_bInFrame = false;
    m_currentTexture = 0;

    memset(m_projectionMatrix, 0, sizeof(m_projectionMatrix));

    // Line drawing
    m_glWhiteTexture = 0;

    // Font atlas initialization
    m_fontAtlasTexture = 0;
    m_fontAtlasWidth = 0;
    m_fontAtlasHeight = 0;
    m_fontCharHeight = 0;
    memset(m_charMetrics, 0, sizeof(m_charMetrics));
    m_bFontAtlasReady = false;

    // Map texture initialization
    m_mapTexture = 0;
    m_mapTexWidth = 0;
    m_mapTexHeight = 0;
    m_pMapRGBABuffer = NULL;

    // Default render config
    m_renderConfig.virtualWidth = 640;
    m_renderConfig.virtualHeight = 480;
    m_renderConfig.nativeWidth = 640;
    m_renderConfig.nativeHeight = 480;
    m_renderConfig.scaleX = 1.0f;
    m_renderConfig.scaleY = 1.0f;
    m_renderConfig.viewportX = 0;
    m_renderConfig.viewportY = 0;
    m_renderConfig.viewportW = 640;
    m_renderConfig.viewportH = 480;
    m_renderConfig.uniformScale = 1.0f;

    // Reserve space for batching
    m_vertices.reserve(MAX_SPRITES_PER_BATCH * VERTICES_PER_SPRITE);
    m_indices.reserve(MAX_SPRITES_PER_BATCH * INDICES_PER_SPRITE);
}

CGPURenderer::~CGPURenderer()
{
    Shutdown();
}

bool CGPURenderer::Init(HWND hWnd, int virtualWidth, int virtualHeight)
{
    if (m_bInitialized) return true;
    if (hWnd == NULL) {
        OutputDebugString("GPURenderer: NULL window handle\n");
        return false;
    }

    m_hWnd = hWnd;
    m_renderConfig.virtualWidth = virtualWidth;
    m_renderConfig.virtualHeight = virtualHeight;

    // Get device context from the Win32 window
    m_hDC = GetDC(m_hWnd);
    if (m_hDC == NULL) {
        OutputDebugString("GPURenderer: Failed to get device context\n");
        return false;
    }

    // Set up pixel format for OpenGL
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR),
        1,                             // Version number
        PFD_DRAW_TO_WINDOW |           // Support window
        PFD_SUPPORT_OPENGL |           // Support OpenGL
        PFD_DOUBLEBUFFER,              // Double buffered
        PFD_TYPE_RGBA,                 // RGBA type
        32,                            // 32-bit color depth
        0, 0, 0, 0, 0, 0,              // Color bits ignored
        0,                             // No alpha buffer
        0,                             // Shift bit ignored
        0,                             // No accumulation buffer
        0, 0, 0, 0,                    // Accum bits ignored
        24,                            // 24-bit z-buffer
        8,                             // 8-bit stencil buffer
        0,                             // No auxiliary buffer
        PFD_MAIN_PLANE,                // Main layer
        0,                             // Reserved
        0, 0, 0                        // Layer masks ignored
    };

    int pixelFormat = ChoosePixelFormat(m_hDC, &pfd);
    if (pixelFormat == 0) {
        OutputDebugString("GPURenderer: Failed to choose pixel format\n");
        ReleaseDC(m_hWnd, m_hDC);
        m_hDC = NULL;
        return false;
    }

    if (!SetPixelFormat(m_hDC, pixelFormat, &pfd)) {
        OutputDebugString("GPURenderer: Failed to set pixel format\n");
        ReleaseDC(m_hWnd, m_hDC);
        m_hDC = NULL;
        return false;
    }

    // Create OpenGL rendering context
    m_hGLRC = wglCreateContext(m_hDC);
    if (m_hGLRC == NULL) {
        OutputDebugString("GPURenderer: Failed to create OpenGL context\n");
        ReleaseDC(m_hWnd, m_hDC);
        m_hDC = NULL;
        return false;
    }

    // Make the context current
    if (!wglMakeCurrent(m_hDC, m_hGLRC)) {
        OutputDebugString("GPURenderer: Failed to make OpenGL context current\n");
        wglDeleteContext(m_hGLRC);
        m_hGLRC = NULL;
        ReleaseDC(m_hWnd, m_hDC);
        m_hDC = NULL;
        return false;
    }

    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        OutputDebugString("GPURenderer: Failed to initialize GLEW\n");
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(m_hGLRC);
        m_hGLRC = NULL;
        ReleaseDC(m_hWnd, m_hDC);
        m_hDC = NULL;
        return false;
    }

    // Disable VSync — the software frame limiter (UpdateScreen, ~60 FPS) provides
    // smooth pacing, and DWM handles tearing prevention for windowed/borderless apps.
    // With VSync ON, SwapBuffers blocks up to 16ms which compounds with the frame
    // limiter sleep, causing missed vblanks and frame rate drops to ~30 FPS.
    if (WGLEW_EXT_swap_control) {
        wglSwapIntervalEXT(0);
    }

    // Get window dimensions for native resolution
    RECT rect;
    GetClientRect(m_hWnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    m_renderConfig.nativeWidth = width;
    m_renderConfig.nativeHeight = height;
    m_renderConfig.scaleX = (float)width / (float)virtualWidth;
    m_renderConfig.scaleY = (float)height / (float)virtualHeight;

    // Letterbox: use uniform scale to preserve 4:3 aspect ratio
    float scale = (m_renderConfig.scaleX < m_renderConfig.scaleY)
                  ? m_renderConfig.scaleX : m_renderConfig.scaleY;
    m_renderConfig.uniformScale = scale;
    m_renderConfig.viewportW = (int)(m_renderConfig.virtualWidth * scale);
    m_renderConfig.viewportH = (int)(m_renderConfig.virtualHeight * scale);
    m_renderConfig.viewportX = (width - m_renderConfig.viewportW) / 2;
    m_renderConfig.viewportY = (height - m_renderConfig.viewportH) / 2;

    // Set up OpenGL state for 2D rendering
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    if (!CreateShaders()) {
        OutputDebugString("GPURenderer: Failed to create shaders\n");
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(m_hGLRC);
        m_hGLRC = NULL;
        ReleaseDC(m_hWnd, m_hDC);
        m_hDC = NULL;
        return false;
    }

    if (!CreateBuffers()) {
        OutputDebugString("GPURenderer: Failed to create buffers\n");
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(m_hGLRC);
        m_hGLRC = NULL;
        ReleaseDC(m_hWnd, m_hDC);
        m_hDC = NULL;
        return false;
    }

    UpdateProjectionMatrix();

    // Create 1x1 white texture for line drawing
    uint8_t whitePixel[4] = { 255, 255, 255, 255 };
    glGenTextures(1, &m_glWhiteTexture);
    glBindTexture(GL_TEXTURE_2D, m_glWhiteTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, whitePixel);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    m_bInitialized = true;
    OutputDebugString("GPURenderer: Initialized successfully with WGL\n");
    return true;
}

void CGPURenderer::Shutdown()
{
    if (!m_bInitialized) return;

    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        m_vao = 0;
    }
    if (m_vbo) {
        glDeleteBuffers(1, &m_vbo);
        m_vbo = 0;
    }
    if (m_ebo) {
        glDeleteBuffers(1, &m_ebo);
        m_ebo = 0;
    }
    if (m_shaderProgram) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }

    m_vertices.clear();
    m_indices.clear();
    m_drawCmds.clear();

    // Clean up font atlas
    if (m_fontAtlasTexture) {
        glDeleteTextures(1, &m_fontAtlasTexture);
        m_fontAtlasTexture = 0;
    }
    m_bFontAtlasReady = false;

    // Clean up map texture
    if (m_mapTexture) {
        glDeleteTextures(1, &m_mapTexture);
        m_mapTexture = 0;
    }
    if (m_pMapRGBABuffer) {
        delete[] m_pMapRGBABuffer;
        m_pMapRGBABuffer = NULL;
    }
    m_mapTexWidth = 0;
    m_mapTexHeight = 0;

    // Clean up WGL context
    if (m_hGLRC != NULL) {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(m_hGLRC);
        m_hGLRC = NULL;
    }
    if (m_hDC != NULL && m_hWnd != NULL) {
        ReleaseDC(m_hWnd, m_hDC);
        m_hDC = NULL;
    }
    m_hWnd = NULL;

    m_bInitialized = false;
}

void CGPURenderer::SwapBuffers()
{
    if (m_hDC != NULL) {
        ::SwapBuffers(m_hDC);
    }
}

bool CGPURenderer::CreateShaders()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &s_vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLint success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        OutputDebugString("Vertex shader compilation failed: ");
        OutputDebugString(infoLog);
        OutputDebugString("\n");
        return false;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &s_fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        OutputDebugString("Fragment shader compilation failed: ");
        OutputDebugString(infoLog);
        OutputDebugString("\n");
        glDeleteShader(vertexShader);
        return false;
    }

    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);

    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_shaderProgram, 512, NULL, infoLog);
        OutputDebugString("Shader program linking failed: ");
        OutputDebugString(infoLog);
        OutputDebugString("\n");
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return false;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    // Get uniform locations
    m_uProjection = glGetUniformLocation(m_shaderProgram, "uProjection");
    m_uTexture = glGetUniformLocation(m_shaderProgram, "uTexture");
    m_uBlendMode = glGetUniformLocation(m_shaderProgram, "uBlendMode");
    m_uAlpha = glGetUniformLocation(m_shaderProgram, "uAlpha");
    m_uColorTint = glGetUniformLocation(m_shaderProgram, "uColorTint");

    return true;
}

bool CGPURenderer::CreateBuffers()
{
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    // Allocate vertex buffer with enough space for max sprites
    size_t vboSize = MAX_SPRITES_PER_BATCH * VERTICES_PER_SPRITE * sizeof(SpriteVertex);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vboSize, NULL, GL_DYNAMIC_DRAW);

    // Allocate element buffer
    size_t eboSize = MAX_SPRITES_PER_BATCH * INDICES_PER_SPRITE * sizeof(GLuint);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, eboSize, NULL, GL_DYNAMIC_DRAW);

    // Position attribute (location 0)
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
                          (void*)offsetof(SpriteVertex, x));
    glEnableVertexAttribArray(0);

    // Texture coord attribute (location 1)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
                          (void*)offsetof(SpriteVertex, u));
    glEnableVertexAttribArray(1);

    // Color attribute (location 2)
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(SpriteVertex),
                          (void*)offsetof(SpriteVertex, r));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    return true;
}

void CGPURenderer::UpdateProjectionMatrix()
{
    // Create orthographic projection matrix
    // Maps (0, virtualWidth) x (0, virtualHeight) to (-1, 1) x (-1, 1)
    // Note: Y is flipped for top-left origin (like DirectDraw)
    float left = 0.0f;
    float right = (float)m_renderConfig.virtualWidth;
    float bottom = (float)m_renderConfig.virtualHeight;  // Flipped
    float top = 0.0f;
    float nearPlane = -1.0f;
    float farPlane = 1.0f;

    // Column-major order for OpenGL
    memset(m_projectionMatrix, 0, sizeof(m_projectionMatrix));
    m_projectionMatrix[0] = 2.0f / (right - left);
    m_projectionMatrix[5] = 2.0f / (top - bottom);
    m_projectionMatrix[10] = -2.0f / (farPlane - nearPlane);
    m_projectionMatrix[12] = -(right + left) / (right - left);
    m_projectionMatrix[13] = -(top + bottom) / (top - bottom);
    m_projectionMatrix[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    m_projectionMatrix[15] = 1.0f;
}

void CGPURenderer::SetProjectionZoom(float zoom)
{
    if (!m_bInitialized) return;

    // Flush pending draws before changing projection
    Flush();

    // Compute zoomed orthographic bounds centered on virtual screen center
    float centerX = (float)m_renderConfig.virtualWidth * 0.5f;   // 320
    float centerY = (float)m_renderConfig.virtualHeight * 0.5f;  // 240
    float halfW = centerX * zoom;  // 368 at zoom=1.15
    float halfH = centerY * zoom;  // 276 at zoom=1.15

    float left = centerX - halfW;     // -48 at zoom=1.15
    float right = centerX + halfW;    // 688
    float top = centerY - halfH;      // -36 (Y-down: top < bottom)
    float bottom = centerY + halfH;   // 516
    float nearPlane = -1.0f;
    float farPlane = 1.0f;

    // Column-major orthographic matrix (same layout as UpdateProjectionMatrix)
    memset(m_projectionMatrix, 0, sizeof(m_projectionMatrix));
    m_projectionMatrix[0] = 2.0f / (right - left);
    m_projectionMatrix[5] = 2.0f / (top - bottom);
    m_projectionMatrix[10] = -2.0f / (farPlane - nearPlane);
    m_projectionMatrix[12] = -(right + left) / (right - left);
    m_projectionMatrix[13] = -(top + bottom) / (top - bottom);
    m_projectionMatrix[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    m_projectionMatrix[15] = 1.0f;
}

void CGPURenderer::BeginFrame()
{
    if (!m_bInitialized) return;

    m_bInFrame = true;
    m_vertices.clear();
    m_indices.clear();
    m_drawCmds.clear();
    m_currentTexture = 0;

    // Clear entire window to black (covers letterbox bars)
    glViewport(0, 0, m_renderConfig.nativeWidth, m_renderConfig.nativeHeight);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Set viewport to letterboxed game area
    // OpenGL Y=0 is bottom, so flip Y offset
    glViewport(m_renderConfig.viewportX,
               m_renderConfig.nativeHeight - m_renderConfig.viewportY - m_renderConfig.viewportH,
               m_renderConfig.viewportW, m_renderConfig.viewportH);
}

void CGPURenderer::EndFrame()
{
    if (!m_bInitialized) return;

    Flush();
    FlushLines();
    m_bInFrame = false;
}

void CGPURenderer::QueueSprite(GLuint textureID, int dstX, int dstY,
                               int srcX, int srcY, int width, int height,
                               int texWidth, int texHeight,
                               int spriteScale,
                               GPUBlendMode blendMode, float alpha,
                               float colorR, float colorG, float colorB)
{
    if (!m_bInitialized) return;

    // Check if we need to flush due to state change
    if (!m_drawCmds.empty()) {
        const SpriteDrawCmd& last = m_drawCmds.back();
        if (last.textureID != textureID ||
            last.blendMode != blendMode ||
            last.alpha != alpha ||
            last.colorR != colorR ||
            last.colorG != colorG ||
            last.colorB != colorB ||
            m_vertices.size() >= MAX_SPRITES_PER_BATCH * VERTICES_PER_SPRITE) {
            Flush();
        }
    }

    // Calculate texture coordinates
    // For upscaled sprites (spriteScale > 1), brush coords are in 1x game space
    // but the texture is Nx larger. Scale UV coords to address the correct texels.
    // Half-texel inset prevents sampling adjacent texels at boundaries with GL_NEAREST.
    float halfTexelU = 0.5f / (float)texWidth;
    float halfTexelV = 0.5f / (float)texHeight;
    float u0 = (float)(srcX * spriteScale) / (float)texWidth + halfTexelU;
    float v0 = (float)(srcY * spriteScale) / (float)texHeight + halfTexelV;
    float u1 = (float)((srcX + width) * spriteScale) / (float)texWidth - halfTexelU;
    float v1 = (float)((srcY + height) * spriteScale) / (float)texHeight - halfTexelV;

    // Calculate vertex positions (virtual coordinates, will be transformed by projection)
    // Vertex size stays at 1x game coordinates regardless of sprite scale
    float x0 = (float)dstX;
    float y0 = (float)dstY;
    float x1 = (float)(dstX + width);
    float y1 = (float)(dstY + height);

    // Add vertices (4 per sprite)
    GLuint baseIndex = (GLuint)m_vertices.size();

    SpriteVertex v;
    v.r = 1.0f; v.g = 1.0f; v.b = 1.0f; v.a = 1.0f;

    // Top-left
    v.x = x0; v.y = y0; v.u = u0; v.v = v0;
    m_vertices.push_back(v);

    // Top-right
    v.x = x1; v.y = y0; v.u = u1; v.v = v0;
    m_vertices.push_back(v);

    // Bottom-right
    v.x = x1; v.y = y1; v.u = u1; v.v = v1;
    m_vertices.push_back(v);

    // Bottom-left
    v.x = x0; v.y = y1; v.u = u0; v.v = v1;
    m_vertices.push_back(v);

    // Add indices (2 triangles per sprite)
    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 1);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 3);

    // Store draw command for batch state
    SpriteDrawCmd cmd;
    cmd.textureID = textureID;
    cmd.dstX = dstX;
    cmd.dstY = dstY;
    cmd.srcX = srcX;
    cmd.srcY = srcY;
    cmd.width = width;
    cmd.height = height;
    cmd.texWidth = texWidth;
    cmd.texHeight = texHeight;
    cmd.blendMode = blendMode;
    cmd.alpha = alpha;
    cmd.colorR = colorR;
    cmd.colorG = colorG;
    cmd.colorB = colorB;
    m_drawCmds.push_back(cmd);
}

void CGPURenderer::QueueSpriteStretched(GLuint textureID,
                                         int dstX, int dstY, int dstW, int dstH,
                                         int srcX, int srcY, int srcW, int srcH,
                                         int texWidth, int texHeight,
                                         GPUBlendMode blendMode, float alpha)
{
    if (!m_bInitialized) return;

    // Flush if state changed
    if (!m_drawCmds.empty()) {
        const SpriteDrawCmd& last = m_drawCmds.back();
        if (last.textureID != textureID ||
            last.blendMode != blendMode ||
            last.alpha != alpha ||
            m_vertices.size() >= MAX_SPRITES_PER_BATCH * VERTICES_PER_SPRITE) {
            Flush();
        }
    }

    // UV coordinates from source rect in texture space
    float u0 = (float)srcX / (float)texWidth;
    float v0 = (float)srcY / (float)texHeight;
    float u1 = (float)(srcX + srcW) / (float)texWidth;
    float v1 = (float)(srcY + srcH) / (float)texHeight;

    // Destination quad in virtual coordinates (independent of source size)
    float x0 = (float)dstX;
    float y0 = (float)dstY;
    float x1 = (float)(dstX + dstW);
    float y1 = (float)(dstY + dstH);

    GLuint baseIndex = (GLuint)m_vertices.size();
    SpriteVertex v;
    v.r = 1.0f; v.g = 1.0f; v.b = 1.0f; v.a = 1.0f;

    v.x = x0; v.y = y0; v.u = u0; v.v = v0; m_vertices.push_back(v);
    v.x = x1; v.y = y0; v.u = u1; v.v = v0; m_vertices.push_back(v);
    v.x = x1; v.y = y1; v.u = u1; v.v = v1; m_vertices.push_back(v);
    v.x = x0; v.y = y1; v.u = u0; v.v = v1; m_vertices.push_back(v);

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 1);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 3);

    SpriteDrawCmd cmd;
    cmd.textureID = textureID;
    cmd.dstX = dstX; cmd.dstY = dstY;
    cmd.srcX = srcX; cmd.srcY = srcY;
    cmd.width = dstW; cmd.height = dstH;
    cmd.texWidth = texWidth; cmd.texHeight = texHeight;
    cmd.blendMode = blendMode;
    cmd.alpha = alpha;
    cmd.colorR = 0.0f; cmd.colorG = 0.0f; cmd.colorB = 0.0f;
    m_drawCmds.push_back(cmd);
}

void CGPURenderer::Flush()
{
    if (m_vertices.empty() || !m_bInitialized) return;

    // Get the state from the first command (all commands in batch have same state)
    if (m_drawCmds.empty()) return;
    const SpriteDrawCmd& cmd = m_drawCmds[0];

    // Set blend state based on mode
    switch (cmd.blendMode) {
    case BLEND_OPAQUE:
        glDisable(GL_BLEND);
        break;
    case BLEND_ADDITIVE:
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    case BLEND_DARKEN:
    case BLEND_SUBTRACTIVE:
        glEnable(GL_BLEND);
        glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        break;
    case BLEND_TINTED:
    case BLEND_SHADOW:
    case BLEND_FADE:
    case BLEND_AVERAGE:
    case BLEND_ALPHA:
    case BLEND_COLORKEY:
    default:
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }

    // Use shader program
    glUseProgram(m_shaderProgram);

    // Set uniforms
    glUniformMatrix4fv(m_uProjection, 1, GL_FALSE, m_projectionMatrix);
    glUniform1i(m_uTexture, 0);
    glUniform1i(m_uBlendMode, (int)cmd.blendMode);
    glUniform1f(m_uAlpha, cmd.alpha);
    glUniform3f(m_uColorTint, cmd.colorR, cmd.colorG, cmd.colorB);

    // Bind texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, cmd.textureID);

    // Update vertex buffer
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, m_vertices.size() * sizeof(SpriteVertex), m_vertices.data());

    // Update element buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, m_indices.size() * sizeof(GLuint), m_indices.data());

    // Draw
    glDrawElements(GL_TRIANGLES, (GLsizei)m_indices.size(), GL_UNSIGNED_INT, 0);

    glBindVertexArray(0);

    // Reset blend state if modified
    if (cmd.blendMode == BLEND_DARKEN || cmd.blendMode == BLEND_SUBTRACTIVE) {
        glBlendEquation(GL_FUNC_ADD);
    }
    if (cmd.blendMode == BLEND_OPAQUE) {
        glEnable(GL_BLEND);
    }

    // Clear batch
    m_vertices.clear();
    m_indices.clear();
    m_drawCmds.clear();
}

void CGPURenderer::DrawPixel(int x, int y, float r, float g, float b)
{
    if (!m_bInitialized) return;
    // Queue a 1x1 sprite using the white texture with BLEND_TEXT (uses uColorTint for color)
    QueueSprite(m_glWhiteTexture, x, y, 0, 0, 1, 1, 1, 1, 1, BLEND_TEXT, 1.0f, r, g, b);
}

void CGPURenderer::DrawFilledRect(int x, int y, int w, int h, float r, float g, float b, float a)
{
    if (!m_bInitialized) return;
    // Flush any pending sprites to avoid state mixing
    Flush();

    // Build a quad with the desired color as vertex color
    // Uses white texture + BLEND_COLORKEY: FragColor = texColor * Color = (1,1,1,1) * (r,g,b,a)
    float x0 = (float)x, y0 = (float)y;
    float x1 = (float)(x + w), y1 = (float)(y + h);

    GLuint baseIndex = (GLuint)m_vertices.size();
    SpriteVertex v;
    v.r = r; v.g = g; v.b = b; v.a = a;
    v.u = 0.0f; v.v = 0.0f;

    v.x = x0; v.y = y0; m_vertices.push_back(v);
    v.x = x1; v.y = y0; m_vertices.push_back(v);
    v.x = x1; v.y = y1; m_vertices.push_back(v);
    v.x = x0; v.y = y1; m_vertices.push_back(v);

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 1);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 3);

    SpriteDrawCmd cmd;
    cmd.textureID = m_glWhiteTexture;
    cmd.blendMode = BLEND_COLORKEY;
    cmd.alpha = 1.0f;
    cmd.colorR = 0.0f; cmd.colorG = 0.0f; cmd.colorB = 0.0f;
    cmd.dstX = x; cmd.dstY = y;
    cmd.srcX = 0; cmd.srcY = 0;
    cmd.width = w; cmd.height = h;
    cmd.texWidth = 1; cmd.texHeight = 1;
    m_drawCmds.push_back(cmd);
}

void CGPURenderer::QueueFilledRect(int x, int y, int w, int h, float r, float g, float b, float a)
{
    if (!m_bInitialized) return;
    // No Flush() — caller is responsible for flushing before and after batched calls

    float x0 = (float)x, y0 = (float)y;
    float x1 = (float)(x + w), y1 = (float)(y + h);

    GLuint baseIndex = (GLuint)m_vertices.size();
    SpriteVertex v;
    v.r = r; v.g = g; v.b = b; v.a = a;
    v.u = 0.0f; v.v = 0.0f;

    v.x = x0; v.y = y0; m_vertices.push_back(v);
    v.x = x1; v.y = y0; m_vertices.push_back(v);
    v.x = x1; v.y = y1; m_vertices.push_back(v);
    v.x = x0; v.y = y1; m_vertices.push_back(v);

    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 1);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 0);
    m_indices.push_back(baseIndex + 2);
    m_indices.push_back(baseIndex + 3);

    SpriteDrawCmd cmd;
    cmd.textureID = m_glWhiteTexture;
    cmd.blendMode = BLEND_COLORKEY;
    cmd.alpha = 1.0f;
    cmd.colorR = 0.0f; cmd.colorG = 0.0f; cmd.colorB = 0.0f;
    cmd.dstX = x; cmd.dstY = y;
    cmd.srcX = 0; cmd.srcY = 0;
    cmd.width = w; cmd.height = h;
    cmd.texWidth = 1; cmd.texHeight = 1;
    m_drawCmds.push_back(cmd);
}

void CGPURenderer::QueueLine(int x0, int y0, int x1, int y1, float r, float g, float b)
{
    // Queue two vertices for a line segment (drawn with GL_LINES)
    SpriteVertex v0 = { (float)x0, (float)y0, 0.0f, 0.0f, r, g, b, 1.0f };
    SpriteVertex v1 = { (float)x1, (float)y1, 0.0f, 0.0f, r, g, b, 1.0f };
    m_lineVertices.push_back(v0);
    m_lineVertices.push_back(v1);
}

void CGPURenderer::FlushLines()
{
    if (m_lineVertices.empty() || !m_bInitialized) return;

    // Additive blending: result = dst + src (dark lines add nothing = invisible)
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glUseProgram(m_shaderProgram);
    glUniformMatrix4fv(m_uProjection, 1, GL_FALSE, m_projectionMatrix);
    glUniform1i(m_uTexture, 0);
    // Use COLORKEY mode (0): FragColor = texColor * Color
    // With 1x1 white texture: FragColor = (1,1,1,1) * vertexColor = vertexColor
    glUniform1i(m_uBlendMode, 0);
    glUniform1f(m_uAlpha, 1.0f);
    glUniform3f(m_uColorTint, 0.0f, 0.0f, 0.0f);

    // Bind white texture so texColor = (1,1,1,1)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_glWhiteTexture);

    // Upload line vertices to the existing VBO and draw as GL_LINES
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0,
                    m_lineVertices.size() * sizeof(SpriteVertex),
                    m_lineVertices.data());

    glDrawArrays(GL_LINES, 0, (GLsizei)m_lineVertices.size());

    glBindVertexArray(0);

    // Restore default blend state
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_lineVertices.clear();
}

GLuint CGPURenderer::CreateTexture(const uint8_t* rgbaData, int width, int height)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgbaData);

    // Check for GPU texture upload errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        char buf[256];
        sprintf(buf, "[GPU ERROR] glTexImage2D failed: 0x%X for %dx%d texture (ID=%u)\n",
                err, width, height, textureID);
        OutputDebugStringA(buf);
    }

    // Nearest-neighbor filtering for pixel art
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return textureID;
}

GLuint CGPURenderer::CreateTextureLinear(const uint8_t* rgbaData, int width, int height)
{
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgbaData);

    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        char buf[256];
        sprintf(buf, "[GPU ERROR] glTexImage2D failed: 0x%X for %dx%d texture (ID=%u)\n",
                err, width, height, textureID);
        OutputDebugStringA(buf);
    }

    // Bilinear filtering for photographic/painted images
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    return textureID;
}

void CGPURenderer::DeleteTexture(GLuint textureID)
{
    if (textureID != 0) {
        glDeleteTextures(1, &textureID);
    }
}

bool CGPURenderer::DiagnoseTexture(GLuint textureID, int width, int height, TextureDiagnostic& diag)
{
    memset(&diag, 0, sizeof(diag));
    diag.totalPixels = width * height;
    diag.readbackOK = false;
    if (textureID == 0 || width <= 0 || height <= 0) return false;

    uint8_t* pixels = new (std::nothrow) uint8_t[width * height * 4];
    if (!pixels) return false;

    glBindTexture(GL_TEXTURE_2D, textureID);
    glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        delete[] pixels;
        return false;
    }

    diag.readbackOK = true;
    bool foundColored = false;
    for (int i = 0; i < width * height; i++) {
        uint8_t r = pixels[i*4+0], g = pixels[i*4+1], b = pixels[i*4+2], a = pixels[i*4+3];
        if (a < 13) {
            diag.transparentPixels++;
        } else if (r == 0 && g == 0 && b == 0) {
            diag.blackOpaquePixels++;
        } else {
            diag.coloredOpaquePixels++;
            if (!foundColored) {
                diag.firstColorR = r; diag.firstColorG = g;
                diag.firstColorB = b; diag.firstColorA = a;
                foundColored = true;
            }
        }
    }

    delete[] pixels;
    return true;
}

void CGPURenderer::SetNativeResolution(int width, int height)
{
    m_renderConfig.nativeWidth = width;
    m_renderConfig.nativeHeight = height;
    m_renderConfig.scaleX = (float)width / (float)m_renderConfig.virtualWidth;
    m_renderConfig.scaleY = (float)height / (float)m_renderConfig.virtualHeight;

    // Letterbox: use uniform scale to preserve 4:3 aspect ratio
    float scale = (m_renderConfig.scaleX < m_renderConfig.scaleY)
                  ? m_renderConfig.scaleX : m_renderConfig.scaleY;
    m_renderConfig.uniformScale = scale;
    m_renderConfig.viewportW = (int)(m_renderConfig.virtualWidth * scale);
    m_renderConfig.viewportH = (int)(m_renderConfig.virtualHeight * scale);
    m_renderConfig.viewportX = (width - m_renderConfig.viewportW) / 2;
    m_renderConfig.viewportY = (height - m_renderConfig.viewportH) / 2;

    UpdateProjectionMatrix();
}

void CGPURenderer::OnResize(int newWidth, int newHeight)
{
    if (!m_bInitialized) return;
    if (newWidth <= 0 || newHeight <= 0) return;
    SetNativeResolution(newWidth, newHeight);
}

void CGPURenderer::VirtualToNative(int vx, int vy, int& nx, int& ny)
{
    nx = (int)(vx * m_renderConfig.uniformScale) + m_renderConfig.viewportX;
    ny = (int)(vy * m_renderConfig.uniformScale) + m_renderConfig.viewportY;
}

void CGPURenderer::NativeToVirtual(int nx, int ny, int& vx, int& vy)
{
    vx = (int)((nx - m_renderConfig.viewportX) / m_renderConfig.uniformScale);
    vy = (int)((ny - m_renderConfig.viewportY) / m_renderConfig.uniformScale);
}

void CGPURenderer::Clear(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT);
}

bool CGPURenderer::SaveScreenshot(const char* filename)
{
    if (!m_bInitialized) return false;

    int w = m_renderConfig.viewportW;
    int h = m_renderConfig.viewportH;
    int x = m_renderConfig.viewportX;
    int y = m_renderConfig.viewportY;

    // Read pixels from the OpenGL front buffer (what's currently displayed)
    uint8_t* pixels = new uint8_t[w * h * 3];
    glReadBuffer(GL_FRONT);
    glReadPixels(x, y, w, h, GL_BGR_EXT, GL_UNSIGNED_BYTE, pixels);

    // Write BMP file (bottom-up, matches OpenGL's bottom-left origin)
    FILE* fp = fopen(filename, "wb");
    if (!fp) { delete[] pixels; return false; }

    BITMAPFILEHEADER bfh;
    BITMAPINFOHEADER bih;
    int rowBytes = ((w * 3 + 3) & ~3); // Pad rows to 4-byte boundary
    int imageSize = rowBytes * h;

    ZeroMemory(&bfh, sizeof(bfh));
    bfh.bfType = 0x4D42; // 'BM'
    bfh.bfSize = sizeof(bfh) + sizeof(bih) + imageSize;
    bfh.bfOffBits = sizeof(bfh) + sizeof(bih);

    ZeroMemory(&bih, sizeof(bih));
    bih.biSize = sizeof(bih);
    bih.biWidth = w;
    bih.biHeight = h; // positive = bottom-up (matches GL)
    bih.biPlanes = 1;
    bih.biBitCount = 24;
    bih.biCompression = BI_RGB;
    bih.biSizeImage = imageSize;

    fwrite(&bfh, sizeof(bfh), 1, fp);
    fwrite(&bih, sizeof(bih), 1, fp);

    // Write rows with padding
    for (int row = 0; row < h; row++) {
        fwrite(pixels + row * w * 3, w * 3, 1, fp);
        // Pad to 4-byte boundary
        int pad = rowBytes - w * 3;
        if (pad > 0) { uint8_t zeros[4] = {0}; fwrite(zeros, pad, 1, fp); }
    }

    fclose(fp);
    delete[] pixels;
    return true;
}

//////////////////////////////////////////////////////////////////////
// Font Atlas and Text Rendering
//////////////////////////////////////////////////////////////////////

bool CGPURenderer::InitFontAtlas(HFONT hFont)
{
    if (hFont == NULL) {
        OutputDebugString("GPURenderer: InitFontAtlas called with NULL font\n");
        return false;
    }

    // Create a memory DC for rendering
    HDC hDC = CreateCompatibleDC(NULL);
    if (hDC == NULL) return false;

    HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

    // Get text metrics
    TEXTMETRIC tm;
    GetTextMetrics(hDC, &tm);
    m_fontCharHeight = tm.tmHeight;

    // Get character widths for all chars 0-255
    int charWidths[256];
    memset(charWidths, 0, sizeof(charWidths));
    GetCharWidth32(hDC, 0, 255, charWidths);

    // Atlas dimensions
    int atlasWidth = 512;
    int atlasHeight = 512;

    // Create a DIB section for GDI rendering
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = atlasWidth;
    bmi.bmiHeader.biHeight = -atlasHeight; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pvBits = NULL;
    HBITMAP hBitmap = CreateDIBSection(hDC, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    if (hBitmap == NULL) {
        SelectObject(hDC, hOldFont);
        DeleteDC(hDC);
        return false;
    }

    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hDC, hBitmap);

    // Clear to black
    RECT rc = {0, 0, atlasWidth, atlasHeight};
    FillRect(hDC, &rc, (HBRUSH)GetStockObject(BLACK_BRUSH));

    // Set up text rendering - white text on transparent background
    SetBkMode(hDC, TRANSPARENT);
    SetTextColor(hDC, RGB(255, 255, 255));
    SelectObject(hDC, hFont);

    // Initialize char metrics
    memset(m_charMetrics, 0, sizeof(m_charMetrics));

    // Render each printable character into the atlas
    int curX = 1, curY = 1;
    int maxRowHeight = 0;

    for (int c = 32; c < 256; c++) {
        int w = charWidths[c];
        if (w <= 0) w = 8;

        // Wrap to next row if needed
        if (curX + w + 1 >= atlasWidth) {
            curX = 1;
            curY += maxRowHeight + 1;
            maxRowHeight = 0;
        }

        if (curY + m_fontCharHeight >= atlasHeight) break;

        // Store metrics
        m_charMetrics[c].x = curX;
        m_charMetrics[c].y = curY;
        m_charMetrics[c].width = w;
        m_charMetrics[c].height = m_fontCharHeight;

        // Render the character
        char ch = (char)c;
        ::TextOutA(hDC, curX, curY, &ch, 1);

        curX += w + 1;
        if (m_fontCharHeight > maxRowHeight) maxRowHeight = m_fontCharHeight;
    }

    // Ensure space character has a valid width
    if (m_charMetrics[' '].width <= 0) {
        m_charMetrics[' '].width = charWidths[' '];
        if (m_charMetrics[' '].width <= 0) m_charMetrics[' '].width = 4;
    }

    // Convert DIB to RGBA for OpenGL
    // GDI renders white text on black - use max RGB channel as alpha
    uint8_t* rgbaData = new uint8_t[atlasWidth * atlasHeight * 4];
    uint8_t* srcBits = (uint8_t*)pvBits;

    for (int i = 0; i < atlasWidth * atlasHeight; i++) {
        uint8_t b = srcBits[i * 4 + 0];
        uint8_t g = srcBits[i * 4 + 1];
        uint8_t r = srcBits[i * 4 + 2];

        // Derive alpha from brightest channel (GDI may not write alpha)
        uint8_t alpha = r;
        if (g > alpha) alpha = g;
        if (b > alpha) alpha = b;

        rgbaData[i * 4 + 0] = 255;
        rgbaData[i * 4 + 1] = 255;
        rgbaData[i * 4 + 2] = 255;
        rgbaData[i * 4 + 3] = alpha;
    }

    // Create the OpenGL texture
    m_fontAtlasTexture = CreateTexture(rgbaData, atlasWidth, atlasHeight);
    m_fontAtlasWidth = atlasWidth;
    m_fontAtlasHeight = atlasHeight;
    m_bFontAtlasReady = (m_fontAtlasTexture != 0);

    // Cleanup
    delete[] rgbaData;
    SelectObject(hDC, hOldBitmap);
    SelectObject(hDC, hOldFont);
    DeleteObject(hBitmap);
    DeleteDC(hDC);

    if (m_bFontAtlasReady) {
        char dbg[128];
        sprintf(dbg, "GPURenderer: Font atlas created (%dx%d, charHeight=%d)\n",
                atlasWidth, atlasHeight, m_fontCharHeight);
        OutputDebugString(dbg);
    } else {
        OutputDebugString("GPURenderer: Font atlas creation failed\n");
    }

    return m_bFontAtlasReady;
}

void CGPURenderer::RenderText(int x, int y, const char* text, COLORREF color)
{
    if (!m_bFontAtlasReady || !m_bInitialized || text == NULL) return;

    float r = GetRValue(color) / 255.0f;
    float g = GetGValue(color) / 255.0f;
    float b = GetBValue(color) / 255.0f;

    int curX = x;
    while (*text) {
        unsigned char c = (unsigned char)*text;
        if (c >= 32 && c < 256 && m_charMetrics[c].width > 0) {
            const CharMetrics& cm = m_charMetrics[c];
            QueueSprite(m_fontAtlasTexture, curX, y,
                        cm.x, cm.y, cm.width, cm.height,
                        m_fontAtlasWidth, m_fontAtlasHeight, 1,
                        BLEND_TEXT, 1.0f, r, g, b);
            curX += cm.width;
        }
        text++;
    }
}

void CGPURenderer::RenderTextRect(RECT* pRect, const char* text, COLORREF color)
{
    if (!m_bFontAtlasReady || !m_bInitialized || text == NULL || pRect == NULL) return;

    float r = GetRValue(color) / 255.0f;
    float g = GetGValue(color) / 255.0f;
    float b = GetBValue(color) / 255.0f;

    int rectWidth = pRect->right - pRect->left;
    int curY = pRect->top;
    int len = (int)strlen(text);
    int lineStart = 0;

    while (lineStart < len) {
        // Find how many characters fit on this line (word-wrap)
        int lineEnd = lineStart;
        int lastSpace = -1;
        int lineWidth = 0;

        while (lineEnd < len && text[lineEnd] != '\n') {
            if (text[lineEnd] == ' ') lastSpace = lineEnd;

            unsigned char c = (unsigned char)text[lineEnd];
            int charW = (c < 256) ? m_charMetrics[c].width : 8;

            if (lineWidth + charW > rectWidth && lineEnd > lineStart) {
                if (lastSpace > lineStart) {
                    lineEnd = lastSpace;
                }
                break;
            }

            lineWidth += charW;
            lineEnd++;
        }

        // Calculate actual width of this line for centering
        int actualWidth = GetTextWidth(text + lineStart, lineEnd - lineStart);
        int centerX = pRect->left + (rectWidth - actualWidth) / 2;

        // Render this line
        int curX = centerX;
        for (int i = lineStart; i < lineEnd; i++) {
            unsigned char c = (unsigned char)text[i];
            if (c >= 32 && c < 256 && m_charMetrics[c].width > 0) {
                const CharMetrics& cm = m_charMetrics[c];
                QueueSprite(m_fontAtlasTexture, curX, curY,
                            cm.x, cm.y, cm.width, cm.height,
                            m_fontAtlasWidth, m_fontAtlasHeight, 1,
                            BLEND_TEXT, 1.0f, r, g, b);
                curX += cm.width;
            }
        }

        curY += m_fontCharHeight;

        // Skip past line break or trailing space
        if (lineEnd < len && (text[lineEnd] == '\n' || text[lineEnd] == ' '))
            lineEnd++;
        lineStart = lineEnd;
    }
}

int CGPURenderer::GetTextWidth(const char* text, int len)
{
    if (!m_bFontAtlasReady || text == NULL) return 0;
    if (len < 0) len = (int)strlen(text);

    int width = 0;
    for (int i = 0; i < len; i++) {
        unsigned char c = (unsigned char)text[i];
        if (c < 256) width += m_charMetrics[c].width;
    }
    return width;
}

//////////////////////////////////////////////////////////////////////
// Map Background Texture
//////////////////////////////////////////////////////////////////////

void CGPURenderer::UpdateMapTexture(const uint8_t* surfaceData, int pitch, int width, int height,
                                     int bitDepth, DWORD dwRMask, DWORD dwGMask, DWORD dwBMask)
{
    if (!m_bInitialized || surfaceData == NULL || width <= 0 || height <= 0) return;

    // Allocate or reallocate RGBA conversion buffer if size changed
    if (m_mapTexWidth != width || m_mapTexHeight != height) {
        if (m_pMapRGBABuffer) delete[] m_pMapRGBABuffer;
        m_pMapRGBABuffer = new uint8_t[width * height * 4];
        m_mapTexWidth = width;
        m_mapTexHeight = height;

        // Create or recreate GL texture
        if (m_mapTexture) glDeleteTextures(1, &m_mapTexture);
        glGenTextures(1, &m_mapTexture);
        glBindTexture(GL_TEXTURE_2D, m_mapTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
                     GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    // Convert surface pixels to RGBA based on actual bit depth
    if (bitDepth == 32) {
        // 32-bit surface (typical on modern Windows with DDSCL_NORMAL)
        // Upload BGRA directly to GPU - no CPU conversion needed
        glBindTexture(GL_TEXTURE_2D, m_mapTexture);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, pitch / 4);  // Handle DD pitch alignment
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                        GL_BGRA, GL_UNSIGNED_BYTE, surfaceData);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);  // Reset
        return;
    } else if (bitDepth == 16) {
        // 16-bit surface (RGB565 or RGB555)
        bool isRGB565 = (dwRMask == 0xF800);
        for (int y = 0; y < height; y++) {
            const uint16_t* srcRow = (const uint16_t*)(surfaceData + y * pitch);
            uint8_t* dstRow = m_pMapRGBABuffer + y * width * 4;
            for (int x = 0; x < width; x++) {
                uint16_t pixel = srcRow[x];
                if (isRGB565) {
                    uint8_t r5 = (pixel >> 11) & 0x1F;
                    uint8_t g6 = (pixel >> 5) & 0x3F;
                    uint8_t b5 = pixel & 0x1F;
                    dstRow[x * 4 + 0] = (r5 << 3) | (r5 >> 2);
                    dstRow[x * 4 + 1] = (g6 << 2) | (g6 >> 4);
                    dstRow[x * 4 + 2] = (b5 << 3) | (b5 >> 2);
                } else {
                    uint8_t r5 = (pixel >> 10) & 0x1F;
                    uint8_t g5 = (pixel >> 5) & 0x1F;
                    uint8_t b5 = pixel & 0x1F;
                    dstRow[x * 4 + 0] = (r5 << 3) | (r5 >> 2);
                    dstRow[x * 4 + 1] = (g5 << 3) | (g5 >> 2);
                    dstRow[x * 4 + 2] = (b5 << 3) | (b5 >> 2);
                }
                dstRow[x * 4 + 3] = 255;
            }
        }
    } else if (bitDepth == 24) {
        // 24-bit surface (unlikely but handle it)
        for (int y = 0; y < height; y++) {
            const uint8_t* srcRow = surfaceData + y * pitch;
            uint8_t* dstRow = m_pMapRGBABuffer + y * width * 4;
            for (int x = 0; x < width; x++) {
                dstRow[x * 4 + 0] = srcRow[x * 3 + 2]; // R
                dstRow[x * 4 + 1] = srcRow[x * 3 + 1]; // G
                dstRow[x * 4 + 2] = srcRow[x * 3 + 0]; // B
                dstRow[x * 4 + 3] = 255;
            }
        }
    } else {
        // Unknown format - fill black
        memset(m_pMapRGBABuffer, 0, width * height * 4);
    }

    // Upload to GPU
    glBindTexture(GL_TEXTURE_2D, m_mapTexture);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                    GL_RGBA, GL_UNSIGNED_BYTE, m_pMapRGBABuffer);
}

void CGPURenderer::RenderMapQuad(int srcX, int srcY, int srcW, int srcH)
{
    if (!m_bInitialized || m_mapTexture == 0) return;

    // Render the map texture region as a full-screen background quad
    // BLEND_OPAQUE: force alpha=1.0, no blending (background is always fully opaque)
    QueueSprite(m_mapTexture, 0, 0,
                srcX, srcY, srcW, srcH,
                m_mapTexWidth, m_mapTexHeight, 1,
                BLEND_OPAQUE, 1.0f, 0.0f, 0.0f, 0.0f);
}

//////////////////////////////////////////////////////////////////////
// Helper functions for color conversion
//////////////////////////////////////////////////////////////////////

void ConvertRGB565ToRGBA(const uint16_t* src, uint8_t* dst,
                          int width, int height, uint16_t colorKey)
{
    int totalPixels = width * height;
    for (int i = 0; i < totalPixels; i++) {
        uint16_t pixel = src[i];
        if (pixel == colorKey) {
            // Transparent pixel
            dst[i * 4 + 0] = 0;
            dst[i * 4 + 1] = 0;
            dst[i * 4 + 2] = 0;
            dst[i * 4 + 3] = 0;  // Fully transparent
        } else {
            // RGB565 to RGBA8888
            // R: bits 15-11 (5 bits) -> 8 bits
            // G: bits 10-5 (6 bits) -> 8 bits
            // B: bits 4-0 (5 bits) -> 8 bits
            uint8_t r5 = (pixel >> 11) & 0x1F;
            uint8_t g6 = (pixel >> 5) & 0x3F;
            uint8_t b5 = pixel & 0x1F;

            // Expand to 8-bit (replicate high bits into low bits for accuracy)
            dst[i * 4 + 0] = (r5 << 3) | (r5 >> 2);
            dst[i * 4 + 1] = (g6 << 2) | (g6 >> 4);
            dst[i * 4 + 2] = (b5 << 3) | (b5 >> 2);
            dst[i * 4 + 3] = 255;  // Fully opaque
        }
    }
}

void ConvertRGB555ToRGBA(const uint16_t* src, uint8_t* dst,
                          int width, int height, uint16_t colorKey)
{
    int totalPixels = width * height;
    for (int i = 0; i < totalPixels; i++) {
        uint16_t pixel = src[i];
        if (pixel == colorKey) {
            // Transparent pixel
            dst[i * 4 + 0] = 0;
            dst[i * 4 + 1] = 0;
            dst[i * 4 + 2] = 0;
            dst[i * 4 + 3] = 0;  // Fully transparent
        } else {
            // RGB555 to RGBA8888
            // R: bits 14-10 (5 bits) -> 8 bits
            // G: bits 9-5 (5 bits) -> 8 bits
            // B: bits 4-0 (5 bits) -> 8 bits
            uint8_t r5 = (pixel >> 10) & 0x1F;
            uint8_t g5 = (pixel >> 5) & 0x1F;
            uint8_t b5 = pixel & 0x1F;

            // Expand to 8-bit
            dst[i * 4 + 0] = (r5 << 3) | (r5 >> 2);
            dst[i * 4 + 1] = (g5 << 3) | (g5 >> 2);
            dst[i * 4 + 2] = (b5 << 3) | (b5 >> 2);
            dst[i * 4 + 3] = 255;  // Fully opaque
        }
    }
}

// ShaderManager.h: GLSL shader management
//
//////////////////////////////////////////////////////////////////////

#ifndef SHADER_MANAGER_H
#define SHADER_MANAGER_H

#include <windows.h>
#include <string>
#include <unordered_map>
#include "..\include\GL\glew.h"

class CShaderManager {
public:
    void* operator new(size_t size) {
        return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    }

    void operator delete(void* mem) {
        HeapFree(GetProcessHeap(), HEAP_NO_SERIALIZE, mem);
    }

    CShaderManager();
    ~CShaderManager();

    // Load shader from file
    GLuint LoadShaderFromFile(const char* vertexPath, const char* fragmentPath);

    // Load shader from source strings
    GLuint LoadShaderFromSource(const char* vertexSource, const char* fragmentSource);

    // Get cached shader program
    GLuint GetProgram(const std::string& name);

    // Cache a shader program with a name
    void CacheProgram(const std::string& name, GLuint program);

    // Delete all cached programs
    void ClearCache();

    // Utility functions
    static bool CompileShader(GLuint shader, const char* source);
    static bool LinkProgram(GLuint program);
    static std::string ReadFile(const char* filepath);

private:
    std::unordered_map<std::string, GLuint> m_programCache;
};

#endif // SHADER_MANAGER_H

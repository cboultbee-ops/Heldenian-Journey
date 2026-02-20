// ShaderManager.cpp: GLSL shader management implementation
//
//////////////////////////////////////////////////////////////////////

#include "ShaderManager.h"
#include <fstream>
#include <sstream>
#include <cstdio>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CShaderManager::CShaderManager()
{
}

CShaderManager::~CShaderManager()
{
    ClearCache();
}

GLuint CShaderManager::LoadShaderFromFile(const char* vertexPath, const char* fragmentPath)
{
    std::string vertexSource = ReadFile(vertexPath);
    std::string fragmentSource = ReadFile(fragmentPath);

    if (vertexSource.empty() || fragmentSource.empty()) {
        OutputDebugString("ShaderManager: Failed to read shader files\n");
        return 0;
    }

    return LoadShaderFromSource(vertexSource.c_str(), fragmentSource.c_str());
}

GLuint CShaderManager::LoadShaderFromSource(const char* vertexSource, const char* fragmentSource)
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    if (!CompileShader(vertexShader, vertexSource)) {
        glDeleteShader(vertexShader);
        return 0;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    if (!CompileShader(fragmentShader, fragmentSource)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        return 0;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    if (!LinkProgram(program)) {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        glDeleteProgram(program);
        return 0;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return program;
}

GLuint CShaderManager::GetProgram(const std::string& name)
{
    auto it = m_programCache.find(name);
    if (it != m_programCache.end()) {
        return it->second;
    }
    return 0;
}

void CShaderManager::CacheProgram(const std::string& name, GLuint program)
{
    // Delete old program if exists
    auto it = m_programCache.find(name);
    if (it != m_programCache.end()) {
        glDeleteProgram(it->second);
    }
    m_programCache[name] = program;
}

void CShaderManager::ClearCache()
{
    for (auto& pair : m_programCache) {
        if (pair.second != 0) {
            glDeleteProgram(pair.second);
        }
    }
    m_programCache.clear();
}

bool CShaderManager::CompileShader(GLuint shader, const char* source)
{
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        OutputDebugString("Shader compilation error: ");
        OutputDebugString(infoLog);
        OutputDebugString("\n");
        return false;
    }

    return true;
}

bool CShaderManager::LinkProgram(GLuint program)
{
    glLinkProgram(program);

    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);

    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        OutputDebugString("Shader program link error: ");
        OutputDebugString(infoLog);
        OutputDebugString("\n");
        return false;
    }

    return true;
}

std::string CShaderManager::ReadFile(const char* filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        char msg[256];
        sprintf(msg, "Failed to open shader file: %s\n", filepath);
        OutputDebugString(msg);
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

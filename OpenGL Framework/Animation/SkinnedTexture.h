
#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <stb_image.h>

#include <iostream>

using namespace std;

class SkinnedTexture
{
public:
    SkinnedTexture(GLenum TextureTarget, const std::string & fileName)
    {
        m_textureTarget = TextureTarget;
        m_fileName = fileName;
    }

    bool Load()
    {
        m_textureObj = SkinnedTexture_TextureFromFile(m_fileName.c_str(), true);

        return true;
    }

    void Bind(GLenum TextureUnit)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_textureObj);
    }

    unsigned int SkinnedTexture_TextureFromFile(const char* path, bool gamma)
    {
        unsigned int textureID = 0;
        glGenTextures(1, &textureID);

        int width, height, nrComponents;
        unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
        if (data)
        {
            GLenum format;
            if (nrComponents == 1)
            {
                format = GL_RED;
            }
            else if (nrComponents == 3)
            {
                format = GL_RGB;
            }
            else if (nrComponents == 4)
            {
                format = GL_RGBA;
            }

            glBindTexture(m_textureTarget, textureID);
            glTexImage2D(m_textureTarget, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(m_textureTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(m_textureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(m_textureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            stbi_image_free(data);
        }
        else
        {
            std::cout << "texture failed to load at path:" << path << std::endl;
        }

        return textureID;
    }

private:
    std::string m_fileName;
    GLenum m_textureTarget;
    GLuint m_textureObj;
};
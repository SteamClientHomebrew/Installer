/**
 * ==================================================
 *   _____ _ _ _             _
 *  |     |_| | |___ ___ ___|_|_ _ _____
 *  | | | | | | | -_|   |   | | | |     |
 *  |_|_|_|_|_|_|___|_|_|_|_|_|___|_|_|_|
 *
 * ==================================================
 *
 * Copyright (c) 2025 Project Millennium
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <texture.h>
#include <memory.h>
#include <vector>
#include <iostream>
#include <imgui.h>

GLuint logoTexture;
GLuint infoIconTexture;
GLuint closeButtonTexture;
GLuint discordIconTexture;
GLuint gtihubIconTexture;
GLuint backBtnTexture;
GLuint excludedIconTexture;
GLuint errorIconTexture;
GLuint successIconTexture;

bool LoadTextureFromMemory(const void* data, size_t data_size, GLuint* out_texture, int* out_width, int* out_height)
{
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL) {
        return false;
    }

    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    if (out_width) {
        *out_width = image_width;
    }

    if (out_height) {
        *out_height = image_height;
    }
    return true;
}

void LoadTextures()
{
    LoadTextureFromMemory(logo, sizeof(logo), &logoTexture);
    LoadTextureFromMemory(infoIcon, sizeof(infoIcon), &infoIconTexture);
    LoadTextureFromMemory(closeBtn, sizeof(closeBtn), &closeButtonTexture);
    LoadTextureFromMemory(discordIcon, sizeof(discordIcon), &discordIconTexture);
    LoadTextureFromMemory(githubIcon, sizeof(githubIcon), &gtihubIconTexture);
    LoadTextureFromMemory(backBtn, sizeof(backBtn), &backBtnTexture);
    LoadTextureFromMemory(excludedIcon, sizeof(excludedIcon), &excludedIconTexture);
    LoadTextureFromMemory(errorIcon, sizeof(errorIcon), &errorIconTexture);
    LoadTextureFromMemory(successIcon, sizeof(successIcon), &successIconTexture);
}

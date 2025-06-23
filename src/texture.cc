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
#include <SDL_render.h>

SDL_Texture* logoTexture;
SDL_Texture* infoIconTexture;
SDL_Texture* closeButtonTexture;
SDL_Texture* discordIconTexture;
SDL_Texture* gtihubIconTexture;
SDL_Texture* backBtnTexture;
SDL_Texture* excludedIconTexture;
SDL_Texture* errorIconTexture;
SDL_Texture* successIconTexture;

bool LoadTextureFromMemory(SDL_Renderer* renderer, const void* data, size_t data_size, SDL_Texture** out_texture)
{
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load_from_memory((const unsigned char*)data, (int)data_size, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
    {
        return false;
    }
    
    // Create SDL surface from image data
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
        image_data,
        image_width,
        image_height,
        32,
        image_width * 4,
        SDL_PIXELFORMAT_RGBA32
    );
    
    if (surface == NULL)
    {
        stbi_image_free(image_data);
        return false;
    }
    
    // Create texture from surface
    SDL_Texture* image_texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    stbi_image_free(image_data);
    
    if (image_texture == NULL)
    {
        return false;
    }
    
    // Set blend mode for proper alpha handling
    SDL_SetTextureBlendMode(image_texture, SDL_BLENDMODE_BLEND);
    
    *out_texture = image_texture;
    return true;
}

void LoadTextures(SDL_Renderer* renderer)
{
    LoadTextureFromMemory(renderer, logo, sizeof(logo), &logoTexture);
    LoadTextureFromMemory(renderer, infoIcon, sizeof(infoIcon), &infoIconTexture);
    LoadTextureFromMemory(renderer, closeBtn, sizeof(closeBtn), &closeButtonTexture);
    LoadTextureFromMemory(renderer, discordIcon, sizeof(discordIcon), &discordIconTexture);
    LoadTextureFromMemory(renderer, githubIcon, sizeof(githubIcon), &gtihubIconTexture);
    LoadTextureFromMemory(renderer, backBtn, sizeof(backBtn), &backBtnTexture);
    LoadTextureFromMemory(renderer, excludedIcon, sizeof(excludedIcon), &excludedIconTexture);
    LoadTextureFromMemory(renderer, errorIcon, sizeof(errorIcon), &errorIconTexture);
    LoadTextureFromMemory(renderer, successIcon, sizeof(successIcon), &successIconTexture);
}

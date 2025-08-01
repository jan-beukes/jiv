#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define INIT_WINDOW_HEIGHT 720

#define ERROR(fmt, ...) do { \
    fprintf(stderr, "\x1b[31mERROR\x1b[0m: " fmt "\n", ##__VA_ARGS__); \
    exit(1); \
} while (0)
#define LOG(fmt, ...) fprintf(stderr, "\x1b[96mINFO\x1b[0m: " fmt "\n", ##__VA_ARGS__)
#define WARN(fmt, ...) fprintf(stderr, "\x1b[33mWARNING\x1b[0m: " fmt "\n", ##__VA_ARGS__)

#define USAGE_TEXT "Usage: jiv [img]"

static struct {
    bool quit;
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Texture *img_texture;
} g_state;

void update_frame()
{
    int win_width, win_height;
    int tex_width = g_state.img_texture->w, tex_height = g_state.img_texture->h;
    SDL_GetWindowSize(g_state.window, &win_width, &win_height);

    SDL_SetRenderDrawColor(g_state.renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_state.renderer);

    // Scale and position texture
    float dst_height = (float)win_height;
    float dst_width = (float)win_height * (float)tex_width / tex_height;
    if (win_width < dst_width) {
        dst_width = (float)win_width;
        dst_height = (float)win_width * (float)tex_height / tex_width;
    }
    float dst_x = (win_width - dst_width) / 2.0f;
    float dst_y = (win_height - dst_height) / 2.0f;

    SDL_FRect dstrect = { dst_x, dst_y, dst_width, dst_height };

    SDL_RenderTexture(g_state.renderer, g_state.img_texture, NULL, &dstrect);
    SDL_RenderPresent(g_state.renderer);

}

int main(int argc, char *argv[])
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        ERROR("Could not init SDL");
    }
    
    if (argc < 2) {
        fprintf(stderr, USAGE_TEXT);
        return 1;
    }
    const char *path = argv[1];

    int w, h, nb_comps;
    Uint8 *data = stbi_load(path, &w, &h, &nb_comps, 4);
    if (!data) {
        ERROR("Could not load file '%s'", path);
    }

    int window_width = INIT_WINDOW_HEIGHT * w / h;
    if (!SDL_CreateWindowAndRenderer("jiv", window_width, INIT_WINDOW_HEIGHT,
                                     SDL_WINDOW_RESIZABLE, &g_state.window, &g_state.renderer)) {
        ERROR("Could not create window/renderer");
    }

    g_state.img_texture = SDL_CreateTexture(g_state.renderer, SDL_PIXELFORMAT_RGBA32,
                                            SDL_TEXTUREACCESS_STATIC, w, h);
    if (!g_state.img_texture) {
        ERROR("Could not create texture");
    }
    SDL_UpdateTexture(g_state.img_texture, NULL, data, 4 * w);
    stbi_image_free(data);

    g_state.quit = false;
    while (!g_state.quit) {

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_QUIT: g_state.quit = true;
                break;
                case SDL_EVENT_KEY_DOWN:
                    SDL_Keycode keycode = event.key.key;

                    if (keycode == SDLK_ESCAPE) g_state.quit = true;

                break;
                default:
            }
        }

        update_frame();

    }

    return 0;
}


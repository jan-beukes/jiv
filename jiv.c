#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL3/SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define INIT_WINDOW_HEIGHT 720
#define ZOOM_STEP 1.2f
#define MIN_SCALE 0.5f
#define MAX_SCALE 120.0f

#define ERROR(fmt, ...) do { \
    fprintf(stderr, "[\x1b[31mERROR\x1b[0m] " fmt "\n", ##__VA_ARGS__); \
    exit(1); \
} while (0)
#define LOG(fmt, ...) fprintf(stderr, "[\x1b[96mINFO\x1b[0m] " fmt "\n", ##__VA_ARGS__)
#define WARN(fmt, ...) fprintf(stderr, "[\x1b[33mWARNING\x1b[0m] " fmt "\n", ##__VA_ARGS__)

#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

#define USAGE_TEXT "Usage: jiv [img]"

typedef struct {
    SDL_FRect rect;
    float scale;
    SDL_Texture *texture;
} JIV_Image;

typedef struct {
    bool quit;
    bool button_down;

    SDL_Renderer *renderer;
    SDL_Window *window;
} JIV_State;

static JIV_State g_state = {
    .quit = false,
    .button_down = false,
};

static JIV_Image g_image;

void load_image(const char *path)
{
    int w, h, nb_comps;
    Uint8 *data = stbi_load(path, &w, &h, &nb_comps, 4);
    if (!data) {
        ERROR("Could not load file '%s'", path);
    }
    SDL_Texture *image_texture = SDL_CreateTexture(g_state.renderer, SDL_PIXELFORMAT_RGBA32,
                                            SDL_TEXTUREACCESS_STATIC, w, h);
    if (!image_texture) {
        ERROR("Could not create texture");
    }
    SDL_UpdateTexture(image_texture, NULL, data, 4 * w);
    stbi_image_free(data);

    // setup the current image
    int win_width, win_height;
    SDL_GetWindowSize(g_state.window, &win_width, &win_height);

    // Center image
    SDL_FRect rect;
    rect.h = (float)INIT_WINDOW_HEIGHT;
    rect.w = rect.h * (float)w / h;
    if (win_width < rect.w) {
        rect.w = (float)win_width;
        rect.h = (float)win_width * (float)h / w;
    }
    rect.x = (win_width - rect.w) / 2.0f;
    rect.y = (win_height - rect.h) / 2.0f;
    g_image = (JIV_Image){ rect, 1.0f, image_texture };
}

void update_frame()
{
    int win_width, win_height;
    SDL_GetWindowSize(g_state.window, &win_width, &win_height);

    SDL_SetRenderDrawColor(g_state.renderer, 0, 0, 0, 255);
    SDL_RenderClear(g_state.renderer);

    if (win_width >= g_image.rect.w)
        g_image.rect.x = MIN(MAX(0, g_image.rect.x), win_width - g_image.rect.w);
    else
        g_image.rect.x = MAX(MIN(0, g_image.rect.x), win_width - g_image.rect.w);

    if (win_height >= g_image.rect.h)
        g_image.rect.y = MIN(MAX(0, g_image.rect.y), win_height - g_image.rect.h);
    else
        g_image.rect.y = MAX(MIN(0, g_image.rect.y), win_height - g_image.rect.h);

    SDL_RenderTexture(g_state.renderer, g_image.texture, NULL, &g_image.rect);
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
    if (!stbi_info(path, &w, &h, &nb_comps)) {
        ERROR("Could not load file '%s'", path);
    }
    int window_width = INIT_WINDOW_HEIGHT * w / h;
    if (!SDL_CreateWindowAndRenderer("jiv", window_width, INIT_WINDOW_HEIGHT,
                                     SDL_WINDOW_RESIZABLE, &g_state.window, &g_state.renderer)) {
        ERROR("Could not create window/renderer");
    }
    
    // Actually load the image
    load_image(path);

    SDL_Cursor *move_cursor, *default_cursor;
    move_cursor = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_MOVE);
    default_cursor = SDL_GetDefaultCursor();
    assert(move_cursor != NULL && default_cursor != NULL);

    g_state.quit = false;
    while (!g_state.quit) {
        int win_width, win_height;
        SDL_GetWindowSize(g_state.window, &win_width, &win_height);

        float mouse_x, mouse_y;
        SDL_GetMouseState(&mouse_x, &mouse_y);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {

                case SDL_EVENT_QUIT: g_state.quit = true;
                break;

                case SDL_EVENT_WINDOW_RESIZED:
                    win_width = event.window.data1, win_height = event.window.data2;
                    g_image.rect.x = (win_width - g_image.rect.w) / 2.0f;
                    g_image.rect.y = (win_height - g_image.rect.h) / 2.0f;
                break;
                //---MOUSE EVENTS---
                case SDL_EVENT_MOUSE_BUTTON_DOWN:
                    g_state.button_down = true;
                    SDL_SetCursor(move_cursor);
                break;
                case SDL_EVENT_MOUSE_BUTTON_UP:
                    g_state.button_down = false;
                    SDL_SetCursor(default_cursor);
                break;
                case SDL_EVENT_MOUSE_MOTION:
                    if (g_state.button_down) {
                        g_image.rect.x += event.motion.xrel;
                        g_image.rect.y += event.motion.yrel;
                    }
                break;
                case SDL_EVENT_MOUSE_WHEEL:
                    float prev_x = (mouse_x - g_image.rect.x) / g_image.rect.w;
                    float prev_y = (mouse_y - g_image.rect.y) / g_image.rect.h;
                    // x = a - cb

                    if (event.wheel.y > 0.0f) {
                        if (g_image.scale < MAX_SCALE) {
                            g_image.scale *= ZOOM_STEP;
                            g_image.rect.w *= ZOOM_STEP;
                            g_image.rect.h *= ZOOM_STEP;
                        }
                    } else {
                        if (g_image.scale > MIN_SCALE) {
                            g_image.scale /= ZOOM_STEP;
                            g_image.rect.w /= ZOOM_STEP;
                            g_image.rect.h /= ZOOM_STEP;
                        }
                    }
                    g_image.rect.x = mouse_x - prev_x*g_image.rect.w;
                    g_image.rect.y = mouse_y - prev_y*g_image.rect.h;

                break;

                //---KEY EVENTS---
                case SDL_EVENT_KEY_DOWN:
                    SDL_Keycode keycode = event.key.key;

                    if (keycode == SDLK_ESCAPE) g_state.quit = true;
                    else if (keycode == SDLK_F) {
                        SDL_ScaleMode scale_mode;
                        SDL_GetTextureScaleMode(g_image.texture, &scale_mode);
                        scale_mode = scale_mode == SDL_SCALEMODE_LINEAR ? 
                            SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR;
                        SDL_SetTextureScaleMode(g_image.texture, scale_mode);
                    }
                break;
                default:
            }
        }

        update_frame();
    }

    return 0;
}


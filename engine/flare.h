// The C side of the engine of the port of Flare.
//
// It is included by `engine/engine.atm`, in a prelude, after the header
// of the runtime of Atmos. It holds what is not game logic, and what
// works per pixel:
//
//   - the driver: the window, the loop of logic frames, the events of
//     SDL, and the test mode;
//   - the generator of random numbers of the patched original;
//   - the trace;
//   - images: loading with a cache by path, and drawing of parts;
//   - fonts (TrueType) and texts;
//   - sounds and music.
//
// Values cross to Atmos as numbers: an image, a font, or a sound is an
// index in a table of this file, which lives until the program ends.
//
// The loop runs in logic frames, at 60 per second, as the original
// (`max_fps`). Each frame emits, in order:
//
//      :key.dn [name]          a key was pressed (name: SDL scancode name)
//      :key.up [name]          a key was released
//      :mouse.motion [x,y]     the mouse moved
//      :mouse.dn [x,y,but]     a button was pressed (1 left, 3 right)
//      :mouse.up [x,y,but]     a button was released
//      :quit                   the window was closed
//      :frame [n]              one logic frame (n from 0)
//      :draw                   to draw the screen
//
// Coordinates are those of the render, 1706x960 for a window of
// 1280x720, as the original computes them (`windowResizeInternal`). The
// render is a texture, scaled to the window when presented.
//
// The test mode takes the variables of the patched original (see
// `doc/fidelity.md`): FLARE_SEED, FLARE_FAST, FLARE_FRAMES, FLARE_INPUT,
// FLARE_TRACE, FLARE_SHOT, and also FLARE_MUTE (no audio).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_mixer.h>
#include <SDL2/SDL_ttf.h>

static SDL_Window*   FL_WIN = NULL;
static SDL_Renderer* FL_REN = NULL;
static SDL_Texture*  FL_TGT = NULL;     // the render, at FL_W x FL_H
static int           FL_W   = 1706;
static int           FL_H   = 960;
static int           FL_SND = 0;        // audio is open
static unsigned long FL_FRAME = 0;      // logic frames done
static float         FL_SCALE = 1.0f;   // render / window, as view_scaling

static int fl_int (AVal v) {
    return a_isnum(v) ? (int)a_dbl(v) : 0;
}

static const char* fl_cstr (AVal v) {
    return (v.type == A_STR) ? A_STR_OF(v)->s : "";
}

static const char* fl_env (const char* name) {
    const char* v = getenv(name);
    return (v != NULL && *v != '\0') ? v : NULL;
}

///////////////////////////////////////////////////////////////////////////////
// RANDOM: the generator of the patched original (`Bench.h`)
///////////////////////////////////////////////////////////////////////////////

static uint64_t FL_RND = 0;

// from 0 to 2^31-1, as rand() of glibc
static int fl_rand (void) {
    FL_RND = FL_RND * 6364136223846793005ULL + 1442695040888963407ULL;
    return (int)(FL_RND >> 33);
}

///////////////////////////////////////////////////////////////////////////////
// TRACE
///////////////////////////////////////////////////////////////////////////////

static FILE* FL_TRACE = NULL;

// one line: the logic frame, then the text
static void fl_trace (const char* s) {
    if (FL_TRACE != NULL) {
        fprintf(FL_TRACE, "%lu %s\n", FL_FRAME, s);
        fflush(FL_TRACE);
    }
}

///////////////////////////////////////////////////////////////////////////////
// IMAGES
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    char*        path;
    SDL_Texture* tex;
    int          w, h;
} FlImg;

#define FL_IMGS_MAX 4096
static FlImg FL_IMGS[FL_IMGS_MAX];
static int   FL_IMGS_N = 0;

// The image of a path, loaded once. Returns its index, or -1.
static int fl_img_load (const char* path) {
    for (int i=0; i<FL_IMGS_N; i++) {
        if (strcmp(FL_IMGS[i].path, path) == 0) {
            return FL_IMGS[i].tex ? i : -1;
        }
    }
    if (FL_IMGS_N == FL_IMGS_MAX) {
        return -1;
    }
    FlImg* img = &FL_IMGS[FL_IMGS_N];
    img->path = strdup(path);
    img->tex = IMG_LoadTexture(FL_REN, path);
    img->w = img->h = 0;
    if (img->tex == NULL) {
        fprintf(stderr, "flare : image : %s : %s\n", path, IMG_GetError());
    } else {
        SDL_QueryTexture(img->tex, NULL, NULL, &img->w, &img->h);
        SDL_SetTextureBlendMode(img->tex, SDL_BLENDMODE_BLEND);
    }
    return (img->tex != NULL) ? FL_IMGS_N++ : (FL_IMGS_N++, -1);
}

static FlImg* fl_img (int i) {
    return (i >= 0 && i < FL_IMGS_N && FL_IMGS[i].tex) ? &FL_IMGS[i] : NULL;
}

// Draws the part `sx,sy,sw,sh` of an image at `dx,dy`, with the same
// size (or the whole image if `sw` is 0), with an alpha, a color mod
// (0xRRGGBB), and a blend mode (0 normal, 1 add).
static void fl_img_draw (int i, int sx, int sy, int sw, int sh, int dx, int dy,
                         int alpha, int color, int blend) {
    FlImg* img = fl_img(i);
    if (img == NULL) {
        return;
    }
    if (sw == 0) {
        sw = img->w;
        sh = img->h;
    }
    SDL_Rect src = { sx, sy, sw, sh };
    SDL_Rect dst = { dx, dy, sw, sh };
    SDL_SetTextureAlphaMod(img->tex, (Uint8)alpha);
    SDL_SetTextureColorMod(img->tex, (color>>16)&0xFF, (color>>8)&0xFF, color&0xFF);
    SDL_SetTextureBlendMode(img->tex, blend ? SDL_BLENDMODE_ADD : SDL_BLENDMODE_BLEND);
    SDL_RenderCopy(FL_REN, img->tex, &src, &dst);
}

///////////////////////////////////////////////////////////////////////////////
// SHAPES
///////////////////////////////////////////////////////////////////////////////

static void fl_color (int64_t rgba) {
    SDL_SetRenderDrawBlendMode(FL_REN, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(FL_REN, (rgba>>24)&0xFF, (rgba>>16)&0xFF, (rgba>>8)&0xFF, rgba&0xFF);
}

static void fl_fill (int64_t rgba, int x, int y, int w, int h) {
    SDL_Rect r = { x, y, w, h };
    fl_color(rgba);
    SDL_RenderFillRect(FL_REN, &r);
}

///////////////////////////////////////////////////////////////////////////////
// FONTS
///////////////////////////////////////////////////////////////////////////////

#define FL_FONTS_MAX 64
static TTF_Font* FL_FONTS[FL_FONTS_MAX];
static int       FL_FONTS_N = 0;

// A font of a path and a size, or -1.
static int fl_font_load (const char* path, int size) {
    if (FL_FONTS_N == FL_FONTS_MAX) {
        return -1;
    }
    TTF_Font* f = TTF_OpenFont(path, size);
    if (f == NULL) {
        fprintf(stderr, "flare : font : %s : %s\n", path, TTF_GetError());
        return -1;
    }
    FL_FONTS[FL_FONTS_N] = f;
    return FL_FONTS_N++;
}

static int fl_font_width (int fi, const char* s) {
    int w = 0, h = 0;
    if (fi >= 0 && fi < FL_FONTS_N && *s) {
        TTF_SizeUTF8(FL_FONTS[fi], s, &w, &h);
    }
    return w;
}

static int fl_font_height (int fi) {
    return (fi >= 0 && fi < FL_FONTS_N) ? TTF_FontLineSkip(FL_FONTS[fi]) : 0;
}

// Draws a text at x,y (its top-left), with a color 0xRRGGBB.
static void fl_font_draw (int fi, const char* s, int x, int y, int color) {
    if (fi < 0 || fi >= FL_FONTS_N || *s == '\0') {
        return;
    }
    SDL_Color c = { (color>>16)&0xFF, (color>>8)&0xFF, color&0xFF, 255 };
    SDL_Surface* sfc = TTF_RenderUTF8_Blended(FL_FONTS[fi], s, c);
    if (sfc == NULL) {
        return;
    }
    SDL_Texture* tex = SDL_CreateTextureFromSurface(FL_REN, sfc);
    SDL_Rect dst = { x, y, sfc->w, sfc->h };
    SDL_RenderCopy(FL_REN, tex, NULL, &dst);
    SDL_DestroyTexture(tex);
    SDL_FreeSurface(sfc);
}

///////////////////////////////////////////////////////////////////////////////
// SOUNDS AND MUSIC
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    char*      path;
    Mix_Chunk* chunk;
} FlSnd;

#define FL_SNDS_MAX 1024
static FlSnd FL_SNDS[FL_SNDS_MAX];
static int   FL_SNDS_N = 0;

// The sound of a path, loaded once: its index, or -1 (also with no audio).
static int fl_snd_load (const char* path) {
    if (!FL_SND) {
        return -1;
    }
    for (int i=0; i<FL_SNDS_N; i++) {
        if (strcmp(FL_SNDS[i].path, path) == 0) {
            return FL_SNDS[i].chunk ? i : -1;
        }
    }
    if (FL_SNDS_N == FL_SNDS_MAX) {
        return -1;
    }
    FL_SNDS[FL_SNDS_N].path = strdup(path);
    FL_SNDS[FL_SNDS_N].chunk = Mix_LoadWAV(path);
    return FL_SNDS[FL_SNDS_N].chunk ? FL_SNDS_N++ : (FL_SNDS_N++, -1);
}

// Plays a sound with a volume from 0 to 128.
static void fl_snd_play (int i, int volume) {
    if (FL_SND && i >= 0 && i < FL_SNDS_N && FL_SNDS[i].chunk) {
        int ch = Mix_PlayChannel(-1, FL_SNDS[i].chunk, 0);
        if (ch >= 0) {
            Mix_Volume(ch, volume);
        }
    }
}

static Mix_Music* FL_MUS = NULL;

static void fl_music_stop (void) {
    if (FL_MUS != NULL) {
        Mix_HaltMusic();
        Mix_FreeMusic(FL_MUS);
        FL_MUS = NULL;
    }
}

static void fl_music (const char* path) {
    if (!FL_SND) {
        return;
    }
    fl_music_stop();
    FL_MUS = Mix_LoadMUS(path);
    if (FL_MUS != NULL) {
        Mix_PlayMusic(FL_MUS, -1);
    }
}

///////////////////////////////////////////////////////////////////////////////
// DRIVER
///////////////////////////////////////////////////////////////////////////////

static AVal fl_field (AVal t, const char* k, AVal v) {
    AVal key = a_strz(k);
    a_setindex(t, key, v);
    a_dec(key);
    a_dec(v);
    return t;
}

// Emits `:tag`, `:tag [name]`, or `:tag [x,y,but]`, with the values at
// the positions 0, 1, 2 of the table.
static AStatus fl_emit (const char* tag, const char* name, int x, int y, int but) {
    AVal t = a_strz(tag);
    AVal evt = a_tab();
    a_tag(evt, t);
    if (name != NULL) {
        AVal v = a_strz(name);
        a_setindex(evt, a_int(0), v);
        a_dec(v);
    } else if (x >= 0) {
        a_setindex(evt, a_int(0), a_int(x));
        a_setindex(evt, a_int(1), a_int(y));
        if (but >= 0) {
            a_setindex(evt, a_int(2), a_int(but));
        }
    }
    AStatus st = atm_emit(evt);
    a_dec(evt);
    a_dec(t);
    return st;
}

// Saves the render as a BMP, as the patched original does.
static void fl_shot (const char* path) {
    SDL_Surface* sfc = SDL_CreateRGBSurfaceWithFormat(0, FL_W, FL_H, 32, SDL_PIXELFORMAT_ARGB8888);
    SDL_RenderReadPixels(FL_REN, NULL, SDL_PIXELFORMAT_ARGB8888, sfc->pixels, sfc->pitch);
    SDL_SaveBMP(sfc, path);
    SDL_FreeSurface(sfc);
}

// The scripted input (FLARE_INPUT): the lines of the frame that are
// due, as events, in the format of the patched original:
//      <frame> move <x> <y> | down <b> | up <b> | key <name> | keyup <name> | quit
static FILE* FL_INPUT = NULL;
static char  FL_INPUT_LINE[256];
static int   FL_INPUT_PEND = 0;
static int   FL_MX = 0, FL_MY = 0;

static AStatus fl_script (int* quit) {
    AStatus st = A_OK;
    while (FL_INPUT != NULL && st == A_OK) {
        if (!FL_INPUT_PEND) {
            if (fgets(FL_INPUT_LINE, sizeof(FL_INPUT_LINE), FL_INPUT) == NULL) {
                fclose(FL_INPUT);
                FL_INPUT = NULL;
                break;
            }
            if (FL_INPUT_LINE[0] == '#' || FL_INPUT_LINE[0] == '\n') {
                continue;
            }
            FL_INPUT_PEND = 1;
        }
        if (strtoul(FL_INPUT_LINE, NULL, 10) > FL_FRAME) {
            break;
        }
        FL_INPUT_PEND = 0;
        char cmd[32] = "", arg[64] = "";
        int b = 0;
        if (sscanf(FL_INPUT_LINE, "%*u %31s %63s %d", cmd, arg, &b) < 1) {
            continue;
        }
        int a = atoi(arg);
        if (!strcmp(cmd, "move")) {
            FL_MX = a; FL_MY = b;
            st = fl_emit("mouse.motion", NULL, FL_MX, FL_MY, -1);
        } else if (!strcmp(cmd, "down")) {
            st = fl_emit("mouse.dn", NULL, FL_MX, FL_MY, a);
        } else if (!strcmp(cmd, "up")) {
            st = fl_emit("mouse.up", NULL, FL_MX, FL_MY, a);
        } else if (!strcmp(cmd, "key")) {
            st = fl_emit("key.dn", arg, -1, -1, -1);
        } else if (!strcmp(cmd, "keyup")) {
            st = fl_emit("key.up", arg, -1, -1, -1);
        } else if (!strcmp(cmd, "quit")) {
            st = fl_emit("quit", NULL, -1, -1, -1);
            *quit = 1;
        }
    }
    return st;
}

int main (int argc, char** argv) {
    int mute = fl_env("FLARE_MUTE") != NULL;
    if (SDL_Init(SDL_INIT_VIDEO | (mute ? 0 : SDL_INIT_AUDIO)) != 0) {
        SDL_Init(SDL_INIT_VIDEO);
    }
    FL_SND = !mute && (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) == 0);
    if (FL_SND) {
        Mix_AllocateChannels(128);
    }
    TTF_Init();

    // the seed, as the patched original: FLARE_SEED, or the time
    FL_RND = fl_env("FLARE_SEED") ? strtoull(fl_env("FLARE_SEED"), NULL, 10)
                                  : (uint64_t)time(NULL);
    if (fl_env("FLARE_TRACE")) {
        FL_TRACE = strcmp(fl_env("FLARE_TRACE"), "-") ? fopen(fl_env("FLARE_TRACE"), "w") : stdout;
    }
    if (fl_env("FLARE_INPUT")) {
        FL_INPUT = fopen(fl_env("FLARE_INPUT"), "r");
    }
    unsigned long frames = fl_env("FLARE_FRAMES") ? strtoul(fl_env("FLARE_FRAMES"), NULL, 10) : 0;
    int fast = fl_env("FLARE_FAST") != NULL;
    long shot = -1;
    const char* shot_path = NULL;
    if (fl_env("FLARE_SHOT")) {
        shot = strtol(fl_env("FLARE_SHOT"), NULL, 10);
        shot_path = strchr(fl_env("FLARE_SHOT"), ':');
        shot_path = shot_path ? shot_path + 1 : NULL;
    }

    // the window and the render, as the original computes them
    int sw = 1280, sh = 720;
    FL_H = 960;
    FL_W = (int)((float)sw * ((float)FL_H / (float)sh));
    FL_WIN = SDL_CreateWindow("Flare", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, sw, sh, 0);
    if (FL_WIN != NULL) {
        FL_REN = SDL_CreateRenderer(FL_WIN, -1, SDL_RENDERER_TARGETTEXTURE);
    }
    if (FL_REN == NULL) {
        fprintf(stderr, "flare : %s\n", SDL_GetError());
        exit(1);
    }
    FL_SCALE = (float)FL_H / (float)sh;
    FL_TGT = SDL_CreateTexture(FL_REN, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, FL_W, FL_H);

    atm_init(argc, argv);
    AStatus st = atm_main();

    int quit = 0;
    const Uint32 period = 1000 / 60;
    while (st == A_OK && !quit && atm_alive()) {
        Uint32 t0 = SDL_GetTicks();
        SDL_Event e;
        while (st == A_OK && SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    st = fl_emit("quit", NULL, -1, -1, -1);
                    quit = 1;
                    break;
                case SDL_KEYDOWN:
                    if (!e.key.repeat) {
                        st = fl_emit("key.dn", SDL_GetScancodeName(e.key.keysym.scancode), -1, -1, -1);
                    }
                    break;
                case SDL_KEYUP:
                    st = fl_emit("key.up", SDL_GetScancodeName(e.key.keysym.scancode), -1, -1, -1);
                    break;
                // from the window to the render, as InputState::scaleMouse
                case SDL_MOUSEMOTION:
                    st = fl_emit("mouse.motion", NULL, (int)((float)e.motion.x * FL_SCALE),
                                 (int)((float)e.motion.y * FL_SCALE), -1);
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    st = fl_emit("mouse.dn", NULL, (int)((float)e.button.x * FL_SCALE),
                                 (int)((float)e.button.y * FL_SCALE), e.button.button);
                    break;
                case SDL_MOUSEBUTTONUP:
                    st = fl_emit("mouse.up", NULL, (int)((float)e.button.x * FL_SCALE),
                                 (int)((float)e.button.y * FL_SCALE), e.button.button);
                    break;
            }
        }
        if (st == A_OK && !quit) {
            st = fl_script(&quit);
        }

        // one logic frame
        if (st == A_OK && !quit) {
            AVal t = a_strz("frame");
            AVal evt = a_tab();
            a_tag(evt, t);
            a_setindex(evt, a_int(0), a_int((int64_t)FL_FRAME));
            st = atm_emit(evt);
            a_dec(evt);
            a_dec(t);
            FL_FRAME++;
            if (frames > 0 && FL_FRAME >= frames) {
                quit = 1;
            }
        }

        // the screen
        if (st == A_OK) {
            SDL_SetRenderTarget(FL_REN, FL_TGT);
            SDL_SetRenderDrawColor(FL_REN, 0, 0, 0, 255);
            SDL_RenderClear(FL_REN);
            st = fl_emit("draw", NULL, -1, -1, -1);
            if (shot_path && FL_FRAME == (unsigned long)shot) {
                fl_shot(shot_path);
            }
            SDL_SetRenderTarget(FL_REN, NULL);
            SDL_RenderCopy(FL_REN, FL_TGT, NULL, NULL);
            SDL_RenderPresent(FL_REN);
        }

        if (!fast) {
            Uint32 spent = SDL_GetTicks() - t0;
            if (spent < period) {
                SDL_Delay(period - spent);
            }
        }
    }

    if (st == A_OK) {
        st = atm_stop();
    }
    fflush(stdout);
    if (st == A_ESCAPE) {
        st = a_escape_err();
    }
    if (st != A_OK) {
        a_error_dump();
    }
    fprintf(stderr, "flare: frames %lu\n", FL_FRAME);
    atm_quit();
    fl_music_stop();
    if (FL_SND) {
        Mix_CloseAudio();
    }
    TTF_Quit();
    SDL_Quit();
    return (st == A_OK) ? 0 : 1;
}

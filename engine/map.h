// The map in C: the tileset, the layers, the camera, and the isometric
// drawing of tiles and objects, translated closely from MapRenderer,
// TileSet, Camera, and Utils of the original (`flare-engine/src`).
//
// It is included by `engine/flare.h`. Atmos reads the files of the map
// and of the tileset, and gives their contents to the functions here.
//
// Only the isometric orientation is done (fantasycore and both campaigns
// use it). Positions of the map are 32-bit floats, as in the original.

#include <math.h>

// the settings of fantasycore (engine/tileset_config.txt) and of the render
static int   FL_TILE_W = 192, FL_TILE_H = 96;
static float FL_UPP_X = 2.0f / 192.0f;  // units per pixel
static float FL_UPP_Y = 2.0f / 96.0f;

///////////////////////////////////////////////////////////////////////////////
// TILESET
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    int img;                // -1: no tile
    int x, y, w, h;         // the clip in the image
    int ox, oy;             // the offset
} FlTile;

typedef struct {
    int frames;
    int* x; int* y;         // the clip position of each frame
    int* dur;               // the duration of each frame, in logic frames
    int cur;                // the current frame
    int duration;           // logic frames since the last change
} FlTileAnim;

static FlTile*     FL_TILES = NULL;
static int         FL_TILES_N = 0;
static FlTileAnim* FL_TANIMS = NULL;
static int         FL_TANIMS_N = 0;
static int         FL_TMAX_X = 0, FL_TMAX_Y = 0;     // max_size_x, max_size_y

static void fl_tset_reset (void) {
    for (int i=0; i<FL_TANIMS_N; i++) {
        free(FL_TANIMS[i].x); free(FL_TANIMS[i].y); free(FL_TANIMS[i].dur);
    }
    free(FL_TANIMS); FL_TANIMS = NULL; FL_TANIMS_N = 0;
    free(FL_TILES); FL_TILES = NULL; FL_TILES_N = 0;
    FL_TMAX_X = FL_TMAX_Y = 0;
}

// tile=index,x,y,w,h,ox,oy of an image
static void fl_tset_tile (int i, int img, int x, int y, int w, int h, int ox, int oy) {
    if (i < 0) {
        return;
    }
    if (i >= FL_TILES_N) {
        FL_TILES = realloc(FL_TILES, sizeof(FlTile) * (i + 1));
        for (int k=FL_TILES_N; k<=i; k++) {
            FL_TILES[k].img = -1;
        }
        FL_TILES_N = i + 1;
    }
    FlTile t = { img, x, y, w, h, ox, oy };
    FL_TILES[i] = t;
}

// after all tiles: max_size_x and max_size_y, as TileSet::load
static void fl_tset_done (void) {
    for (int i=0; i<FL_TILES_N; i++) {
        if (FL_TILES[i].img >= 0) {
            if (FL_TILES[i].w / FL_TILE_W + 1 > FL_TMAX_X) FL_TMAX_X = FL_TILES[i].w / FL_TILE_W + 1;
            if (FL_TILES[i].h / FL_TILE_H + 1 > FL_TMAX_Y) FL_TMAX_Y = FL_TILES[i].h / FL_TILE_H + 1;
        }
    }
}

// one frame of the animation of a tile
static void fl_tset_anim (int i, int x, int y, int dur) {
    if (i < 0) {
        return;
    }
    if (i >= FL_TANIMS_N) {
        FL_TANIMS = realloc(FL_TANIMS, sizeof(FlTileAnim) * (i + 1));
        memset(&FL_TANIMS[FL_TANIMS_N], 0, sizeof(FlTileAnim) * (i + 1 - FL_TANIMS_N));
        FL_TANIMS_N = i + 1;
    }
    FlTileAnim* a = &FL_TANIMS[i];
    a->x = realloc(a->x, sizeof(int) * (a->frames + 1));
    a->y = realloc(a->y, sizeof(int) * (a->frames + 1));
    a->dur = realloc(a->dur, sizeof(int) * (a->frames + 1));
    a->x[a->frames] = x;
    a->y[a->frames] = y;
    a->dur[a->frames] = dur;
    a->frames++;
}

// TileSet::logic, once per logic frame
static void fl_tset_logic (void) {
    for (int i=0; i<FL_TANIMS_N; i++) {
        FlTileAnim* a = &FL_TANIMS[i];
        if (!a->frames) {
            continue;
        }
        if (i < FL_TILES_N && FL_TILES[i].img >= 0 && a->duration >= a->dur[a->cur]) {
            FL_TILES[i].x = a->x[a->cur];
            FL_TILES[i].y = a->y[a->cur];
            a->duration = 0;
            a->cur = (a->cur + 1) % a->frames;
        }
        a->duration++;
    }
}

static FlTile* fl_tile (int i) {
    return (i > 0 && i < FL_TILES_N && FL_TILES[i].img >= 0) ? &FL_TILES[i] : NULL;
}

///////////////////////////////////////////////////////////////////////////////
// LAYERS
///////////////////////////////////////////////////////////////////////////////

#define FL_LAYERS_MAX 32
static int             FL_MAP_W = 0, FL_MAP_H = 0;
static unsigned short* FL_LAYERS[FL_LAYERS_MAX];        // [x*h + y]
static char*           FL_LAYER_NAMES[FL_LAYERS_MAX];
static int             FL_LAYERS_N = 0;
static int             FL_OBJ_LAYER = 0;                // index_objectlayer
static unsigned short* FL_COLLISION = NULL;             // the collision layer
static unsigned char*  FL_DRAWN = NULL;                 // drawn_tiles
static int64_t         FL_BG = 0x000000FF;              // background_color

static void fl_map_free (void) {
    for (int i=0; i<FL_LAYERS_N; i++) {
        free(FL_LAYERS[i]);
        free(FL_LAYER_NAMES[i]);
    }
    FL_LAYERS_N = 0;
    free(FL_COLLISION); FL_COLLISION = NULL;
    free(FL_DRAWN); FL_DRAWN = NULL;
    FL_MAP_W = FL_MAP_H = 0;
    FL_OBJ_LAYER = 0;
}

static void fl_map_new (int w, int h) {
    fl_map_free();
    FL_MAP_W = w;
    FL_MAP_H = h;
    FL_DRAWN = calloc((size_t)w * h, 1);
}

// a new layer, as Map::loadLayer with `type=`: its index
static int fl_map_layer (const char* name) {
    if (FL_LAYERS_N == FL_LAYERS_MAX) {
        return -1;
    }
    FL_LAYERS[FL_LAYERS_N] = calloc((size_t)FL_MAP_W * FL_MAP_H, sizeof(unsigned short));
    FL_LAYER_NAMES[FL_LAYERS_N] = strdup(name);
    return FL_LAYERS_N++;
}

// a row of a layer: the comma-separated numbers of `data=`, as
// Parse::popFirstInt in Map::loadLayer. Returns 0 if the width is wrong.
static int fl_map_row (int l, int y, const char* s) {
    if (l < 0 || l >= FL_LAYERS_N || y < 0 || y >= FL_MAP_H) {
        return 0;
    }
    int commas = 0;
    for (const char* c=s; *c; c++) {
        if (*c == ',') commas++;
    }
    size_t n = strlen(s);
    if (n > 0 && s[n-1] != ',') {
        commas++;
    }
    if (commas != FL_MAP_W) {
        return 0;
    }
    const char* c = s;
    for (int x=0; x<FL_MAP_W; x++) {
        FL_LAYERS[l][x * FL_MAP_H + y] = (unsigned short)atoi(c);
        while (*c && *c != ',') c++;
        if (*c == ',') c++;
    }
    return 1;
}

// as MapRenderer::load after Map::load: the collision layer leaves the
// layers, the object layer is found, and tiles not in the tileset become 0
static void fl_map_done (void) {
    for (int i=0; i<FL_LAYERS_N; i++) {
        if (!strcmp(FL_LAYER_NAMES[i], "collision")) {
            free(FL_COLLISION);
            FL_COLLISION = FL_LAYERS[i];
            free(FL_LAYER_NAMES[i]);
            for (int k=i; k<FL_LAYERS_N-1; k++) {
                FL_LAYERS[k] = FL_LAYERS[k+1];
                FL_LAYER_NAMES[k] = FL_LAYER_NAMES[k+1];
            }
            FL_LAYERS_N--;
        }
    }
    if (FL_COLLISION == NULL) {
        FL_COLLISION = calloc((size_t)FL_MAP_W * FL_MAP_H, sizeof(unsigned short));
    }
    for (int i=0; i<FL_LAYERS_N; i++) {
        if (!strcmp(FL_LAYER_NAMES[i], "object")) {
            FL_OBJ_LAYER = i;
        }
    }
    for (int i=0; i<FL_LAYERS_N; i++) {
        for (int k=0; k<FL_MAP_W*FL_MAP_H; k++) {
            unsigned short t = FL_LAYERS[i][k];
            if (t > 0 && fl_tile(t) == NULL) {
                FL_LAYERS[i][k] = 0;
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
// CAMERA, as Camera of the original
///////////////////////////////////////////////////////////////////////////////

typedef struct { float x, y; } FlPt;

static float FL_CAM_SPEED = 10.0f;          // camera_speed of engine/misc.txt
static FlPt  FL_CAM, FL_SHAKE, FL_CAM_TGT, FL_CAM_PREV;
static float FL_CAM_PDX = 0, FL_CAM_PDY = 0;
static int   FL_SHAKE_TIMER = 0;            // the frames of shaking left
static int   FL_SHAKE_STRENGTH = 8;

static float fl_dist (FlPt a, FlPt b) {
    return sqrtf((b.x-a.x)*(b.x-a.x) + (b.y-a.y)*(b.y-a.y));
}

static void fl_cam_warp (float x, float y) {
    FlPt p = { x, y };
    FL_CAM = FL_SHAKE = FL_CAM_TGT = FL_CAM_PREV = p;
    FL_SHAKE_TIMER = 0;
    FL_CAM_PDX = FL_CAM_PDY = 0;
}

static void fl_cam_target (float x, float y) {
    FL_CAM_TGT.x = x;
    FL_CAM_TGT.y = y;
}

static void fl_cam_logic (void) {
    // cam_threshold = camera_speed / LOGIC_FPS / 50
    float threshold = FL_CAM_SPEED / 60.0f / 50.f;
    FlPt pos = FL_CAM, tgt = FL_CAM_TGT;
    float delta = fl_dist(pos, tgt);
    FlPt a = { pos.x, tgt.y }, b = { tgt.x, pos.y };
    float dx = fl_dist(a, tgt) / FL_CAM_SPEED;
    float dy = fl_dist(b, tgt) / FL_CAM_SPEED;
    if (FL_CAM_PREV.x == tgt.x && FL_CAM_PREV.y == tgt.y) {
        if (delta == 0 || delta >= threshold) {
            FL_CAM_PDX = dx;
            FL_CAM_PDY = dy;
        } else if (delta < threshold) {
            if (dx < FL_CAM_PDX || dy < FL_CAM_PDY) {
                dx = FL_CAM_PDX;
                dy = FL_CAM_PDY;
            } else {
                float bb = fabsf(pos.x - tgt.x);
                float alpha = acosf(bb / delta);
                float fast_dx = threshold * cosf(alpha);
                float fast_dy = threshold * sinf(alpha);
                FL_CAM_PDX = fast_dx / FL_CAM_SPEED;
                FL_CAM_PDY = fast_dy / FL_CAM_SPEED;
            }
        }
    } else {
        FL_CAM_PREV = tgt;
        FL_CAM_PDX = 0;
        FL_CAM_PDY = 0;
    }
    if (pos.x < tgt.x) {
        pos.x += dx;
        if (pos.x > tgt.x) pos.x = tgt.x;
    } else if (pos.x > tgt.x) {
        pos.x -= dx;
        if (pos.x < tgt.x) pos.x = tgt.x;
    }
    if (pos.y < tgt.y) {
        pos.y += dy;
        if (pos.y > tgt.y) pos.y = tgt.y;
    } else if (pos.y > tgt.y) {
        pos.y -= dy;
        if (pos.y < tgt.y) pos.y = tgt.y;
    }
    FL_CAM = pos;
    // the shaking, as a Timer: ticked, then tested
    if (FL_SHAKE_TIMER > 0) {
        FL_SHAKE_TIMER--;
    }
    if (FL_SHAKE_TIMER == 0) {
        FL_SHAKE = FL_CAM;
    } else {
        FL_SHAKE.x = FL_CAM.x + (float)((fl_rand() % (FL_SHAKE_STRENGTH * 2)) - FL_SHAKE_STRENGTH) * 0.0078125f;
        FL_SHAKE.y = FL_CAM.y + (float)((fl_rand() % (FL_SHAKE_STRENGTH * 2)) - FL_SHAKE_STRENGTH) * 0.0078125f;
    }
}

///////////////////////////////////////////////////////////////////////////////
// SCREEN AND MAP, as Utils::mapToScreen and Utils::screenToMap
///////////////////////////////////////////////////////////////////////////////

typedef struct { int x, y; } FlIPt;

static FlIPt fl_map2scr (float x, float y, float camx, float camy) {
    int vw_half = FL_W / 2, vh_half = FL_H / 2;
    float adjust_x = ((float)vw_half + 0.5f) * FL_UPP_X;
    float adjust_y = ((float)vh_half + 0.5f) * FL_UPP_Y;
    FlIPt r;
    r.x = (int)(floorf(((x - camx - y + camy + adjust_x) / FL_UPP_X) + 0.5f));
    r.y = (int)(floorf(((x - camx + y - camy + adjust_y) / FL_UPP_Y) + 0.5f));
    return r;
}

static FlPt fl_scr2map (int x, int y, float camx, float camy) {
    int vw_half = FL_W / 2, vh_half = FL_H / 2;
    float scrx = (float)(x - vw_half) * 0.5f;
    float scry = (float)(y - vh_half) * 0.5f;
    FlPt r;
    r.x = (FL_UPP_X * scrx) + (FL_UPP_Y * scry) + camx;
    r.y = (FL_UPP_Y * scry) - (FL_UPP_X * scrx) + camy;
    return r;
}

///////////////////////////////////////////////////////////////////////////////
// RENDERABLES: the objects to draw in a frame
///////////////////////////////////////////////////////////////////////////////

typedef struct {
    int img;
    int sx, sy, sw, sh;     // the clip
    int ox, oy;             // the offset
    float mx, my;           // the position in the map
    uint64_t prio;
    int hero;               // TYPE_HERO
    int alpha, color, blend;
} FlRen;

#define FL_RENS_MAX 4096
static FlRen FL_RENS[2][FL_RENS_MAX];      // 0: alive (r), 1: dead (r_dead)
static int   FL_RENS_N[2] = { 0, 0 };

static void fl_ren_clear (void) {
    FL_RENS_N[0] = FL_RENS_N[1] = 0;
}

static void fl_ren_push (int dead, int img, int sx, int sy, int sw, int sh, int ox, int oy,
                         float mx, float my, int prio, int hero, int alpha, int color, int blend) {
    int k = dead ? 1 : 0;
    if (FL_RENS_N[k] == FL_RENS_MAX) {
        return;
    }
    FlRen r = { img, sx, sy, sw, sh, ox, oy, mx, my, (uint64_t)prio, hero, alpha, color, blend };
    FL_RENS[k][FL_RENS_N[k]++] = r;
}

// calculatePriosIso
static void fl_ren_prios (FlRen* r, int n) {
    for (int i=0; i<n; i++) {
        unsigned tilex = (unsigned)floorf(r[i].mx);
        unsigned tiley = (unsigned)floorf(r[i].my);
        int commax = (int)((r[i].mx - (float)tilex) * (1<<10));
        int commay = (int)((r[i].my - (float)tiley) * (1<<10));
        r[i].prio += ((uint64_t)(tilex + tiley) << 37) + ((uint64_t)tilex << 20) + ((uint64_t)(commax + commay) << 8);
    }
}

static int fl_ren_cmp (const void* a, const void* b) {
    uint64_t pa = ((const FlRen*)a)->prio, pb = ((const FlRen*)b)->prio;
    return (pa < pb) ? -1 : (pa > pb) ? 1 : 0;
}

///////////////////////////////////////////////////////////////////////////////
// DRAWING, as MapRenderer::renderIso
///////////////////////////////////////////////////////////////////////////////

static int     FL_HB_X, FL_HB_Y, FL_HB_W, FL_HB_H;     // hero_bounds

static int fl_within (int rx, int ry, int rw, int rh, int px, int py) {
    return px >= rx && py >= ry && px <= rx + rw && py <= ry + rh;
}

static void fl_draw_tile (FlTile* t, int dx, int dy) {
    fl_img_draw(t->img, t->x, t->y, t->w, t->h, dx, dy, 255, 0xFFFFFF, 0);
}

static void fl_draw_ren (FlRen* r) {
    if (r->img < 0) {
        return;
    }
    FlIPt p = fl_map2scr(r->mx, r->my, FL_SHAKE.x, FL_SHAKE.y);
    fl_img_draw(r->img, r->sx, r->sy, r->sw, r->sh, p.x - r->ox, p.y - r->oy,
                r->alpha, r->color, r->blend);
}

// centerTile, isometric
static FlIPt fl_center (FlIPt p) {
    p.y += FL_TILE_H / 2;
    return p;
}

static void fl_tile_bounds (int x, int y, unsigned short* layer, int* bx, int* by, int* bw, int* bh,
                            FlIPt* center) {
    if (x >= 0 && x < FL_MAP_W && y >= 0 && y < FL_MAP_H) {
        FlTile* t = fl_tile(layer[x * FL_MAP_H + y]);
        if (t != NULL) {
            *center = fl_center(fl_map2scr((float)x, (float)y, FL_SHAKE.x, FL_SHAKE.y));
            *bx = center->x - t->ox;
            *by = center->y - t->oy;
            *bw = t->w;
            *bh = t->h;
        }
    }
}

static void fl_render_layer (unsigned short* layer) {
    int w = FL_MAP_W, h = FL_MAP_H;
    FlPt ul = fl_scr2map(0, 0, FL_SHAKE.x, FL_SHAKE.y);
    FlIPt upperleft = { (int)ul.x, (int)ul.y };
    int max_tiles_width = (FL_W / FL_TILE_W) + 2 * FL_TMAX_X;
    int max_tiles_height = (2 * FL_H / FL_TILE_H) + 2 * (FL_TMAX_Y + 1);
    int j = upperleft.y - FL_TMAX_Y / 2 + FL_TMAX_X;
    int i = upperleft.x - FL_TMAX_Y / 2 - FL_TMAX_X;
    for (int y = max_tiles_height; y; --y) {
        int tiles_width = 0;
        if (i < -1) {
            j = j + i + 1;
            tiles_width = tiles_width - (i + 1);
            i = -1;
        }
        int d = j - h;
        if (d >= 0) {
            j = j - d;
            tiles_width = tiles_width + d;
            i = i + d;
        }
        int j_end = j + i - w + 1;
        int m = (j - max_tiles_width > 0) ? j - max_tiles_width : 0;
        if (m > j_end) j_end = m;
        FlIPt p = fl_center(fl_map2scr((float)i, (float)j, FL_SHAKE.x, FL_SHAKE.y));
        while (j > j_end) {
            --j;
            ++i;
            ++tiles_width;
            p.x += FL_TILE_W;
            FlTile* t = fl_tile(layer[i * h + j]);
            if (t != NULL) {
                fl_draw_tile(t, p.x - t->ox, p.y - t->oy);
            }
        }
        j = j + tiles_width;
        i = i - tiles_width;
        if (y % 2) i++; else j++;
    }
}

static void fl_render_front (FlRen* r, int n) {
    int w = FL_MAP_W, h = FL_MAP_H;
    FlPt ul = fl_scr2map(0, 0, FL_SHAKE.x, FL_SHAKE.y);
    FlIPt upperleft = { (int)ul.x, (int)ul.y };
    int max_tiles_width = (FL_W / FL_TILE_W) + 2 * FL_TMAX_X;
    int max_tiles_height = ((FL_H / FL_TILE_H) + 2 * FL_TMAX_Y) * 2;
    int cur = 0;
    int j = upperleft.y - FL_TMAX_Y + FL_TMAX_X;
    int i = upperleft.x - FL_TMAX_Y - FL_TMAX_X;
    while (cur < n && ((int)r[cur].mx + (int)r[cur].my < i + j || (int)r[cur].mx < i)) {
        ++cur;
    }
    if (FL_OBJ_LAYER >= FL_LAYERS_N) {
        return;
    }
    // the queues of objects behind the tiles to the SW, NE, or none
    static int q_sw[FL_RENS_MAX], q_ne[FL_RENS_MAX], q_none[FL_RENS_MAX];
    memset(FL_DRAWN, 0, (size_t)w * h);
    unsigned short* layer = FL_LAYERS[FL_OBJ_LAYER];

    for (int y = max_tiles_height; y; --y) {
        int tiles_width = 0;
        if (i < -1) {
            j = j + i + 1;
            tiles_width = tiles_width - (i + 1);
            i = -1;
        }
        int d = j - h;
        if (d >= 0) {
            j = j - d;
            tiles_width = tiles_width + d;
            i = i + d;
        }
        int j_end = j + i - w + 1;
        int m = (j - max_tiles_width > 0) ? j - max_tiles_width : 0;
        if (m > j_end) j_end = m;
        FlIPt p = fl_center(fl_map2scr((float)i, (float)j, FL_SHAKE.x, FL_SHAKE.y));
        while (j > j_end) {
            --j;
            ++i;
            ++tiles_width;
            p.x += FL_TILE_W;

            int draw_tile = 1;
            for (int pre = cur; pre < n; pre++) {
                int rx = (int)r[pre].mx, ry = (int)r[pre].my;
                if ((rx-1 == i && ry+1 == j) || (rx+1 == i && ry-1 == j)) {
                    draw_tile = 0;
                    break;
                } else if (rx+1 > i || ry+1 > j) {
                    break;
                }
            }

            if (draw_tile && !FL_DRAWN[i * h + j]) {
                FlTile* t = fl_tile(layer[i * h + j]);
                if (t != NULL) {
                    fl_draw_tile(t, p.x - t->ox, p.y - t->oy);
                    FL_DRAWN[i * h + j] = 1;
                }
            }

            if (cur == n) {
                continue;
            }

            int sw_x=0, sw_y=0, sw_w=0, sw_h=0, s_x=0, s_y=0, s_w=0, s_h=0;
            int ne_x=0, ne_y=0, ne_w=0, ne_h=0, e_x=0, e_y=0, e_w=0, e_h=0;
            FlIPt sw_c = {0,0}, s_c = {0,0}, ne_c = {0,0}, e_c = {0,0};
            fl_tile_bounds(i-2, j+2, layer, &sw_x, &sw_y, &sw_w, &sw_h, &sw_c);
            fl_tile_bounds(i-1, j+2, layer, &s_x, &s_y, &s_w, &s_h, &s_c);
            fl_tile_bounds(i, j, layer, &ne_x, &ne_y, &ne_w, &ne_h, &ne_c);
            fl_tile_bounds(i, j+1, layer, &e_x, &e_y, &e_w, &e_h, &e_c);

            int draw_sw_tile = 0;
            int n_sw = 0, n_ne = 0, n_none = 0;
            while (cur < n) {
                int rx = (int)r[cur].mx, ry = (int)r[cur].my;
                if (rx+1 == i && ry-1 == j) {
                    draw_sw_tile = 1;
                    FlIPt left = fl_map2scr(r[cur].mx, r[cur].my, FL_SHAKE.x, FL_SHAKE.y);
                    left.y -= r[cur].oy;
                    FlIPt right = left;
                    left.x -= r[cur].ox;
                    right.x += r[cur].sw - r[cur].ox;
                    if (r[cur].hero) {
                        left.x = FL_HB_X;
                        left.y = FL_HB_Y + FL_HB_H;
                        right.x = FL_HB_X + FL_HB_W;
                        right.y = FL_HB_Y + FL_HB_H;
                    }
                    int behind_sw = fl_within(s_x, s_y, s_w, s_h, right.x, right.y) &&
                                    fl_within(sw_x, sw_y, sw_w, sw_h, left.x, left.y);
                    int behind_ne = fl_within(e_x, e_y, e_w, e_h, left.x, left.y) &&
                                    fl_within(ne_x, ne_y, ne_w, ne_h, right.x, right.y);
                    if (behind_sw) q_sw[n_sw++] = cur;
                    else if (behind_ne) q_ne[n_ne++] = cur;
                    else q_none[n_none++] = cur;
                    ++cur;
                } else {
                    break;
                }
            }

            for (int k=0; k<n_sw; k++) fl_draw_ren(&r[q_sw[k]]);

            if (draw_sw_tile && i-2 >= 0 && j+2 < h && !FL_DRAWN[(i-2) * h + (j+2)]) {
                FlTile* t = fl_tile(layer[(i-2) * h + (j+2)]);
                if (t != NULL) {
                    fl_draw_tile(t, sw_c.x - t->ox, sw_c.y - t->oy);
                    FL_DRAWN[(i-2) * h + (j+2)] = 1;
                }
            }

            for (int k=0; k<n_ne; k++) fl_draw_ren(&r[q_ne[k]]);

            if (!draw_tile && i < w && j < h && !FL_DRAWN[i * h + j]) {
                FlTile* t = fl_tile(layer[i * h + j]);
                if (t != NULL) {
                    fl_draw_tile(t, ne_c.x - t->ox, ne_c.y - t->oy);
                    FL_DRAWN[i * h + j] = 1;
                }
            }

            for (int k=0; k<n_none; k++) fl_draw_ren(&r[q_none[k]]);
        }
        j = j + tiles_width;
        i = i - tiles_width;
        if (y % 2) i++; else j++;
        while (cur < n && ((int)r[cur].mx + (int)r[cur].my < i + j || (int)r[cur].mx <= i)) {
            ++cur;
        }
    }
}

// MapRenderer::render and renderIso: the background color, the layers
// below the object layer, the dead objects, the object layer with the
// objects, and the layers above
static void fl_map_render (void) {
    fl_fill(FL_BG, 0, 0, FL_W, FL_H);
    if (FL_LAYERS_N == 0) {
        return;
    }
    // hero_bounds
    FL_HB_X = FL_HB_Y = FL_HB_W = FL_HB_H = 0;
    FlRen* r = FL_RENS[0];
    for (int i=0; i<FL_RENS_N[0]; i++) {
        if (r[i].hero) {
            FlIPt p = fl_map2scr(r[i].mx, r[i].my, FL_SHAKE.x, FL_SHAKE.y);
            p.x -= r[i].ox;
            p.y -= r[i].oy;
            if (p.x < FL_HB_X || FL_HB_W == 0) FL_HB_X = p.x;
            if (p.x + r[i].sw > FL_HB_X + FL_HB_W || FL_HB_W == 0) FL_HB_W = p.x + r[i].sw - FL_HB_X;
            if (p.y < FL_HB_Y || FL_HB_H == 0) FL_HB_Y = p.y;
            if (p.y + r[i].sh > FL_HB_Y + FL_HB_W || FL_HB_H == 0) FL_HB_H = p.y + r[i].sh - FL_HB_Y;
        }
    }
    for (int k=0; k<2; k++) {
        fl_ren_prios(FL_RENS[k], FL_RENS_N[k]);
        qsort(FL_RENS[k], FL_RENS_N[k], sizeof(FlRen), fl_ren_cmp);
    }
    int index = 0;
    while (index < FL_OBJ_LAYER) {
        fl_render_layer(FL_LAYERS[index]);
        index++;
    }
    for (int i=0; i<FL_RENS_N[1]; i++) {
        fl_draw_ren(&FL_RENS[1][i]);
    }
    fl_render_front(FL_RENS[0], FL_RENS_N[0]);
    index++;
    while (index < FL_LAYERS_N) {
        if (strcmp(FL_LAYER_NAMES[index], "fow_dark") && strcmp(FL_LAYER_NAMES[index], "fow_fog")) {
            fl_render_layer(FL_LAYERS[index]);
        }
        index++;
    }
}

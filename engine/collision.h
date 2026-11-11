// Collision in C, translated closely from MapCollision and from the move
// of Entity of the original (`flare-engine/src`): the collision layer of
// the map (`FL_COLLISION` of `engine/map.h`), with the entities that
// block tiles, and the movement of an entity, with 32-bit floats.
//
// It is included by `engine/flare.h`, after `engine/map.h`.

// the tile types, as MapCollision
enum {
    FL_BLOCKS_NONE = 0, FL_BLOCKS_ALL = 1, FL_BLOCKS_MOVEMENT = 2,
    FL_BLOCKS_ALL_HIDDEN = 3, FL_BLOCKS_MOVEMENT_HIDDEN = 4,
    FL_MAP_ONLY = 5, FL_MAP_ONLY_ALT = 6,
    FL_BLOCKS_ENTITIES = 7,     // the hero or an enemy is there
    FL_BLOCKS_ENEMIES = 8,      // an ally is there
};
// the movement types and the collide types
enum { FL_MOVE_NORMAL = 0, FL_MOVE_FLYING = 1, FL_MOVE_INTANGIBLE = 2 };
enum { FL_COLLIDE_NONE = 0, FL_COLLIDE_ALL = 1, FL_COLLIDE_HERO = 2, FL_COLLIDE_HAZARD = 3 };

static const float FL_MIN_TILE_GAP = 0.001f;
static int   FL_ALLY_COLLISION = 1;         // enable_ally_collision of misc.txt
static float FL_RAYCAST = 0.1f;             // raycast_resolution of misc.txt

static const float FL_SPEED_MULT[8] = {
    (float)(1.0/M_SQRT2), 1.0f, (float)(1.0/M_SQRT2), 1.0f,
    (float)(1.0/M_SQRT2), 1.0f, (float)(1.0/M_SQRT2), 1.0f
};
static const float FL_DIR_DX[8] = { -1, -1, -1,  0,  1,  1,  1,  0 };
static const float FL_DIR_DY[8] = {  1,  0, -1, -1, -1,  0,  1,  1 };

static int fl_col_get (int x, int y) {
    return FL_COLLISION[x * FL_MAP_H + y];
}

static int fl_col_outside (int x, int y) {
    return x < 0 || y < 0 || x >= FL_MAP_W || y >= FL_MAP_H;
}

static int fl_col_wall (float x, float y) {
    int tx = (int)x, ty = (int)y;
    if (fl_col_outside(tx, ty)) return 1;
    int c = fl_col_get(tx, ty);
    return c == FL_BLOCKS_ALL || c == FL_BLOCKS_ALL_HIDDEN;
}

static int fl_col_valid_tile (int tx, int ty, int mt, int ct) {
    if (fl_col_outside(tx, ty)) return 0;
    int c = fl_col_get(tx, ty);
    if (ct == FL_COLLIDE_ALL) {
        if (c == FL_BLOCKS_ENEMIES || c == FL_BLOCKS_ENTITIES) return 0;
    } else if (ct == FL_COLLIDE_HERO) {
        if (c == FL_BLOCKS_ENEMIES && !FL_ALLY_COLLISION) return 1;
    }
    if (mt == FL_MOVE_INTANGIBLE) return 1;
    if (mt == FL_MOVE_FLYING) return !(c == FL_BLOCKS_ALL || c == FL_BLOCKS_ALL_HIDDEN);
    if (c == FL_MAP_ONLY || c == FL_MAP_ONLY_ALT) return 1;
    return c == FL_BLOCKS_NONE ||
           (ct == FL_COLLIDE_HAZARD && (c == FL_BLOCKS_ENEMIES || c == FL_BLOCKS_ENTITIES));
}

static int fl_col_valid (float x, float y, int mt, int ct) {
    if (x < 0 || y < 0) return 0;
    return fl_col_valid_tile((int)x, (int)y, mt, ct);
}

static int fl_sgn (float f) {
    return (f > 0) ? 1 : (f < 0) ? -1 : 0;
}

static int fl_col_small_step (float* x, float* y, float sx, float sy, int mt, int ct) {
    if (fl_col_valid(*x + sx, *y + sy, mt, ct)) {
        *x += sx;
        *y += sy;
        return 1;
    }
    return 0;
}

static int fl_col_slide_grid (float* x, float* y, float sx, float sy, int mt, int ct) {
    if (fl_col_valid(*x + sx, *y, mt, ct)) {
        if (sx == 0) return 1;
        *x += sx;
    } else if (fl_col_valid(*x, *y + sy, mt, ct)) {
        if (sy == 0) return 1;
        *y += sy;
    } else {
        return 0;
    }
    return 1;
}

static int fl_col_slide (float* x, float* y, float sx, float sy, int mt, int ct) {
    const float eps = 0.01f;
    if (sx != 0) {
        float dy = *y - floorf(*y);
        if (fl_col_valid_tile((int)*x, (int)*y + 1, mt, ct)
                && fl_col_valid_tile((int)*x + fl_sgn(sx), (int)*y + 1, mt, ct) && dy > 0.5) {
            float a = 1 - dy + eps, b = (float)fabs(sx);
            *y += (a < b) ? a : b;
        } else if (fl_col_valid_tile((int)*x, (int)*y - 1, mt, ct)
                && fl_col_valid_tile((int)*x + fl_sgn(sx), (int)*y - 1, mt, ct) && dy < 0.5) {
            float a = dy + eps, b = (float)fabs(sx);
            *y -= (a < b) ? a : b;
        } else {
            return 0;
        }
    } else if (sy != 0) {
        float dx = *x - floorf(*x);
        if (fl_col_valid_tile((int)*x + 1, (int)*y, mt, ct)
                && fl_col_valid_tile((int)*x + 1, (int)*y + fl_sgn(sy), mt, ct) && dx > 0.5) {
            float a = 1 - dx + eps, b = (float)fabs(sy);
            *x += (a < b) ? a : b;
        } else if (fl_col_valid_tile((int)*x - 1, (int)*y, mt, ct)
                && fl_col_valid_tile((int)*x - 1, (int)*y + fl_sgn(sy), mt, ct) && dx < 0.5) {
            float a = dx + eps, b = (float)fabs(sy);
            *x -= (a < b) ? a : b;
        } else {
            return 0;
        }
    }
    return 1;
}

// MapCollision::move: false if it stopped at an obstacle
static int fl_col_move (float* x, float* y, float step_x, float step_y, int mt, int ct) {
    int force_slide = (step_x != 0 && step_y != 0);
    while (step_x != 0 || step_y != 0) {
        float sx = 0;
        if (step_x > 0) {
            sx = (ceilf(*x) - *x < step_x) ? ceilf(*x) - *x : step_x;
            if (sx <= FL_MIN_TILE_GAP) sx = (1.f < step_x) ? 1.f : step_x;
        } else if (step_x < 0) {
            sx = (floorf(*x) - *x > step_x) ? floorf(*x) - *x : step_x;
            if (sx == 0) sx = (-1.f > step_x) ? -1.f : step_x;
        }
        float sy = 0;
        if (step_y > 0) {
            sy = (ceilf(*y) - *y < step_y) ? ceilf(*y) - *y : step_y;
            if (sy <= FL_MIN_TILE_GAP) sy = (1.f < step_y) ? 1.f : step_y;
        } else if (step_y < 0) {
            sy = (floorf(*y) - *y > step_y) ? floorf(*y) - *y : step_y;
            if (sy == 0) sy = (-1.f > step_y) ? -1.f : step_y;
        }
        step_x -= sx;
        step_y -= sy;
        if (!fl_col_small_step(x, y, sx, sy, mt, ct)) {
            if (force_slide) {
                if (!fl_col_slide_grid(x, y, sx, sy, mt, ct)) return 0;
            } else {
                if (!fl_col_slide(x, y, sx, sy, mt, ct)) return 0;
            }
        }
    }
    return 1;
}

static void fl_col_block (float mx, float my, int ally) {
    int tx = (int)mx, ty = (int)my;
    if (fl_col_outside(tx, ty)) return;
    if (fl_col_get(tx, ty) == FL_BLOCKS_NONE) {
        FL_COLLISION[tx * FL_MAP_H + ty] = ally ? FL_BLOCKS_ENEMIES : FL_BLOCKS_ENTITIES;
    }
}

static void fl_col_unblock (float mx, float my) {
    int tx = (int)mx, ty = (int)my;
    if (fl_col_outside(tx, ty)) return;
    int c = fl_col_get(tx, ty);
    if (c == FL_BLOCKS_ENTITIES || c == FL_BLOCKS_ENEMIES) {
        FL_COLLISION[tx * FL_MAP_H + ty] = FL_BLOCKS_NONE;
    }
}

// Entity::move_from_offending_tile
static void fl_col_unstuck (float* x, float* y, int mt, int ct) {
    if (!FL_COL_EMPTY) return;
    while (!fl_col_valid(*x, *y, mt, ct)) {
        float px = 0, py = 0;
        if (fl_col_valid(*x + 1, *y, mt, ct))
            px += 0.1f * (2 - ((float)(int)(*x + 1) + 0.5f - *x));
        if (fl_col_valid(*x - 1, *y, mt, ct))
            px -= 0.1f * (2 - (*x - ((float)(int)(*x - 1) + 0.5f)));
        if (fl_col_valid(*x, *y + 1, mt, ct))
            py += 0.1f * (2 - ((float)(int)(*y + 1) + 0.5f - *y));
        if (fl_col_valid(*x, *y - 1, mt, ct))
            py -= 0.1f * (2 - (*y - ((float)(int)(*y - 1) + 0.5f)));
        *x += px;
        *y += py;
        if (px == 0 && py == 0) {
            int sx0 = (int)*x, sy0 = (int)*y;
            FlPt shortest = { 0, 0 };
            float sd = 0;
            int maxr = (FL_MAP_W > FL_MAP_H) ? FL_MAP_W : FL_MAP_H;
            for (int r = 1; r <= maxr; r++) {
                for (int i = sx0 - r; i <= sx0 + r; ++i) {
                    for (int j = sy0 - r; j <= sy0 + r; ++j) {
                        if (fl_col_valid((float)i, (float)j, mt, ct)) {
                            FlPt p = { *x, *y };
                            float d = fl_dist(p, shortest);
                            if (sd == 0 || d < sd) {
                                sd = d;
                                shortest.x = (float)i + 0.5f;
                                shortest.y = (float)j + 0.5f;
                            }
                        }
                    }
                }
                if (sd != 0) {
                    *x = shortest.x;
                    *y = shortest.y;
                    break;
                }
            }
        }
    }
}

// Entity::move with no knockback, stun, or charge: the position moves by
// speed in the direction, and it is false if it stopped at an obstacle
static float FL_MOVE_X, FL_MOVE_Y;
static int fl_entity_move (float x, float y, int dir, float speed, float effect_speed, int mt, int hero) {
    int ct = hero ? FL_COLLIDE_HERO : FL_COLLIDE_ALL;
    fl_col_unstuck(&x, &y, mt, ct);
    float s = speed * FL_SPEED_MULT[dir] * effect_speed / 100;
    float dx = s * FL_DIR_DX[dir];
    float dy = s * FL_DIR_DY[dir];
    int ok = fl_col_move(&x, &y, dx, dy, mt, ct);
    FL_MOVE_X = x;
    FL_MOVE_Y = y;
    return ok;
}

// MapCollision::lineCheck
static int fl_col_line (float x1, float y1, float x2, float y2, int sight, int mt) {
    float x = x1, y = y1;
    float dx = (float)fabs(x2 - x1), dy = (float)fabs(y2 - y1);
    float m = (dx > dy) ? dx : dy;
    float t = m * (1.f / FL_RAYCAST);
    int steps = (int)((1.f > t) ? 1.f : t);
    float sx, sy;
    if (dx > dy) { sx = 1; sy = dy / dx; } else { sy = 1; sx = dx / dy; }
    if (x1 > x2) sx = -sx;
    if (y1 > y2) sy = -sy;
    sx *= FL_RAYCAST;
    sy *= FL_RAYCAST;
    for (int i=0; i<steps; i++) {
        x += sx;
        y += sy;
        if (sight ? fl_col_wall(x, y) : !fl_col_valid(x, y, mt, FL_COLLIDE_ALL)) return 0;
    }
    return 1;
}

static int fl_col_sight (float x1, float y1, float x2, float y2) {
    return fl_col_line(x1, y1, x2, y2, 1, FL_MOVE_NORMAL);
}

// Path finding in C, translated closely from MapCollision::computePath,
// AStarContainer, AStarCloseContainer, and AStarNode of the original: the
// open list is the same binary heap, so that ties go the same way.
//
// It is included by `engine/flare.h`, after `engine/collision.h`.

#include <float.h>

typedef struct { int x, y; float g, h; int px, py; } FlNode;

typedef struct {
    FlNode** nodes;
    short*   pos;           // [x*h + y]: the index in nodes, or -1
    unsigned size, limit;
} FlHeap;

static float fl_node_f (FlNode* n) {
    return n->g + n->h * 2.f;
}

static void fl_heap_init (FlHeap* c, unsigned limit) {
    c->nodes = calloc(limit ? limit : 1, sizeof(FlNode*));
    c->pos = malloc(sizeof(short) * FL_MAP_W * FL_MAP_H);
    for (int k=0; k<FL_MAP_W*FL_MAP_H; k++) c->pos[k] = -1;
    c->size = 0;
    c->limit = limit;
}

static void fl_heap_free (FlHeap* c) {
    for (unsigned i=0; i<c->size; i++) free(c->nodes[i]);
    free(c->nodes);
    free(c->pos);
}

#define FL_HPOS(c,n) ((c)->pos[(n)->x * FL_MAP_H + (n)->y])

static void fl_heap_up (FlHeap* c, int m) {
    while (m != 0) {
        if (fl_node_f(c->nodes[m]) <= fl_node_f(c->nodes[m/2])) {
            FlNode* t = c->nodes[m/2];
            c->nodes[m/2] = c->nodes[m];
            FL_HPOS(c, c->nodes[m/2]) = (short)(m/2);
            c->nodes[m] = t;
            FL_HPOS(c, c->nodes[m]) = (short)m;
            m = m/2;
        } else {
            break;
        }
    }
}

// AStarContainer::add
static void fl_heap_add (FlHeap* c, FlNode* n) {
    if (c->size >= c->limit) { free(n); return; }
    c->nodes[c->size] = n;
    FL_HPOS(c, n) = (short)c->size;
    fl_heap_up(c, (int)c->size);
    c->size++;
}

// AStarContainer::remove (the node stays allocated: it goes to close)
static void fl_heap_remove (FlHeap* c, FlNode* n) {
    unsigned v = FL_HPOS(c, n) + 1;
    c->nodes[v-1] = c->nodes[c->size-1];
    FL_HPOS(c, c->nodes[v-1]) = (short)(v-1);
    c->size--;
    if (c->size == 0) {
        FL_HPOS(c, n) = -1;
        return;
    }
    while (1) {
        unsigned u = v;
        if (2*u+1 <= c->size) {
            if (fl_node_f(c->nodes[u-1]) >= fl_node_f(c->nodes[2*u-1])) v = 2*u;
            if (fl_node_f(c->nodes[v-1]) >= fl_node_f(c->nodes[2*u])) v = 2*u+1;
        } else if (2*u <= c->size) {
            if (fl_node_f(c->nodes[u-1]) >= fl_node_f(c->nodes[2*u-1])) v = 2*u;
        }
        if (u != v) {
            FlNode* t = c->nodes[u-1];
            c->nodes[u-1] = c->nodes[v-1];
            FL_HPOS(c, c->nodes[u-1]) = (short)(u-1);
            c->nodes[v-1] = t;
            FL_HPOS(c, c->nodes[v-1]) = (short)(v-1);
        } else {
            break;
        }
    }
    FL_HPOS(c, n) = -1;
}

// AStarCloseContainer::add: a node that does not fit is still used by the
// search (as in the original), and freed at its end
static FlNode* FL_SPARE[64];
static int     FL_SPARE_N = 0;
static void fl_close_add (FlHeap* c, FlNode* n) {
    if (c->size >= c->limit) {
        if (FL_SPARE_N < 64) FL_SPARE[FL_SPARE_N++] = n;
        return;
    }
    c->nodes[c->size] = n;
    FL_HPOS(c, n) = (short)c->size;
    c->size++;
}

static FlNode* fl_node_new (int x, int y) {
    FlNode* n = calloc(1, sizeof(FlNode));
    n->x = x; n->y = y;
    return n;
}

static float fl_pdist (int x1, int y1, int x2, int y2) {
    return fl_distf((float)x1, (float)y1, (float)x2, (float)y2);
}

// the path, from the end to the start (as the original: path.back() is next)
#define FL_PATH_MAX 4096
static FlPt FL_PATH[FL_PATH_MAX];
static int  FL_PATH_N = 0;

static void fl_path_push (int x, int y) {
    if (FL_PATH_N < FL_PATH_MAX) {
        FlPt p = { (float)x + 0.5f, (float)y + 0.5f };
        FL_PATH[FL_PATH_N++] = p;
    }
}

// MapCollision::computePath, with limit 0 (DEFAULT_PATH_LIMIT): w*h/10
static int fl_path (float sx, float sy, float ex, float ey, int mt, unsigned limit) {
    FL_PATH_N = 0;
    if (ex < 0 || ey < 0 || ex >= (float)FL_MAP_W || ey >= (float)FL_MAP_H) return 0;
    if (limit == 0) limit = (unsigned)(FL_MAP_W * FL_MAP_H) / 10;
    int stx = (int)sx, sty = (int)sy, enx = (int)ex, eny = (int)ey;
    int c = fl_col_get(enx, eny);
    int target_blocks = (c == FL_BLOCKS_ENTITIES || c == FL_BLOCKS_ENEMIES);
    if (target_blocks) fl_col_unblock(ex, ey);

    FlHeap open, close;
    fl_heap_init(&open, limit);
    fl_heap_init(&close, limit);
    int cx = stx, cy = sty;
    FlNode* node = fl_node_new(stx, sty);
    node->g = 0;
    node->h = fl_pdist(stx, sty, enx, eny);
    node->px = cx; node->py = cy;
    fl_heap_add(&open, node);

    while (open.size > 0 && close.size < limit) {
        node = open.nodes[0];
        cx = node->x; cy = node->y;
        fl_close_add(&close, node);
        fl_heap_remove(&open, node);
        if (cx == enx && cy == eny) break;
        // AStarNode::getNeighbours, node_stride 1
        int nx[8], ny[8], nn = 0, x = node->x, y = node->y;
        if (x>1 && y>1) { nx[nn]=x-1; ny[nn++]=y-1; }
        if (x>1 && y<FL_MAP_H-1) { nx[nn]=x-1; ny[nn++]=y+1; }
        if (y>1 && x<FL_MAP_W-1) { nx[nn]=x+1; ny[nn++]=y-1; }
        if (x<FL_MAP_W-1 && y<FL_MAP_H-1) { nx[nn]=x+1; ny[nn++]=y+1; }
        if (x>1) { nx[nn]=x-1; ny[nn++]=y; }
        if (y>1) { nx[nn]=x; ny[nn++]=y-1; }
        if (x<FL_MAP_W-1) { nx[nn]=x+1; ny[nn++]=y; }
        if (y<FL_MAP_H-1) { nx[nn]=x; ny[nn++]=y+1; }
        for (int k=0; k<nn; k++) {
            if (open.size >= limit) break;
            if (!fl_col_valid_tile(nx[k], ny[k], mt, FL_COLLIDE_ALL)) continue;
            if (close.pos[nx[k]*FL_MAP_H + ny[k]] != -1) continue;
            float g = node->g + fl_pdist(cx, cy, nx[k], ny[k]);
            short i = open.pos[nx[k]*FL_MAP_H + ny[k]];
            if (i == -1) {
                FlNode* nw = fl_node_new(nx[k], ny[k]);
                nw->g = g;
                nw->px = cx; nw->py = cy;
                nw->h = fl_pdist(nx[k], ny[k], enx, eny);
                fl_heap_add(&open, nw);
            } else if (g < open.nodes[i]->g) {
                // AStarContainer::updateParent
                open.nodes[i]->px = node->x; open.nodes[i]->py = node->y;
                open.nodes[i]->g = g;
                fl_heap_up(&open, i);
            }
        }
    }

    if (!(cx == enx && cy == eny)) {
        // AStarCloseContainer::get_shortest_h
        FlNode* best = NULL;
        float lowest = FLT_MAX;
        for (unsigned i=0; i<close.size; i++) {
            if (close.nodes[i]->h < lowest) {
                lowest = close.nodes[i]->h;
                best = close.nodes[i];
            }
        }
        cx = best->x; cy = best->y;
        while (!(cx == stx && cy == sty)) {
            fl_path_push(cx, cy);
            FlNode* p = close.nodes[close.pos[cx*FL_MAP_H + cy]];
            cx = p->px; cy = p->py;
        }
    } else {
        fl_path_push(enx, eny);
        while (!(cx == stx && cy == sty)) {
            fl_path_push(cx, cy);
            FlNode* p = close.nodes[close.pos[cx*FL_MAP_H + cy]];
            cx = p->px; cy = p->py;
        }
    }
    fl_heap_free(&open);
    fl_heap_free(&close);
    while (FL_SPARE_N > 0) free(FL_SPARE[--FL_SPARE_N]);
    if (target_blocks) fl_col_block(ex, ey, c == FL_BLOCKS_ENEMIES);
    return FL_PATH_N > 0;
}

// MapCollision::lineOfMovement
static int fl_col_line_move (float x1, float y1, float x2, float y2, int mt) {
    if (x2 < 0 || y2 < 0 || x2 >= (float)FL_MAP_W || y2 >= (float)FL_MAP_H) return 0;
    if (mt == FL_MOVE_INTANGIBLE) return 1;
    int c = fl_col_get((int)x2, (int)y2);
    int target_blocks = (c == FL_BLOCKS_ENTITIES || c == FL_BLOCKS_ENEMIES);
    if (target_blocks) fl_col_unblock(x2, y2);
    int ok = fl_col_line(x1, y1, x2, y2, 0, mt);
    if (target_blocks) fl_col_block(x2, y2, c == FL_BLOCKS_ENEMIES);
    return ok;
}

// Geometry of the original in C, with 32-bit floats (Utils.cpp and
// Entity::faceNextBest): directions, angles, distances, and vectors.
//
// It is included by `engine/flare.h`, after `engine/map.h`.

// Utils::calcTheta
static float fl_theta (float x1, float y1, float x2, float y2) {
    float dx = x2 - x1, dy = y2 - y1;
    float theta;
    if (dx == 0) {
        theta = (dy > 0.0) ? (float)M_PI / 2.0f : (float)-M_PI / 2.0f;
    } else {
        theta = atanf(dy / dx);
        if (dx < 0.0 && dy >= 0.0) theta += (float)M_PI;
        if (dx < 0.0 && dy < 0.0) theta -= (float)M_PI;
    }
    return theta;
}

// Utils::calcDirection: from (x0,y0) to (x1,y1), 0 to 7
static int fl_direction (float x0, float y0, float x1, float y1) {
    float theta = fl_theta(x0, y0, x1, y1);
    float val = theta / ((float)M_PI / 4);
    int dir = (int)(((val < 0) ? ceilf(val - 0.5f) : floorf(val + 0.5f)) + 4);
    dir = (dir + 1) % 8;
    return (dir >= 0) ? dir : 0;
}

// Utils::calcDist
static float fl_distf (float x1, float y1, float x2, float y2) {
    return sqrtf((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

// Utils::calcVector: a point at a distance in a direction
static FlPt fl_vector (float x, float y, int dir, float dist) {
    FlPt p = { x, y };
    float d = dist * 0.7071f;
    switch (dir) {
        case 0: p.x -= d; p.y += d; break;
        case 1: p.x -= dist; break;
        case 2: p.x -= d; p.y -= d; break;
        case 3: p.y -= dist; break;
        case 4: p.x += d; p.y -= d; break;
        case 5: p.x += dist; break;
        case 6: p.x += d; p.y += d; break;
        case 7: p.y += dist; break;
    }
    return p;
}

// Utils::rotateDirection (sic: by 7)
static int fl_rotate (int dir, int val) {
    dir += val;
    if (dir > 7) dir -= 7;
    else if (dir < 0) dir += 7;
    return dir;
}

// Entity::faceNextBest
static int fl_face_next (int dir, float x, float y, float mx, float my) {
    float dx = (float)fabs(mx - x), dy = (float)fabs(my - y);
    switch (dir) {
        case 0: return (dy > dx) ? 7 : 1;
        case 1: return (my > y) ? 0 : 2;
        case 2: return (dx > dy) ? 1 : 3;
        case 3: return (mx < x) ? 2 : 4;
        case 4: return (dy > dx) ? 3 : 5;
        case 5: return (my < y) ? 4 : 6;
        case 6: return (dx > dy) ? 5 : 7;
        case 7: return (mx > x) ? 6 : 0;
    }
    return 0;
}

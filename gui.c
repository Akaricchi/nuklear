/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
*/
#include "gui.h"

#define NULL (void*)0
#define UTF_INVALID 0xFFFD
#define PI 3.141592654f

#define PASTE(a,b) a##b
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a) (sizeof(a)/sizeof(a)[0])
#define ABS(a) (((a) < 0) ? -(a) : (a))
#define UNUSED(a) ((void)(a))
#define DEG2RAD(a) ((a) * (PI / 180.0f))
#define RAD2DEG(a) ((a) * (180.0f / PI))
#define BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))
#define INBOX(x, y, x0, y0, x1, y1) (BETWEEN(x, x0, x1) && BETWEEN(y, y0, y1))

#define ASSERT_LINE(p, l, f) \
    typedef char PASTE(assertion_failed_##f##_,l)[2*!!(p)-1];
#define ASSERT(predicate) ASSERT_LINE(predicate,__LINE__,__FILE__)

#define vec2_load(v,a,b) (v).x = (a), (v).y = (b)
#define vec2_mov(to,from) (to).x = (from).x, (to).y = (from).y
#define vec2_len(v) ((float)fsqrt((v).x*(v).x+(v).y*(v).y))
#define vec2_sub(r,a,b) do {(r).x=(a).x-(b).x; (r).y=(a).y-(b).y;} while(0)
#define vec2_add(r,a,b) do {(r).x=(a).x+(b).x; (r).y=(a).y+(b).y;} while(0)
#define vec2_muls(r, v, s) do {(r).x=(v).x*(s); (r).y=(v).y*(s);} while(0)
#define vec2_norm(r, v) do {float _ = vec2_len(v); if (_) vec2_muls(r, v, 1.0f/_);} while(0)

static const gui_texture null_tex = (void*)0;
static const struct gui_rect null_rect = {-9999, -9999, 9999 * 2, 9999 * 2};
static gui_char utfbyte[GUI_UTF_SIZE+1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static gui_char utfmask[GUI_UTF_SIZE+1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static long utfmin[GUI_UTF_SIZE+1] = {0, 0, 0x80, 0x800, 0x100000};
static long utfmax[GUI_UTF_SIZE+1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

static gui_float
fsqrt(float x)
{
    float xhalf = 0.5f*x;
    union {float f; int i;} val;
    ASSERT(sizeof(int) == sizeof(float));

    val.f = x;
    val.i = 0x5f375a86 - (val.i>>1);
    val.f = val.f*(1.5f-xhalf*val.f*val.f);
    if (val.f != 0.0f)
        return 1.0f/val.f;
    return val.f;
}

static gui_size
utf_validate(long *u, gui_size i)
{
    if (!u) return 0;
    if (!BETWEEN(*u, utfmin[i], utfmax[i]) ||
         BETWEEN(*u, 0xD800, 0xDFFF))
            *u = UTF_INVALID;
    for (i = 1; *u > utfmax[i]; ++i);
    return i;
}

static long
utf_decode_byte(gui_char c, gui_size *i)
{
    if (!i) return 0;
    for(*i = 0; *i < LEN(utfmask); ++(*i)) {
        if ((c & utfmask[*i]) == utfbyte[*i])
            return c & ~utfmask[*i];
    }
    return 0;
}

static gui_size
utf_decode(gui_char *c, long *u, gui_size clen)
{
    gui_size i, j, len, type;
    long udecoded;

    *u = UTF_INVALID;
    if (!c || !u) return 0;
    if (!clen) return 0;
    udecoded = utf_decode_byte(c[0], &len);
    if (!BETWEEN(len, 1, GUI_UTF_SIZE))
        return 1;

    for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
        udecoded = (udecoded << 6) | utf_decode_byte(c[i], &type);
        if (type != 0)
            return j;
    }
    if (j < len)
        return 0;
    *u = udecoded;
    utf_validate(u, len);
    return len;
}

static gui_char
utf_encode_byte(long u, gui_size i)
{
    return utfbyte[i] | (u & ~utfmask[i]);
}

static gui_size
utf_encode(long u, gui_char *c, gui_size clen)
{
    gui_size len, i;
    len = utf_validate(&u, 0);
    if (clen < len || !len)
        return 0;
    for (i = len - 1; i != 0; --i) {
        c[i] = utf_encode_byte(u, 0);
        u >>= 6;
    }
    c[0] = utf_encode_byte(u, len);
    return len;
}

void
gui_input_begin(struct gui_input *in)
{
    if (!in) return;
    in->mouse_clicked = 0;
    in->glyph_count = 0;
    vec2_mov(in->mouse_prev, in->mouse_pos);
}

void
gui_input_motion(struct gui_input *in, gui_int x, gui_int y)
{
    vec2_load(in->mouse_pos, x, y);
}

void
gui_input_key(struct gui_input *in, enum gui_keys key, gui_int down)
{
    if (!in) return;
    in->keys[key] = down;
    if (key == GUI_KEY_SHIFT)
        in->shift = gui_true;
    if (key == GUI_KEY_CTRL)
        in->ctrl = gui_true;
}

void
gui_input_button(struct gui_input *in, gui_int x, gui_int y, gui_int down)
{
    if (!in) return;
    if (in->mouse_down == down) return;
    vec2_load(in->mouse_clicked_pos, x, y);
    in->mouse_down = down;
    in->mouse_clicked++;
}

void
gui_input_char(struct gui_input *in, gui_glyph glyph)
{
    gui_size len = 0;
    long unicode;
    if (!in) return;
    len = utf_decode(glyph, &unicode, GUI_UTF_SIZE);
    if (len && in->glyph_count < GUI_INPUT_MAX) {
        utf_encode(unicode, in->text[in->glyph_count], GUI_UTF_SIZE);
        in->glyph_count++;
    }
}

void
gui_input_end(struct gui_input *in)
{
    if (!in) return;
    vec2_sub(in->mouse_delta, in->mouse_pos, in->mouse_prev);
}

static struct gui_draw_command*
gui_push_command(struct gui_draw_list *list, gui_size count,
    const struct gui_rect *rect, gui_texture tex)
{
    gui_size memory = 0;
    gui_size current;
    struct gui_draw_command *cmd = NULL;
    if (!list || !rect) return NULL;
    if (!list->end || !list->begin || list->end < list->begin)
        return NULL;

    memory += sizeof(struct gui_draw_command);
    memory += sizeof(struct gui_vertex) * count;
    list->needed += memory;
    current = (gui_byte*)list->end - (gui_byte*)list->begin;
    if (list->size <= (current + memory))
        return NULL;

    cmd = list->end;
    list->end = (struct gui_draw_command*)((gui_byte*)list->end + memory);
    cmd->vertexes = (struct gui_vertex*)(cmd + 1);
    cmd->vertex_write = 0;
    cmd->vertex_count = count;
    cmd->clip_rect = *rect;
    cmd->texture = tex;
    return cmd;
}

static void
gui_vertex(struct gui_draw_command *cmd, gui_float x, gui_float y,
    struct gui_color col)
{
    gui_size i;
    if (!cmd) return;
    i = cmd->vertex_write;
    if (i >= cmd->vertex_count) return;
    cmd->vertexes[i].pos.x = x;
    cmd->vertexes[i].pos.y = y;
    cmd->vertexes[i].color = col;
    cmd->vertexes[i].uv.u = 0.0f;
    cmd->vertexes[i].uv.v = 0.0f;
    cmd->vertex_write++;
}

static void
gui_vertex_line(struct gui_draw_command* cmd, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, struct gui_color col)
{
    gui_float len;
    struct gui_vec2 a, b, d;
    struct gui_vec2 hn, hp0, hp1;
    if (!cmd) return;

    vec2_load(a, x0, y0);
    vec2_load(b, x1, y1);
    vec2_sub(d, b, a);

    if (d.x == 0.0f && d.y == 0.0f)
        return;

    len = 0.5f / vec2_len(d);
    vec2_muls(hn, d, len);
    vec2_load(hp0, +hn.y, -hn.x);
    vec2_load(hp1, -hn.y, +hn.x);

    gui_vertex(cmd, a.x + hp0.x, a.y + hp0.y, col);
    gui_vertex(cmd, b.x + hp0.x, b.y + hp0.y, col);
    gui_vertex(cmd, a.x + hp1.x, a.y + hp1.y, col);
    gui_vertex(cmd, b.x + hp0.x, b.y + hp0.y, col);
    gui_vertex(cmd, b.x + hp1.x, b.y + hp1.y, col);
    gui_vertex(cmd, a.x + hp1.x, a.y + hp1.y, col);
}

static void
gui_line(struct gui_draw_list *list, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, struct gui_color col)
{
    struct gui_draw_command *cmd;
    if (!list) return;
    if (col.a == 0) return;
    cmd = gui_push_command(list, 6, &null_rect, null_tex);
    if (!cmd) return;
    gui_vertex_line(cmd, x0, y0, x1, y1, col);
}

static void
gui_trianglef(struct gui_draw_list *list, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, gui_float x2, gui_float y2, struct gui_color c)
{
    struct gui_draw_command *cmd;
    if (!list) return;
    if (c.a == 0) return;
    cmd = gui_push_command(list, 3, &null_rect, null_tex);
    if (!cmd) return;
    gui_vertex(cmd, x0, y0, c);
    gui_vertex(cmd, x1, y1, c);
    gui_vertex(cmd, x2, y2, c);
}

static void
gui_rect(struct gui_draw_list *list, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color col)
{
    struct gui_draw_command *cmd;
    if (!list) return;
    if (col.a == 0) return;
    cmd = gui_push_command(list, 6 * 4, &null_rect, null_tex);
    if (!cmd) return;

    gui_vertex_line(cmd, x, y, x + w, y, col);
    gui_vertex_line(cmd, x + w, y, x + w, y + h, col);
    gui_vertex_line(cmd, x + w, y + h, x, y + h, col);
    gui_vertex_line(cmd, x, y + h, x, y, col);
}

static void
gui_rectf(struct gui_draw_list *list, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color col)
{
    struct gui_draw_command *cmd;
    if (!list) return;
    if (col.a == 0) return;
    cmd = gui_push_command(list, 6, &null_rect, null_tex);
    if (!cmd) return;

    gui_vertex(cmd, x, y, col);
    gui_vertex(cmd, x + w, y, col);
    gui_vertex(cmd, x + w, y + h, col);
    gui_vertex(cmd, x, y, col);
    gui_vertex(cmd, x + w, y + h, col);
    gui_vertex(cmd, x, y + h, col);
}

void
gui_begin(struct gui_draw_list *list, gui_byte *memory, gui_size size)
{
    if (!list || !memory || !size) return;
    list->begin = (struct gui_draw_command*)memory;
    list->end = (struct gui_draw_command*)memory;
    list->memory = memory;
    list->size = size;
    list->needed = 0;
}

const struct gui_draw_command*
gui_next(const struct gui_draw_list *list, const struct gui_draw_command *iter)
{
    gui_size size = 0;
    const struct gui_draw_command *cmd = NULL;
    if (!list || !list->memory || !list->begin || !list->end || !list->size)
        return NULL;
    if (!iter || !iter->vertexes || iter < list->begin || iter > list->end)
        return NULL;

    size += sizeof(struct gui_draw_command);
    size += sizeof(struct gui_vertex) * iter->vertex_count;
    cmd = (const struct gui_draw_command*)((const gui_byte*)iter + size);
    if (cmd >= list->end) return NULL;
    return cmd;
}

gui_size
gui_end(struct gui_draw_list *list)
{
    gui_size needed;
    if (!list) return 0;
    needed = list->needed;
    list->needed = 0;
    return needed;
}

gui_int
gui_button(struct gui_draw_list *list, const struct gui_input *in,
    struct gui_color bg, struct gui_color fg,
    gui_int x, gui_int y, gui_int w, gui_int h, gui_int pad,
    const char *str, gui_int len)
{
    gui_int ret = gui_false;
    if (!list || !in) return gui_false;
    if (!in->mouse_down && in->mouse_clicked) {
        const gui_int clicked_x = in->mouse_clicked_pos.x;
        const gui_int clicked_y = in->mouse_clicked_pos.y;
        if (INBOX(clicked_x, clicked_y, x, y, x+w, y+h))
            ret = gui_true;
    }
    gui_rectf(list, x, y, w, h, bg);
    gui_rect(list, x, y, w, h, fg);
    return ret;
}

gui_int
gui_slider(struct gui_draw_list *list, const struct gui_input *in,
    struct gui_color bg, struct gui_color fg,
    gui_int x, gui_int y, gui_int w, gui_int h, gui_int pad,
    gui_float min, gui_float value, gui_float max, gui_float step)
{
    gui_int mouse_x, mouse_y;
    gui_int clicked_x, clicked_y;

    gui_float cursor_w;
    gui_int cursor_x;
    gui_int cursor_y;
    gui_int cursor_h;

    if (step == 0.0f) return value;
    w = MAX(w, 2 * pad);
    h = MAX(h, 2 * pad);
    max = MAX(min, max);
    min = MIN(min, max);
    value = CLAMP(min, value, max);

    cursor_w = (w - 2 * pad) / (((max - min) + step) / step);
    cursor_h = h - 2 * pad;
    cursor_x = x + pad + (cursor_w * (value - min));
    cursor_y = y + pad;

    if (!list || !in) return 0;
    mouse_x = in->mouse_pos.x;
    mouse_y = in->mouse_pos.y;
    clicked_x = in->mouse_clicked_pos.x;
    clicked_y = in->mouse_clicked_pos.y;

    if (in->mouse_down &&
        INBOX(clicked_x, clicked_y, x, y, x + w, y + h) &&
        INBOX(mouse_x, mouse_y, x, y, x + w, y + h))
    {
        gui_float cursor_next_x;
        const gui_float tmp = mouse_x - cursor_x + (cursor_w / 2);
        gui_float next_value = value + step * ((tmp < 0) ? -1.0f : 1.0f);
        next_value = CLAMP(min, next_value, max);
        cursor_next_x = x + pad + (cursor_w * (next_value - min));
        if (INBOX(mouse_x, mouse_y, cursor_next_x, cursor_y,
                cursor_next_x + cursor_w, cursor_y + cursor_h))
        {
            value = next_value;
            cursor_x = cursor_next_x;
        }
    }
    gui_rectf(list, x, y, w, h, bg);
    gui_rectf(list, cursor_x, cursor_y, cursor_w, cursor_h, fg);
    return value;
}

gui_int
gui_progress(struct gui_draw_list *list, const struct gui_input *in,
    struct gui_color bg, struct gui_color fg,
    gui_int x, gui_int y, gui_int w, gui_int h, gui_int pad,
    gui_size value, gui_size max, gui_bool modifyable)
{
    gui_int mouse_x, mouse_y;
    gui_float scale;
    gui_int cursor_x;
    gui_int cursor_y;
    gui_int cursor_w;
    gui_int cursor_h;

    if (!list || !in) return 0;
    mouse_x = in->mouse_pos.x;
    mouse_y = in->mouse_pos.y;

    w = MAX(w, 2 * pad + 1);
    h = MAX(h, 2 * pad + 1);
    value = MIN(value, max);

    if (modifyable && in->mouse_down && INBOX(mouse_x, mouse_y, x, y, x+w, y+h)) {
        gui_float ratio = (gui_float)(mouse_x - x) / (gui_float)w;
        value = (gui_size)((gui_float)max * ratio);
    }

    if (!max) return value;
    value = MIN(value, max);
    scale = (gui_float)value / (gui_float)max;
    cursor_h = h - 2 * pad;
    cursor_w = (w - 2 * pad) * scale;
    cursor_x = x + pad;
    cursor_y = y + pad;
    gui_rectf(list, x, y, w, h, bg);
    gui_rectf(list, cursor_x, cursor_y, cursor_w, cursor_h, fg);
    return value;
}

gui_int
gui_scroll(struct gui_draw_list *list, const struct gui_input *in,
    struct gui_color bg, struct gui_color fg,
    gui_int x, gui_int y, gui_int w, gui_int h,
    gui_int offset, gui_int dst)
{
    gui_int mouse_x;
    gui_int mouse_y;
    gui_int prev_x;
    gui_int prev_y;

    gui_bool up, down;
    gui_int button_size;
    gui_int button_half;
    gui_int button_y;
    gui_int bar_h;
    gui_int bar_y;

    gui_float ratio;
    gui_float off;
    gui_int cursor_x;
    gui_int cursor_y;
    gui_int cursor_w;
    gui_int cursor_h;
    gui_int cursor_px;
    gui_int cursor_py;
    gui_bool inscroll;
    gui_bool incursor;

    gui_int pad;
    gui_int xoff, yoff, boff;
    gui_int xpad, ypad;
    gui_int xmid;

    if (!list || !in) return 0;
    gui_rectf(list, x, y, w, h, bg);
    if (dst <= h) return 0;

    mouse_x = in->mouse_pos.x;
    mouse_y = in->mouse_pos.y;
    prev_x = in->mouse_prev.x;
    prev_y = in->mouse_prev.y;

    w = MAX(w, 0);
    h = MAX(h, 2 * w);

    pad = w / 4;
    button_size = w;
    button_half = button_size / 2;
    button_y = y + h - button_size;
    bar_h = h - 2 * button_size;
    bar_y = y + button_size;

    ratio = (gui_float)bar_h/(gui_float)dst;
    off = (gui_float)offset/(gui_float)dst;
    cursor_h = (gui_int)(ratio * (gui_float)bar_h);
    cursor_y = bar_y + (gui_int)(off * bar_h);
    cursor_w = w;
    cursor_x = x;

    xpad = x + pad;
    ypad = button_y + pad;
    xmid = x + button_half;
    xoff = x + (button_size - pad);
    yoff = y + (button_size - pad);
    boff = button_y + (button_size - pad);

    up = gui_button(list, in, fg, bg, x, y, button_size, button_size, 0, "", 0);
    down = gui_button(list,in,fg,bg,x,button_y,button_size,button_size,0,"", 0);
    gui_trianglef(list, xmid, y + pad, xoff, yoff, xpad, yoff, bg);
    gui_trianglef(list, xpad, ypad, xoff, ypad, xmid, boff, bg);

    cursor_px = cursor_x + cursor_w;
    cursor_py = cursor_y + cursor_h;
    inscroll = INBOX(mouse_x, mouse_y, x, y, x + w, y + h);
    incursor = INBOX(prev_x, prev_y, cursor_x, cursor_y, cursor_px, cursor_py);
    if (in->mouse_down && inscroll && incursor)
    {
        const gui_float pixel = in->mouse_delta.y;
        const gui_float delta = (pixel/(gui_float)bar_h) * (gui_float)dst;
        offset = CLAMP(0, offset + delta, dst - bar_h);
        cursor_y += pixel;
    } else if (up || down) {
        const gui_int h2 = h/2;
        offset = (down) ? MIN(offset + h2, dst - bar_h) : MAX(0, offset - h2);
        off = (gui_float)offset/(gui_float)dst;
        cursor_y = bar_y + (gui_int)(off * bar_h);
    }

    gui_rectf(list, cursor_x, cursor_y, cursor_w, cursor_h, fg);
    gui_rect(list, cursor_x, cursor_y, cursor_w, cursor_h, bg);
    return offset;
}


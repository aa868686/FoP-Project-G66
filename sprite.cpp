#include "sprite.h"
#include <SDL2/SDL_ttf.h>
#include "font_manager.h"
#include <algorithm>
#include <cmath>
#include "font_manager.h"

namespace gfx {

    static float deg_to_rad(float deg) {
        return deg * M_PI / 180.0f;
    }

    // Converts direction to unit vector
    // direction = 90 means facing right
    static void direction_to_unit(float direction_deg, float &out_dx, float &out_dy) {
        float rad = deg_to_rad(direction_deg);
        out_dx = std::cos(rad);
        out_dy = -std::sin(rad);
    }

    static bool has_valid_costume(const sprite &s) {
        if (s.costumes.empty()) {
            return false;
        }
        int i = s.current_costume;
        if (i < 0 || i >= (int) s.costumes.size()) {
            return false;
        }
        const Costume &c = s.costumes[i];
        return c.texture != nullptr && c.tex_w > 0 && c.tex_h > 0;
    }


    sprite sprite_make(int id, const char *name) {
        sprite s;
        s.id = id;
        if (name) {
            s.name = name;
        }

        s.visible = true;
        s.draggable = false;

        s.x = 0.0f;
        s.y = 0.0f;

        s.direction_deg = 90.0f;
        s.size_percent = 100.0f;

        s.current_costume = 0;
        s.drag = dragState{};

        return s;
    }

    void sprite_set_visible(sprite &s, bool v) {
        s.visible = v;
    }

    void sprite_set_draggable(sprite &s, bool d) {
        s.draggable = d;
    }

    void sprite_set_position(sprite &s, float x, float y) {
        s.x = x;
        s.y = y;
    }

    void sprite_set_direction(sprite &s, float deg) {
        s.direction_deg = deg;
    }

    void sprite_set_size(sprite &s, float size_percent) {
        s.size_percent = std::max(0.0f, size_percent);
    }


    bool sprite_get_render_size(const sprite &s, float &out_w, float &out_h) {
        if (!has_valid_costume(s)) {
            out_w = 0;
            out_h = 0;
            return false;
        }
        const Costume &c = s.costumes[s.current_costume];
        float scale = s.size_percent / 100.0f;
        out_w = (float) c.tex_w * scale;
        out_h = (float) c.tex_h * scale;
        return true;
    }

    SDL_FRect sprite_get_aabb(const sprite &s) {
        float w = 0, h = 0;
        sprite_get_render_size(s, w, h);

        SDL_FRect r{};
        r.w = w;
        r.h = h;
        r.x = s.x - (w * 0.5f);
        r.y = s.y - (h * 0.5f);
        return r;
    }

    void sprite_move_steps(sprite &s, float steps) {
        float dx, dy;
        direction_to_unit(s.direction_deg, dx, dy);
        s.x += dx * steps;
        s.y += dy * steps;
    }

    void sprite_turn_degree(sprite &s, float deg) {
        s.direction_deg += deg;

        // Normalize angle to [0,360)
        while (s.direction_deg >= 360.0f) {
            s.direction_deg -= 360.0f;
        }
        while (s.direction_deg < 0.0f) {
            s.direction_deg += 360.0f;
        }
    }

    void sprite_clamp_to_stage(sprite &s, const stage_rectangle &stage) {
        if (stage.w <= 0 || stage.h <= 0) {
            return;
        }


        const float min_x = 0.0f;
        const float max_x = (float) stage.w;
        const float min_y = 0.0f;
        const float max_y = (float) stage.h;

        s.x = std::min(std::max(s.x, min_x), max_x);
        s.y = std::min(std::max(s.y, min_y), max_y);

    }


    int sprite_add_costume(sprite &s, SDL_Texture *tex, int tex_w, int tex_h, const char *costume_name , const char * filepath ) {
        Costume c;
        c.texture = tex;
        c.tex_w = tex_w;
        c.tex_h = tex_h;
        if (costume_name) {
            c.name = costume_name;
        }

        if ( filepath ) {
            c.filepath = filepath ;
        }

        s.costumes.push_back(c);

        if ((int) s.costumes.size() == 1) {
            s.current_costume = 0;
        }

        return (int) s.costumes.size() - 1;
    }


    bool sprite_set_costume(sprite &s, int index) {
        if (index < 0 || index >= (int) s.costumes.size()) {
            return false;
        }
        s.current_costume = index;
        return true;
    }

    bool sprite_set_costume_by_name(sprite &s, const char *costume_name) {
        if (!costume_name) {
            return false;
        }

        for (int i = 0; i < (int) s.costumes.size(); ++i) {
            if (s.costumes[i].name == costume_name) {
                s.current_costume = i;
                return true;
            }
        }
        return false;
    }


    bool sprite_replace_costume_texture(sprite &s, int index, SDL_Texture *new_tex, int new_w, int new_h) {
        if (index < 0 || index >= (int) s.costumes.size()) {
            return false;
        }
        if (!new_tex || new_w <= 0 || new_h <= 0) {
            return false;
        }

        s.costumes[index].texture = new_tex;
        s.costumes[index].tex_w = new_w;
        s.costumes[index].tex_h = new_h;
        return true;
    }


    bool sprite_replace_current_texture(sprite &s, SDL_Texture *new_tex, int new_w, int new_h) {
        if (s.costumes.empty()) {
            return false;
        }

        return sprite_replace_costume_texture(s, s.current_costume, new_tex, new_w, new_h);
    }


    void sprite_draw(SDL_Renderer *ren, const sprite &s, const stage_rectangle &stage, TTF_Font *font) {
        if (!ren || !s.visible || !has_valid_costume(s)) {
            return;
        }

        const Costume &c = s.costumes[s.current_costume];

        SDL_FRect aabb = sprite_get_aabb(s);

        SDL_FRect dst;
        dst.x = aabb.x + (float) stage.x;
        dst.y = aabb.y + (float) stage.y;
        dst.w = aabb.w;
        dst.h = aabb.h;

        SDL_FPoint center{dst.w * 0.5f, dst.h * 0.5f};


        double angle = (double) (s.direction_deg - 90.0f);

        SDL_RenderCopyExF(ren, c.texture, nullptr, &dst, angle, &center, SDL_FLIP_NONE);

        if (s.say_visible && !s.say_text.empty()) {
            float rw, rh;
            sprite_get_render_size(s, rw, rh);

            int cx = stage.x + (int) s.x;
            int cy = stage.y + (int) s.y;
            int sprite_top = cy - (int) (rh * 0.5f);

            const int pad = 12;
            const int bubble_h = 34;
            const int bubble_w = std::max(80, (int) s.say_text.size() * 10 + pad * 2);
            const int bx = cx - bubble_w / 2;
            const int by = sprite_top - bubble_h - 14;

            if (!s.think_bubble) {
                SDL_Rect r{bx, by, bubble_w, bubble_h};

                // fill white
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                SDL_RenderFillRect(ren, &r);

                // border — draw 4 sides manually, skip corners to fake rounding
                SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
                // top
                SDL_RenderDrawLine(ren, bx + 4, by, bx + bubble_w - 4, by);
                // bottom
                SDL_RenderDrawLine(ren, bx + 4, by + bubble_h, bx + bubble_w - 4, by + bubble_h);
                // left
                SDL_RenderDrawLine(ren, bx, by + 4, bx, by + bubble_h - 4);
                // right
                SDL_RenderDrawLine(ren, bx + bubble_w, by + 4, bx + bubble_w, by + bubble_h - 4);

                // tail
                int tail_tip_x = cx - 8;
                int tail_tip_y = sprite_top;
                int tail_base_x = bx + 20;
                int tail_base_y = by + bubble_h;

                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                for (int i = 0; i <= 10; i++) {
                    SDL_RenderDrawLine(ren, tail_base_x + i, tail_base_y, tail_tip_x, tail_tip_y);
                }
                SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
                SDL_RenderDrawLine(ren, tail_base_x, tail_base_y, tail_tip_x, tail_tip_y);
                SDL_RenderDrawLine(ren, tail_base_x + 10, tail_base_y, tail_tip_x, tail_tip_y);
            } else {
                // ── THINK bubble ────────────────────────────
                // cloud shape: center + bumps around edges
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                SDL_Rect center{bx + 6, by + 6, bubble_w - 12, bubble_h - 12};
                SDL_Rect wide{bx, by + 10, bubble_w, bubble_h - 20};
                SDL_Rect tall{bx + 10, by, bubble_w - 20, bubble_h};
                SDL_Rect bl{bx, by + bubble_h / 2, 14, 14};
                SDL_Rect br{bx + bubble_w - 14, by + bubble_h / 2, 14, 14};
                SDL_Rect bt{bx + bubble_w / 2 - 7, by - 2, 14, 14};
                SDL_Rect bb{bx + bubble_w / 2 - 7, by + bubble_h - 8, 14, 14};

                // border FIRST
                SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
                SDL_RenderDrawRect(ren, &wide);
                SDL_RenderDrawRect(ren, &bl);
                SDL_RenderDrawRect(ren, &br);
                SDL_RenderDrawRect(ren, &bt);

// fill WHITE on top (covers inner border edges)
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                SDL_RenderFillRect(ren, &center);
                SDL_RenderFillRect(ren, &wide);
                SDL_RenderFillRect(ren, &tall);
                SDL_RenderFillRect(ren, &bl);
                SDL_RenderFillRect(ren, &br);
                SDL_RenderFillRect(ren, &bt);
                SDL_RenderFillRect(ren, &bb);

                // think dots leading to sprite
                SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                int dot_sizes[] = {7, 5, 3};
                int dot_xs[] = {cx - 6, cx - 2, cx + 2};
                int dot_ys[] = {sprite_top - 4, sprite_top + 4, sprite_top + 10};
                for (int i = 0; i < 3; i++) {
                    SDL_Rect dot{dot_xs[i] - dot_sizes[i],
                                 dot_ys[i] - dot_sizes[i],
                                 dot_sizes[i] * 2, dot_sizes[i] * 2};
                    SDL_RenderFillRect(ren, &dot);
                    SDL_SetRenderDrawColor(ren, 100, 100, 100, 255);
                    SDL_RenderDrawRect(ren, &dot);
                    SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
                }
            }

            // text — same for both
            if (font) {
                SDL_Rect text_area{bx + pad, by + (bubble_h - 16) / 2, bubble_w - pad * 2, 20};
                fnt::draw_text_left(ren, font, s.say_text.c_str(), text_area, {30, 30, 30, 255});
            }
        }
    }


    bool sprite_hit_test_aabb(const sprite &s, int mouse_px, int mouse_py, const stage_rectangle &stage) {
        if (!s.visible || !has_valid_costume(s)) {
            return false;
        }

        SDL_FRect aabb = sprite_get_aabb(s);

        float x = aabb.x + (float) stage.x;
        float y = aabb.y + (float) stage.y;

        return (mouse_px >= x &&
                mouse_px <= x + aabb.w &&
                mouse_py >= y &&
                mouse_py <= y + aabb.h
        );
    }

    void sprite_drag_begin(sprite &s, int mouse_px, int mouse_py, const stage_rectangle &stage) {
        if (!s.draggable) {
            return;
        }
        if (!sprite_hit_test_aabb(s, mouse_px, mouse_py, stage)) {
            return;
        }


        float mx_stage = (float) (mouse_px - stage.x);
        float my_stage = (float) (mouse_py - stage.y);

        s.drag.dragging = true;
        s.drag.grab_dx = s.x - mx_stage;
        s.drag.grab_dy = s.y - my_stage;

    }

    void sprite_drag_update(sprite &s, int mouse_px, int mouse_py, const stage_rectangle &stage) {
        if (!s.drag.dragging) {
            return;
        }

        float mx_stage = (float) (mouse_px - stage.x);
        float my_stage = (float) (mouse_py - stage.y);

        s.x = mx_stage + s.drag.grab_dx;
        s.y = my_stage + s.drag.grab_dy;

        sprite_clamp_to_stage(s, stage);
    }

    void sprite_drag_end(sprite &s) {
        s.drag.dragging = false;
    }


    static constexpr int sm_icon_w = 64;
    static constexpr int sm_icon_h = 64;
    static constexpr int sm_pad = 6;
    static constexpr int sm_name_h = 16;
    static constexpr int sm_item_w = sm_icon_w + (sm_pad * 2);
    static constexpr int sm_item_h = sm_icon_h + sm_name_h + (sm_pad * 3);
    static constexpr int sm_btn_w = 28;
    static constexpr int sm_btn_h = 28;

    int sprite_manager_add(sprite_manager &sm, sprite s) {
        sm.sprites.push_back(s);
        return static_cast <int> ( sm.sprites.size()) - 1;
    }

    void sprite_manager_remove(sprite_manager &sm, int index) {
        if (index < 0 || index >= static_cast <int> ( sm.sprites.size())) {
            return;
        }

        sm.sprites.erase(sm.sprites.begin() + index);
        if (sm.active >= static_cast <int> ( sm.sprites.size())) {
            sm.active = static_cast <int> ( sm.sprites.size()) - 1;
        }
    }

    void sprite_manager_select(sprite_manager &sm, int index) {
        if (index >= -1 && index < static_cast <int> ( sm.sprites.size())) {
            sm.active = index;
        }
    }

    static void sm_fill(SDL_Renderer *ren, SDL_Rect r,
                        Uint8 rr, Uint8 g, Uint8 b
    ) {
        SDL_SetRenderDrawColor(ren, rr, g, b, 255);
        SDL_RenderFillRect(ren, &r);
    }

    static void sm_outline(SDL_Renderer *ren, SDL_Rect r,
                           Uint8 rr, Uint8 g, Uint8 b
    ) {
        SDL_SetRenderDrawColor(ren, rr, g, b, 255);
        SDL_RenderDrawRect(ren, &r);
    }

    static bool sm_hit(SDL_Rect r, int x, int y) {
        return x >= r.x && x < r.x + r.w &&
               y >= r.y && y < r.y + r.h;
    }


    void sprite_manager_render(SDL_Renderer *ren,
                               sprite_manager &sm,
                               SDL_Rect panel,
                               TTF_Font *font
    ) {

        if (!ren) {
            return;
        }

        SDL_RenderSetClipRect(ren, &panel);

        sm_fill(ren, panel, 26, 26, 26);
        sm_outline(ren, panel, 60, 60, 60);

        SDL_Rect add_btn{
                panel.x + panel.w - sm_btn_w - sm_pad,
                panel.y + sm_pad,
                sm_btn_w, sm_btn_h
        };
        sm_fill(ren, add_btn, 40, 140, 60);
        sm_outline(ren, add_btn, 80, 80, 80);
        if (font) {
            fnt::draw_text_centered(ren, font, "+", add_btn, {255, 255, 255, 255});
        }

        const int count = static_cast <int> ( sm.sprites.size());
        for (int i = 0; i < count; ++i) {
            const sprite &s = sm.sprites[i];

            SDL_Rect item{
                    panel.x + sm_pad + (i * (sm_item_w + sm_pad)),
                    panel.y + sm_pad,
                    sm_item_w,
                    sm_item_h
            };

            if ((item.x + item.w) < panel.x || item.x > (panel.x + panel.w)) {
                continue;
            }

            if (i == sm.active) {
                sm_fill(ren, item, 50, 80, 120);
                sm_outline(ren, item, 80, 140, 200);
            } else {
                sm_fill(ren, item, 35, 35, 35);
                sm_outline(ren, item, 60, 60, 60);
            }


            SDL_Rect icon{
                    item.x + sm_pad,
                    item.y + sm_pad,
                    sm_icon_w,
                    sm_icon_h
            };

            if (!s.costumes.empty() && s.costumes[s.current_costume].texture) {
                SDL_RenderCopy(ren, s.costumes[s.current_costume].texture, nullptr, &icon);
            } else {
                sm_fill(ren, icon, 60, 60, 60);
                sm_outline(ren, icon, 90, 90, 90);
            }

            if (font && !s.name.empty()) {
                SDL_Rect name_rect{
                        item.x,
                        item.y + sm_pad + sm_icon_h + sm_pad,
                        sm_item_w,
                        sm_name_h
                };
                fnt::draw_text_centered(ren, font, s.name.c_str(),
                                        name_rect, {200, 200, 200, 255}
                );

            }

            SDL_Rect del_btn{
                    item.x + item.w - 16,
                    item.y,
                    16, 16
            };

            sm_fill(ren, del_btn, 160, 40, 40);
            sm_outline(ren, del_btn, 80, 80, 80);
            if (font) {
                fnt::draw_text_centered(ren, font, "x", del_btn, {255, 255, 255, 255});
            }

        }

        SDL_RenderSetClipRect(ren, nullptr);

    }


    bool sprite_manager_handle_click(sprite_manager &sm,
                                     SDL_Rect panel,
                                     int mx, int my
    ) {
        if (!sm_hit(panel, mx, my)) {
            return false;
        }

        SDL_Rect add_btn{
                panel.x + panel.w - sm_btn_w - sm_pad,
                panel.y + sm_pad,
                sm_btn_w, sm_btn_h
        };

        if (sm_hit(add_btn, mx, my)) {
            // TODO: open file dialog to load sprite image
            return true;
        }


        const int count = static_cast <int> ( sm.sprites.size());
        for (int i = 0; i < count; ++i) {
            SDL_Rect item{
                    panel.x + sm_pad + (i * (sm_item_w + sm_pad)),
                    panel.y + sm_pad,
                    sm_item_w, sm_item_h
            };

            if (!sm_hit(item, mx, my)) {
                continue;
            }


            SDL_Rect del_btn{
                    item.x + item.w - 16, item.y, 16, 16
            };

            if (sm_hit(del_btn, mx, my)) {
                sprite_manager_remove(sm, i);
                return true;
            }


            sprite_manager_select(sm, i);
            return true;
        }

        return true;
    }


}

#include "sound_manager.h"
#include "font_manager.h"
#include <cstdio>
#include <algorithm>

namespace snd {

    static constexpr int row_h = 36;
    static constexpr int mute_w = 28;
    static constexpr int mute_h = 22;
    static constexpr int slider_h = 8;
    static constexpr int pad = 6;


    bool sound_init() {
        if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) != 0) {
            std::fprintf(stderr, "Mix_OpenAudio failed: %s\n", Mix_GetError());
            return false;
        }
        return true;
    }

    void sound_quit() {
        Mix_CloseAudio();
        Mix_Quit();
    }


    int sound_add(sound_manager &sm,
                  const std::string &filepath,
                  const std::string &name) {
        sound_entry se{};
        se.name = name.empty() ? filepath : name;
        se.filepath = filepath;
        se.chunk = Mix_LoadWAV (filepath.c_str());

        if (!se.chunk) {
            std::fprintf(stderr, "Mix_LoadWAV failed for '%s': %s\n", filepath.c_str(), Mix_GetError());
            return -1;
        }

        Mix_VolumeChunk(se.chunk, static_cast <int> ( se.volume / 100.0f * MIX_MAX_VOLUME ));

        sm.sounds.push_back(se);
        return static_cast <int> ( sm.sounds.size()) - 1;
    }


    void sound_remove(sound_manager &sm, int index) {
        if (index < 0 || index >= static_cast <int> ( sm.sounds.size())) {
            return;
        }
        sound_stop(sm.sounds[index]);

        if (sm.sounds[index].chunk) {
            Mix_FreeChunk(sm.sounds[index].chunk);
        }

        sm.sounds.erase(sm.sounds.begin() + index);

        if (sm.selected >= static_cast<int> ( sm.sounds.size())) {
            sm.selected = static_cast<int> ( sm.sounds.size()) - 1;
        }
    }


    void sound_play(sound_entry &se) {
        if (!se.chunk || se.muted) {
            return;
        }
        se.channel = Mix_PlayChannel (-1, se.chunk, 0);
    }

    void sound_stop(sound_entry &se) {
        if (se.channel >= 0) {
            Mix_HaltChannel(se.channel);
            se.channel = -1;
        }
    }

    void sound_stop_all(sound_manager &sm) {
        for (auto &se: sm.sounds) {
            sound_stop(se);
        }
    }


    void sound_set_volume(sound_entry &se, int vol) {
        se.volume = std::max(0, std::min(100, vol));
        if (se.chunk) {
            const int mix_vol = static_cast <int> (
                    se.volume / 100.0f * MIX_MAX_VOLUME );
            Mix_VolumeChunk(se.chunk, se.muted ? 0 : mix_vol);
        }
    }

    void sound_set_mute(sound_entry &se, bool m) {
        se.muted = m;
        if (se.chunk) {
            const int mix_vol = m ? 0 : static_cast <int> ( se.volume / 100.0f * MIX_MAX_VOLUME );
            Mix_VolumeChunk(se.chunk, mix_vol);
        }
        if (m) {
            sound_stop(se);
        }
    }


    static SDL_Rect row_rect(SDL_Rect panel, int i) {
        return SDL_Rect{panel.x,
                        panel.y + i * row_h,
                        panel.w,
                        row_h};
    }


    static SDL_Rect mute_rect(SDL_Rect row) {
        return SDL_Rect{row.x + pad,
                        row.y + (row_h - mute_h) / 2,
                        mute_w,
                        mute_h};
    }


    static SDL_Rect slider_track_rect(SDL_Rect row) {
        const int left = pad + mute_h + pad;
        const int right = pad;
        return SDL_Rect{row.x + left,
                        row.y + (row_h - slider_h) / 2,
                        row.w - left - right,
                        slider_h};
    }


    static SDL_Rect slider_handle_rect(SDL_Rect track, int volume) {
        const int hW = 10;
        const int hH = 16;
        const float t = volume / 100.0f;
        const int cx = track.x + static_cast <int> ( t * track.w );
        return SDL_Rect{cx - hW / 2,
                        track.y + (slider_h - hH) / 2,
                        hW, hH};
    }

    static bool pt_in(SDL_Rect r, int x, int y) {
        return x >= r.x && x < r.x + r.w &&
               y >= r.y && y < r.y + r.h;
    }

    static void fill(SDL_Renderer *ren, SDL_Rect r,
                     Uint8 rr, Uint8 g, Uint8 b) {
        SDL_SetRenderDrawColor(ren, rr, g, b, 255);
        SDL_RenderFillRect(ren, &r);
    }

    static void outline(SDL_Renderer *ren, SDL_Rect r,
                        Uint8 rr, Uint8 g, Uint8 b) {
        SDL_SetRenderDrawColor(ren, rr, g, b, 255);
        SDL_RenderDrawRect(ren, &r);
    }


    void sound_render(SDL_Renderer *ren ,
                      sound_manager &sm ,
                      SDL_Rect panel ,
                      TTF_Font * font
                      ) {
        if (!ren) { return; }


        fill(ren, panel, 26, 26, 26);
        outline(ren, panel, 60, 60, 60);

        const int visible_rows = panel.h / row_h;
        const int count = static_cast <int> ( sm.sounds.size());

        for (int i = 0; i < count && i < visible_rows; ++i) {
            const sound_entry &se = sm.sounds[i];
            SDL_Rect row = row_rect(panel, i);


            if (i == sm.selected) {
                fill(ren, row, 50, 80, 50);
            } else {
                fill(ren, row, 32, 32, 32);
            }
            outline(ren, row, 55, 55, 55);


            SDL_Rect mb = mute_rect(row);
            if (se.muted) {
                fill(ren, mb, 160, 40, 40);
            } else {
                fill(ren, mb, 40, 140, 60);
            }
            outline(ren, mb, 80, 80, 80);


            SDL_Rect track = slider_track_rect(row);
            fill(ren, track, 50, 50, 50);
            outline(ren, track, 80, 80, 80);


            if (track.w > 0) {
                SDL_Rect filled = track;
                filled.w = static_cast <int> ( se.volume / 100.0f * track.w );
                fill(ren, filled, 60, 140, 200);
            }


            SDL_Rect handle = slider_handle_rect(track, se.volume);
            fill(ren, handle, 200, 200, 200);
            outline(ren, handle, 120, 120, 120);

            if ( font && !se.name.empty() ) {
                SDL_Rect name_rect { row.x + pad + mute_w + pad ,
                                     row.y ,
                                     row.w / 3 ,
                                     row_h
                                     } ;
                fnt :: draw_text_left ( ren , font , se.name.c_str () , name_rect , { 200 , 200 , 200 , 255 } ) ;
            }
        }


        if (count == 0) {
            fill(ren, panel, 22, 22, 22);
        }
    }


    bool sound_handle_click(sound_manager &sm,
                            SDL_Rect panel,
                            int mx, int my) {
        if (!pt_in(panel, mx, my)) { return false; }

        const int i = (my - panel.y) / row_h;
        if (i < 0 || i >= static_cast <int> ( sm.sounds.size())) {
            return true;
        }

        sm.selected = i;
        sound_entry &se = sm.sounds[i];

        SDL_Rect row = row_rect(panel, i);
        SDL_Rect mb = mute_rect(row);


        if (pt_in(mb, mx, my)) {
            sound_set_mute(se, !se.muted);
            return true;
        }


        SDL_Rect track = slider_track_rect(row);
        if (pt_in(track, mx, my) && track.w > 0) {
            const float t = static_cast <float> ( mx - track.x ) / track.w;
            const int vol = static_cast<int> ( t * 100.0f );
            sound_set_volume(se, vol);
            return true;
        }

        return true;
    }


    void sound_handle_drag(sound_manager &sm,
                           SDL_Rect panel,
                           int mx, int my,
                           bool mouse_down) {
        if (!mouse_down) { return; }
        if (sm.selected < 0 ||
            sm.selected >= static_cast <int> ( sm.sounds.size())) { return; }

        sound_entry &se = sm.sounds[sm.selected];
        SDL_Rect row = row_rect(panel, sm.selected);
        SDL_Rect track = slider_track_rect(row);

        if (pt_in(track, mx, my) && track.w > 0) {
            const float t = static_cast <float> ( mx - track.x ) / track.w;
            const int vol = static_cast <int> ( t * 100.0f );
            sound_set_volume(se, vol);
        }
    }

}
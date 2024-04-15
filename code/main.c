#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <SDL.h>
#include <SDL_ttf.h>

#define internal static
#define local_persist static
#define global_variable static
#define Pi32 3.14159265359
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 680
#define SDL_COLOR_BACKGROUND(surface) SDL_MapRGB(surface->format, 0, 2, 114)
#define SDL_COLOR_AUX(surface) SDL_MapRGB(surface->format, 52, 22, 119)
#define SDL_COLOR_FOREGROUND(surface) SDL_MapRGB(surface->format, 163, 47, 128)
#define SDL_COLOR_HIGHLIGHT(surface) SDL_MapRGB(surface->format, 255, 99, 99)

#define array_count(a) (sizeof(a)/(sizeof((a)[0])))

typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;
typedef float real32;
typedef double real64;

global_variable bool running = true;
global_variable bool paused = true;
global_variable SDL_AudioDeviceID audio_dev;
global_variable SDL_Color font_color = {255, 99, 99, 00};
const int FPS = 5; //it's a timer, we don't need to refresh it often
const int frame_delay = 1000 / FPS;

enum font_type {small_font, normal_font, big_font};

typedef struct
{
    int16 *samples;
    int samples_per_second;
    int samples_count;
    int tone_hz;
    int tone_volume;
    int len;
} app_sound_buffer;

typedef struct
{
    int primary_tick;
    int secondary_tick;
    int primary_tone;
    int secondary_tone;
    int max_primary_count;
    int max_secondary_count;
    int current_primary_count;
    int current_secondary_count;
} app_config_t;

typedef struct
{
    SDL_Surface *back_buffer;
    SDL_Window *window;
    TTF_Font *fonts[3];
} render_context_t;

#include "ui.c"
#include "q_and_d.c"

typedef struct
{
    int ticks;
    int seconds;
    int minutes;
    bool show_info;
    app_sound_buffer sound_buffer;
    app_config_t app_config;
    render_context_t render_context;
    q_and_d_workout_t q_and_d_workout;
} app_state;

void init_audio(void)
{
    SDL_AudioSpec want, have;
    SDL_zero(want);
    want.freq = 48000;
    want.format = AUDIO_S16SYS;
    want.channels = 2;
    want.samples = 4096;
    want.callback = NULL;
    audio_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, SDL_AUDIO_ALLOW_FORMAT_CHANGE);

    if (audio_dev == 0)
    {
        SDL_Log("Failed to open audio device: %s", SDL_GetError());
    }
    else
    {
        if (have.format != want.format)
        {
            SDL_Log("We didn't get audio format. %hu", have.format);
        }

        SDL_PauseAudioDevice(audio_dev, 0);
    }
}

internal void
get_sound(app_sound_buffer *sound_buffer)
{
    local_persist real32 sine_t = 0;
    int wave_period = sound_buffer->samples_per_second / sound_buffer->tone_hz;
    int16 *sample_output = sound_buffer->samples;

    for (int sample_index = 0;
         sample_index < sound_buffer->samples_count;
         ++sample_index)
    {
        real32 sine_value = sinf(sine_t);
        int16 sample_value = (int16)(sine_value * sound_buffer->tone_volume);

        *sample_output++ = sample_value;
        *sample_output++ = sample_value;

        sine_t += 2.0f * Pi32 * 1.0f / (real32)wave_period;
    }
}

bool is_tick_active(int current_ticks, int tick)
{
    return current_ticks % tick == 0;
}

SDL_Rect
get_text_position(TTF_Font *font, const char *text)
{
    int big_text_width;
    int big_text_height;
    TTF_SizeText(font, text, &big_text_width, &big_text_height);

    SDL_Rect text_position;
    text_position.x = (WINDOW_WIDTH / 2) - (big_text_width / 2);
    text_position.y = (WINDOW_HEIGHT / 2) - (big_text_height / 2) + 100;
    text_position.w = big_text_width;
    text_position.h = big_text_height;

    return text_position;
}

void
render_timeline(app_state state)
{
    local_persist uint16 bar_w = WINDOW_WIDTH - 40;
    local_persist uint16 bar_h = 25;
    local_persist uint16 bar_x = 20;
    local_persist uint16 bar_y = 250;
    local_persist uint16 border_width = 3;

    int primary_steps = bar_w / state.app_config.max_primary_count;

    // Render the bar
    rect(state.render_context.back_buffer,
        bar_x,
        bar_y,
        bar_w,
        bar_h,
        border_width,
        SDL_COLOR_FOREGROUND(state.render_context.back_buffer),
        SDL_COLOR_BACKGROUND(state.render_context.back_buffer));

    int total_ticks = state.app_config.primary_tick * state.app_config.max_primary_count;
    float line_in_ticks = (float)bar_w / (float)total_ticks;
    int current_position = line_in_ticks * state.ticks;

    if (current_position > 0)
    {
        rect(state.render_context.back_buffer,
            bar_x,
            bar_y,
            current_position,
            bar_h,
            border_width,
            SDL_COLOR_FOREGROUND(state.render_context.back_buffer),
            SDL_COLOR_FOREGROUND(state.render_context.back_buffer));
    }

    // Render the series steps
    // Don't want to draw the first notch
    for (int i = 1; i < state.app_config.max_primary_count; i++)
    {
        rect(state.render_context.back_buffer,
            bar_x + (primary_steps * i),
            bar_y,
            3,
            bar_h,
            0,
            SDL_COLOR_AUX(state.render_context.back_buffer),
            SDL_COLOR_AUX(state.render_context.back_buffer));
    }
}

void
render_clock(app_state state)
{
    char string_buffer[10];

    sprintf(string_buffer, "%02d:%02d", state.minutes, abs(state.seconds));
    SDL_Surface *text_surface = TTF_RenderText_Solid(state.render_context.fonts[big_font], string_buffer, font_color);

    int big_text_width;
    int big_text_height;
    TTF_SizeText(state.render_context.fonts[big_font], string_buffer, &big_text_width, &big_text_height);

    SDL_Rect text_position;
    text_position.x = WINDOW_WIDTH - big_text_width - 20;
    text_position.y = 10;
    text_position.w = big_text_width;
    text_position.h = big_text_height;

    SDL_BlitSurface(text_surface, NULL, state.render_context.back_buffer, &text_position);

    SDL_FreeSurface(text_surface);
}

internal void
render_workout(app_state state)
{
    if (!state.show_info)
    {
        return;
    }

    char series[10];
    sprintf(series, "Series: %d", state.q_and_d_workout.series);

    SDL_Rect text_position;
    text_position.x = 10;
    text_position.y = 10;

    copy_text_as_surface(state.render_context.back_buffer, exercise_combo[state.q_and_d_workout.exercise_combo_index], text_position, state.render_context.fonts[small_font], font_color);
    text_position.y += 30;
    copy_text_as_surface(state.render_context.back_buffer, series, text_position, state.render_context.fonts[small_font], font_color);
    text_position.y += 30;
    copy_text_as_surface(state.render_context.back_buffer, reps_schemes[state.q_and_d_workout.reps_index], text_position, state.render_context.fonts[small_font], font_color);

    if (state.q_and_d_workout.exercise_combo_index == PU_SWINGS)
    {
        text_position.y += 30;
        copy_text_as_surface(state.render_context.back_buffer, swings_variants[state.q_and_d_workout.swings_index], text_position, state.render_context.fonts[small_font], font_color);
    }
}

void
render(app_state state)
{
    SDL_Surface *window_surface = SDL_GetWindowSurface(state.render_context.window);
    SDL_FillRect(state.render_context.back_buffer, NULL, SDL_COLOR_BACKGROUND(state.render_context.back_buffer));

    render_workout(state);
    render_clock(state);
    render_timeline(state);

    SDL_BlitScaled(state.render_context.back_buffer, NULL, window_surface, NULL);
    SDL_UpdateWindowSurface(state.render_context.window);
}


void
update_timer(app_state *state)
{
    if (!paused)
    {
        int minutes = state->ticks/60;
        state->minutes = minutes;
        state->seconds = state->ticks - (minutes * 60);

        if (state->app_config.secondary_tick > 0 &&
            state->app_config.max_secondary_count > 0 &&
            state->app_config.current_secondary_count < state->app_config.max_secondary_count &&
            is_tick_active(state->ticks, state->app_config.secondary_tick))
        {
            state->sound_buffer.tone_hz = state->app_config.secondary_tone;
            get_sound(&state->sound_buffer);
            SDL_QueueAudio(audio_dev, state->sound_buffer.samples, state->sound_buffer.len);
            state->app_config.current_secondary_count++;
        }

        if (state->ticks > 0 &&
            is_tick_active(state->ticks, state->app_config.primary_tick) &&
            (state->app_config.current_primary_count < state->app_config.max_primary_count))
        {
            state->sound_buffer.tone_hz = state->app_config.primary_tone;
            get_sound(&state->sound_buffer);
            SDL_QueueAudio(audio_dev, state->sound_buffer.samples, state->sound_buffer.len);
            state->app_config.current_primary_count++;
            state->app_config.current_secondary_count = 1;
        }

        if (state->app_config.current_primary_count == state->app_config.max_primary_count)
        {
            paused = true;
        }
        state->ticks++;
    }

    return;
}

void handle_event(SDL_Event event, app_state *state)
{
    switch(event.type)
    {
        case SDL_QUIT: {
            running = false;
        } break;

        case SDL_WINDOWEVENT: {
            switch(event.window.event)
            {
                case SDL_WINDOWEVENT_SIZE_CHANGED: {
                } break;
            }
        } break;

        case SDL_KEYDOWN: {
            switch(event.key.keysym.scancode)
            {
                case SDL_SCANCODE_ESCAPE: {
                    running = false;
                }break;

                case SDL_SCANCODE_SPACE: {
                    paused = !paused;
                }break;

                case SDL_SCANCODE_I: {
                    state->show_info = !state->show_info;
                    SDL_Log("Show_info = %d", state->show_info);
                }break;

                default: {} break;
            }
        } break;
    }
}

int main(void)
{
    SDL_DisableScreenSaver();

    SDL_Window *window = NULL;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();

    window = SDL_CreateWindow("Mutant Sunflower",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            WINDOW_WIDTH,
            WINDOW_HEIGHT,
            SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    SDL_Surface *back_buffer_surface = SDL_CreateRGBSurface(0, WINDOW_WIDTH, WINDOW_HEIGHT, 32, 0, 0, 0, 0);

    SDL_FillRect(back_buffer_surface, NULL, SDL_COLOR_BACKGROUND(back_buffer_surface));

    app_state state = {0};
    state.ticks = -10;

    q_and_d_workout_t workout = get_q_and_d_workout();
    state.app_config = get_q_and_d_config(workout);
    state.show_info = false;
    state.q_and_d_workout = workout;

    app_sound_buffer sound_buffer;
    int bytes_per_sample = sizeof(int16) * 2;
    sound_buffer.len = bytes_per_sample * 30000;
    sound_buffer.samples = alloca(sound_buffer.len);
    sound_buffer.samples_count = sound_buffer.len/bytes_per_sample;;
    sound_buffer.samples_per_second = 48000;
    sound_buffer.tone_volume = 10000;
    state.sound_buffer = sound_buffer;
    state.render_context.back_buffer = back_buffer_surface;
    state.render_context.window = window;
    state.render_context.fonts[small_font] = TTF_OpenFont("data/FiraCode-Regular.ttf", 21);
    state.render_context.fonts[normal_font] = TTF_OpenFont("data/FiraCode-Regular.ttf", 30);
    state.render_context.fonts[big_font] = TTF_OpenFont("data/FiraCode-Regular.ttf", 120);

    init_audio();
    render(state);

    uint32 frame_start;
    int frame_time;
    int tick = 0;

    while(running)
    {
        frame_start = SDL_GetTicks();

        SDL_Event event;
        while (running && paused && SDL_WaitEvent(&event))
        {
            handle_event(event, &state);
            render(state);
        }
        while (!paused && SDL_PollEvent(&event))
        {
            handle_event(event, &state);
        }

        render(state);
        frame_time = SDL_GetTicks() - frame_start;
        tick += frame_time;
        if (frame_delay > frame_time) {
            int sleep_time = frame_delay - frame_time;
            tick += sleep_time;
            SDL_Delay(sleep_time);
        }

        if (tick >= 1000) {
            update_timer(&state);
            tick = 0;
        }
    }


    SDL_DestroyWindow(state.render_context.window);
    SDL_Quit();

    return 0;
}

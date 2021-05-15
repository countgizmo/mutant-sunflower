#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <SDL2/SDL.h>
#include <SDL_ttf.h>

#define internal static
#define local_persist static
#define global_variable static
#define Pi32 3.14159265359
#define WINDOW_WIDTH 1280
#define WINDOW_HEIGHT 680

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
global_variable SDL_Color font_color = {255, 99, 99};
global_variable SDL_Color aux_font_color = {163, 47, 128};

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

typedef struct
{
    int ticks;
    int seconds;
    int minutes;
    app_sound_buffer sound_buffer;
    app_config_t app_config;
    render_context_t render_context;
} app_state;



global_variable int width = 1280, height = 680;

global_variable SDL_Rect update_rect = {
    .x = 0,
    .y = 200,
    .w = WINDOW_WIDTH,
    .h = WINDOW_HEIGHT - 100
};

#include "ui.c"
#include "q_and_d.c"

void
init_audio()
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

void 
sleep_ms(int milliseconds)
{
    uint64 nanoseconds = milliseconds * 1000000L;

    struct timespec sleep_time;
    sleep_time.tv_nsec = nanoseconds;
    sleep_time.tv_sec = 0;
    nanosleep(&sleep_time, NULL);
}

SDL_Rect
get_text_position(TTF_Font *font, const char *text)
{
    int big_text_width;
    int big_text_height;
    TTF_SizeText(font, text, &big_text_width, &big_text_height);

    SDL_Rect text_position;
    text_position.x = (WINDOW_WIDTH / 2) - (big_text_width / 2);
    text_position.y = (WINDOW_HEIGHT / 2) - (big_text_height / 2);
    text_position.w = big_text_width;
    text_position.h = big_text_height;

    return text_position;
}

void
render(app_state state, SDL_Rect update_rect)
{
    char string_buffer[10];
    Uint32 background_color = SDL_MapRGB(state.render_context.back_buffer->format, 0, 2, 114);

    sprintf(string_buffer, "%02d:%02d", state.minutes, abs(state.seconds));
    SDL_Surface *text_surface = TTF_RenderText_Solid(state.render_context.fonts[big_font], string_buffer, font_color);
    SDL_Surface *window_surface = SDL_GetWindowSurface(state.render_context.window);

    SDL_Rect text_position = get_text_position(state.render_context.fonts[big_font], string_buffer);


    // Rendering
    SDL_FillRect(state.render_context.back_buffer, &update_rect, background_color);
    SDL_BlitSurface(text_surface, NULL, state.render_context.back_buffer, &text_position);
    SDL_BlitScaled(state.render_context.back_buffer, NULL, window_surface, NULL);

    SDL_FreeSurface(text_surface);
    SDL_UpdateWindowSurface(state.render_context.window);
}

Uint32 timer_update(Uint32 interval, void *param)
{
    app_state *state = (app_state *)param;

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

        render(*state, update_rect);


        state->ticks++;

    }

    return(interval);
}

int main()
{
    SDL_DisableScreenSaver();

    SDL_Window *window = NULL;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_AUDIO);
    TTF_Init();

    window = SDL_CreateWindow("Mutant Sunflower", 
            SDL_WINDOWPOS_CENTERED, 
            SDL_WINDOWPOS_CENTERED, 
            WINDOW_WIDTH, 
            WINDOW_HEIGHT, 
            SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    SDL_Surface *window_surface = SDL_GetWindowSurface(window);
    SDL_Surface *back_buffer_surface = SDL_CreateRGBSurface(0, WINDOW_WIDTH, WINDOW_HEIGHT, 32, 0, 0, 0, 0);
    Uint32 background_color = SDL_MapRGB(back_buffer_surface->format, 0, 2, 114);
    SDL_FillRect(back_buffer_surface, NULL, background_color);

    

    app_state state = {};
    state.ticks = -10;

    q_and_d_workout_t workout = get_q_and_d_workout();
    state.app_config = get_q_and_d_config(workout);

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

    q_and_d_workout_sheet(workout, back_buffer_surface, state.render_context.fonts[normal_font], font_color);
    init_audio();
    render(state, update_rect);


    char string_buffer[10];

    // button(back_buffer_surface, small_font, 200, 10, "HELP!");

    SDL_TimerID main_timer = SDL_AddTimer(1000, timer_update, (void *)&state);

    while(running)
    {
        SDL_Event event;
        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT:
                {
                    running = false;
                } break;

                case SDL_WINDOWEVENT:
                {
                    switch(event.window.event)
                    {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:
                        {
                            window_surface = SDL_GetWindowSurface(window);
                        } break;
                    }
                } break;

                case SDL_KEYDOWN:
                {
                    switch(event.key.keysym.scancode)
                    {
                        case SDL_SCANCODE_ESCAPE:
                        {
                            running = false;
                        }break;

                        case SDL_SCANCODE_SPACE:
                        {
                            paused = !paused;
                        }

                        default:
                        {} break;
                    }
                } break;
            }
        }
    }

    SDL_Quit();

    return 0;
}


/* IDEAS:

- separate thread that re-renders the graphics? I dunno anymore
*/
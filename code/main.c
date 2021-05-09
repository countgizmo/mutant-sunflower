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
global_variable bool paused = false;
global_variable SDL_AudioDeviceID audio_dev;

typedef struct
{
    int samples_per_second;
    int samples_count;
    int16 *samples;
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
    int ticks;
    int seconds;
    int minutes;
    app_sound_buffer sound_buffer;
    app_config_t app_config;
} app_state;

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
    // struct timespec start, end;
    // clock_gettime(CLOCK_MONOTONIC_RAW, &start);
    // clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    
    // while (end.tv_nsec - start.tv_nsec < nanoseconds)
    // {
    //     clock_gettime(CLOCK_MONOTONIC_RAW, &end);
    // }
}

void *timer_update(void *vargp)
{
    app_state *state = (app_state *)vargp;
    int minutes = 0;

    while (running)
    {
        if (paused)
        {
            continue;
        }

        minutes = state->ticks/60;
        state->minutes = minutes;
        state->seconds = state->ticks - (minutes * 60);

        if (state->ticks > 0 &&
            state->app_config.secondary_tick > 0 &&
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
            state->app_config.current_secondary_count = 0;
        }

        if (state->app_config.current_primary_count == state->app_config.max_primary_count)
        {
            paused = true;
        }


        //sleep(1);

        struct timespec sleep_time;
        sleep_time.tv_nsec = 0;
        sleep_time.tv_sec = 1;
        nanosleep(&sleep_time, NULL);

        state->ticks++;
    }

    return NULL;
}

int main()
{
    SDL_DisableScreenSaver();

    SDL_Window *window = NULL;

    int width = 1280, height = 680;

    SDL_Rect update_rect = {};
    update_rect.x = 0;
    update_rect.y = 200;
    update_rect.w = width - 0;
    update_rect.h = height - 100;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);
    TTF_Init();

    TTF_Font *small_font = TTF_OpenFont("data/FiraCode-Regular.ttf", 21);
    TTF_Font *normal_font = TTF_OpenFont("data/FiraCode-Regular.ttf", 30);
    TTF_Font *big_font = TTF_OpenFont("data/FiraCode-Regular.ttf", 120);

    window = SDL_CreateWindow("Mutant Sunflower", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, 
    SDL_WINDOW_ALLOW_HIGHDPI | SDL_WINDOW_RESIZABLE);

    SDL_Surface *window_surface = SDL_GetWindowSurface(window);
    SDL_Surface *back_buffer_surface = SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
    Uint32 background_color = SDL_MapRGB(back_buffer_surface->format, 0, 2, 114);
    SDL_FillRect(back_buffer_surface, NULL, background_color);

    SDL_Color font_color = {255, 99, 99};
    SDL_Color aux_font_color = {163, 47, 128};

    SDL_Surface *text_surface = NULL;    

    int big_text_width;
    int big_text_height;
    TTF_SizeText(big_font, "00:00", &big_text_width, &big_text_height);

    SDL_Rect text_position;
    text_position.x = (width / 2) - (big_text_width / 2);
    text_position.y = (height / 2) - (big_text_height / 2);
    text_position.w = big_text_width;
    text_position.h = big_text_height;

    int normal_text_width;
    int normal_text_height;
    TTF_SizeText(normal_font, exercises[1], &normal_text_width, &normal_text_height);

    SDL_Rect exercise_text_position;
    exercise_text_position.x = (width / 2) - (normal_text_width / 2);
    exercise_text_position.y = (height / 2) - 100;
    exercise_text_position.w = normal_text_width;
    exercise_text_position.h = normal_text_height;

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

    init_audio();

    char string_buffer[10];
    
    pthread_t tid;
    
    pthread_create(&tid, NULL, timer_update, (void *)&state);

    // button(back_buffer_surface, small_font, 200, 10, "HELP!");

    int previous_seconds = 0;

    q_and_d_workout_sheet(workout, back_buffer_surface, normal_font, font_color);
    while(running && !paused)
    {
        SDL_Event event;
        SDL_PollEvent(&event);
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

                    default:
                    {} break;
                }
            } break;
        }

        if (previous_seconds == state.seconds)
        {
            continue;
        }

        previous_seconds = state.seconds;

        sprintf(string_buffer, "%02d:%02d", state.minutes, abs(state.seconds));
        text_surface = TTF_RenderText_Solid(big_font, string_buffer, font_color);

        // Rendering
        SDL_FillRect(back_buffer_surface, &update_rect, background_color);
        SDL_BlitSurface(text_surface, NULL, back_buffer_surface, &text_position);
        SDL_BlitScaled(back_buffer_surface, NULL, window_surface, NULL);

        SDL_FreeSurface(text_surface);
        SDL_UpdateWindowSurface(window);
        sleep_ms(500);
    }

    SDL_Quit();

    return 0;
}


/* IDEAS:

- separate thread that re-renders the graphics? I dunno anymore
*/
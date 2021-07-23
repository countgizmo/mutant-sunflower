/**
 * Based on the books "The Quick and the Dead"
 * 
 * Creates a training plan for the day based on the instructions from the book.
 */

const int seconds_per_week = 604800;

const char exercises[2][20] = {
        "One-arm swings",
        "Push-ups"
};

const char exercise_combo[2][20] = {
    "Snatches",
    "Push-ups + Swings"
};

const char exercise_combo_reverse[2][20] = {
    "Push-ups + Swings",
    "Snatches"
};

const int SNATCHES = 0;
const int PU_SWINGS = 1;

const char swings_variants[2][10] = {
    "1-handed",
    "2-handed"
};

const int FIVE_BY_FOUR = 0;
const int ALT_REPS = 1;
const int TEN_BY_TWO = 2;

const char reps_schemes[3][5] = {
    "5/4",
    "Alt",
    "10/2"
};

const int series[6] = {2, 3, 3, 4, 4, 5};

typedef struct
{
    int series;
    int reps_index;
    int exercise_combo_index;
} workout;

typedef struct
{
    int exercise_combo_index;
    int series;
    int reps_index;
    int swings_index;
} q_and_d_workout_t;

typedef struct
{
    time_t start_time;
    q_and_d_workout_t workout; 
} save_data_t;

internal void
init_save_data_file(char *file_name, save_data_t *data, time_t current_time)
{
    FILE *data_file = fopen(file_name, "rb");

    if (!data_file)
    {
        data_file = fopen(file_name, "wb");
        data->start_time = current_time;
        data->workout.series = 0;
        fwrite(data, sizeof(save_data_t), 1, data_file);
    }

    fclose(data_file);
}

internal void
read_save_data_file(char *file_name, save_data_t *data)
{
    FILE *data_file = fopen(file_name, "rb");
    fread(data, sizeof(save_data_t), 1, data_file);
    fclose(data_file);
}

internal void
update_save_data_file(char *file_name, save_data_t *data)
{
    FILE *data_file = fopen(file_name, "wb");
    fwrite(data, sizeof(save_data_t), 1, data_file);
    fclose(data_file);
}

internal int
weeks_passed(time_t current_time, time_t start_time)
{
    real32 diff_seconds = difftime(current_time, start_time);
    int weeks = ceil((diff_seconds / seconds_per_week));
    return(weeks);
}

// Returns random 0 - 5 inclusive
// cause our arrays are 0-indexed our die is magical.
internal int
die_roll(void)
{
    return(rand() % 6);
}

internal int
die_roll_to_coin_flip(int roll)
{
    return (roll < 3) ? 0 : 1;
}

internal int
get_series(int current_series, int weeks)
{
    int max_series;
    int series_index;

    if (weeks == 1)
    {
        max_series = array_count(series) - 3;
    } else if (weeks == 2)
    {
        max_series = array_count(series) - 1;
    } else 
    {
        max_series = array_count(series);
    }
    
    do
    {
        series_index = rand() % max_series;
    } while (series[series_index] == current_series);

    return series[series_index];
}

internal int
get_reps(int weeks)
{
    int reps;

    if (weeks == 1)
    {
        reps = 0;
    } else if (weeks == 2)
    {
        reps = rand() % (array_count(reps_schemes) - 1);
    } else
    {
        reps = rand() % array_count(reps_schemes);
    }

    return reps;
}

internal int
get_swings_index(void)
{
    return(rand() % 2);
}

internal int
get_exercise_combo_index(void)
{
    return(die_roll_to_coin_flip(die_roll()));
}

internal app_config_t 
get_q_and_d_config(q_and_d_workout_t workout)
{
    app_config_t config;
    config.primary_tone = 300;
    config.secondary_tone = 256;
    config.max_primary_count = workout.series;
    config.current_primary_count = 0;
    config.current_secondary_count = 0;

    if (workout.exercise_combo_index == SNATCHES)
    {
        config.primary_tick = 240;
    } else if (workout.exercise_combo_index == PU_SWINGS)
    {
        config.primary_tick = 180;
    }

    if (workout.reps_index == FIVE_BY_FOUR)
    {
        config.secondary_tick = 30;
        config.max_secondary_count = 4;
    } else if (workout.reps_index == TEN_BY_TWO)
    {
        config.secondary_tick = 60;
        config.max_secondary_count = 2;
    }

    return config;
}

internal q_and_d_workout_t
get_q_and_d_workout(void)
{
    FILE *data_file;
    char data_file_name[10] = "data.bin";

    q_and_d_workout_t workout = {};

    time_t current_time;
    time(&current_time);
    srand((unsigned int) current_time);

    save_data_t saved_app_data = {};
    init_save_data_file(data_file_name, &saved_app_data, current_time);
    read_save_data_file(data_file_name, &saved_app_data);
    
    int weeks = weeks_passed(current_time, saved_app_data.start_time);

    workout.exercise_combo_index = get_exercise_combo_index();
    workout.series = get_series(saved_app_data.workout.series, weeks);
    workout.reps_index = get_reps(weeks);
    workout.swings_index = get_swings_index();

    saved_app_data.workout = workout;

    update_save_data_file(data_file_name, &saved_app_data);
    
    return workout;
}
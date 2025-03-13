#include <stdio.h>

#include "raylib.h"

#define MSQ_USE_OPENMP
#include "squares.h"

#define WINDOW_WIDTH  1200
#define WINDOW_HEIGHT 900

#define GRID_ROWS 100
#define GRID_COLS 100 * WINDOW_WIDTH / WINDOW_HEIGHT

#define MAX_THRESHOLD 16

#define BALLS_COUNT 5

typedef struct {
    float pos_x[BALLS_COUNT];
    float pos_y[BALLS_COUNT];

    float vel_x[BALLS_COUNT];
    float vel_y[BALLS_COUNT];

    float rad[BALLS_COUNT];
} metaballs;

void create_balls(metaballs* balls)
{

#ifdef MSQ_USE_OPENMP
    #pragma omp parallel for 
#endif
    
    for(int i = 0; i < BALLS_COUNT; ++i)
    {
        balls->rad[i] = GetRandomValue(3, 15);

        balls->pos_x[i] = GetRandomValue(balls->rad[i], GRID_COLS - balls->rad[i]);
        balls->pos_y[i] = GetRandomValue(balls->rad[i], GRID_ROWS - balls->rad[i]);

        balls->vel_x[i] = GetRandomValue(-5, 5);
        balls->vel_y[i] = GetRandomValue(-5, 5);
    }
}

void put_balls(msq_squares_grid* grid, metaballs* balls)
{
    for(int i = 0; i < grid->rows_count; ++i)
    {
        for(int j = 0; j < grid->cols_count; ++j)
        {
            grid->field[i][j] = 0;

            for(int k = 0; k < BALLS_COUNT; ++k)
            {
                grid->field[i][j] += (balls->rad[k] * balls->rad[k]) / ((i - balls->pos_y[k]) * (i - balls->pos_y[k]) + (j - balls->pos_x[k]) * (j - balls->pos_x[k]) + 0.0001);
            }
        }
    }
}

void move_balls(metaballs* balls)
{

#ifdef MSQ_USE_OPENMP
    #pragma omp parallel for 
#endif

    for(int i = 0; i < BALLS_COUNT; ++i)
    {
        if(balls->pos_x[i] <= balls->rad[i] || balls->pos_x[i] >= GRID_COLS - balls->rad[i])
        {
            balls->vel_x[i] *= -1;
        }

        if(balls->pos_y[i] <= balls->rad[i] || balls->pos_y[i] >= GRID_ROWS - balls->rad[i])
        {
            balls->vel_y[i] *= -1;
        }

        balls->pos_x[i] += balls->vel_x[i] * GetFrameTime();
        balls->pos_y[i] += balls->vel_y[i] * GetFrameTime();
    }
}

int main()
{
    srand(time(0));

    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "squares");

    msq_squares_grid* grid = msq_get_grid(GRID_ROWS, GRID_COLS, MAX_THRESHOLD);

    metaballs balls_list;
    create_balls(&balls_list);

    put_balls(grid, &balls_list);
    msq_grid_get_indices(grid);

    while(!WindowShouldClose())
    {   
        move_balls(&balls_list);
        put_balls(grid, &balls_list);
                
        if(IsKeyPressed(KEY_N))
        {
            create_balls(&balls_list);
        }

        BeginDrawing();
        ClearBackground(BLACK);

        msq_draw_grid(grid);
        grid->threshold = 1;
        msq_grid_get_indices(grid);
        msq_grid_march(grid, RAYWHITE, 1.0);

        grid->threshold = 1.4;
        msq_grid_get_indices(grid);
        msq_grid_march(grid, GREEN, 2.0);

        EndDrawing();
    }

    msq_grid_free(grid);
    CloseWindow();

    return 0;
}

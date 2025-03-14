#ifndef MSQ_INCLUDE_SQUARES_H
#define MSQ_INCLUDE_SQUARES_H

#include <stdint.h>
#include "raylib.h"

/* 
 * Possible vertex positions for isolines
 *
 * 4 -- 5 -- 6
 * |         |
 * 3         7
 * |         |
 * 2 -- 1 -- 0
 *
 *
 * Each bit in 8-bit look-up table represents a vertex position for isoline
 *
 * If bit is 1, msq_grid_march() adds a position of new vertex to the array of line vertices
 * msq_grid_march() then uses this array to draw isolines
 * 
 */


typedef struct {
    uint32_t rows_count;
    uint32_t cols_count;

    float** field;
    uint32_t** binary_index;

    float threshold;
    float max_threshold;
} msq_squares_grid;


static const uint8_t lut_contours[16] = {
    0b00000000,
    0b00001010,
    0b10000010,
    0b10001000,
    0b10100000,
    0b10101010,
    0b00100010,
    0b00101000,
    0b00101000,
    0b00100010,
    0b10101010,
    0b10100000,
    0b10001000,
    0b10000010,
    0b00001010,
    0b00000000,
};


msq_squares_grid* msq_grid_create(uint32_t rows_count, uint32_t cols_count, float max_threshold);

void msq_grid_free(msq_squares_grid* grid);

static inline uint32_t msq_threshold(float threshold, float value);

static inline float msq_lerp(float threshold, float pos_a, float pos_b, float grid_a, float grid_b);

// fills scalar field with random values in range [0, max_threshold]
void msq_grid_fill_random(msq_squares_grid* grid);

// draws grid lines with circles
// green circle => value is above threshold
// red circle   => value is below threshold
void msq_grid_draw(msq_squares_grid* grid);

// helper function for msq_grid_get_indices
// converts scalar value to binary index
static inline uint32_t msq_get_index(msq_squares_grid* grid, uint32_t i, uint32_t j);

// initializes binary indices array for grid
// must be called before msq_grid_march
void msq_grid_get_indices(msq_squares_grid* grid);

// draws isolines based on given scalar field
void msq_grid_march(msq_squares_grid* grid, Color color, float line_width);


#endif // MSQ_INCLUDE_SQUARES_H


#ifdef MSQ_SQUARES_IMPLEMENTATION

#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifdef MSQ_USE_OPENMP
#include <omp.h>
#endif 


msq_squares_grid* msq_grid_create(uint32_t rows_count, uint32_t cols_count, float max_threshold)
{
    msq_squares_grid* grid = malloc(sizeof *grid);

    grid->rows_count = rows_count;
    grid->cols_count = cols_count;

    grid->field = malloc(rows_count * sizeof(*grid->field));
    for(uint32_t i = 0; i < rows_count; ++i)
    {
        grid->field[i] = malloc(cols_count * sizeof(grid->field[0]));
    }

    grid->binary_index = malloc((rows_count - 1) * sizeof(*grid->binary_index));

    for(uint32_t i = 0; i < rows_count - 1; ++i)
    {
        grid->binary_index[i] = malloc((cols_count - 1) * sizeof(grid->binary_index[0]));
    }

    grid->threshold = max_threshold / 2;
    grid->max_threshold = max_threshold;

    return grid;
}

void msq_grid_free(msq_squares_grid* grid)
{
    if(grid == NULL)
    {
        return;
    }

    for(uint32_t i = 0; i < grid->rows_count; ++i)
    {
        free(grid->field[i]);
    }
    free(grid->field); 

    for(uint32_t i = 0; i < grid->rows_count - 1; ++i)
    {
        free(grid->binary_index[i]);
    }
    free(grid->binary_index);

    free(grid);
    grid = NULL;
}

static inline uint32_t msq_threshold(float threshold, float value)
{
    return value > threshold ? 1 : 0;
}

static inline float msq_lerp(float threshold, float pos_a, float pos_b, float grid_a, float grid_b)
{
    grid_a /= threshold;
    grid_b /= threshold;
    return pos_a + (pos_b - pos_a) * (1 - grid_a) / (grid_b - grid_a);
}

void msq_grid_fill_random(msq_squares_grid* grid)
{

#ifdef MSQ_USE_OPENMP
    #pragma omp parallel for collapse(2)
#endif

    for(uint32_t i = 0; i < grid->rows_count; ++i)
    {
        for(uint32_t j = 0; j < grid->cols_count; ++j)
        {
            grid->field[i][j] = rand() % (int)grid->max_threshold;
        }
    }
}

void msq_grid_draw(msq_squares_grid* grid)
{
    int width  = GetScreenWidth();
    int height = GetScreenHeight();

    int x_pos = 0;
    int width_spacing = width / grid->cols_count;

    while(x_pos <= width)
    {
        DrawLine(x_pos, height, x_pos, 0, BLACK);
        x_pos += width_spacing;
    }

    int y_pos = 0;
    int height_spacing = height / grid->rows_count;

    while(y_pos <= height)
    {
        DrawLine(0, y_pos, width, y_pos, BLACK);
        y_pos += height_spacing;
    }

    int width_halfspacing = width_spacing * 0.5;
    int height_halfspacing = height_spacing * 0.5;


    for(uint32_t i = 0, y_pos = height_halfspacing; i < grid->rows_count; y_pos += height_spacing, ++i)
    {
        for(uint32_t j = 0, x_pos = width_halfspacing; j < grid->cols_count; x_pos += width_spacing, ++j)
        {
            Color circle_color = msq_threshold(grid->threshold, grid->field[i][j]) ? RED : GREEN;

            DrawCircle(x_pos, y_pos, 2, circle_color);
        }
    }
}

static inline uint32_t msq_get_index(msq_squares_grid* grid, uint32_t i, uint32_t j)
{
    uint32_t index = 0;

    index |= msq_threshold(grid->threshold, grid->field[i - 1][j]);
    index <<= 1;

    index |= msq_threshold(grid->threshold, grid->field[i - 1][j + 1]);
    index <<= 1;

    index |= msq_threshold(grid->threshold, grid->field[i][j + 1]);
    index <<= 1;

    index |= msq_threshold(grid->threshold, grid->field[i][j]);

    return index;
}


void msq_grid_get_indices(msq_squares_grid* grid)
{

#ifndef MSQ_USE_OPENMP
    #pragma omp parallel for collapse(2)
#endif

    for(uint32_t i = 1; i < grid->rows_count; ++i)
    {
        for(uint32_t j = 0; j < grid->cols_count - 1; ++j)
        {
            grid->binary_index[i - 1][j] = msq_get_index(grid, i, j);
        }
    }
}

void msq_grid_march(msq_squares_grid* grid, Color color, float line_width)
{
    int width_spacing = (GetScreenWidth() / grid->cols_count);
    int width_halfspacing = width_spacing * 0.5;

    int height_spacing = (GetScreenHeight() / grid->rows_count);
    int height_halfspacing = height_spacing * 0.5;

    for(uint32_t i = 0; i < grid->rows_count - 1; ++i)
    {
        for(uint32_t j = 0; j < grid->cols_count - 1; ++j)
        {
            int cell_x = j * width_spacing + width_halfspacing;
            int cell_y = i * height_spacing + height_halfspacing;

            int lut_index = grid->binary_index[i][j];

            if(lut_index == 0 || lut_index == 15)
            {
                continue;
            }

            Vector2 vertices[4];
            int index = 0;

            if(lut_contours[lut_index] & 128)
            {
                vertices[index].x = cell_x + width_spacing;

#ifdef NO_LERP
                vertices[index].y = cell_y + height_halfspacing;  
#else
                vertices[index].y = msq_lerp(grid->threshold, cell_y, cell_y + height_spacing, grid->field[i][j + 1], grid->field[i + 1][j + 1]);
#endif

                index++;
            }

            if(lut_contours[lut_index] & 64)
            {
                vertices[index].x = cell_x + width_spacing;
                vertices[index].y = cell_y;

                index++;
            }
            
            if(lut_contours[lut_index] & 32)
            {

#ifdef MSQ_NO_LERP
                vertices[index].x = cell_x + width_halfspacing; 
#else
                vertices[index].x = msq_lerp(grid->threshold, cell_x, cell_x + width_spacing, grid->field[i][j], grid->field[i][j + 1]); 
#endif 

                vertices[index].y = cell_y;

                index++;
            }

            if(lut_contours[lut_index] & 16)
            {
                vertices[index].x = cell_x;
                vertices[index].y = cell_y;

                index++;
            }

            if(lut_contours[lut_index] & 8) 
            {
                vertices[index].x = cell_x;

#ifdef MSQ_NO_LERP
                vertices[index].y = cell_y + height_halfspacing;
#else
                vertices[index].y = msq_lerp(grid->threshold, cell_y, cell_y + height_spacing, grid->field[i][j], grid->field[i + 1][j]);
#endif 

                index++;
            }

            if(lut_contours[lut_index] & 4) 
            {
                vertices[index].x = cell_x;
                vertices[index].y = cell_y + height_spacing;

                index++;
            } 

            if(lut_contours[lut_index] & 2) 
            {

#ifdef MSQ_NO_LERP
                vertices[index].x = cell_x + width_halfspacing;
#else
                vertices[index].x = msq_lerp(grid->threshold, cell_x, cell_x + width_spacing, grid->field[i + 1][j], grid->field[i + 1][j + 1]);
#endif 

                vertices[index].y = cell_y + height_spacing;

                index++;
            } 

            if(lut_contours[lut_index] & 1)
            {
                vertices[index].x = cell_x + width_spacing; 
                vertices[index].y = cell_y + height_spacing; 

                index++;
            } 

            if(index == 4)
            {
                if(msq_threshold(grid->threshold, (grid->field[i][j] + grid->field[i][j + 1] + grid->field[i + 1][j] + grid->field[i + 1][j + 1]) / 4.0))
                {
                     DrawLineEx(vertices[0], vertices[3], line_width, color);
                     DrawLineEx(vertices[1], vertices[2], line_width, color); 
                } else {
                    DrawLineEx(vertices[0], vertices[1], line_width, color);
                    DrawLineEx(vertices[2], vertices[3], line_width, color);
                }

                continue;
            }

            DrawLineEx(vertices[0], vertices[1], line_width, color);
        }
    }
}


#endif // MSQ_SQUARES_IMPLEMENTATION

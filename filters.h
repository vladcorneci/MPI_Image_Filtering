#include <stdio.h>
#include <stdlib.h>
#include "my_alloc.h"
#include "topology.h"

#define FILTER_SIZE 3

#define TAG_DIM1 32
#define TAG_DIM2 33

#define TAG_RESULT  420

typedef struct {
    int matrix[FILTER_SIZE][FILTER_SIZE];
    int fraction;
    int offset;
}Filter;

typedef struct {
  int ** pixels;
  int width;
  int height;
}Image;

enum filter_types { SMOOTH, BLUR, SHARPEN, MEAN_REMOVAL, EMBOSS, EDGE_DETECTION};

const static Filter smooth_filter = {{
  {1, 1, 1},
  {1, 1, 1},
  {1, 1, 1}}, 9, 0};

const static Filter blur_filter = {{
  {1, 2, 1},
  {2, 4, 2},
  {1, 2, 1}}, 16, 0};

const static Filter sharpen_filter = {{
  {0,  -2,  0},
  {-2, 11, -2},
  {0,  -2,  0}}, 3, 0};

const static Filter mean_removal_filter = {{
  {-1, -1, -1},
  {-1,  9, -1},
  {-1, -1, -1}}, 1, 0};

const static Filter emboss_filter = {{
  {-1, -1, 0},
  {-1,  0, -1},
  {0,   1, 1}}, 1, 128};

const static Filter edge_detection_filter = {{
  {-1, -1, -1},
  {-1,  8, -1},
  {-1, -1, -1}}, 1, 0};

int conv_matrix(int ** pixel, Filter filter);
int modify_pixel(int ** pixel, enum filter_types filter_type);
Image * parse_image(char * file_name);
void write_image(int ** image_pixels, int width_start, int width_end,
  int height_start, int height_end, char * output_file);
  void apply_filter(int ** adjacent_matrix, int N, int rank, int parent,
    char * input_file, char * output_file, enum filter_types filter_type,
    int * lines_processed);

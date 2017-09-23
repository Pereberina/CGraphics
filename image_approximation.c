#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#define MAGIC1 "P2"
#define MAGIC2 "P5"
#define MAGIC_SIZE sizeof(MAGIC1)
#define COMMENT (image_info->image[image_info->position] == '#')
#define SKIP_SEP while ((!COMMENT) && \
  ((image_info->image[image_info->position] <= ' ') || \
  (image_info->image[image_info->position] >= 0x7f))) { \
    image_info->position++; \
} \

#define NEXT_LINE while (image_info->image[image_info->position++] != '\n');
#define SKIP_COMMENT while(COMMENT) { \
  NEXT_LINE \
} \

#define NEXT_ELEMENT do { \
  SKIP_SEP \
  SKIP_COMMENT \
} while(0); \

#define GET_INT(element) do { \
  sscanf(&image_info->image[image_info->position], "%d%n", \
    &(element), \
    &step); \
  image_info->position += step; \
} while(0); \

typedef struct _IMAGE_INFO {
  char *image; // pointer to file memory mapping
  int height; // image height
  int width; // image width
  int maxgrey; // image max grey
  int position; // current position in file mapping
  int str_num; // current string number in pixmap
  int pix_num; // current pixel number in current string
  int size; // mapping size
} IMAGE_INFO, *PIMAGE_INFO;

/*
 * Function: parse_pgm
 * ----------------------------
 * Parses pgm image file and sets height, width,
 * maxgrey, position, str_num, pix_num fields. Position points to
 * pixmap, str_num and pix_num points to 1st pixel
 *
 * image_info: pointer to IMAGE_INFO structure
 *
 * returns: 0 if succeeded
 */
int parse_pgm(PIMAGE_INFO image_info) {
  char magic[MAGIC_SIZE];
  int step = 0;
  image_info->position = 0;

  sscanf(image_info->image, "%[^# \t\n]%n", magic, &step);
  image_info->position += step;
  if ((strcmp(magic, MAGIC1) != 0) && (strcmp(magic, MAGIC2) != 0)) {
    printf("Error: image has not PMG format\n");
    return -1;
  }
  NEXT_ELEMENT

  GET_INT(image_info->width)
  NEXT_ELEMENT

  GET_INT(image_info->height)
  NEXT_ELEMENT

  GET_INT(image_info->maxgrey)
  NEXT_LINE
  image_info->str_num = 1;
  image_info->pix_num = 1;

  return 0;
}

/*
 * Function: create_image
 * ----------------------------
 * Creates new image file, creates mapping in memory,
 * copies original image to it
 *
 * orig_image: original image path
 * new_image: new image path
 *
 * returns: pointer to IMAGE_INFO structure,
 * returns NULL if failed
 */
PIMAGE_INFO create_image(char *orig_image, char *new_image) {
  PIMAGE_INFO image_info;
  int fdin, fdout;
  struct stat statbuf;

  image_info = (PIMAGE_INFO)malloc(sizeof(IMAGE_INFO));
  if (image_info == NULL) {
    printf("Error: not enough memory");
    return NULL;
  }
  if ((fdin = open(orig_image, O_RDONLY)) < 0) {
    perror("Open input file");
    exit(errno);
  }
  if ((fdout = open(new_image, O_RDWR | O_CREAT | O_TRUNC, 0644)) < 0) {
    perror("Open output file");
    exit(errno);
  }
  if (fstat(fdin, &statbuf) < 0) {
    perror("fstat");
    exit(errno);
  }
  if (ftruncate(fdout, statbuf.st_size) < 0) {
    perror("ftruncate");
    exit(errno);
  }
  if ((orig_image = mmap(0, statbuf.st_size, PROT_READ,
    MAP_SHARED, fdin, 0)) == MAP_FAILED) {
    perror("mmap");
    exit(errno);
  }
  if ((image_info->image = mmap(0, statbuf.st_size, PROT_READ | PROT_WRITE,
    MAP_SHARED, fdout, 0)) == MAP_FAILED) {
    perror("mmap");
    exit(errno);
  }
  memcpy(image_info->image, orig_image, statbuf.st_size);
  image_info->size = statbuf.st_size;

  munmap(orig_image, statbuf.st_size);
  close(fdin);
  close(fdout);
  return image_info;
}

/*
 * Function: image_deinit
 * ----------------------------
 * Removes memory mapping and frees pointer to IMAGE_INFO
 *
 * image_info: pointer to IMAGE_INFO structure created by image_init function
 *
 * returns: void
 */
void image_deinit(PIMAGE_INFO image_info) {
  munmap(image_info->image, image_info->size);
  free(image_info);
}

/*
 * Function: image_init
 * ----------------------------
 * Creates new image file, creates mapping in memory
 * copies original image to it and parses pgm image
 *
 * orig_image: original image path
 * new_image: new image path
 *
 * returns: pointer to IMAGE_INFO structure,
 * returns NULL if failed
 */
PIMAGE_INFO image_init(char *orig_image, char *new_image) {
  PIMAGE_INFO image_info;

  image_info = create_image(orig_image, new_image);
  if (image_info != NULL)
    if (parse_pgm(image_info) < 0) {
      image_deinit(image_info);
      return NULL;
    }

  return image_info;
}

/* Algorithms */
#define BLACK 0
#define WHITE 255
#define DEFAULT_THRESHOLD ((image_info->maxgrey + 1)/2)
#define SET_PIXEL_VALUE(pixel, threshold) do { \
  pixel = ((unsigned char)pixel > (unsigned char)(threshold))?WHITE:BLACK; \
} while(0); \

#define NEXT_PIXEL do { \
  image_info->position++; \
  image_info->pix_num++; \
  if (image_info->pix_num == image_info->width + 1) { \
    image_info->pix_num = 1; \
    image_info->str_num++; \
  } \
} while(0); \

/*
 * Function: seq_way
 * ----------------------------
 * Processes pixels sequentially
 *
 * image_info: pointer to IMAGE_INFO structure
 * algo: pointer to function to call for each pixel
 * params: pointer to parameters for algo,
 * depends on algo, could be NULL
 *
 * returns: void
 */
void seq_way(PIMAGE_INFO image_info,
  void (*algo)(PIMAGE_INFO, void *),
  void *params) {
  while(image_info->str_num <= image_info->height) {
    algo(image_info, params);
    NEXT_PIXEL
  }
}

/* Thresholding algo */
/*
 * Function: thresholding
 * ----------------------------
 * Sets pixel on current pixmap position to black
 * if pixel value is less than half-maxgrey threshold,
 * to white otherwise
 *
 * image_info: pointer to IMAGE_INFO structure
 * params: unused, should be NULL
 *
 * returns: void
 */
void thresholding(PIMAGE_INFO image_info, void *params) {
  SET_PIXEL_VALUE(image_info->image[image_info->position],
    DEFAULT_THRESHOLD);
}

/* Random dithering algo */
const float RAND_MAX_F = RAND_MAX;

float get_rand() {
	return rand() / RAND_MAX_F;
}

float get_rand_range(const float min, const float max) {
	return get_rand() * (max - min) + min;
}

/*
 * Function: random_dithering
 * ----------------------------
 * Sets pixel on current pixmap position to black
 * if pixel value is less than random threshold
 * from 0 to maxgrey, to white otherwise
 *
 * image_info: pointer to IMAGE_INFO structure
 * params: unused, should be NULL
 *
 * returns: void
 */
void random_dithering(PIMAGE_INFO image_info, void *params) {
  SET_PIXEL_VALUE(image_info->image[image_info->position],
    get_rand_range(0, image_info->maxgrey));
}

/* Error diffusion algos */
#define GET_PIXEL_VALUE(pixel, error, theshold) \
  (((unsigned char)(pixel) + (error) > (theshold))? WHITE:BLACK) \

/* error diffusion type */
enum DIFFUSION_TYPE {
  DIFFUSION_TYPE_FULL = 1, // 100% error propagation
  DIFFUSION_TYPE_FLOYD // Floyd–Steinberg propagation
};

typedef struct _ERROR_FLOYD {
  int *current_str;
  int *below_str;
} ERROR_FLOYD, *PERROR_FLOYD;

/*
 * Function: error_diffusion
 * ----------------------------
 * Sets pixel on current pixmap position to black if pixel value
 * is less than half-maxgrey threshold, to white otherwise
 * Adds current pixel value error to neighbouring pixel
 * For forward propagation:
 * _____________________________
 * |        |    X    |  100%  |
 * -----------------------------
 * For back propagation:
 * _____________________________
 * |  100%  |    X    |        |
 * -----------------------------
 *
 * image_info: pointer to IMAGE_INFO structure
 * back: true for back propagation
 * diffusion_error: pointer to int
 *
 * returns: void
 */
void error_diffusion(PIMAGE_INFO image_info, bool back, void *diffusion_error) {
  unsigned char value;

  value = GET_PIXEL_VALUE(image_info->image[image_info->position],
    *((int *)diffusion_error),
    DEFAULT_THRESHOLD);
  if (back && (image_info->pix_num == 1) ||
    !back && (image_info->pix_num == image_info->width)) {
      *(int *)diffusion_error = 0;
   } else {
      *(int *)diffusion_error +=
            (unsigned char)image_info->image[image_info->position] - value;
   }
  image_info->image[image_info->position] = value;
}

#define UPDATE_ERROR_FLOYD do { \
  int *tmp; \
  int i; \
 \
  tmp = error->current_str; \
  error->current_str = error->below_str; \
  error->below_str = tmp; \
  for (i = 0; i < image_info->width; i++) \
    error->below_str[i] = 0; \
} while(0); \

/*
 * Function: error_diffusion_floyd
 * ----------------------------
 * Sets pixel on current pixmap position to black if pixel value
 * is less than half-maxgrey threshold, to white otherwise
 * Adds current pixel value error to neighbouring pixels using
 * Floyd–Steinberg algorithm
 * For forward propagation:
 * _____________________________
 * |        |    X    |  7/16  |
 * -----------------------------
 * |  3/16  |  5/16   |  1/16  |
 * _____________________________
 * For back propagation:
 * _____________________________
 * |  7/16  |    X    |        |
 * -----------------------------
 * |  1/16  |  5/16   |  3/16  |
 * _____________________________
 *
 * image_info: pointer to IMAGE_INFO structure
 * back: true for back propagation
 * diffusion_error: pointer to PERROR_FLOYD structure
 *
 * returns: void
 */
void error_diffusion_floyd(PIMAGE_INFO image_info,
  bool back,
  void *diffusion_error) {
  unsigned char value;
  PERROR_FLOYD error = (PERROR_FLOYD)diffusion_error;
  int value_error = 0;

  value = GET_PIXEL_VALUE(image_info->image[image_info->position],
    error->current_str[image_info->pix_num - 1],
    DEFAULT_THRESHOLD);
  value_error = (unsigned char)image_info->image[image_info->position] +
    error->current_str[image_info->pix_num - 1] - value;
  if (back) {
    if (image_info->pix_num > 1) {
      error->current_str[image_info->pix_num - 2] += 7./16*value_error;
      error->below_str[image_info->pix_num - 2] += 1./16*value_error;
    }
    error->below_str[image_info->pix_num - 1] += 5./16*value_error;
    if (image_info->pix_num < image_info->width)
      error->below_str[image_info->pix_num] += 3./16*value_error;

    if (image_info->pix_num == 1) {
      UPDATE_ERROR_FLOYD
    }
  } else {
    if (image_info->pix_num < image_info->width) {
      error->current_str[image_info->pix_num] += 7./16*value_error;
      error->below_str[image_info->pix_num] += 1./16*value_error;
    }
    error->below_str[image_info->pix_num - 1] += 5./16*value_error;
    if (image_info->pix_num > 1)
      error->below_str[image_info->pix_num - 2] += 3./16*value_error;
    if (image_info->pix_num == image_info->width) {
      UPDATE_ERROR_FLOYD
    }
  }
  image_info->image[image_info->position] = value;
}

/*
 * Function: init_error
 * ----------------------------
 * Alloces memory for error according to diffusion type
 *
 * width: image width
 * type: error diffustion type
 *
 * returns: pointer to error
 */
void *init_error(int width, int type) {
  if (type == DIFFUSION_TYPE_FLOYD) {
    PERROR_FLOYD error;
    int i;

    error = (PERROR_FLOYD)malloc(sizeof(ERROR_FLOYD));
    error->below_str = (int *)malloc(width*sizeof(int));
    error->current_str = (int *)malloc(width*sizeof(int));
    for (i = 0; i < width; i++) {
      error->current_str[i] = 0;
      error->below_str[i] = 0;
    }

    return error;
  }
  int *error;

  error = (int *)malloc(sizeof(int));
  *error = 0;

  return error;
}

/*
 * Function: deinit_error
 * ----------------------------
 * Deinits error pointer created by init_error
 *
 * diffusion_error: pointer to error
 * type: error diffustion type
 *
 * returns: void
 */
void deinit_error(void *diffusion_error, int type) {
  if (type == DIFFUSION_TYPE_FLOYD) {
    PERROR_FLOYD error = (PERROR_FLOYD)diffusion_error;
    free(error->below_str);
    free(error->current_str);
  }
  free(diffusion_error);
}

/*
 * Function: error_diffusion_seq_way
 * ----------------------------
 * Processes all strings from left to right
 * according to error diffusion type
 *
 * image_info: pointer to IMAGE_INFO structure
 * type: error diffustion type
 * algo: pointer to function to call for each pixel
 *
 * returns: void
 */
void error_diffusion_seq_way(PIMAGE_INFO image_info,
  int type,
  void (*algo)(PIMAGE_INFO, bool, void*)) {
  void *error = init_error(image_info->width, type);

  while(image_info->str_num <= image_info->height) {
    algo(image_info, false, error);
    NEXT_PIXEL
  }

  deinit_error(error, type);
}

#define NEXT_POSITION(last_pixel, step) do { \
  if (image_info->pix_num == last_pixel) { \
    image_info->position += image_info->width; \
    image_info->pix_num = last_pixel; \
    image_info->str_num++; \
  } else { \
    image_info->position += step; \
    image_info->pix_num += step; \
  } \
} while(0); \

/*
 * Function: error_diffusion_alt_way
 * ----------------------------
 * Processes even strings from left to right,
 * odd strings from right to left
 * according to diffusion type
 *
 * image_info: pointer to IMAGE_INFO structure
 * type: error diffustion type
 * algo: pointer to function to call for each pixel
 *
 * returns: void
 */
void error_diffusion_alt_way(PIMAGE_INFO image_info,
  int type,
  void (*algo)(PIMAGE_INFO, bool, void*)) {
  void *error = init_error(image_info->width, type);

  while(image_info->str_num <= image_info->height) {
    if (image_info->str_num % 2 == 0) {
      algo(image_info, false, error);
      NEXT_POSITION(image_info->width, 1)
    } else {
      algo(image_info, true, error);
      NEXT_POSITION(1, -1)
    }
  }

  deinit_error(error, type);
}

/* Ordered dithering algo */
typedef struct _ORDERED_DITHERING_PARAMS {
  int **index_matrix;
  int matrix_size;
} ORDERED_DITHERING_PARAMS, *PORDERED_DITHERING_PARAMS;

#define ORDERED_THRESHOLD(matrix, size, str, pix, maxgrey) \
    (matrix[str % size][pix % size]*(maxgrey)/size/size) \

/*
 * Function: ordered_dithering
 * ----------------------------
 * Sets pixel on current pixmap position to black
 * if pixel value is less than ordered threshold
 * from index matrix, to white otherwise
 *
 * image_info: pointer to IMAGE_INFO structure
 * params: pointer to ORDERED_DITHERING_PARAMS structure
 *
 * returns: void
 */
void ordered_dithering(PIMAGE_INFO image_info, void *params) {
  PORDERED_DITHERING_PARAMS od_params = (PORDERED_DITHERING_PARAMS)params;

  double threshold = ORDERED_THRESHOLD(od_params->index_matrix,
    od_params->matrix_size,
    image_info->str_num,
    image_info->pix_num,
    image_info->maxgrey);
  SET_PIXEL_VALUE(image_info->image[image_info->position], threshold)
}

/*
 * Function: matrix_alloc
 * ----------------------------
 * Allocates memory for matrix size*size
 *
 * size: size of matrix
 *
 * returns: pointer to matrix
 */
int **matrix_alloc(int size) {
  int i;
  int **matrix;

  matrix = (int **)malloc(size*sizeof(int*));
  if (matrix == NULL) {
    printf("Error: not enough memory\n");
    exit(1);
  }
  for (i = 0; i < size; i++) {
    matrix[i] = (int *)malloc(size*sizeof(int));
    if (matrix[i] == NULL) {
      printf("Error: not enough memory\n");
      exit(1);
    }
  }

  return matrix;
}

/*
 * Function: matrix_dealloc
 * ----------------------------
 * Frees memory allocated by matrix_alloc
 *
 * matrix: pointer to matrix
 * size: size of matrix
 *
 * returns: void
 */
void matrix_dealloc(int **matrix, int size) {
  int i;

  for (i = 0; i < size; i++)
    free(matrix[i]);
  free(matrix);
}

/*
 * Function: print_matrix
 * ----------------------------
 * Prints matrix to stdout
 *
 * matrix: pointer to matrix
 * size: size of matrix
 *
 * returns: void
 */
void print_matrix(int **matrix, int size) {
  int i, j;

  printf("Size is %d\n", size);
  for (i = 0; i < size; i++) {
    for (j = 0; j < size; j++)
      printf("%d ", matrix[i][j]);
    printf("\n");
  }
}

/*
 * Function: init_index_matrix
 * ----------------------------
 * Generates index matrix for ordered dithering
 * D(2) = | 0 2 |
 *        | 3 1 |
 *
 * D(n) = | 4D(n/2)         4D(n/2)+2U(n/2) |
 *        | 4D(n/2)+3U(n/2) 4D(n/2)+ U(n/2) |

 * U(n) = | 1 1 ... 1 |
 *        | 1 1 ... 1 |
 *        | ......... |
 *        | 1 1 ... 1 |
 *
 *
 * size: size of matrix
 *
 * returns: pointer to matrix
 */
int **init_index_matrix(int size) {
  int i;
  int j;
  int prev_size;
  int **d_prev;
  int **d_next = NULL;

  if (size == 2) {
    d_prev = matrix_alloc(size);
    d_prev[0][0] = 0;
    d_prev[0][1] = 2;
    d_prev[1][0] = 3;
    d_prev[1][1] = 1;

    return d_prev;
  }
  prev_size = size/2;
  d_prev = init_index_matrix(prev_size);
  d_next = matrix_alloc(size);

  for (i = 0; i < prev_size; i++)
    for (j = 0; j < prev_size; j++)
      d_next[i][j] = 4*d_prev[i][j];

  for (i = 0; i < prev_size; i++)
    for (j = prev_size; j < size; j++)
      d_next[i][j] = 4*d_prev[i][j - prev_size] + 2;

  for (i = prev_size; i < size; i++)
    for (j = 0; j < prev_size; j++)
      d_next[i][j] = 4*d_prev[i - prev_size][j] + 3;

  for (i = prev_size; i < size; i++)
    for (j = prev_size; j < size; j++)
      d_next[i][j] = 4*d_prev[i - prev_size][j - prev_size] + 1;

  print_matrix(d_prev, prev_size);

  matrix_dealloc(d_prev, prev_size);
  return d_next;
}

/*
 * Function: deinit_index_matrix
 * ----------------------------
 * Deinits matrix generated by init_index_matrix
 *
 * metrix: pointer to matrix
 * size: size of matrix
 *
 * returns: void
 */
void deinit_index_matrix(int **matrix, int size) {
  matrix_dealloc(matrix, size);
}

enum ALGO_TYPE {
  THRESHOLDING = 1,
  RANDOM_DITHERING,
  ORDERED_DITHERING,
  ERROR_DIFFUSION_FORWARD,
  ERROR_DIFFUSION_FORWARD_BACK,
  ERROR_DIFFUSION_FLOYD_FORWARD,
  ERROR_DIFFUSION_FLOYD_FORWARD_BACK
};

int main(int argc, char *argv[]) {
  PIMAGE_INFO image_info;
  ORDERED_DITHERING_PARAMS od_params;

  if (!argv[1] || !argv[2] || !argv[3]) {
    printf("Usage: %s <orig_image_path> <new_image_path> <algo_number> [index_matrix_size]\nAlgos:\n%d\tThresholding\n%d\tRandom dithering\n%d \tOrdered dithering\n%d\tError diffusion forward\n\t\terror diffusion forward to string\n%d\tError diffusion forward back\n\t\terror diffusion forward for even strings and back for odd strings\n%d\tError diffusion Floyd forward\n\t\tFloyd–Steinberg error diffusion forward to string\n%d\tError diffusion Floyd forward back\n\t\tFloyd–Steinberg error diffusion forward for even strings and back for odd strings\n", argv[0], THRESHOLDING, RANDOM_DITHERING, ORDERED_DITHERING, ERROR_DIFFUSION_FORWARD, ERROR_DIFFUSION_FORWARD_BACK, ERROR_DIFFUSION_FLOYD_FORWARD, ERROR_DIFFUSION_FLOYD_FORWARD_BACK);
    return 1;
  }
  image_info = image_init(argv[1], argv[2]);
  if (image_info == NULL)
    return 1;

  switch(atoi(argv[3])) {
    case THRESHOLDING:
      seq_way(image_info, thresholding, NULL);
      break;
    case RANDOM_DITHERING:
      srand(time(NULL));
      seq_way(image_info, random_dithering, NULL);
      break;
    case ORDERED_DITHERING:
      if (!argv[4]) {
        printf("Matrix size was not passed. Set to 8\n");
        od_params.matrix_size = 8;
      } else {
        od_params.matrix_size = atoi(argv[4]);
      }
      od_params.index_matrix = init_index_matrix(od_params.matrix_size);
      seq_way(image_info, ordered_dithering, &od_params);
      deinit_index_matrix(od_params.index_matrix, od_params.matrix_size);
      break;
    case ERROR_DIFFUSION_FORWARD:
      error_diffusion_seq_way(image_info, DIFFUSION_TYPE_FULL, error_diffusion);
      break;
    case ERROR_DIFFUSION_FORWARD_BACK:
      error_diffusion_alt_way(image_info, DIFFUSION_TYPE_FULL, error_diffusion);
      break;
    case ERROR_DIFFUSION_FLOYD_FORWARD:
      error_diffusion_seq_way(image_info, DIFFUSION_TYPE_FLOYD,
        error_diffusion_floyd);
      break;
    case ERROR_DIFFUSION_FLOYD_FORWARD_BACK:
      error_diffusion_alt_way(image_info, DIFFUSION_TYPE_FLOYD,
        error_diffusion_floyd);
      break;
    default:
      printf("Unknown algorithm\n");
  }

  image_deinit(image_info);
  return 0;
}

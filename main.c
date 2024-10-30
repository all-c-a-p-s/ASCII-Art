#include <stdio.h>
#include <stdlib.h>

/*
 * I need to experiment with how many characters to have. If there aren't enough
 * then I can't express enough different brightnesses but if there are too many
 * then the effect might be worse because areas of similar brightness won't have
 * the same character. Another thing to try is adding spaces to the start of the
 * array, which will essentially create a threshold of a minimum brightness
 * which a pixel must have in order to be displayed at all in the ASCII
 * representation.
 */

const char CHARS_ARRAY[120] = {
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ',
    '`', '.', '-', ':', '_', ',', '^', '=', ';', '>', '<', '+', '!', 'r', 'c',
    '*', '/', 'z', '?', 's', 'L', 'T', 'v', ')', 'J', '7', '(', '|', 'F', 'i',
    '{', 'C', '}', 'f', 'I', '3', '1', 't', 'l', 'u', '[', 'n', 'e', 'o', 'Z',
    '5', 'Y', 'x', 'j', 'y', 'a', ']', '2', 'E', 'S', 'w', 'q', 'k', 'P', '6',
    'h', '9', 'd', '4', 'V', 'p', 'O', 'G', 'b', 'U', 'A', 'K', 'X', 'H', 'm',
    '8', 'R', 'D', '#', '$', 'B', 'g', '0', 'M', 'N', 'W', 'Q', '%', '&', '@'};
;

#pragma pack(push, 1) // ensures that the compiler doesn't insert padding in
                      // between struct fields

typedef struct {
  unsigned char signature[2];
  unsigned int file_size;
  unsigned short reserved1;
  unsigned short reserved2;
  unsigned int data_offset;
} BMPFileHeader;

typedef struct {
  unsigned int header_size;
  int width;
  int height;
  unsigned short planes;
  unsigned short bits_per_pixel;
  unsigned int compression;
  unsigned int image_size;
  int x_pixels_per_meter;
  int y_pixels_per_meter;
  unsigned int colors_used;
  unsigned int important_colors;
} BMPInfoHeader;

#pragma pack(pop)

typedef struct {
  unsigned char r;
  unsigned char g;
  unsigned char b;
  unsigned long long idx; // store original index so that after sorting you can
                          // know where each pixel came from
} Pixel;

typedef struct {
  Pixel *pixels;
  int image_width;
  int image_height;
  int total_pixels;
} ImageData;

ImageData read_bmp(const char *filename) {
  FILE *f = fopen(filename, "rb");
  if (!f) {
    perror("failed to open file");
    exit(EXIT_FAILURE);
  }

  BMPFileHeader file_header;
  fread(&file_header, sizeof(BMPFileHeader), 1, f);
  if (file_header.signature[0] != 'B' || file_header.signature[1] != 'M') {
    fprintf(stderr, "file was not in .bmp format\n");
    fclose(f);
    exit(EXIT_FAILURE);
  }

  BMPInfoHeader info_header;
  fread(&info_header, sizeof(BMPInfoHeader), 1, f);

  if (info_header.bits_per_pixel != 24) {
    fprintf(stderr, "only 24-bit .bmp files are supported\n");
    fclose(f);
    exit(EXIT_FAILURE);
  }

  fseek(f, file_header.data_offset, SEEK_SET);

  int total_pixels = info_header.width * info_header.height;
  Pixel *pixels = (Pixel *)malloc(sizeof(Pixel) * total_pixels);
  if (!pixels) {
    perror("failed to allocate memory for pixels");
    fclose(f);
    exit(EXIT_FAILURE);
  }

  for (int y = info_header.height - 1; y >= 0;
       y--) { // bmp stores pixels from bottom to top
    for (int x = 0; x < info_header.width; x++) {
      int idx = y * info_header.width + x;
      unsigned char bgr[3];
      fread(bgr, sizeof(unsigned char), 3, f); // read in order B G R

      pixels[idx].b = bgr[0];
      pixels[idx].g = bgr[1];
      pixels[idx].r = bgr[2];
      pixels[idx].idx = idx; // actual index of each pixel
    }
  }

  fclose(f);

  return (ImageData){pixels, info_header.width, info_header.height,
                     info_header.width * info_header.height};
}

float get_brightness(const Pixel *p) {
  return p->r * 0.299 + p->g * 0.587 + p->b * 0.114;
}

/*
 * Mapping Pixels to characters:
 *  (1) - rank all of the Pixels and assign them characters based on their rank
 * (for example, the 15th brightest out) out 100 Pixels is mapped to the 15th
 * brightest out of 100 characters.
 *
 * (2) - find a way of mapping each ASCII
 * character to a brightness value between 0 and the maximum possible output of
 * get_brightness() and then map each Pixel to the brightest character
 * that it is brighter than.
 */

char get_character_representation(int rank, int pixels) {
  return CHARS_ARRAY[rank * (sizeof(CHARS_ARRAY) / sizeof(CHARS_ARRAY[0])) /
                     pixels];
}

int compare_pixels(const void *a, const void *b) {
  const Pixel *pixelA = (const Pixel *)a;
  const Pixel *pixelB = (const Pixel *)b;

  float brightness_a = get_brightness(pixelA);
  float brightness_b = get_brightness(pixelB);

  return brightness_a - brightness_b;
}

int main() {
  const char *filename = "image.bmp";
  ImageData data = read_bmp(filename);

  Pixel *img = data.pixels;
  char characters_array[data.total_pixels];

  qsort(img, data.total_pixels, sizeof(Pixel), compare_pixels);

  for (int i = 0; i < data.total_pixels; i++) {
    characters_array[img[i].idx] =
        get_character_representation(i, data.total_pixels);
  }

  for (int i = 0; i < data.total_pixels; i++) {
    printf("%c", characters_array[i]);
    if ((i + 1) % data.image_width == 0)
      printf("\n");
  }

  free(img);
  return 0;
}

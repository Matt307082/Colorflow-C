 /*
 * A simple libpng example program
 * http://zarb.org/~gc/html/libpng.html
 *
 * Modified by Yoshimasa Niwa to make it much simpler
 * and support all defined color_type.
 *
 * To build, use the next instruction on OS X.
 * $ brew install libpng
 * $ clang -lz -lpng16 libpng_test.c
 *
 * Copyright 2002-2010 Guillaume Cottenceau.
 *
 * This software may be freely redistributed under the terms
 * of the X11 license.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <png.h>
#include <jpeglib.h>
#include "coverflow.h"


#define EXIT_FAILURE_OPEN_FAILED 1
#define EXIT_FAILURE_BAD_FILE 2
#define EXIT_FAILURE_USUPPORTED_FILE_FORMAT 3
#define EXIT_FAILURE_MALLOC 4
#define EXIT_FAILURE_BAD_PERCENTAGE 5

int width, height;

pixel* createPixel(unsigned char R, unsigned char G, unsigned char B, unsigned char A){
  pixel* p = (pixel*)malloc(sizeof(pixel));
  if (!p) {
        fprintf(stderr,"Error while allocating memory for pixel.\n");
        exit(EXIT_FAILURE_MALLOC);
  }
  p->red = R;
  p->green = G;
  p->blue = B;
  p->alpha = A;
  return p;
}

void freePixel(pixel** pixel){
  free(*pixel);
  *pixel=NULL;
}

void freePixels(pixel*** pixels) {
  for (int y = 0; y < height; y++) {
          for (int x = 0; x < width; x++) {
              if (pixels[y][x]) {
                  freePixel(&pixels[y][x]);
              }
          }
      }
  free(pixels);
  pixels=NULL;
}

pixel*** read_png_file(FILE *file) {
  png_byte color_type;
  png_byte bit_depth;
  png_bytep *row_pointers = NULL;

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!png) abort();

  png_infop info = png_create_info_struct(png);
  if(!info) abort();

  if(setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, file);

  png_read_info(png, info);

  width      = png_get_image_width(png, info);
  height     = png_get_image_height(png, info);
  color_type = png_get_color_type(png, info);
  bit_depth  = png_get_bit_depth(png, info);

  // Read any color_type into 8bit depth, RGBA format.
  // See http://www.libpng.org/pub/png/libpng-manual.txt

  if(bit_depth == 16)
    png_set_strip_16(png);

  if(color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_palette_to_rgb(png);

  // PNG_COLOR_TYPE_GRAY_ALPHA is always 8 or 16bit depth.
  if(color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
    png_set_expand_gray_1_2_4_to_8(png);

  if(png_get_valid(png, info, PNG_INFO_tRNS))
    png_set_tRNS_to_alpha(png);

  // These color_type don't have an alpha channel then fill it with 0xff.
  if(color_type == PNG_COLOR_TYPE_RGB ||
     color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_PALETTE)
    png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

  if(color_type == PNG_COLOR_TYPE_GRAY ||
     color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png);

  png_read_update_info(png, info);

  if (row_pointers) abort();

  row_pointers = (png_bytep*)malloc(sizeof(png_bytep) * height);
  for(int y = 0; y < height; y++) {
    row_pointers[y] = (png_byte*)malloc(png_get_rowbytes(png,info));
  }

  png_read_image(png, row_pointers);

  pixel*** pixels = (pixel***)malloc(height*sizeof(pixel**) + height*width*sizeof(pixel*));
  if(!pixels){
        fprintf(stderr,"Error while allowing memory for pixel matrix.\n");
        exit(EXIT_FAILURE_MALLOC);
  }

  for(int y = 0; y < height; y++) {
    pixels[y] = (pixel**)(pixels + height) + width * y;
    png_bytep row = row_pointers[y];
    for(int x = 0; x < width; x++) {
      png_bytep px = &(row[x * 4]);
      pixels[y][x] = createPixel(px[0],px[1],px[2],px[3]);
    }
  }

  png_destroy_read_struct(&png, &info, NULL);

  return pixels;
}

pixel*** read_jpg_file(FILE *file){

  // Structure for JPEG picture
  struct jpeg_decompress_struct cinfo;
  struct jpeg_error_mgr jerr;

  cinfo.err = jpeg_std_error(&jerr);
  jpeg_create_decompress(&cinfo);

  // Link with JPEG file
  jpeg_stdio_src(&cinfo, file);

  // Reading headers
  (void) jpeg_read_header(&cinfo, TRUE);

  // Start decompress
  (void) jpeg_start_decompress(&cinfo);

  // Reading pictures informations
  width = cinfo.output_width;
  height = cinfo.output_height;
  int numComponents = cinfo.output_components;
  int row_stride = width * numComponents;

  // Allow memory to be able to read 1 line of the picture
  JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray)((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

  pixel*** pixels = (pixel***)malloc(height*sizeof(pixel**) + height*width*sizeof(pixel*));
  if(!pixels){
        fprintf(stderr,"Error while allowing memory.\n");
        exit(EXIT_FAILURE_MALLOC);
  }


  for(int y=0; y<height;y++){
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
    pixels[y] = (pixel**)(pixels + height) + width * y;
    for (int x = 0; x < width; x++) {
      pixels[y][x] = createPixel(buffer[0][x * numComponents],buffer[0][x * numComponents + 1],buffer[0][x * numComponents + 2],buffer[0][x * numComponents + 3]);
    }
  }

  // Finish decompress
  (void) jpeg_finish_decompress(&cinfo);

  // Free ressources
  jpeg_destroy_decompress(&cinfo);

  return pixels;
}

void getAverageBorderColor(pixel*** pixels_image, int *border_average_color, int start_row, int end_row, int start_column, int end_column){
  // establishing avegarge color of the selected border
  int pixel_amount = ((end_row-start_row)*(end_column-start_column));
  for(int y=start_row; y<end_row; y++){
    for(int x=start_column; x<end_column; x++){
      border_average_color[0] += (int)pixels_image[y][x]->red;
      border_average_color[1] += (int)pixels_image[y][x]->green;
      border_average_color[2] += (int)pixels_image[y][x]->blue;
      border_average_color[3] += (int)pixels_image[y][x]->alpha;
    }
  }
  for(int i=0;i<4;i++){
    border_average_color[i] /= pixel_amount;
  }
}

int* getAverageColor(pixel*** pixels_image, float frame_percentage){

  // establishing avegarge color of the upper border
  int up_border_average_color[4] = {0,0,0,0};
  getAverageBorderColor(pixels_image, up_border_average_color, 0, (int)(height*frame_percentage), 0, width);

  // establishing avegarge color of the right border
  int right_border_average_color[4] = {0,0,0,0};
  getAverageBorderColor(pixels_image, right_border_average_color, 0, height, (int)(width*(1-frame_percentage)), width);

  // establishing avegarge color of the left border
  int down_border_average_color[4] = {0,0,0,0};
  getAverageBorderColor(pixels_image, down_border_average_color, (int)((1-frame_percentage)*height), height, 0, width);

  // establishing avegarge color of the lower border
  int left_border_average_color[4] = {0,0,0,0};
  getAverageBorderColor(pixels_image, left_border_average_color, 0, height, 0, (int)(width*frame_percentage));

  printf("RGBA(%3d,%3d,%3d,%3d)\n",up_border_average_color[0],up_border_average_color[1],up_border_average_color[2],up_border_average_color[3]);
  printf("RGBA(%3d,%3d,%3d,%3d)\n",right_border_average_color[0],right_border_average_color[1],right_border_average_color[2],right_border_average_color[3]);
  printf("RGBA(%3d,%3d,%3d,%3d)\n",down_border_average_color[0],down_border_average_color[1],down_border_average_color[2],down_border_average_color[3]);
  printf("RGBA(%3d,%3d,%3d,%3d)\n",left_border_average_color[0],left_border_average_color[1],left_border_average_color[2],left_border_average_color[3]);

  // establishing the average color of the frame
  static int average_RGBA[4];
  for(int i=0;i<4;i++){
    average_RGBA[i] = up_border_average_color[i]+right_border_average_color[i]+down_border_average_color[i]+left_border_average_color[i];
    average_RGBA[i] /= 4;
  }
  return average_RGBA;
}


pixel*** read_data(FILE *file, unsigned char* buffer){
  pixel*** pixels_image;
  // compare with png signature
  if (png_sig_cmp(buffer, 0, sizeof(buffer)) == 0) {
    pixels_image = read_png_file(file);
  } // compare with jpg signature
  else if (buffer[0] == 0xFF && buffer[1] == 0xD8 && buffer[2] == 0xFF) {
    pixels_image = read_jpg_file(file);
  } 
  else {
    fclose(file);
    printf("Unsupported file format.\n");
    exit(EXIT_FAILURE_USUPPORTED_FILE_FORMAT);
  }
  return pixels_image;
}

int main(int argc, char *argv[]) {
  if(argc != 3){
    puts("Error, format must be: ./coverflow imagename frame_percentage\n");
    abort();
  } 

  FILE *file = fopen(argv[1], "rb");
  if(!file){
    fprintf(stderr,"Error while opening file %s\n", argv[1]);
    perror("open");
    exit(EXIT_FAILURE_OPEN_FAILED);
  } 

  unsigned char buffer[8];
  int read_len = fread(buffer, 1, sizeof(buffer), file);
  if(read_len != 8){
    fclose(file);
    fprintf(stderr,"Error while reading file %s\n", argv[1]);
    exit(EXIT_FAILURE_BAD_FILE);
  }

  int seek = fseek(file,0,SEEK_SET);
  if(seek){
    fclose(file);
    perror("seek");
    exit(EXIT_FAILURE_BAD_FILE);
  }

  pixel*** pixels_image = read_data(file, buffer);

  fclose(file);

  float frame_percentage = (float)(atoi(argv[2])/100.0);
  if(frame_percentage > 100.0 || frame_percentage < 0.0){
    fprintf(stderr,"Error, frame_percentage must be a value between 0 and 100");
    exit(EXIT_FAILURE_BAD_PERCENTAGE);
  }
  int* average_RGBA = getAverageColor(pixels_image, frame_percentage);
  puts("\n");
  printf("RGBA(%3d,%3d,%3d,%3d)\n",average_RGBA[0],average_RGBA[1],average_RGBA[2],average_RGBA[3]);


  freePixels(pixels_image);

  return 0;
}
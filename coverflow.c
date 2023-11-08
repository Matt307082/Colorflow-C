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
#include <png.h>

int width, height;
png_byte color_type;
png_byte bit_depth;
png_bytep *row_pointers = NULL;

void read_png_file(char *filename) {
  FILE *fp = fopen(filename, "rb");
  if(!fp) abort();

  png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
  if(!png) abort();

  png_infop info = png_create_info_struct(png);
  if(!info) abort();

  if(setjmp(png_jmpbuf(png))) abort();

  png_init_io(png, fp);

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

  fclose(fp);

  png_destroy_read_struct(&png, &info, NULL);
}

png_color_8* createPixel(png_byte R, png_byte G, png_byte B, png_byte A){
  png_color_8* pixel = (png_color_8*)malloc(sizeof(png_color_8));
  if (!pixel) {
        puts("Error while allocating memory for pixel.\n");
        abort();
  }
  pixel->red = R;
  pixel->green = G;
  pixel->blue = B;
  pixel->alpha = A;
  return pixel;
}

void freeColorPixel(png_color_8** pixel){
  free(*pixel);
  *pixel=NULL;
}

png_color_8*** getPixelsImage() {
  png_color_8*** pixels = (png_color_8***)malloc(height*sizeof(png_color_8**) + height*width*sizeof(png_color_8*));
  if(!pixels){
        puts("Error while allowing memory.\n");
        abort();
  }

  for(int y = 0; y < height; y++) {
    pixels[y] = (png_color_8**)(pixels + height) + width * y;
    png_bytep row = row_pointers[y];
    for(int x = 0; x < width; x++) {
      png_bytep px = &(row[x * 4]);
      pixels[y][x] = createPixel(px[0],px[1],px[2],px[3]);
    }
  }
  return pixels;
}

void freePixels(png_color_8*** tab) {
  for (int y = 0; y < height; y++) {
          for (int x = 0; x < width; x++) {
              if (tab[y][x]) {
                  freeColorPixel(&tab[y][x]);
              }
          }
      }
  free(tab);
  tab=NULL;
}

void getAverageBorderColor(png_color_8*** pixels_image, int *border_average_color, int start_row, int end_row, int start_column, int end_column){
  // calcul de la couleur moyenne de la bordure supérieure
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

int* getAverageColor(){
  png_color_8*** pixels_image = getPixelsImage();

  // calcul de la couleur moyenne de la bordure supérieure
  int up_border_average_color[4] = {0,0,0,0};
  getAverageBorderColor(pixels_image, up_border_average_color, 0, (int)(height*0.1), 0, width);

  // calcul de la couleur moyenne de la bordure droite
  int right_border_average_color[4] = {0,0,0,0};
  getAverageBorderColor(pixels_image, right_border_average_color, 0, height, (int)(width*0.9), width);

  // calcul de la couleur moyenne de la bordure inférieure
  int down_border_average_color[4] = {0,0,0,0};
  getAverageBorderColor(pixels_image, down_border_average_color, (int)(0.9*height), height, 0, width);

  // calcul de la couleur moyenne de la bordure gauche
  int left_border_average_color[4] = {0,0,0,0};
  getAverageBorderColor(pixels_image, left_border_average_color, 0, height, 0, (int)(width*0.1));

  printf("RGBA(%3d,%3d,%3d,%3d)\n",up_border_average_color[0],up_border_average_color[1],up_border_average_color[2],up_border_average_color[3]);
  printf("RGBA(%3d,%3d,%3d,%3d)\n",right_border_average_color[0],right_border_average_color[1],right_border_average_color[2],right_border_average_color[3]);
  printf("RGBA(%3d,%3d,%3d,%3d)\n",down_border_average_color[0],down_border_average_color[1],down_border_average_color[2],down_border_average_color[3]);
  printf("RGBA(%3d,%3d,%3d,%3d)\n",left_border_average_color[0],left_border_average_color[1],left_border_average_color[2],left_border_average_color[3]);

  freePixels(pixels_image);

  // calcul de la couleur moyenne des 4 bordures
  static int average_RGBA[4];
  for(int i=0;i<4;i++){
    average_RGBA[i] = up_border_average_color[i]+right_border_average_color[i]+down_border_average_color[i]+left_border_average_color[i];
    average_RGBA[i] /= 4;
  }
  return average_RGBA;
}


int main(int argc, char *argv[]) {
  if(argc != 2){
    puts("Error, format must be: coverflow imagename.png\n");
    abort();
  } 

  read_png_file(argv[1]);
  int* average_RGBA = getAverageColor();
  puts("\n");
  printf("RGBA(%3d,%3d,%3d,%3d)\n",average_RGBA[0],average_RGBA[1],average_RGBA[2],average_RGBA[3]);
  return 0;
}
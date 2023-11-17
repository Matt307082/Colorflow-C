#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "include/png.h"
#include "include/jpeglib.h"
#include "include/colorflow.h"


#define EXIT_FAILURE_OPEN_FAILED 1
#define EXIT_FAILURE_BAD_FILE 2
#define EXIT_FAILURE_USUPPORTED_FILE_FORMAT 3
#define EXIT_FAILURE_MALLOC 4
#define EXIT_FAILURE_BAD_PERCENTAGE 5
#define EXIT_FAILURE_UNKNOWN_OPTION 6
#define EXIT_FAILURE_NEEDS_ARGUMENT 7

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

/// @brief 
/// @param file 
/// @return 
/// @author code from https://gist.github.com/niw/5963798
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

/// @brief 
/// @param file 
/// @return 
/// @author code inspired by this code https://github.com/LuaDist/libjpeg/blob/master/example.c
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
      pixels[y][x] = createPixel(buffer[0][x * numComponents],buffer[0][x * numComponents + 1],buffer[0][x * numComponents + 2],(cinfo.output_components == 4) ? buffer[0][x * numComponents + 2] : 255);
    }
  }

  // Finish decompress
  (void) jpeg_finish_decompress(&cinfo);

  // Free ressources
  jpeg_destroy_decompress(&cinfo);

  return pixels;
}

/// @brief 
/// @param file 
/// @return 
/// @authors code inspired by https://stackoverflow.com/questions/14279242/read-bitmap-file-into-structure
pixel*** read_bmp_file(FILE *file){
  BMPHeader header;
  int read_len = fread(&header, sizeof(BMPHeader), 1, file);
  if(read_len != 1){
    fclose(file);
    fprintf(stderr,"Error while reading file\n");
    exit(EXIT_FAILURE_BAD_FILE);
  }

  int seek = fseek(file, header.offset, SEEK_SET);
  if(seek){
    fclose(file);
    perror("seek");
    exit(EXIT_FAILURE_BAD_FILE);
  }

  height = header.height;
  width = header.width;

  pixel*** pixels = (pixel***)malloc(height*sizeof(pixel**) + height*width*sizeof(pixel*));
  if(!pixels){
        fprintf(stderr,"Error while allowing memory for pixel matrix.\n");
        exit(EXIT_FAILURE_MALLOC);
  }

  int padding = (4 - (width * 3) % 4) % 4; // BMP rows are padded to be multiples of 4 bytes
  for (int y = height - 1; y >= 0; y--) {
    pixels[y] = (pixel**)(pixels + height) + width * y;
      for (int x = 0; x < width; x++) {
          unsigned char pixel[3];
          int read_len = fread(&pixel, sizeof(unsigned char), 3, file);
          if(read_len != 3){
            fclose(file);
            fprintf(stderr,"Error while reading file\n");
            exit(EXIT_FAILURE_BAD_FILE);
          }
          pixels[y][x] = createPixel(pixel[2],pixel[1],pixel[0],255); // because in bmp format pixel colors are stored as BGR not RGB
      }
      int seek = fseek(file, padding, SEEK_CUR); // Skip padding at the end of each row
      if(seek){
        fclose(file);
        perror("seek");
        exit(EXIT_FAILURE_BAD_FILE);
      }
  }

    return pixels;
}

void getAverageBorderColor(pixel*** pixels_image, int *border_average_color, int start_row, int end_row, int start_column, int end_column){
  // establishing average color of the selected border
  int pixel_amount = ((end_row-start_row)*(end_column-start_column));

  if(pixel_amount == 0){
    pixel_amount = 1; 
  }

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

int* getAverageColor(pixel*** pixels_image, double frame_percentage){

  // establishing average color of the upper border
  int up_border_average_color[4] = {0,0,0,0};
  getAverageBorderColor(pixels_image, up_border_average_color, 0, (int)(height*frame_percentage), 0, width);

  // establishing average color of the right border
  int right_border_average_color[4] = {0,0,0,0};
  getAverageBorderColor(pixels_image, right_border_average_color, 0, height, (int)(width*(1-frame_percentage)), width);

  // establishing average color of the left border
  int down_border_average_color[4] = {0,0,0,0};
  getAverageBorderColor(pixels_image, down_border_average_color, (int)((1-frame_percentage)*height), height, 0, width);

  // establishing average color of the lower border
  int left_border_average_color[4] = {0,0,0,0};
  getAverageBorderColor(pixels_image, left_border_average_color, 0, height, 0, (int)(width*frame_percentage));

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
  } // compare with bmp signature
  else if (buffer[0] == 'B' && buffer[1] == 'M') {
    pixels_image = read_bmp_file(file);
  } // compare with heif signature
  else if (buffer[8] == 'f' && buffer[9] == 't' && buffer[10] == 'y' && buffer[11] == 'p') {
    printf("We got an HEIF file !");
    abort();
  }
  else {
    fclose(file);
    printf("Unsupported file format.\n");
    exit(EXIT_FAILURE_USUPPORTED_FILE_FORMAT);
  }
  return pixels_image;
}

void displayHelp(){
  char* filename = "help.txt";
  FILE *file = fopen(filename, "rt");
  if(!file){
    fprintf(stderr,"Error while opening file %s\n", filename);
    perror("open");
    exit(EXIT_FAILURE_OPEN_FAILED);
  }
  char c; 
  while((c=fgetc(file))!=EOF){
    printf("%c",c);
  }
  fclose(file);
}

int main(int argc, char *argv[]) {
  if(argc == 1){
    fprintf(stderr,"Error: colorflow needs arguments\n\nRun \"colorflow -h\" to get more details\n");
    exit(EXIT_FAILURE_NEEDS_ARGUMENT);
  }
  //parsing command line arguments
  int opt;
  char* filename;
  int percentage = -1 ;

  while((opt = getopt(argc, argv, "dh?f:n:")) != -1){
    switch(opt){
      case 'f':
        filename = optarg;
        break;
      case 'n':
      case '?':
        percentage = atoi(optarg);
        break;
      case 'h':
        displayHelp();
        return 0;
      case 'd':
        puts("Run debug\n");
        abort();
      default:
        fprintf(stderr,"Error: Unknown option -%c\n", optopt);
        exit(EXIT_FAILURE_UNKNOWN_OPTION);
    }
  }

  FILE *file = fopen(filename, "rb");
  if(!file){
    fprintf(stderr,"Error while opening file %s\n", filename);
    perror("open");
    exit(EXIT_FAILURE_OPEN_FAILED);
  } 

  unsigned char buffer[8];
  int read_len = fread(buffer, 1, sizeof(buffer), file);
  if(read_len != 8){
    fclose(file);
    fprintf(stderr,"Error while reading file %s\n", filename);
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

  if(percentage == -1){
    percentage = 10; // setting default value
  }
  double frame_percentage = (double)(percentage/100.0);
  if(frame_percentage > 1.0 || frame_percentage <= 0.0){
    fprintf(stderr,"Error : frame_percentage must be a value between 0 and 100\n");
    exit(EXIT_FAILURE_BAD_PERCENTAGE);
  }

  int* average_RGBA = getAverageColor(pixels_image, frame_percentage);
  printf("%02X%02X%02X-%02X\n",average_RGBA[0],average_RGBA[1],average_RGBA[2],average_RGBA[3]);

  freePixels(pixels_image);

  return 0;
}
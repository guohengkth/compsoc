#include <stdlib.h>
#include <stdio.h>
#include <math.h>

typedef struct {
  unsigned char R;
  unsigned char G;
  unsigned char B;
} FrameBuffer_t;

int printBMP(char* file, FrameBuffer_t* FrameBuffer, int x_size, int y_size)
{
  FILE *fpBMP;

  // Header and 3 bytes per pixel
  unsigned long ulBitmapSize = (y_size * x_size * 3)+54; 
  char ucaBitmapSize[4];

  ucaBitmapSize[3]= (ulBitmapSize & 0xFF000000) >> 24;
  ucaBitmapSize[2]= (ulBitmapSize & 0x00FF0000) >> 16;
  ucaBitmapSize[1]= (ulBitmapSize & 0x0000FF00) >> 8;
  ucaBitmapSize[0]= (ulBitmapSize & 0x000000FF);

  // Create bitmap file
  fpBMP=fopen(file,"wb");
  if (fpBMP == 0) return 0;

  // Write header
  // All values are in big-endian order (LSB first)

  // BMP signature + file size
  fprintf(fpBMP,"%c%c%c%c%c%c%c%c%c%c", 66, 77, ucaBitmapSize[0], 
      ucaBitmapSize[1], ucaBitmapSize[2], ucaBitmapSize[3], 0, 0, 0, 0); 

  // Image offset, infoheader size, image width
  fprintf(fpBMP,"%c%c%c%c%c%c%c%c%c%c", 54, 0, 0, 0, 40, 0 , 0, 0, (x_size & 0x00FF), (x_size & 0xFF00)>>8); 

  // Image height, number of panels, num bits per pixel
  fprintf(fpBMP,"%c%c%c%c%c%c%c%c%c%c", 0, 0, (y_size & 0x00FF), (y_size & 0xFF00) >> 8, 0, 0, 1, 0, 24, 0); 

  // Compression type 0, Size of image in bytes 0 because uncompressed
  fprintf(fpBMP,"%c%c%c%c%c%c%c%c%c%c", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0); 
  fprintf(fpBMP,"%c%c%c%c%c%c%c%c%c%c", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
  fprintf(fpBMP,"%c%c%c%c", 0, 0 ,0, 0);

  for(int i = y_size-1; i >= 0; i--) {
    // In bitmaps the bottom line of the image is at the beginning of the file
    for(int j = 0; j < x_size; j++){
      putc((FrameBuffer[i * x_size + j].B), fpBMP);
      putc((FrameBuffer[i * x_size + j].G), fpBMP);
      putc((FrameBuffer[i * x_size + j].R), fpBMP);
    }
    // Pad line until it's word (4 byte) aligned
    for(int j = 0; j < x_size % 4; j++) putc(0, fpBMP);
  }
  fclose(fpBMP);
  return 1;
}

void fractal(FrameBuffer_t* FrameBuffer, int x_size, int y_size)
{
  const double x_min = -1.5;
  const double x_max = 1.5;
  const double x_step = (x_max-x_min)/x_size;
  const double y_min = -1.5;
  const double y_max = 1.5;
  const double y_step = (y_max-y_min)/y_size;
  const double real = 0.005; //0.123
  const double im = 0.8; //0.745
  unsigned char R,G,B;

  for (int n=0; n < y_size; n++) {
    for (int m=0; m < x_size; m++) {
      double x = x_min + x_step * m;
      double y = y_min + y_step * n;

      int num;
      for (num=0; (x*x + y*y <= 4) && num < 0xFF; num++) {
	double new_x = x*x - y*y + real;
	double new_y = 2*x*y + im;
	x = new_x;
	y = new_y;
      }

      if (num > 15){
	R = 0x00;
	G = 0x00;
	B = 0x35;
      }
      else if ((num <= 15)&&(num > 8)) { 
	R = 0x67*num;
	G = 0x32*num;
	B = 00;
      }
      else if ((num <= 8)&&(num > 5)) { 
	R = 0x32*num;
	G = 0x00;
	B = 0x00;
      }
      else if ((num <= 5)&&(num > 0)) {
	R = 0x00*num;
	G = 0x02*num;
	B = 0xCC*num;
      }	
      else if (num == 0) {
	R = 0x00;
	G = 0x00;
	B = 0xCC;
      }			
      FrameBuffer[n * x_size + m].R = R;
      FrameBuffer[n * x_size + m].G = G;
      FrameBuffer[n * x_size + m].B = B;
    }
  }
}

int main (int argc, char**argv)
{
  int x_size = 320;
  int y_size = 240;

  FrameBuffer_t* FrameBuffer = (FrameBuffer_t*) malloc(x_size * y_size * sizeof(FrameBuffer_t));
  fractal(FrameBuffer,x_size,y_size);
  int i = !printBMP(argv[1],FrameBuffer,x_size,y_size);
  free(FrameBuffer);
  return i;
}

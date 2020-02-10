#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "FreeImage.h"
#include <math.h>
#include <time.h>


inline int getPixel(unsigned char* image, int width, int height, int y, int x)
{
	if (x < 0 || x >= width)
		return 0;
	if (y < 0 || y >= height)
		return 0;
	return image[y * width + x];
}

void sobelCPU(unsigned char* imageIn, unsigned char* imageOut, int width, int height)
{
	int i, j;
	int Gx, Gy;
	int tempPixel;

	//za vsak piksel v sliki
	for (i = 0; i < height; i++)
		for (j = 0; j < width; j++)
		{
			Gx = -getPixel(imageIn, width, height, i - 1, j - 1) - 2 * getPixel(imageIn, width, height, i - 1, j) -
				getPixel(imageIn, width, height, i - 1, j + 1) + getPixel(imageIn, width, height, i + 1, j - 1) +
				2 * getPixel(imageIn, width, height, i + 1, j) + getPixel(imageIn, width, height, i + 1, j + 1);
			Gy = -getPixel(imageIn, width, height, i - 1, j - 1) - 2 * getPixel(imageIn, width, height, i, j - 1) -
				getPixel(imageIn, width, height, i + 1, j - 1) + getPixel(imageIn, width, height, i - 1, j + 1) +
				2 * getPixel(imageIn, width, height, i, j + 1) + getPixel(imageIn, width, height, i + 1, j + 1);
			tempPixel = sqrt((float)(Gx * Gx + Gy * Gy));
			if (tempPixel > 255)
				imageOut[i * width + j] = 255;
			else
				imageOut[i * width + j] = tempPixel;
		}
}

void calculate_cumulative(unsigned char* image, int* cumulative, int width, int height, int new_width) {
	// prepise zadnjo vrstico sobela v novo sliko
	for (int j = 0; j < new_width; j++) {
		cumulative[(height - 1) * width + j] = (int)image[(height - 1) * width + j];
	}

	int a, b, c, min, score;
	for (int i = height - 2; i >= 0; i--) {
		for (int j = 0; j < new_width; j++) {
			a = cumulative[(i + 1) * width + j];
			score = a;
			if (j - 1 < 0) {
				c = cumulative[(i + 1) * width + j + 1];
				if (c < a)
					score = c;
			}
			else if (j + 1 >= new_width) {
				b = cumulative[(i + 1) * width + j - 1];
				if (b < a)
					score = b;
			}
			else {
				b = cumulative[(i + 1) * width + j - 1];
				c = cumulative[(i + 1) * width + j + 1];
				if (a < b && a < c)
					score = a;
				else if (b < c)
					score = b;
				else
					score = c;
			}
			cumulative[i * width + j] = image[i * width + j] + score;
		}
	}

}

int find_lowest_value_index(int* a, int width) {
	int min = a[0];
	int index = 0;
	int temp;
	for (int j = 0; j < width; j++) {
		temp = a[j];
		if (temp < min) {
			min = temp;
			index = j;
		}
	}
	return index;
}

void shift_element(unsigned char* image, int* cumulative, unsigned char* imageRGB, int width, int new_width, int i, int index) {
	for (int j = index; j < new_width; j++) {
		image[i * width + j] = image[i * width + j + 1];
		cumulative[i * width + j] = cumulative[i * width + j + 1];

		imageRGB[i * width * 3 + j * 3] = imageRGB[i * width * 3 + j * 3 + 3];
		imageRGB[i * width * 3 + j * 3 + 1] = imageRGB[i * width * 3 + j * 3 + 1 + 3];
		imageRGB[i * width * 3 + j * 3 + 2] = imageRGB[i * width * 3 + j * 3 + 2 + 3];
	}
}

void remove_seam(unsigned char* image, int* cumulative, unsigned char* imageOutRGB, int width, int height, int index, int new_width) {
	int a, b, c;


	shift_element(image, cumulative, imageOutRGB, width, new_width, 0, index);

	for (int i = 1; i < height; i++) {
		a = cumulative[i * width + index];
		if (index - 1 < 0) {
			c = cumulative[i * width + index + 1];
			if (c < a)
				index++;
		}
		else if (index + 1 >= new_width) {
			b = cumulative[i * width + index - 1];
			if (b < a)
				index--;
		}
		else {
			b = cumulative[i * width + index - 1];

			c = cumulative[i * width + index + 1];
			if (a < b && a < c)
				index = index;
			else if (b < c)
				index--;
			else
				index++;
		}

		shift_element(image, cumulative, imageOutRGB, width, new_width, i, index);
	}
}

void seam_carving(unsigned char* image, unsigned char* imageOutRGB, int width, int height, int n) {
	int new_width = width;
	int* cumulative = (int*)malloc(width * height * sizeof(int));
	int index;
	for (int i = 0; i < n; i++) {
		calculate_cumulative(image, cumulative, width, height, new_width);
		index = find_lowest_value_index(cumulative, new_width);
		remove_seam(image, cumulative, imageOutRGB, width, height, index, new_width);
		new_width--;
	}
}

int main(void)
{
	int n = 500;
	clock_t start, stop;

	double time;
	start = clock();

	FIBITMAP* imageBitmap = FreeImage_Load(FIF_PNG, "slika.png", 0);

	FIBITMAP* imageBitmapRGB = FreeImage_ConvertTo24Bits(imageBitmap);
	FIBITMAP* imageBitmapGrey = FreeImage_ConvertToGreyscale(imageBitmap);

	int width = FreeImage_GetWidth(imageBitmapGrey);
	int height = FreeImage_GetHeight(imageBitmapGrey);
	int pitch = FreeImage_GetPitch(imageBitmapGrey);

	int pitch_rgb = FreeImage_GetPitch(imageBitmapRGB);

	unsigned char* imageIn = (unsigned char*)malloc(height * width * sizeof(unsigned char));
	unsigned char* imageOut = (unsigned char*)malloc(height * width * sizeof(unsigned char));
	// unsigned char *imageOut = (unsigned char*)malloc(height*width * sizeof(unsigned char));
	unsigned char* imageOutRGB = (unsigned char*)malloc(height * width * 3 * sizeof(unsigned char));

	FreeImage_ConvertToRawBits(imageIn, imageBitmapGrey, pitch, 8, 0xFF, 0xFF, 0xFF, TRUE);
	/*
	imageIn[0] = 3;
	imageIn[1] = 4;
	imageIn[2] = 6;
	imageIn[3] = 2;
	imageIn[4] = 4;
	imageIn[5] = 8;
	imageIn[6] = 1;
	imageIn[7] = 10;
	imageIn[8] = 11;
	imageIn[9] = 2;
	imageIn[10] = 9;
	imageIn[11] = 5;
	imageIn[12] = 3;
	imageIn[13] = 7;
	imageIn[14] = 6;
	imageIn[15] = 4;

	width = 4;
	height = 4;
	*/

	FreeImage_ConvertToRawBits(imageOutRGB, imageBitmapRGB, pitch_rgb, 24, 0xFF, 0xFF, 0xFF, TRUE);

	FreeImage_Unload(imageBitmapGrey);
	FreeImage_Unload(imageBitmap);
	FreeImage_Unload(imageBitmapRGB);

	start = clock();
	sobelCPU(imageIn, imageOut, width, height);
	seam_carving(imageOut, imageOutRGB, width, height, n);
	stop = clock();
	time = (double)(stop - start) / CLOCKS_PER_SEC;
	printf("Serial:     %f \n", time);
	
	// FIBITMAP *imageOutBitmap = FreeImage_ConvertFromRawBits(imageOut, width, height, pitch, 8, 0xFF, 0xFF, 0xFF, TRUE);
	// FreeImage_Save(FIF_PNG, imageOutBitmap, "sobel_slika2.png", 0);
	FIBITMAP* imageOutBitmap2 = FreeImage_ConvertFromRawBits(imageOutRGB, width - n, height, pitch_rgb, 24, 0xFF, 0xFF, 0xFF, TRUE);
	FreeImage_Save(FIF_PNG, imageOutBitmap2, "seam_result.png", 0);

	//FreeImage_Unload(imageOutBitmap2);

	

	return 0;
}


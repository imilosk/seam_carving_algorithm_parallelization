
__kernel void sobelGPU(__global unsigned char *imageIn, __global unsigned char *imageOut, const int width, const int height)
{
	int j = get_global_id(0);
	int i = get_global_id(1);

	if (j >= width || i >= height)
		return;

	int Gx, Gy;
	int tempPixel;

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

__kernel void cumulativeGPU(__global unsigned char *image, __global unsigned char *imageOutRGB, __global int *cumulative, const int width, const int height, __global int *indices, const int row_num, const int new_width)
{
    //int i = get_global_id(1);
    int j = get_global_id(0);

    if (j < new_width) {
		int a, b, c, min, score;
		a = cumulative[(row_num + 1) * width + j];
		score = a;

		if (j == 0) {
			c = cumulative[(row_num + 1) * width + j + 1];
			if (c < a)
				score = c;
		}
		else if (j + 1 >= new_width) {
			b = cumulative[(row_num + 1) * width + j - 1];
			if (b < a)
				score = b;
		}
		else {
			b = cumulative[(row_num + 1) * width + j - 1];
			c = cumulative[(row_num + 1) * width + j + 1];
			if (a < b && a < c)
				score = a;
			else if (b < c)
				score = b;
			else
				score = c;
		}
		cumulative[row_num * width + j] = image[row_num * width + j] + score;
    }

}
__kernel void find_seamGPU(__global int* cumulative, __global int* indices, const int width, const int new_width, const int height) {
	int i = get_global_id(1);
	int j = get_global_id(0);
	
	if (i == 0 && j == 0) {
		int index, a, b, c;
		index = find_lowest(cumulative, new_width);
		indices[0] = index;
		for (int k = 1; k < height; k++) {
			a = cumulative[k * width + index];
			if (index - 1 < 0) {
				c = cumulative[k * width + index + 1];
				if (c < a)
					index++;
			}
			else if (index + 1 >= new_width) {
				b = cumulative[k * width + index - 1];
				if (b < a)
					index--;
			}
			else {
				b = cumulative[k * width + index - 1];
				c = cumulative[k * width + index + 1];
				if (a < b && a < c)
					index = index;
				else if (b < c)
					index--;
				else
					index++;
			}
			indices[k] = index;
		}
	}
}

__kernel void removeSeamGPU(__global unsigned char* image, __global unsigned char* imageOutRGB, __global int* cumulative, __global int* indices, const int width, const int new_width, const int height) {
	//int i = get_global_id(1);
	int i = get_global_id(0);

	if (i < height) {
		shift_element(image, cumulative, imageOutRGB, width, new_width, i, indices[i]);
	}
}

__kernel void copyLastRowGPU(__global unsigned char* image, __global int* cumulative, const int width, const int height) {
	//int i = get_global_id(1);
	int j = get_global_id(0);

	if (j < width) {
		cumulative[width * height - 1 - j] = image[width * height - 1 - j];
	}
}

int shift_element(unsigned char *image, int *cumulative, unsigned char *imageRGB, int width, int new_width, int row, int index) {
    for (int k = index; k < new_width; k++) {
        image[row * width + k] = image[row * width + k + 1];
		cumulative[row * width + k] = cumulative[row * width + k + 1];

        imageRGB[row * width * 3 + k * 3]     = imageRGB[row * width * 3 + k * 3 + 3];
        imageRGB[row * width * 3 + k * 3 + 1] = imageRGB[row * width * 3 + k * 3 + 1 + 3];
        imageRGB[row * width * 3 + k * 3 + 2] = imageRGB[row * width * 3 + k * 3 + 2 + 3];
    }
    return 0;
}

int find_lowest(int *a, int width) {
    int min = a[0];
    int index = 0;
    int temp;
    for (int k = 0; k < width; k++) {
        temp = a[k];
        if (temp < min) {
            min = temp;
            index = k;
        }
    }
    return index;
}

inline int getPixel(unsigned char *image, int width, int height, int y, int x)
{
    if (x < 0 || x >= width)
        return 0;
    if (y < 0 || y >= height)
        return 0;
    return image[y*width + x];
}


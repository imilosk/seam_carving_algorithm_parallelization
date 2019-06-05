
__kernel void sobelGPU(__global unsigned char *imageIn, __global unsigned char *imageOut, const int width, const int height)
{

    int j = get_global_id(0);
    int i = get_global_id(1);

    /*
     if (i == 0 && j == 0) {
        for (int k = 0; k < height; k++) {
            for (int l = 0; l < width; l++) {
                printf("%2d ", imageIn[k * width + l]);
                imageOut[k * width + l] = imageIn[k * width + l];
            }
            printf("\n");
        }
        printf("\n");
    }
    */


    // __local unsigned char image_local[32][32];


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
        imageOut[i*width + j] = 255;
    else
        imageOut[i*width + j] = tempPixel;


}

__kernel void seamGPU(__global unsigned char *image, __global unsigned char *imageOutRGB, __global int *cumulative, const int width, const int height, __global int *indices)
{
    int i = get_global_id(1);
    int j = get_global_id(0);

    int new_width = width;
    int index;
    if (j < width && i < height) {
        // for (int a = 0; a < 500; a++) {
            calculate_cumulative(image, cumulative, width, height, new_width, i, j);
            // barrier(CLK_GLOBAL_MEM_FENCE);
            // remove_seam(image, cumulative, imageOutRGB, width, height, index, new_width, i, j, indices);
            // new_width--;
            // barrier(CLK_GLOBAL_MEM_FENCE);
        // }
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

int remove_seam(unsigned char *image, int *cumulative, unsigned char *imageOutRGB, int width, int height, int index, int new_width, int i, int j, int *indices)
{
    int a, b, c;

    // shift_element(image, cumulative, imageOutRGB, width, new_width, 0, index);
    // image[index] = 255;
    // shift_element(image, cumulative, imageOutRGB, width, new_width, 0, index);
    indices[0] = index;

    if (i == 0 && j == 0){
        index = find_lowest(cumulative, new_width);
        for (int k = 1; k < height; k++) {
            a = cumulative[k*width + index];
            if (index - 1 < 0) {
                c = cumulative[k*width + index + 1];
                if (c < a)
                    index++;
            } else if (index + 1 >= new_width) {
                b = cumulative[k*width + index - 1];
                if (b < a)
                    index--;
            } else {
                b = cumulative[k*width + index - 1];
                c = cumulative[k*width + index + 1];
                if(a < b && a < c)
                    index = index;
                else if(b < c)
                    index--;
                else
                    index++;
           }
           indices[k] = index;
           //shift_element(image, cumulative, imageOutRGB, width, new_width, k, index);
        }
    }
    barrier(CLK_GLOBAL_MEM_FENCE);
    if (i < height && j == 0)
        shift_element(image, cumulative, imageOutRGB, width, new_width, i, indices[i]);
    // image[k*width + index] = 255;

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


int calculate_cumulative(unsigned char *image, int *cumulative, int width, int height, int new_width, int i, int j)
{

    if (i == 0 && j < width) {
        cumulative[(height - 1) * width + j] = (int) image[(height - 1) * width + j];
    }

    barrier(CLK_GLOBAL_MEM_FENCE);

    int a, b, c, min, score;
    for (int k = height - 2; k >= 0; k--) {
        if (i > 0){
            a = cumulative[(k + 1)*width + j];
            score = a;
            if (j - 1 < 0) {
                c = cumulative[(k + 1)*width + j + 1];
                if (c < a)
                    score = c;
            }
            else if (j + 1 >= new_width) {
                b = cumulative[(k + 1)*width + j - 1];
                if (b < a)
                    score = b;
            }
            else {
                b = cumulative[(k + 1)*width + j - 1];
                c = cumulative[(k + 1)*width + j + 1];
                if (a < b && a < c)
                    score = a;
                else if (b < c)
                    score = b;
                else
                    score = c;
            }
            // printf("%d %d %d %d \n", b, a, c, j);
            cumulative[k * width + j] = image[k * width + j] + score;
        }
        barrier(CLK_GLOBAL_MEM_FENCE);
    }

    return 0;
}

inline int getPixel(unsigned char *image, int width, int height, int y, int x)
{
    if (x < 0 || x >= width)
        return 0;
    if (y < 0 || y >= height)
        return 0;
    return image[y*width + x];
}


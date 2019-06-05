#include <stdio.h>
#include <CL/opencl.h>
#include "FreeImage.h"
#include <math.h>
#include <time.h>

cl_int status;
cl_uint numDevices;
cl_device_id *devices = NULL;
cl_uint numPlatforms;
cl_platform_id platforms[10];

char buffer[100000];
cl_uint buf_uint;
cl_ulong buf_ulong;
size_t buf_sizet;

cl_int ciErr;

int main(int argc, const char * argv[]) {

    int n = 100;

    FIBITMAP *imageBitmap = FreeImage_Load(FIF_PNG, "slika.png", 0);

    FIBITMAP *imageBitmapRGB = FreeImage_ConvertTo24Bits(imageBitmap);
    FIBITMAP *imageBitmapGrey = FreeImage_ConvertToGreyscale(imageBitmap);

    int width = FreeImage_GetWidth(imageBitmapGrey);
    int height = FreeImage_GetHeight(imageBitmapGrey);
    int pitch = FreeImage_GetPitch(imageBitmapGrey);

    int pitch_rgb = FreeImage_GetPitch(imageBitmapRGB);

    unsigned char *imageIn = (unsigned char *) malloc(height * width * sizeof(unsigned char));
    unsigned char *imageOutCumulative = (unsigned char *) malloc(height * width * sizeof(unsigned char));
    unsigned char *imageOut = (unsigned char *) malloc(height * width * sizeof(unsigned char));
    unsigned char *imageOutRGB = (unsigned char *) malloc(height * width * 3 * sizeof(unsigned char));
    int *cumulative = (int *) malloc(width * height * sizeof(int));
    int *indices = (int *) malloc(height * sizeof(int));

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

    int szi = 32, szj = 32;


    int temp, width2, height2;

    temp = (int) width / szi + 1;
    width2 = szi * temp;

    temp = (int) height / szi + 1;
    height2 = szj * temp;

    const size_t szGlobalWorkSize[2] = {width2, height2};
    const size_t szLocalWorkSize[2] = {szi, szj};


    int numElements = width * height;

    //***************************************************
    // STEP 1: Discover and initialize the devices
    //***************************************************

    // Use clGetDeviceIDs() to retrieve the number of
    // devices present
    //
    //

    char ch;
    int i;
    cl_int ret;

    cl_platform_id platform_id[10];
    cl_uint ret_num_platforms;
    char *buf;
    size_t buf_len;
    ret = clGetPlatformIDs(10, platform_id, &ret_num_platforms);

    // Use clGetDeviceIDs() to retrieve the number of
    // devices present
    status = clGetDeviceIDs(
            platform_id[0],
            CL_DEVICE_TYPE_ALL,
            0,
            NULL,
            &numDevices);
    if (status != CL_SUCCESS) {
        printf("Error: Failed to create a device group1!\n");
        return EXIT_FAILURE;
    }

    // printf("The number of devices found = %d \n", numDevices);

    // Allocate enough space for each device
    devices = (cl_device_id *) malloc(numDevices * sizeof(cl_device_id));

    // Fill in devices with clGetDeviceIDs()
    status = clGetDeviceIDs(
            platform_id[0],
            CL_DEVICE_TYPE_GPU,
            numDevices,
            devices,
            NULL);
    if (status != CL_SUCCESS) {
        printf("Error: Failed to create a device group2!\n");
        return EXIT_FAILURE;
    }

    // printf("=== OpenCL devices: ===\n");
    /*
    for (int i=0; i<numDevices; i++)
    {

        printf("  -- The device with the index %d --\n", i);
        clGetDeviceInfo(devices[i],
                        CL_DEVICE_NAME,
                        sizeof(buffer),
                        buffer,
                        NULL);
        printf("  DEVICE_NAME = %s\n", buffer);


        clGetDeviceInfo(devices[i],
                        CL_DEVICE_MAX_COMPUTE_UNITS,
                        sizeof(buf_uint),
                        &buf_uint,
                        NULL);
        printf("  DEVICE_MAX_COMPUTE_UNITS = %u\n",
               (unsigned int)buf_uint);
        clGetDeviceInfo(devices[i],
                        CL_DEVICE_MAX_WORK_GROUP_SIZE,
                        sizeof(buf_sizet),
                        &buf_sizet,
                        NULL);
        printf("  CL_DEVICE_MAX_WORK_GROUP_SIZE = %u\n",
               (unsigned int)buf_sizet);
        clGetDeviceInfo(devices[i],
                        CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS,
                        sizeof(buf_uint),
                        &buf_uint,
                        NULL);
        printf("  CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS = %u\n",
               (unsigned int)buf_uint);

        size_t workitem_size[3];
        clGetDeviceInfo(devices[i],
                        CL_DEVICE_MAX_WORK_ITEM_SIZES,
                        sizeof(workitem_size),
                        &workitem_size,
                        NULL);
        printf("  CL_DEVICE_MAX_WORK_ITEM_SIZES = %u, %u, %u \n",
               (unsigned int)workitem_size[0],
               (unsigned int)workitem_size[1],
               (unsigned int)workitem_size[2]);

        clGetDeviceInfo(devices[i],
                        CL_DEVICE_LOCAL_MEM_SIZE,
                        sizeof(buf_ulong),
                        &buf_ulong,
                        NULL);
        printf("  CL_DEVICE_LOCAL_MEM_SIZE = %u\n",
               (unsigned int)buf_ulong);

    }
    */
    // STEP 2: Create a context
    //***************************************************

    cl_context context = NULL;
    // Create a context using clCreateContext() and
    // associate it with the devices
    context = clCreateContext(
            NULL,
            numDevices,
            &devices[0],
            NULL,
            NULL,
            &status);
    if (!context) {
        printf("Error: Failed to create a compute context!\n");
        return EXIT_FAILURE;
    }

    //***************************************************
    // STEP 3: Create a command queue
    //***************************************************
    cl_command_queue cmdQueue;
    // Create a command queue using clCreateCommandQueue(),
    // and associate it with the device you want to execute
    // on
    cmdQueue = clCreateCommandQueue(
            context,
            devices[0], // GPU
            CL_QUEUE_PROFILING_ENABLE,
            &status);

    if (!cmdQueue) {
        printf("Error: Failed to create a command commands!\n");
        return EXIT_FAILURE;
    }

    //***************************************************
    // STEP 4: Create the program object for a context
    //***************************************************
    FILE *programHandle;            // File that contains kernel functions
    size_t programSize;
    char *programBuffer;
    cl_program cpProgram;
    // 4 a: Read the OpenCL kernel from the source file and
    //      get the size of the kernel source
    programHandle = fopen("/home/Studenti/63160172/seminarska/kernel.cl", "rb");
    fseek(programHandle, 0, SEEK_END);
    programSize = ftell(programHandle);
    rewind(programHandle);

    printf("Program size = %lu B \n", programSize);

    // 4 b: read the kernel source into the buffer programBuffer
    //      add null-termination-required by clCreateProgramWithSource
    programBuffer = (char *) malloc(programSize + 1);

    programBuffer[programSize] = '\0'; // add null-termination
    fread(programBuffer, sizeof(char), programSize, programHandle);
    fclose(programHandle);

    // 4 c: Create the program from the source
    //
    cpProgram = clCreateProgramWithSource(
            context,
            1,
            (const char **) &programBuffer,
            &programSize,
            &ciErr);
    if (!cpProgram) {
        printf("Error: Failed to create compute program!\n");
        return EXIT_FAILURE;
    }
    free(programBuffer);

    //***************************************************
    // STEP 5: Build the program
    //***************************************************
    ciErr = clBuildProgram(
            cpProgram,
            0,
            NULL,
            NULL,
            NULL,
            NULL);

    if (ciErr != CL_SUCCESS) {
        size_t len;
        char buffer[4096 * 16];

        printf("Error: Failed to build program executable!\n");
        clGetProgramBuildInfo(cpProgram,
                              devices[0],
                              CL_PROGRAM_BUILD_LOG,
                              sizeof(buffer),
                              buffer,
                              &len);
        printf("%s\n", buffer);
        exit(1);
    }

    //***************************************************
    // STEP 6: Create device buffers
    //***************************************************
    cl_mem bufferImageIn, bufferImageOut, bufferImageRGB; // Input array on the device
    size_t datasize = sizeof(cl_char) * numElements;
    size_t datasize_rgb = sizeof(cl_char) * numElements * 3;

    // Use clCreateBuffer() to create a buffer object (d_A)
    // that will contain the data from the host array A
    bufferImageIn = clCreateBuffer(
            context,
            CL_MEM_READ_ONLY,
            datasize,
            NULL,
            &status);

    bufferImageOut = clCreateBuffer(
            context,
            CL_MEM_WRITE_ONLY,
            datasize,
            NULL,
            &status);

    /* init timer */
    struct timespec start, stop;
    double time;
    clock_gettime(CLOCK_MONOTONIC, &start);

    //***************************************************
    // STEP 7: Write host data to device buffers
    //***************************************************

    // Use clEnqueueWriteBuffer() to write input array to
    // the device buffer bufferImageIn
    status = clEnqueueWriteBuffer(cmdQueue,
                                  bufferImageIn,
                                  CL_FALSE,
                                  0,
                                  datasize,
                                  imageIn,
                                  0,
                                  NULL,
                                  NULL);

    //***************************************************
    // STEP 8: Create and compile the kernel
    //***************************************************
    cl_kernel ckKernel;
    // Create the kernel
    ckKernel = clCreateKernel(
            cpProgram,
            "sobelGPU",
            &ciErr);
    if (!ckKernel || ciErr != CL_SUCCESS) {
        printf("Error: Failed to create compute kernel!\n");
        exit(1);
    }

    //***************************************************
    // STEP 9: Set the kernel arguments
    //***************************************************
    // Set the Argument values
    ciErr = clSetKernelArg(ckKernel,
                           0,
                           sizeof(cl_mem),
                           (void *) &bufferImageIn);
    ciErr = clSetKernelArg(ckKernel,
                           1,
                           sizeof(cl_mem),
                           (void *) &bufferImageOut);
    ciErr |= clSetKernelArg(ckKernel,
                            2,
                            sizeof(cl_int),
                            (void *) &width);
    ciErr |= clSetKernelArg(ckKernel,
                            3,
                            sizeof(cl_int),
                            (void *) &height);

    //***************************************************
    // STEP 10: Enqueue the kernel for execution
    //***************************************************
    // Launch kernel

    ciErr = clEnqueueNDRangeKernel(cmdQueue,
                                   ckKernel,
                                   2,
                                   NULL,
                                   szGlobalWorkSize,
                                   szLocalWorkSize,
                                   0,
                                   NULL,
                                   NULL);

    if (ciErr != CL_SUCCESS) {
        printf("%d", ciErr);
        printf("Error launching kernel!\n");
    }


    // wait for kernel to finish:
    //
    clFinish(cmdQueue);

    //***************************************************
    // STEP 11: Read the output buffer back to the host
    //***************************************************
    // Synchronous/blocking read of results
    ciErr = clEnqueueReadBuffer(
            cmdQueue,
            bufferImageOut,
            CL_TRUE,
            0,
            datasize,
            imageOut,
            0,
            NULL,
            NULL);



    //***************************************************
    // STEP 6: Create device buffers 2
    //***************************************************
    // cl_mem bufferImageIn, bufferImageOut, bufferImageRGB; // Input array on the device
    size_t datasize2 = sizeof(cl_int) * numElements;
    size_t datasize_indices = sizeof(cl_int) * height;
    cl_mem bufferCumulative, bufferIndices;
    // Use clCreateBuffer() to create a buffer object (d_A)
    // that will contain the data from the host array A
    bufferImageOut = clCreateBuffer(
            context,
            CL_MEM_READ_WRITE,
            datasize,
            NULL,
            &status);

    bufferImageRGB = clCreateBuffer(
            context,
            CL_MEM_READ_WRITE,
            datasize * 3,
            NULL,
            &status);

    bufferCumulative = clCreateBuffer(
            context,
            CL_MEM_READ_WRITE,
            datasize2,
            NULL,
            &status);
    bufferIndices = clCreateBuffer(
            context,
            CL_MEM_READ_WRITE,
            datasize_indices,
            NULL,
            &status);



        //***************************************************
        // STEP 7: Write host data to device buffers 2
        //***************************************************

        // Use clEnqueueWriteBuffer() to write input array to
        // the device buffer bufferImageIn
        status = clEnqueueWriteBuffer(cmdQueue,
                                      bufferImageOut,
                                      CL_FALSE,
                                      0,
                                      datasize,
                                      imageOut,
                                      0,
                                      NULL,
                                      NULL);

        status = clEnqueueWriteBuffer(cmdQueue,
                                      bufferImageRGB,
                                      CL_FALSE,
                                      0,
                                      datasize * 3,
                                      imageOutRGB,
                                      0,
                                      NULL,
                                      NULL);

        status = clEnqueueWriteBuffer(cmdQueue,
                                      bufferCumulative,
                                      CL_FALSE,
                                      0,
                                      datasize2,
                                      cumulative,
                                      0,
                                      NULL,
                                      NULL);

        //***************************************************
        // STEP 8: Create and compile the kernel 2
        //***************************************************
        cl_kernel ckKernel2;
        // Create the kernel
        ckKernel2 = clCreateKernel(
                cpProgram,
                "seamGPU",
                &ciErr);
        if (!ckKernel2 || ciErr != CL_SUCCESS) {
            printf("%d", ciErr);
            printf("Error: Failed to create compute kernel!\n");
            exit(1);
        }

        //***************************************************
        // STEP 9: Set the kernel arguments 2
        //***************************************************
        // Set the Argument values
        ciErr = clSetKernelArg(ckKernel2,
                               0,
                               sizeof(cl_mem),
                               (void *) &bufferImageOut);
        ciErr = clSetKernelArg(ckKernel2,
                               1,
                               sizeof(cl_mem),
                               (void *) &bufferImageRGB);
        ciErr = clSetKernelArg(ckKernel2,
                               2,
                               sizeof(cl_mem),
                               (void *) &bufferCumulative);
        ciErr |= clSetKernelArg(ckKernel2,
                                3,
                                sizeof(cl_int),
                                (void *) &width);
        ciErr |= clSetKernelArg(ckKernel2,
                                4,
                                sizeof(cl_int),
                                (void *) &height);
        ciErr |= clSetKernelArg(ckKernel2,
                                5,
                                sizeof(cl_mem),
                                (void *) &bufferIndices);

    int k;
    for (k = 0; k < 100; k++) {
        //***************************************************
        // STEP 10: Enqueue the kernel for execution 2
        //***************************************************
        // Launch kernel
        ciErr = clEnqueueNDRangeKernel(cmdQueue,
                                       ckKernel2,
                                       2,
                                       NULL,
                                       szGlobalWorkSize,
                                       szLocalWorkSize,
                                       0,
                                       NULL,
                                       NULL);

        if (ciErr != CL_SUCCESS) {
            printf("%d", ciErr);
            printf("Error launching kernel!\n");
        }

        //***************************************************
        // STEP 11: Read the output buffer back to the host 2
        //***************************************************
        // Synchronous/blocking read of results
        ciErr = clEnqueueReadBuffer(
                cmdQueue,
                bufferImageRGB,
                CL_TRUE,
                0,
                datasize_rgb,
                imageOutRGB,
                0,
                NULL,
                NULL);



        // Wait for the command commands to get serviced before reading back results
        clFinish(cmdQueue);
    }

    clock_gettime(CLOCK_MONOTONIC, &stop);
    time = (stop.tv_sec - start.tv_sec);
    time += (stop.tv_nsec - start.tv_nsec) / 1000000000.0;
    printf("Parallel:     %f \n", time);

    FIBITMAP *imageOutBitmap = FreeImage_ConvertFromRawBits(imageOut, width - n, height, pitch, 8, 0xFF, 0xFF, 0xFF, TRUE);
    FreeImage_Save(FIF_PNG, imageOutBitmap, "sobel_slika.png", 0);
    FIBITMAP *imageOutBitmap2 = FreeImage_ConvertFromRawBits(imageOutRGB, width - n, height, pitch_rgb, 24, 0xFF, 0xFF, 0xFF, TRUE);
    FreeImage_Save(FIF_PNG, imageOutBitmap2, "seam_result_parallel.png", 0);

    FreeImage_Unload(imageOutBitmap);
    FreeImage_Unload(imageOutBitmap2);

    free(imageIn);
    free(imageOut);

    if(ckKernel) clReleaseKernel(ckKernel);
    if(cpProgram) clReleaseProgram(cpProgram);
    if(cmdQueue) clReleaseCommandQueue(cmdQueue);
    if(context) clReleaseContext(context);

    if(bufferImageIn) clReleaseMemObject(bufferImageIn);
    if(bufferImageOut) clReleaseMemObject(bufferImageOut);

    

    return 0;
}



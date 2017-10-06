#define _USE_MATH_DEFINES
#include <iostream>
#include <vector>
#include <chrono>
#include <math.h>

#include <tbb\parallel_for.h>
#include <tbb/task_scheduler_init.h>
#include <tbb\blocked_range2d.h>
#include <FreeImagePlus.h>

using namespace std;
using namespace tbb;
using namespace std::chrono;

void SequentialBlur();
void ParallelBlur();
void SequentialColorProcessing();
void ParallelColorProcessing();

int main()
{
	int nt = task_scheduler_init::default_num_threads();
	task_scheduler_init T(nt);

	//Uncomment the one you want to use/test

	//SequentialBlur();

	//ParallelBlur();

	//SequentialColorProcessing();

	//ParallelColorProcessing();

	return 0;
}

void SequentialBlur() {

	fipImage inputImage;
	inputImage.load("../Images/render_1.png");
	inputImage.convertToFloat();

	auto width = inputImage.getWidth();
	auto height = inputImage.getHeight();

	const float* inputBuffer = (float*)inputImage.accessPixels();

	int const blurRadius = 3;//The range of the area that will be blurred from the centre pixel. 
	//Feel free to increase it, ive already tested it at 100. That was without using the gaussian blue at the time instead using a flat number. 
	//The image is included in the image folder. Please look at it atleast. It took 20 minutes of my life wait for it to finish
	float const sigma = 1.0f;//Used for calculating the gaussian kernel

	int const size = (blurRadius * 2 + 1)*(blurRadius * 2 + 1);
	float GaussianKernal[size];

	int counter = 0;
	for (int j = -blurRadius; j < blurRadius + 1; j++){
		for (int i = -blurRadius; i < blurRadius + 1; i++){
			GaussianKernal[counter] = 1.0f / (2.0f * float(M_PI) * sigma * sigma) * exp(-((i * i + j * j) / (2.0f * sigma * sigma)));
			counter++;
		}
	}

	fipImage outputImage;
	outputImage = fipImage(FIT_FLOAT, width, height, 32);

	float *outputBuffer = (float*)outputImage.accessPixels();

	auto pt1 = high_resolution_clock::now();

	for (int y = blurRadius; y < height - blurRadius; ++y) {

		for (int x = blurRadius; x < width - blurRadius; ++x) {

			counter = 0;
			//This nested loop is for calculating the blur value (using a the gaussian equation) within a area of a pixel
			for (int j = -blurRadius; j < blurRadius + 1; j++){

				for (int i = -blurRadius; i < blurRadius + 1; i++){

					outputBuffer[y * width + x] += inputBuffer[(y + j) * width + (x + i)] * GaussianKernal[counter];
					//output pixel = Input pixel within area * gaussian equation for kernel
					counter++;
				}
			}
		}
	}

	auto pt2 = high_resolution_clock::now();
	duration<double> pt_dur = duration_cast<duration<double>>(pt2 - pt1);
	std::cout << "parallelBlur took = " << pt_dur.count() << "\n";

	std::cout << "Saving image...\n";

	outputImage.convertToType(FREE_IMAGE_TYPE::FIT_BITMAP);
	outputImage.convertTo24Bits();
	outputImage.save("Grey_Blurred_Result.png");

	std::cout << "...done\n\n";
}

void ParallelBlur() {

	fipImage inputImage;
	inputImage.load("../Images/render_1.png");
	inputImage.convertToFloat();

	auto width = inputImage.getWidth();
	auto height = inputImage.getHeight();

	const float* inputBuffer = (float*)inputImage.accessPixels();

	int const blurRadius = 3;//The range of the area that will be blurred from the centre pixel. 
	//Feel free to increase it, ive already tested it at 100. That was without using the gaussian blue at the time instead using a flat number. 
	//The image is included in the image folder. Please look at it atleast. It took 20 minutes of my life wait for it to finish
	float const sigma = 1.0f;//Used for calculating the gaussian kernel

	int const size = (blurRadius * 2 + 1)*(blurRadius * 2 + 1);
	float GaussianKernal[size];

	int counter = 0;
	for (int j = -blurRadius; j < blurRadius + 1; j++){
		for (int i = -blurRadius; i < blurRadius + 1; i++){
			GaussianKernal[counter] = 1.0f / (2.0f * float(M_PI) * sigma * sigma) * exp(-((i * i + j * j) / (2.0f * sigma * sigma)));
			counter++;
		}
	}

	fipImage outputImage;
	outputImage = fipImage(FIT_FLOAT, width, height, 32);

	float *outputBuffer = (float*)outputImage.accessPixels();

	auto pt1 = high_resolution_clock::now();

	int const stepsize = 1;//Looking at the test data from tweeting this value, it seems a high value only makes performance worse

	parallel_for(blocked_range2d<int, int>(blurRadius, height - blurRadius, stepsize, blurRadius, width - blurRadius, stepsize), [&](const blocked_range2d<int, int>& r) {
		//References of sigma, blurRadius and image datam are taken by the lambda function since it is needless to have its own copies. 
		//Besides most of the variables are constants, why would it need its own copies of that
		int counter;
		//parallel_for(blocked_range2d<int, int>(blurRadius, height - blurRadius,  blurRadius, width - blurRadius), [&](const blocked_range2d<int, int>& r) {
		//Left this line in incase you want to run the code without having a step size set
		auto startY = r.rows().begin();
		auto endY = r.rows().end();
		auto startX = r.cols().begin();
		auto endX = r.cols().end();

		for (int y = startY; y < endY; ++y) {

			for (int x = startX; x < endX; ++x) {
				counter = 0;
				//This nested loop is for calculating the blur value (using a the gaussian equation) within a area of a pixel
				for (int j = -blurRadius; j < blurRadius + 1; j++) {

					for (int i = -blurRadius; i < blurRadius + 1; i++) {
						outputBuffer[y * width + x] += inputBuffer[(y + j) * width + (x + i)] * GaussianKernal[counter];
						//output pixel = Input pixel within area * gaussian equation for kernel
						counter++;
					}
				}
			}
		}
	});

	auto pt2 = high_resolution_clock::now();
	duration<double> pt_dur = duration_cast<duration<double>>(pt2 - pt1);
	std::cout << "parallelBlur took = " << pt_dur.count() << "\n";

	std::cout << "Saving image...\n";

	outputImage.convertToType(FREE_IMAGE_TYPE::FIT_BITMAP);
	outputImage.convertTo24Bits();
	outputImage.save("Grey_Blurred_Result.png");

	std::cout << "...done\n\n";
}

void SequentialColorProcessing() {
	fipImage inputImage, inputImage2;
	inputImage.load("../Images/render_1.png");
	inputImage2.load("../Images/render_2.png");

	unsigned int width = inputImage.getWidth();
	unsigned int height = inputImage.getHeight();

	fipImage outputImage;
	outputImage = fipImage(FIT_BITMAP, width, height, 24);
	BYTE *outputBuffer = outputImage.accessPixels();

	//2D Vector to hold the RGB colour data of an image
	vector<vector<RGBQUAD>> rgbValues;
	rgbValues.resize(height, vector<RGBQUAD>(width));
	int const threshold = 2;//Tweakable value for how closely pixels should match

	RGBQUAD rgb1, rgb2;  //FreeImage structure to hold rgb1 values of a single pixel

	auto pt1 = high_resolution_clock::now();

	for (int y = 0; y < height; ++y) {

		for (int x = 0; x < width; ++x){

			inputImage.getPixelColor(x, y, &rgb1); //Extract pixel(x,y) colour data and place it in rgb1
			inputImage2.getPixelColor(x, y, &rgb2);

			//Subtracting the colour values for each pixel from eachother for the output. 
			//So if the output of this is 1 or 0, pixel is seen at the same for both images and will be turned black
			//The absolute is used to make sure no negative values are stored, otherwise weird noise can appear in the images
			rgbValues[y][x].rgbRed = abs(rgb1.rgbRed - rgb2.rgbRed);
			rgbValues[y][x].rgbGreen = abs(rgb1.rgbGreen - rgb2.rgbGreen);
			rgbValues[y][x].rgbBlue = abs(rgb1.rgbBlue - rgb2.rgbBlue);

			//This used to decide whether a pixel is black or white. No grey or coloured pixels can escape it
			if (rgbValues[y][x].rgbRed < threshold && rgbValues[y][x].rgbGreen < threshold && rgbValues[y][x].rgbBlue < threshold){
				//Make black
				rgbValues[y][x].rgbRed = 0;
				rgbValues[y][x].rgbGreen = 0;
				rgbValues[y][x].rgbBlue = 0;
			}
			else {
				//make white
				rgbValues[y][x].rgbRed = 255;
				rgbValues[y][x].rgbGreen = 255;
				rgbValues[y][x].rgbBlue = 255;
			}
			outputImage.setPixelColor(x, y, &rgbValues[y][x]);
		}
	}

	auto pt2 = high_resolution_clock::now();
	auto pt_dur = duration_cast<seconds>(pt2 - pt1);
	std::cout << "parallel colour subtraction took = " << pt_dur.count() << "\n";

	outputImage.save("RGB_Processed_Result.png");
}

void ParallelColorProcessing() {
	fipImage inputImage, inputImage2;
	inputImage.load("../Images/render_1.png");
	inputImage2.load("../Images/render_2.png");

	unsigned int width = inputImage.getWidth();
	unsigned int height = inputImage.getHeight();

	fipImage outputImage;
	outputImage = fipImage(FIT_BITMAP, width, height, 24);
	BYTE *outputBuffer = outputImage.accessPixels();

	//2D Vector to hold the RGB colour data of an image
	vector<vector<RGBQUAD>> rgbValues;
	rgbValues.resize(height, vector<RGBQUAD>(width));
	int const threshold = 2;//Tweakable value for how closely pixels should match

	RGBQUAD rgb1, rgb2;  //FreeImage structure to hold rgb1 values of a single pixel

	auto pt1 = high_resolution_clock::now();

	parallel_for(blocked_range2d< int, int>(0, height, 0, width), [&](const blocked_range2d<int, int>& r) {

		RGBQUAD rgb1, rgb2;  //FreeImage structure to hold rgb1 values of a single pixel

		auto startY = r.rows().begin();
		auto endY = r.rows().end();
		auto startX = r.cols().begin();
		auto endX = r.cols().end();

		for (int y = startY; y < endY; ++y) {

			for (int x = startX; x < endX; ++x) {

				inputImage.getPixelColor(x, y, &rgb1); //Extract pixel(x,y) colour data and place it in rgb1
				inputImage2.getPixelColor(x, y, &rgb2);

				//Subtracting the colour values for each pixel from eachother for the output. 
				//So if the output of this is 1 or 0, pixel is seen at the same for both images and will be turned black
				//The absolute is used to make sure no negative values are stored, otherwise weird noise can appear in the images
				rgbValues[y][x].rgbRed = abs(rgb1.rgbRed - rgb2.rgbRed);
				rgbValues[y][x].rgbGreen = abs(rgb1.rgbGreen - rgb2.rgbGreen);
				rgbValues[y][x].rgbBlue = abs(rgb1.rgbBlue - rgb2.rgbBlue);

				//This used to decide whether a pixel is black or white. No grey or coloured pixels can escape it
				if (rgbValues[y][x].rgbRed < threshold && rgbValues[y][x].rgbGreen < threshold && rgbValues[y][x].rgbBlue < threshold) {
					//Make black
					rgbValues[y][x].rgbRed = 0;
					rgbValues[y][x].rgbGreen = 0;
					rgbValues[y][x].rgbBlue = 0;
				}
				else {
					//make white
					rgbValues[y][x].rgbRed = 255;
					rgbValues[y][x].rgbGreen = 255;
					rgbValues[y][x].rgbBlue = 255;
				}

				outputImage.setPixelColor(x, y, &rgbValues[y][x]);
			}

		}
	});

	auto pt2 = high_resolution_clock::now();
	auto pt_dur = duration_cast<seconds>(pt2 - pt1);
	std::cout << "parallel colour subtraction took = " << pt_dur.count() << "\n";

	outputImage.save("RGB_Processed_Result.png");
}
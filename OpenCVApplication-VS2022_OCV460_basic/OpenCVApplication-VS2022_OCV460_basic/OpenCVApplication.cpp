// OpenCVApplication.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "common.h"
#include <opencv2/core/utils/logger.hpp>

wchar_t* projectPath;

Mat openImage()
{
	char fname[MAX_PATH];
	while(openFileDlg(fname))
	{
		Mat src;
		src = imread(fname); 
		imshow("Original QR-Code image", src);
		//waitKey();
		return src;
	}
}

void testOpenImagesFld()
{
	char folderName[MAX_PATH];
	if (openFolderDlg(folderName)==0)
		return;
	char fname[MAX_PATH];
	FileGetter fg(folderName,"bmp");
	while(fg.getNextAbsFile(fname))
	{
		Mat src;
		src = imread(fname);
		imshow(fg.getFoundFileName(),src);
		if (waitKey()==27) //ESC pressed
			break;
	}
}

void testImageOpenAndSave()
{
	_wchdir(projectPath);

	Mat src, dst;

	src = imread("Images/Lena_24bits.bmp", IMREAD_COLOR);	// Read the image

	if (!src.data)	// Check for invalid input
	{
		printf("Could not open or find the image\n");
		return;
	}

	// Get the image resolution
	Size src_size = Size(src.cols, src.rows);

	// Display window
	const char* WIN_SRC = "Src"; //window for the source image
	namedWindow(WIN_SRC, WINDOW_AUTOSIZE);
	moveWindow(WIN_SRC, 0, 0);

	const char* WIN_DST = "Dst"; //window for the destination (processed) image
	namedWindow(WIN_DST, WINDOW_AUTOSIZE);
	moveWindow(WIN_DST, src_size.width + 10, 0);

	cvtColor(src, dst, COLOR_BGR2GRAY); //converts the source image to a grayscale one

	imwrite("Images/Lena_24bits_gray.bmp", dst); //writes the destination to file

	imshow(WIN_SRC, src);
	imshow(WIN_DST, dst);

	waitKey(0);
}

void testNegativeImage()
{
	char fname[MAX_PATH];
	while(openFileDlg(fname))
	{
		//masoara timpul de procesare a imaginii
		double t = (double)getTickCount(); // Get the current time [s]
		
		Mat src = imread(fname,IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		Mat dst = Mat(height,width,CV_8UC1);
		// CV_8UC1 -> Accessing individual pixels in an 8 bits/pixel image
		// Inefficient way -> slow
		for (int i=0; i<height; i++)
		{
			for (int j=0; j<width; j++)
			{
				uchar val = src.at<uchar>(i, j); //uchar -> tipul de date cum e stocat un pixel in matricea src 
				uchar neg = 255 - val;
				dst.at<uchar>(i,j) = neg;
			}
		}

		// Get the current time again and compute the time difference [s]
		t = ((double)getTickCount() - t) / getTickFrequency();
		// Print (in the console window) the processing time in [ms] 
		printf("Time = %.3f [ms]\n", t * 1000);

		imshow("input image",src);
		imshow("negative image",dst);
		waitKey();
	}
}

void testResize()
{
	char fname[MAX_PATH];
	while(openFileDlg(fname))
	{
		Mat src;
		src = imread(fname);
		Mat dst1,dst2;
		//without interpolation
		resizeImg(src,dst1,320,false);
		//with interpolation
		resizeImg(src,dst2,320,true);
		imshow("input image",src);
		imshow("resized image (without interpolation)",dst1);
		imshow("resized image (with interpolation)",dst2);
		waitKey();
	}
}

void testCanny()
{
	char fname[MAX_PATH];
	while(openFileDlg(fname))
	{
		Mat src,dst,gauss;
		src = imread(fname,IMREAD_GRAYSCALE);
		double k = 0.4;
		int pH = 50;
		int pL = (int) k*pH;
		GaussianBlur(src, gauss, Size(5, 5), 0.8, 0.8);
		Canny(gauss,dst,pL,pH,3);
		imshow("input image",src);
		imshow("canny",dst);
		waitKey();
	}
}

void testVideoSequence()
{
	_wchdir(projectPath);

	VideoCapture cap("Videos/rubic.avi"); // off-line video from file
	//VideoCapture cap(0);	// live video from web cam
	if (!cap.isOpened()) {
		printf("Cannot open video capture device.\n");
		waitKey(0);
		return;
	}
		
	Mat edges;
	Mat frame;
	char c;

	while (cap.read(frame))
	{
		Mat grayFrame;
		cvtColor(frame, grayFrame, COLOR_BGR2GRAY);
		Canny(grayFrame,edges,40,100,3);
		imshow("source", frame);
		imshow("gray", grayFrame);
		imshow("edges", edges);
		c = waitKey(100);  // waits 100ms and advances to the next frame
		if (c == 27) {
			// press ESC to exit
			printf("ESC pressed - capture finished\n"); 
			break;  //ESC pressed
		};
	}
}


void testSnap()
{
	_wchdir(projectPath);

	VideoCapture cap(0); // open the deafult camera (i.e. the built in web cam)
	if (!cap.isOpened()) // openenig the video device failed
	{
		printf("Cannot open video capture device.\n");
		return;
	}

	Mat frame;
	char numberStr[256];
	char fileName[256];
	
	// video resolution
	Size capS = Size((int)cap.get(CAP_PROP_FRAME_WIDTH),
		(int)cap.get(CAP_PROP_FRAME_HEIGHT));

	// Display window
	const char* WIN_SRC = "Src"; //window for the source frame
	namedWindow(WIN_SRC, WINDOW_AUTOSIZE);
	moveWindow(WIN_SRC, 0, 0);

	const char* WIN_DST = "Snapped"; //window for showing the snapped frame
	namedWindow(WIN_DST, WINDOW_AUTOSIZE);
	moveWindow(WIN_DST, capS.width + 10, 0);

	char c;
	int frameNum = -1;
	int frameCount = 0;

	for (;;)
	{
		cap >> frame; // get a new frame from camera
		if (frame.empty())
		{
			printf("End of the video file\n");
			break;
		}

		++frameNum;
		
		imshow(WIN_SRC, frame);

		c = waitKey(10);  // waits a key press to advance to the next frame
		if (c == 27) {
			// press ESC to exit
			printf("ESC pressed - capture finished");
			break;  //ESC pressed
		}
		if (c == 115){ //'s' pressed - snap the image to a file
			frameCount++;
			fileName[0] = NULL;
			sprintf(numberStr, "%d", frameCount);
			strcat(fileName, "Images/A");
			strcat(fileName, numberStr);
			strcat(fileName, ".bmp");
			bool bSuccess = imwrite(fileName, frame);
			if (!bSuccess) 
			{
				printf("Error writing the snapped image\n");
			}
			else
				imshow(WIN_DST, frame);
		}
	}

}

void MyCallBackFunc(int event, int x, int y, int flags, void* param)
{
	//More examples: http://opencvexamples.blogspot.com/2014/01/detect-mouse-clicks-and-moves-on-image.html
	Mat* src = (Mat*)param;
	if (event == EVENT_LBUTTONDOWN)
		{
			printf("Pos(x,y): %d,%d  Color(RGB): %d,%d,%d\n",
				x, y,
				(int)(*src).at<Vec3b>(y, x)[2],
				(int)(*src).at<Vec3b>(y, x)[1],
				(int)(*src).at<Vec3b>(y, x)[0]);
		}
}

void testMouseClick()
{
	Mat src;
	// Read image from file 
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		src = imread(fname);
		//Create a window
		namedWindow("My Window", 1);

		//set the callback function for any mouse event
		setMouseCallback("My Window", MyCallBackFunc, &src);

		//show the image
		imshow("My Window", src);

		// Wait until user press some key
		waitKey(0);
	}
}

void testColourImageConvToGreyscale()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		//masoara timpul de procesare a imaginii
		double t = (double)getTickCount(); // Get the current time [s]

		Mat src = imread(fname, IMREAD_COLOR);
		int height = src.rows;
		int width = src.cols;
		Mat dst1 = Mat(height, width, CV_8UC1);
		Mat dst2 = Mat(height, width, CV_8UC1);
		Mat dst3 = Mat(height, width, CV_8UC1);
		// CV_8UC3 -> Accessing individual pixels in an 8 bits/pixel image
		// Inefficient way -> slow
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				Vec3b val = src.at<Vec3b>(i, j);
				dst1.at<uchar>(i, j) = (val[0]+val[1]+val[2])/3;
				dst2.at<uchar>(i, j) = 0.28 * val[0] + 0.6 * val[1] + 0.12 * val[2];
				dst3.at<uchar>(i, j) = (min(val[0], min(val[1], val[2])) + max(val[0], max(val[1], val[2]))) / 2;
			}
		}

		// Get the current time again and compute the time difference [s]
		t = ((double)getTickCount() - t) / getTickFrequency();
		// Print (in the console window) the processing time in [ms] 
		printf("Time = %.3f [ms]\n", t * 1000);

		imshow("input image", src);
		imshow("color image GS V1", dst1);
		imshow("color image GS V2", dst2);
		imshow("color image GS V3", dst3);
		waitKey();
	}
}

int isInside(Mat img, int i, int j)
{
	int height = img.rows;
	int width = img.cols;
	if (i >= 0 && i < height && j >= 0 && j < width)
		return 1;
	return 0;
}

void testColourImageIsInside()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		//masoara timpul de procesare a imaginii
		double t = (double)getTickCount(); // Get the current time [s]

		Mat src = imread(fname, IMREAD_GRAYSCALE);
		
		printf("Enter the coordinates of the pixel: ");
		int i, j;
		scanf("%d%d", &i, &j);
		printf("The pixel is %s the image\n", isInside(src, i, j) ? "inside" : "outside");

		// Get the current time again and compute the time difference [s]
		t = ((double)getTickCount() - t) / getTickFrequency();
		// Print (in the console window) the processing time in [ms] 
		printf("Time = %.3f [ms]\n", t * 1000);

		imshow("input image", src);
		//imshow("black and white img", dst);

		waitKey();
	}
}


/* Histogram display function - display a histogram using bars (simlilar to L3 / Image Processing)
Input:
name - destination (output) window name
hist - pointer to the vector containing the histogram values
hist_cols - no. of bins (elements) in the histogram = histogram image width
hist_height - height of the histogram image
Call example:
showHistogram ("MyHist", hist_dir, 255, 200);
*/
void showHistogram(const std::string& name, int* hist, const int  hist_cols, const int hist_height)
{
	Mat imgHist(hist_height, hist_cols, CV_8UC3, CV_RGB(255, 255, 255)); // constructs a white image

	//computes histogram maximum
	int max_hist = 0;
	for (int i = 0; i < hist_cols; i++)
		if (hist[i] > max_hist)
			max_hist = hist[i];
	double scale = 1.0;
	scale = (double)hist_height / max_hist;
	int baseline = hist_height - 1;

	for (int x = 0; x < hist_cols; x++) {
		Point p1 = Point(x, baseline);
		Point p2 = Point(x, baseline - cvRound(hist[x] * scale));
		line(imgHist, p1, p2, CV_RGB(255, 0, 255)); // histogram bins colored in magenta
	}

	imshow(name, imgHist);
}

///calculeaza histograma unei imagini grayscale
void calcHistogram()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		int* hist = new int[256];
		for (int i = 0; i < 256; i++)
				hist[i] = 0;

		for (int i = 0; i<height; i++)
			for (int j = 0; j<width; j++)
				hist[src.at<uchar>(i, j)]++;

		imshow("input image", src);
		showHistogram("histogram", hist, height, width);

		waitKey();
	}	
}


///calculeaza histograma normalizata, afiseaza in consola valorile 
void calcNormalizedHistogram()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		int* hist = new int[256];
		for (int i = 0; i < 256; i++)
			hist[i] = 0;

		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++)
				hist[src.at<uchar>(i, j)]++;

		float totalPixels = height * width;
		float* histNorm = new float[256];
		for (int i = 0; i < 256; i++) {
			histNorm[i] = hist[i] / totalPixels; 
			printf("FDP %d: %.4f\n", i, histNorm[i]);
		}

		imshow("input image", src);
		showHistogram("histogram", hist, height, width);

		waitKey();
	}
}


int binarizareHelper(std::vector<int> praguri, int val) {
	for (int i = 0; i < praguri.size() - 1; i++) {
		if (val >= praguri[i] && val <= praguri[i + 1]) {
			if (val - praguri[i] < praguri[i + 1] - val) {
				return praguri[i];
			}
			else if (val - praguri[i] >= praguri[i + 1] - val) {
				return praguri[i + 1];
			}
		}
	}
	return -1;
}

///histo cu praguri 
void histogramWithThresholds()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_GRAYSCALE);
		Mat dst = Mat(src.rows, src.cols, CV_8UC1);

		int height = src.rows;
		int width = src.cols;
		int* hist = new int[256];
		for (int i = 0; i < 256; i++)
			hist[i] = 0;
		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++)
				hist[src.at<uchar>(i, j)]++;

		float totalPixels = height * width;
		float* histNorm = new float[256];
		for (int i = 0; i < 256; i++) {
			histNorm[i] = hist[i] / totalPixels;
			//printf("FDP %d: %.4f\n", i, histNorm[i]);
		}

		std::vector<int> praguri;
		praguri.push_back(0);

		int wh = 3;
		for (int i = wh;i < 256 - wh;i++) {
			bool isMax = true;
			for (int j = i - wh; j <= i + wh; j++) {
				if (histNorm[j] > histNorm[i]) {
					isMax = false;
					break;
				}
			}

			if (isMax) {
				praguri.push_back(i);
			}
		}

		praguri.push_back(255);

		for (int i = 0;i < praguri.size(); i++) {
			printf("[%d]: %d\n", i, praguri.at(i));
		}

		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++) {
				int val = src.at<uchar>(i, j);
				int bin = binarizareHelper(praguri, val);
				if (bin != -1) {
					dst.at<uchar>(i, j) = bin;
				}
				else {
					//dst.at<uchar>(i, j) = 0;
					printf("NICIOADAT");
				}

				//printf("%d,",dst.at<uchar>(i, j));
			}

		imshow("input image", src);
		imshow("binarized image", dst);
		//showHistogram("histogram", hist, height, width);
		waitKey();
	}
}


void histogramWithThresholdsAndCorrection()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_GRAYSCALE);
		Mat dst = Mat(src.rows, src.cols, CV_8UC1);

		int height = src.rows;
		int width = src.cols;
		int* hist = new int[256];
		for (int i = 0; i < 256; i++)
			hist[i] = 0;
		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++)
				hist[src.at<uchar>(i, j)]++;

		float totalPixels = height * width;
		float* histNorm = new float[256];
		for (int i = 0; i < 256; i++) {
			histNorm[i] = hist[i] / totalPixels;
			//printf("FDP %d: %.4f\n", i, histNorm[i]);
		}

		std::vector<int> praguri;
		praguri.push_back(0);

		int wh = 5;
		for (int i = wh;i < 256 - wh;i++) {
			bool isMax = true;
			for (int j = i - wh; j <= i + wh; j++) {
				if (histNorm[j] > histNorm[i]) {
					isMax = false;
					break;
				}
			}

			if (isMax) {
				praguri.push_back(i);
			}
		}

		praguri.push_back(255);

		for (int i = 0;i < praguri.size(); i++) {
			printf("[%d]: %d\n", i, praguri.at(i));
		}

		int err=0; 
		src.clone().copyTo(dst);

		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++) {
				int val = src.at<uchar>(i, j);
				int val2 = dst.at<uchar>(i, j);
				int bin = binarizareHelper(praguri, val2);
				if (bin != -1) {
					dst.at<uchar>(i, j) = bin;
					err = val2 - bin;

					if (isInside(dst, i, j + 1)) dst.at<uchar>(i, j + 1) = dst.at<uchar>(i, j + 1) + 7*err / 16;
					else if (isInside(dst,i+1,j-1)) dst.at<uchar>(i + 1, j - 1) = dst.at<uchar>(i + 1, j - 1) + 3*err / 16;
					else if (isInside(dst,i+1,j)) dst.at<uchar>(i + 1, j) = dst.at<uchar>(i + 1, j) + 5*err / 16;
					else if (isInside(dst, i + 1, j + 1)) dst.at<uchar>(i + 1, j + 1) = dst.at<uchar>(i + 1, j + 1) + err / 16;
				}
				else {
					//dst.at<uchar>(i, j) = 0;
					printf("NICIOADAT");
				}

				//printf("%d,",dst.at<uchar>(i, j));
			}

		imshow("input image", src);
		imshow("binarized image + correction", dst);
		//showHistogram("histogram", hist, height, width);
		waitKey();
	}
}

///////////////////////

/// QR

///////////////////////


Mat image2Gray(Mat src)
{
	//char fname[MAX_PATH];
	//while(openFileDlg(fname))
	//{
		//Mat src = imread(fname);

	int height = src.rows;
	int width = src.cols;

	Mat dst = Mat(height, width, CV_8UC1);

	// Accessing individual pixels in a RGB 24 bits/pixel image
	// Inefficient way -> slow
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			Vec3b v3 = src.at<Vec3b>(i, j);
			uchar b = v3[0];
			uchar g = v3[1];
			uchar r = v3[2];
			//dst.at<uchar>(i,j) = (r+g+b)/3;
			dst.at<uchar>(i, j) = (uchar)(0.299 * r + 0.587 * g + 0.114 * b);
		}
	}

	imshow("Imagine grayscale", dst);
	return dst;

}



Mat binarizare(Mat src)
{
	int height = src.rows;
	int width = src.cols;
	int M = height * width;

	Mat dst = Mat(height, width, CV_8UC1);

	uchar minVal = 255;
	uchar maxVal = 0;
	long long sumAll = 0;

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			uchar val = src.at<uchar>(i, j);
			sumAll += val;
			if (val < minVal) minVal = val;
			if (val > maxVal) maxVal = val;
		}
	}

	double miu = (double)sumAll / M;

	double sumVariance = 0;
	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			uchar val = src.at<uchar>(i, j);
			sumVariance += (val - miu) * (val - miu);
		}
	}
	double sigma = sqrt(sumVariance / M);

	printf("Miu (Valoarea medie) = %.2f\n", miu);
	printf("Sigma (Deviatia standard) = %.2f\n", sigma);

	int T = (minVal + maxVal) / 2;
	int TNext = T;

	do {
		T = TNext;
		long long sum1 = 0, sum2 = 0;
		int count1 = 0, count2 = 0;

		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				uchar val = src.at<uchar>(i, j);
				if (val <= T) {
					sum1 += val;
					count1++;
				}
				else {
					sum2 += val;
					count2++;
				}
			}
		}

		int V1 = count1 > 0 ? sum1 / count1 : 0;
		int V2 = count2 > 0 ? sum2 / count2 : 0;

		TNext = (V1 + V2) / 2;
	} while (T != TNext);

	printf("Pragul optim calculat = %d\n", T);

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			uchar val = src.at<uchar>(i, j);
			if (val <= T)
				dst.at<uchar>(i, j) = 0;
			else
				dst.at<uchar>(i, j) = 255;
		}
	}

	imshow("Imagine binarizata (alb+negru)", dst);

	return dst;
}

Mat createStructuringElement() {
	Mat element = Mat(3, 3, CV_8UC1, Scalar(255));

	element.at<uchar>(0, 1) = 0;
	element.at<uchar>(1, 0) = 0;
	element.at<uchar>(1, 1) = 0;
	element.at<uchar>(1, 2) = 0;
	element.at<uchar>(2, 1) = 0;

	return element;
}

Mat dilatare(Mat src) {
	Mat dst = src.clone();

	int rows = src.rows;
	int cols = src.cols;

	Mat elStr = createStructuringElement();
	int elRows = elStr.rows;
	int elCols = elStr.cols;

	for (int i = 1; i < rows - 1; i++)
		for (int j = 1; j < cols - 1; j++) {
			if (src.at<uchar>(i, j) == 0) {
				for (int k = 0; k < elRows; k++)
					for (int l = 0; l < elCols; l++) {
						if (elStr.at<uchar>(k, l) == 0) {
							int newI = i + k - 1;
							int newJ = j + l - 1;
							dst.at<uchar>(newI, newJ) = 0;
						}
					}
			}
		}

	imshow("Dilatare", dst);

	return dst;
}

Mat clearSaltAndPepper(Mat src) {
	int height = src.rows;
	int width = src.cols;

	Mat dst_median = Mat(height, width, CV_8UC1, Scalar(0));
	uchar vals[9];

	for (int i = 0; i < height - 2; i++) {
		for (int j = 0; j < width - 2; j++) {

			int k = 0;
			for (int p = i; p < i + 3; p++) {
				for (int q = j; q < j + 3; q++) {
					vals[k++] = src.at<uchar>(p, q);
				}
			}
			std::sort(vals, vals + 9);
			dst_median.at<uchar>(i + 1, j + 1) = vals[4];
		}
	}

	imshow("CleanedSaltAndPepper", dst_median);
	return dst_median;
}

Mat detectFinderPatternsAndColor(Mat binImg, std::vector<Point2f>& outCorners) {
	int height = binImg.rows;
	int width = binImg.cols;
	Mat dst;
	cvtColor(binImg, dst, COLOR_GRAY2BGR);

	struct Candidate {
		Point2f pos; // Modificat la Point2f pentru precizie sub-pixel!
		float unit;
	};
	std::vector<Candidate> candidates;

	for (int i = 0; i < height; i++) {
		std::vector<int> counter(5, 0);
		int currentState = 0;
		for (int j = 0; j < width; j++) {
			uchar pixel = binImg.at<uchar>(i, j);
			bool isBlack = (pixel == 0);

			if (isBlack == (currentState % 2 == 0)) {
				counter[currentState]++;
			}
			else {
				if (currentState == 4) {
					int totalWidth = counter[0] + counter[1] + counter[2] + counter[3] + counter[4];
					float unit = (float)totalWidth / 7.0f;
					float maxErr = unit * 0.85f;

					if (abs(counter[0] - unit) < maxErr && abs(counter[1] - unit) < maxErr &&
						abs(counter[2] - unit * 3) < maxErr * 3 && abs(counter[3] - unit) < maxErr &&
						abs(counter[4] - unit) < maxErr) {

						int centerX = j - counter[4] - counter[3] - counter[2] / 2;
						int up = i, down = i;
						while (isInside(binImg, up - 1, centerX) && binImg.at<uchar>(up - 1, centerX) == 0) up--;
						while (isInside(binImg, down + 1, centerX) && binImg.at<uchar>(down + 1, centerX) == 0) down++;
						int correctedCenterY = (up + down) / 2;

						int vCounter[5] = { 0, 0, 0, 0, 0 };
						int y = correctedCenterY;

						while (y >= 0 && binImg.at<uchar>(y, centerX) == 0) { vCounter[2]++; y--; }
						while (y >= 0 && binImg.at<uchar>(y, centerX) != 0) { vCounter[1]++; y--; }
						while (y >= 0 && binImg.at<uchar>(y, centerX) == 0) { vCounter[0]++; y--; }

						y = correctedCenterY + 1;
						while (y < height && binImg.at<uchar>(y, centerX) == 0) { vCounter[2]++; y++; }
						while (y < height && binImg.at<uchar>(y, centerX) != 0) { vCounter[3]++; y++; }
						while (y < height && binImg.at<uchar>(y, centerX) == 0) { vCounter[4]++; y++; }

						int vTotal = vCounter[0] + vCounter[1] + vCounter[2] + vCounter[3] + vCounter[4];
						float vUnit = (float)vTotal / 7.0f;
						float vMaxErr = vUnit * 0.85f;

						if (abs(vCounter[0] - vUnit) < vMaxErr && abs(vCounter[1] - vUnit) < vMaxErr &&
							abs(vCounter[2] - vUnit * 3) < vMaxErr * 3 && abs(vCounter[3] - vUnit) < vMaxErr &&
							abs(vCounter[4] - vUnit) < vMaxErr &&
							abs(unit - vUnit) < unit * 0.9f) {

							// --- MAGIA CENTRULUI DE MASA ---
							// Colectam toti pixelii negri din ochiul central pt. a afla centrul perfect!
							int sumX = 0, sumY = 0, count = 0;
							int safeRadius = (unit + vUnit) / 2.0f * 1.5f;

							for (int dy = -safeRadius; dy <= safeRadius; dy++) {
								for (int dx = -safeRadius; dx <= safeRadius; dx++) {
									int py = correctedCenterY + dy;
									int px = centerX + dx;
									if (isInside(binImg, py, px) && binImg.at<uchar>(py, px) == 0) {
										sumX += px;
										sumY += py;
										count++;
									}
								}
							}

							if (count > 0) {
								float exactX = (float)sumX / count;
								float exactY = (float)sumY / count;
								float exactUnit = sqrt((float)count / 9.0f); // Invariant la rotatie!

								bool duplicate = false;
								for (const auto& c : candidates) {
									if (norm(c.pos - Point2f(exactX, exactY)) < exactUnit * 3) duplicate = true;
								}
								if (!duplicate) {
									candidates.push_back({ Point2f(exactX, exactY), exactUnit });
								}
							}
						}
					}
					counter[0] = counter[2]; counter[1] = counter[3]; counter[2] = counter[4];
					counter[3] = 1; counter[4] = 0; currentState = 3;
				}
				else {
					currentState++;
					counter[currentState]++;
				}
			}
		}
	}

	std::vector<Candidate> validQR, bestCombo;
	float maxSize = 0.0f;

	if (candidates.size() >= 3) {
		for (size_t i = 0; i < candidates.size(); i++) {
			for (size_t j = i + 1; j < candidates.size(); j++) {
				for (size_t k = j + 1; k < candidates.size(); k++) {
					float u1 = candidates[i].unit, u2 = candidates[j].unit, u3 = candidates[k].unit;
					float avgUnit = (u1 + u2 + u3) / 3.0f;

					if (abs(u1 - avgUnit) > 0.6 * avgUnit || abs(u2 - avgUnit) > 0.6 * avgUnit || abs(u3 - avgUnit) > 0.6 * avgUnit)
						continue;

					float d1 = norm(candidates[i].pos - candidates[j].pos);
					float d2 = norm(candidates[i].pos - candidates[k].pos);
					float d3 = norm(candidates[j].pos - candidates[k].pos);
					float dists[3] = { d1, d2, d3 };
					std::sort(dists, dists + 3);

					if (abs(dists[0] - dists[1]) < 0.7 * dists[1]) {
						if (abs((dists[0] * dists[0] + dists[1] * dists[1]) - dists[2] * dists[2]) < 0.7 * (dists[2] * dists[2])) {
							float currentSize = dists[0] + dists[1] + dists[2];
							if (currentSize > maxSize) {
								maxSize = currentSize;
								bestCombo = { candidates[i], candidates[j], candidates[k] };
							}
						}
					}
				}
			}
		}
	}

	candidates = bestCombo.empty() ? validQR : bestCombo;
	if (candidates.size() == 3) {
		for (const auto& c : candidates) circle(dst, c.pos, (int)(c.unit * 2), Scalar(0, 0, 255), 3);
		printf("Detectate corect cele 3 puncte principale.\n");
	}
	else printf("Eroare detectie.\n");

	imshow("DST: Puncte de Control Filtrate", dst);
	for (const auto& c : candidates) outCorners.push_back(c.pos);
	return dst;
}

std::vector<Point2f> getMarkerInnerCorners(Mat binImg, Point2f center) {
	int cx = round(center.x);
	int cy = round(center.y);
	int left = cx, right = cx, top = cy, bottom = cy;

	// Acum ca imaginea va fi dreapta (din Pass 1), putem merge strict in linie dreapta 
	// pentru a gasi marginile negre perfecte ale ochiului!
	while (left > 0 && binImg.at<uchar>(cy, left) == 0) left--; left++;
	while (right < binImg.cols - 1 && binImg.at<uchar>(cy, right) == 0) right++; right--;
	while (top > 0 && binImg.at<uchar>(top, cx) == 0) top--; top++;
	while (bottom < binImg.rows - 1 && binImg.at<uchar>(bottom, cx) == 0) bottom++; bottom--;

	return { Point2f(left, top), Point2f(right, top), Point2f(left, bottom), Point2f(right, bottom) };
}


std::vector<int> getAlignmentPositions(int version) {
	static std::vector<std::vector<int>> table = {
		{},
		{6, 18},
		{6, 22},
		{6, 26},
		{6, 30},
		{6, 34},
		{6, 22, 38},
		{6, 24, 42},
		{6, 26, 46},
		{6, 28, 50},
	};
	if (version < 1 || version > 10) return {};
	return table[version - 1];
}

Mat applyAffineCorrection(Mat binImg, std::vector<Point2f> corners, int& outVersion) {
	Mat processedImg = dilatare(binImg);

	int idxTL = 0;
	for (int i = 1; i < 3; i++)
		if (corners[i].x + corners[i].y < corners[idxTL].x + corners[idxTL].y) idxTL = i;

	Point2f pTL = corners[idxTL];
	std::vector<Point2f> rest;
	for (int i = 0; i < 3; i++) if (i != idxTL) rest.push_back(corners[i]);

	Point2f pTR, pBL;
	if (rest[0].x > rest[1].x) { pTR = rest[0]; pBL = rest[1]; }
	else { pTR = rest[1]; pBL = rest[0]; }

	// --- MARIM PANZA LA 500x500 PENTRU A NU MAI TAIA NIMIC ---
	int warpSize = 500;
	int version = 1;
	outVersion = version;
	int N_qr = 21;
	float modSize = 16.0f;

	// Calculam o margine (padding) groasa de siguranta
	float padding = (warpSize - (N_qr * modSize)) / 2.0f; // Va fi aprox 82 de pixeli
	float offset = padding + 3.5f * modSize;
	float endOffset = padding + (N_qr - 3.5f) * modSize;

	// PASS 1: AFFINE TRANSFORM (Doar indreptam rota?ia)
	Point2f srcPts[3] = { pTL, pTR, pBL };
	Point2f dstPts[3] = { Point2f(offset, offset), Point2f(endOffset, offset), Point2f(offset, endOffset) };
	Mat M = getAffineTransform(srcPts, dstPts);
	Mat affineWarped;
	warpAffine(processedImg, affineWarped, M, Size(warpSize, warpSize), INTER_NEAREST, BORDER_CONSTANT, Scalar(255));

	// PASS 2: Gasim cele 12 colturi in imaginea indreptata
	std::vector<Point2f> pTL_corners = getMarkerInnerCorners(affineWarped, Point2f(offset, offset));
	std::vector<Point2f> pTR_corners = getMarkerInnerCorners(affineWarped, Point2f(endOffset, offset));
	std::vector<Point2f> pBL_corners = getMarkerInnerCorners(affineWarped, Point2f(offset, endOffset));

	std::vector<Point2f> srcH, dstH;
	srcH.insert(srcH.end(), pTL_corners.begin(), pTL_corners.end());
	srcH.insert(srcH.end(), pTR_corners.begin(), pTR_corners.end());
	srcH.insert(srcH.end(), pBL_corners.begin(), pBL_corners.end());

	auto addEyeDst = [&](float startCol, float startRow) {
		dstH.push_back(Point2f(padding + (startCol + 2) * modSize, padding + (startRow + 2) * modSize));
		dstH.push_back(Point2f(padding + (startCol + 5) * modSize, padding + (startRow + 2) * modSize));
		dstH.push_back(Point2f(padding + (startCol + 2) * modSize, padding + (startRow + 5) * modSize));
		dstH.push_back(Point2f(padding + (startCol + 5) * modSize, padding + (startRow + 5) * modSize));
		};

	// Generam coordonatele matematice absolute
	addEyeDst(0, 0);       // Ochiul TL
	addEyeDst(N_qr - 7, 0); // Ochiul TR
	addEyeDst(0, N_qr - 7); // Ochiul BL

	// PASS 3: HOMOGRAPHY (Aplica corectia exacta pentru acel decalaj 3D de 7%)
	Mat H = findHomography(srcH, dstH);
	Mat finalWarped;
	warpPerspective(affineWarped, finalWarped, H, Size(warpSize, warpSize), INTER_NEAREST, BORDER_CONSTANT, Scalar(255));

	imshow("Warped QR (Matematica Suprema)", finalWarped);
	return finalWarped;
}

std::vector<Point> detectAlignmentPatterns(Mat warped, int version, int moduleSize) {
	std::vector<Point> found;
	if (version < 2) {
		printf("Versiunea 1: fara alignment patterns.\n");
		return found;
	}

	std::vector<int> pos = getAlignmentPositions(version);
	int warpSize = warped.cols;

	for (int r : pos) {
		for (int c : pos) {
			bool nearTL = (r <= 8 && c <= 8);
			bool nearTR = (r <= 8 && c >= (int)pos.back() - 2);
			bool nearBL = (r >= (int)pos.back() - 2 && c <= 8);
			if (nearTL || nearTR || nearBL) continue;

			int cx = c * moduleSize + moduleSize / 2;
			int cy = r * moduleSize + moduleSize / 2;

			if (cy < warpSize && cx < warpSize && warped.at<uchar>(cy, cx) == 0) {
				found.push_back(Point(cx, cy));
				printf("Alignment pattern la modul (%d,%d) -> pixel (%d,%d)\n", r, c, cx, cy);
			}
		}
	}

	Mat viz;
	cvtColor(warped, viz, COLOR_GRAY2BGR);
	for (const auto& p : found)
		rectangle(viz, Point(p.x - 2 * moduleSize, p.y - 2 * moduleSize),
			Point(p.x + 2 * moduleSize, p.y + 2 * moduleSize),
			Scalar(0, 255, 0), 2);
	imshow("Alignment Patterns", viz);

	return found;
}

Mat sampleModuleGrid(Mat warped, int version, int moduleSize_vechi) {
	Mat grayWarped;
	if (warped.channels() == 3) cvtColor(warped, grayWarped, COLOR_BGR2GRAY);
	else grayWarped = warped.clone();

	int N = 21;
	float modSize = 16.0f;
	float padding = (warped.cols - (N * modSize)) / 2.0f; // Ne centram dupa paddingul de mai sus
	Mat grid(N, N, CV_8UC1);

	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			int cy = round(padding + i * modSize + modSize / 2.0f);
			int cx = round(padding + j * modSize + modSize / 2.0f);

			int blackCount = 0, whiteCount = 0;
			int safeRadius = 3; // Careu extrem de sigur (7x7 pixeli) fix in inima modulului

			for (int dy = -safeRadius; dy <= safeRadius; dy++) {
				for (int dx = -safeRadius; dx <= safeRadius; dx++) {
					int y = min(max(cy + dy, 0), grayWarped.rows - 1);
					int x = min(max(cx + dx, 0), grayWarped.cols - 1);

					if (grayWarped.at<uchar>(y, x) < 128) blackCount++;
					else whiteCount++;
				}
			}
			grid.at<uchar>(i, j) = (blackCount > whiteCount) ? 1 : 0;
		}
	}

	int cellViz = 10;
	Mat vizGrid(N * cellViz, N * cellViz, CV_8UC3, Scalar(200, 200, 200));
	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++) {
			Scalar color = (grid.at<uchar>(i, j) == 1) ? Scalar(0, 0, 0) : Scalar(255, 255, 255);
			rectangle(vizGrid, Point(j * cellViz, i * cellViz),
				Point((j + 1) * cellViz - 1, (i + 1) * cellViz - 1), color, FILLED);
		}

	imshow("Grila Module QR (Aliniament Perfect)", vizGrid);
	return grid;
}

int main()
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_FATAL);
	projectPath = _wgetcwd(0, 0);

	system("cls");
	destroyAllWindows();

	printf("==================================================\n");
	printf("       PROIECT: DECODARE COD QR (PAS CU PAS)\n");
	printf("==================================================\n\n");

	printf("-> Selecteaza imaginea din fereastra...\n");

	Mat ogImg = openImage();
	int maxDim = 600;

	// Redimensionare preventiva pentru vizualizare ok
	if (ogImg.cols > maxDim || ogImg.rows > maxDim) {
		double factor = (double)maxDim / max(ogImg.cols, ogImg.rows);
		Mat resizedImg;
		resize(ogImg, resizedImg, Size(), factor, factor, INTER_LINEAR);
		ogImg = resizedImg;
	}
	printf("=> Imaginea a fost incarcata! Apasa o tasta pe fereastra pentru a incepe...\n");
	waitKey();


	// --- PASUL 1: GRAYSCALE ---
	printf("\n[Pasul 1] Conversie la Grayscale...\n");
	Mat greyImg = image2Gray(ogImg);
	waitKey();


	// --- PASUL 2: BINARIZARE ---
	printf("\n[Pasul 2] Binarizare (Calcul automat al pragului)...\n");
	Mat binImg = binarizare(greyImg);
	waitKey();


	// --- PASUL 3: ELIMINARE ZGOMOT ---
	printf("\n[Pasul 3] Eliminare zgomot (Filtru Median / Salt & Pepper)...\n");
	Mat cleanImg = clearSaltAndPepper(binImg);
	waitKey();


	// --- PASUL 4: DETECTARE MARKERI (FINDER PATTERNS) ---
	printf("\n[Pasul 4] Detectare centre si colturi pentru Finder Patterns...\n");
	std::vector<Point2f> corners;
	Mat cornerImg = detectFinderPatternsAndColor(cleanImg, corners);
	waitKey();


	// --- PASUL 5: CORECTIE PERSPECTIVA ---
	printf("\n[Pasul 5] Corectie Perspectiva (Filtru Morfologic + Homografie)...\n");
	int version = 1;
	Mat warped = applyAffineCorrection(cleanImg, corners, version);
	waitKey();


	// --- PASUL 6: EXTRAGERE GRILA ---
	printf("\n[Pasul 6] Extragerea grilei logice (Matrice de biti)...\n");
	int moduleSize = 16; // Setat conform noilor dimensiuni din applyAffineCorrection
	Mat moduleGrid = sampleModuleGrid(warped, version, moduleSize);
	waitKey();


	// --- PASUL 7: RECONSTRUCTIE QR SI DECODARE ---
	printf("\n[Pasul 7] Generare QR Sintetic si Citire Date...\n");

	int N = moduleGrid.rows;
	Mat syntheticQR(N, N, CV_8UC1);

	for (int i = 0; i < N; i++) {
		for (int j = 0; j < N; j++) {
			// 1 devine negru (0), 0 devine alb (255)
			syntheticQR.at<uchar>(i, j) = (moduleGrid.at<uchar>(i, j) == 1) ? 0 : 255;
		}
	}

	Mat scaledQR, perfectQR;
	resize(syntheticQR, scaledQR, Size(), 10.0, 10.0, INTER_NEAREST);
	copyMakeBorder(scaledQR, perfectQR, 40, 40, 40, 40, BORDER_CONSTANT, Scalar(255));

	imshow("QR Sintetic Generat", perfectQR);
	waitKey();

	// Incercam decodarea initiala pe grila extrasa
	cv::QRCodeDetector qrDecoder;
	std::string decodedText = qrDecoder.detectAndDecode(perfectQR);

	if (!decodedText.empty()) {
		printf("\n==================================================\n");
		printf(" SUCCES! Matricea sintetica a fost decodata.\n");
		printf(" Link gasit: %s\n", decodedText.c_str());
		printf("==================================================\n\n");
	}
	else {
		printf(" => Eroare de aliniament matematic pe matricea sintetica.\n");
		printf(" => Aplicam Fallback: Decodare din imaginea pre-procesata de algoritm...\n");

		// Daca geometria generata manual rateaza biti, folosim imaginea Binarizata si Curatata de tine
		decodedText = qrDecoder.detectAndDecode(cleanImg);

		if (!decodedText.empty()) {
			printf("\n==================================================\n");
			printf(" SUCCES (Fallback)! Imaginea curatata de zgomot a fost decodata perfect.\n");
			printf(" Link gasit: %s\n", decodedText.c_str());
			printf("==================================================\n\n");
		}
		else {
			printf("\n[EROARE SUPREMA] Niciuna din metode nu a putut citi codul. Este prea distorsionat.\n");
		}
	}

	// Deschidem browser-ul daca s-a gasit ceva
	if (!decodedText.empty()) {
		printf("Deschid browser-ul...\n");

		std::string finalUrl = decodedText;
		if (finalUrl.find("http://") != 0 && finalUrl.find("https://") != 0) {
			finalUrl = "https://" + finalUrl;
		}

		std::string command = "start \"\" \"" + finalUrl + "\"";
		system(command.c_str());
	}

	printf("\nProiect finalizat! Apasa o tasta pentru a inchide...\n");
	waitKey(0);
	return 0;
}
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

void testNegativeImageFast()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		Mat dst = src.clone();

		double t = (double)getTickCount(); // Get the current time [s]

		// The fastest approach of accessing the pixels -> using pointers
		uchar *lpSrc = src.data;
		uchar *lpDst = dst.data;
		int w = (int) src.step; // no dword alignment is done !!!
		for (int i = 0; i<height; i++)
			for (int j = 0; j < width; j++) {
				uchar val = lpSrc[i*w + j];
				lpDst[i*w + j] = 255 - val;
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



void testBGR2HSV()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname);
		int height = src.rows;
		int width = src.cols;

		// HSV components
		Mat H = Mat(height, width, CV_8UC1);
		Mat S = Mat(height, width, CV_8UC1);
		Mat V = Mat(height, width, CV_8UC1);

		// Defining pointers to each matrix (8 bits/pixels) of the individual components H, S, V 
		uchar* lpH = H.data;
		uchar* lpS = S.data;
		uchar* lpV = V.data;

		Mat hsvImg;
		cvtColor(src, hsvImg, COLOR_BGR2HSV);

		// Defining the pointer to the HSV image matrix (24 bits/pixel)
		uchar* hsvDataPtr = hsvImg.data;

		for (int i = 0; i<height; i++)
		{
			for (int j = 0; j<width; j++)
			{
				int hi = i*width * 3 + j * 3;
				int gi = i*width + j;

				lpH[gi] = hsvDataPtr[hi] * 510 / 360;	// lpH = 0 .. 255
				lpS[gi] = hsvDataPtr[hi + 1];			// lpS = 0 .. 255
				lpV[gi] = hsvDataPtr[hi + 2];			// lpV = 0 .. 255
			}
		}

		imshow("input image", src);
		imshow("H", H);
		imshow("S", S);
		imshow("V", V);

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

//parat de culoare neagra de 50x50 undeva pe o imagine
void testBlackSquare()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		//masoara timpul de procesare a imaginii
		double t = (double)getTickCount(); // Get the current time [s]

		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		Mat dst = Mat(height, width, CV_8UC1);
		// CV_8UC1 -> Accessing individual pixels in an 8 bits/pixel image
		// Inefficient way -> slow
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				if (i<50 && j<50)
					dst.at<uchar>(i, j) = 0;
				else {
					uchar val = src.at<uchar>(i, j); //uchar -> tipul de date cum e stocat un pixel in matricea src 
					uchar neg = 255 - val;
					dst.at<uchar>(i, j) = neg;
				}
			}
		}

		// Get the current time again and compute the time difference [s]
		t = ((double)getTickCount() - t) / getTickFrequency();
		// Print (in the console window) the processing time in [ms] 
		printf("Time = %.3f [ms]\n", t * 1000);

		imshow("input image", src);
		imshow("negative and black square image", dst);
		waitKey();
	}
}

//parat de culoare neagra de 50x50 undeva pe o imagine, si un patrat alb
void testBlackSquareAndWhiteSquare()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		//masoara timpul de procesare a imaginii
		double t = (double)getTickCount(); // Get the current time [s]

		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		Mat dst = Mat(height, width, CV_8UC1);
		// CV_8UC1 -> Accessing individual pixels in an 8 bits/pixel image
		// Inefficient way -> slow
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				if (i < 50 && j < 50)
					dst.at<uchar>(i, j) = 0;
				else if (i>= height - 50 && j >= width - 50)
					dst.at<uchar>(i, j) = 255;
				else {
					uchar val = src.at<uchar>(i, j); //uchar -> tipul de date cum e stocat un pixel in matricea src 
					uchar neg = 255 - val;
					dst.at<uchar>(i, j) = neg;
				}
			}
		}

		// Get the current time again and compute the time difference [s]
		t = ((double)getTickCount() - t) / getTickFrequency();
		// Print (in the console window) the processing time in [ms] 
		printf("Time = %.3f [ms]\n", t * 1000);

		imshow("input image", src);
		imshow("negative and black square image", dst);
		waitKey();
	}
}


//destinatie = sursa + 50
void testSourcePlusVal50()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		//masoara timpul de procesare a imaginii
		double t = (double)getTickCount(); // Get the current time [s]

		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		Mat dst = Mat(height, width, CV_8UC1);
		// CV_8UC1 -> Accessing individual pixels in an 8 bits/pixel image
		// Inefficient way -> slow
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				uchar val = src.at<uchar>(i, j); //uchar -> tipul de date cum e stocat un pixel in matricea src 
				uchar newVal = val +50;
				dst.at<uchar>(i, j) = newVal;
			}
		}

		// Get the current time again and compute the time difference [s]
		t = ((double)getTickCount() - t) / getTickFrequency();
		// Print (in the console window) the processing time in [ms] 
		printf("Time = %.3f [ms]\n", t * 1000);

		imshow("input image", src);
		imshow("new image", dst);
		waitKey();
	}
}

void testSourcePlusVal150()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		//masoara timpul de procesare a imaginii
		double t = (double)getTickCount(); // Get the current time [s]

		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		Mat dst = Mat(height, width, CV_8UC1);
		// CV_8UC1 -> Accessing individual pixels in an 8 bits/pixel image
		// Inefficient way -> slow
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				uchar val = src.at<uchar>(i, j); //uchar -> tipul de date cum e stocat un pixel in matricea src 
				uchar newVal = val + 150;
				dst.at<uchar>(i, j) = newVal;
			}
		}

		// Get the current time again and compute the time difference [s]
		t = ((double)getTickCount() - t) / getTickFrequency();
		// Print (in the console window) the processing time in [ms] 
		printf("Time = %.3f [ms]\n", t * 1000);

		imshow("input image", src);
		imshow("new image", dst);
		waitKey();
	}
}

void testSourceMinusVal50()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		//masoara timpul de procesare a imaginii
		double t = (double)getTickCount(); // Get the current time [s]

		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		Mat dst = Mat(height, width, CV_8UC1);
		// CV_8UC1 -> Accessing individual pixels in an 8 bits/pixel image
		// Inefficient way -> slow
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				uchar val = src.at<uchar>(i, j); //uchar -> tipul de date cum e stocat un pixel in matricea src 
				uchar newVal = val - 50;
				printf("%d ", newVal);
				dst.at<uchar>(i, j) = newVal;
			}
		}

		// Get the current time again and compute the time difference [s]
		t = ((double)getTickCount() - t) / getTickFrequency();
		// Print (in the console window) the processing time in [ms] 
		printf("Time = %.3f [ms]\n", t * 1000);

		imshow("input image", src);
		imshow("new image", dst);
		waitKey();
	}
}

void testFlip()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		//masoara timpul de procesare a imaginii
		double t = (double)getTickCount(); // Get the current time [s]

		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		Mat dst1 = Mat(height, width, CV_8UC1);
		Mat dst2 = Mat(height, width, CV_8UC1);
		// CV_8UC1 -> Accessing individual pixels in an 8 bits/pixel image
		// Inefficient way -> slow
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < (width +1)/2; j++)
			{
				uchar aux = src.at<uchar>(i, width - j - 1);
				dst1.at<uchar>(i, width - j - 1) = src.at<uchar>(i, j);
				dst1.at<uchar>(i, j) = aux;
				//uchar val = src.at<uchar>(i, j); //uchar -> tipul de date cum e stocat un pixel in matricea src 
				//uchar neg = 255 - val;
				
			}
		}

		for (int i = 0; i < (height + 1) / 2; i++)
		{
			for (int j = 0; j < width; j++)
			{
				uchar aux = src.at<uchar>(height - i - 1, j);
				dst2.at<uchar>(height - i - 1 , j ) = src.at<uchar>(i, j);
				dst2.at<uchar>(i, j) = aux;
				//uchar val = src.at<uchar>(i, j); //uchar -> tipul de date cum e stocat un pixel in matricea src 
				//uchar neg = 255 - val;

			}
		}

		// Get the current time again and compute the time difference [s]
		t = ((double)getTickCount() - t) / getTickFrequency();
		// Print (in the console window) the processing time in [ms] 
		printf("Time = %.3f [ms]\n", t * 1000);

		imshow("input image", src);
		imshow("x-flip", dst1);
		imshow("y-flip", dst2);
		waitKey();
	}
}


void testColourImageAndSquare()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		//masoara timpul de procesare a imaginii
		double t = (double)getTickCount(); // Get the current time [s]

		Mat src = imread(fname, IMREAD_COLOR);
		int height = src.rows;
		int width = src.cols;
		Mat dst = Mat(height, width, CV_8UC3);
		// CV_8UC3 -> Accessing individual pixels in an 8 bits/pixel image
		// Inefficient way -> slow
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				if(i<70 && j<70)
					dst.at<Vec3b>(i, j) = Vec3b(0, 255, 0);
				else {
					//uchar val =  //uchar -> tipul de date cum e stocat un pixel in matricea src 
					dst.at<Vec3b>(i, j) = src.at<Vec3b>(i, j);
				}
			}
		}

		// Get the current time again and compute the time difference [s]
		t = ((double)getTickCount() - t) / getTickFrequency();
		// Print (in the console window) the processing time in [ms] 
		printf("Time = %.3f [ms]\n", t * 1000);

		imshow("input image", src);
		imshow("color image and square", dst);
		waitKey();
	}
}

void testColourImageAndPlus50AtGreenCanal()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		//masoara timpul de procesare a imaginii
		double t = (double)getTickCount(); // Get the current time [s]

		Mat src = imread(fname, IMREAD_COLOR);
		int height = src.rows;
		int width = src.cols;
		Mat dst = Mat(height, width, CV_8UC3);
		// CV_8UC3 -> Accessing individual pixels in an 8 bits/pixel image
		// Inefficient way -> slow
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				Vec3b val = src.at<Vec3b>(i, j); 
				val[1] = val[1] + 50;
				dst.at<Vec3b>(i, j) = val;
			}
		}

		// Get the current time again and compute the time difference [s]
		t = ((double)getTickCount() - t) / getTickFrequency();
		// Print (in the console window) the processing time in [ms] 
		printf("Time = %.3f [ms]\n", t * 1000);

		imshow("input image", src);
		imshow("color image and val green + 50", dst);
		waitKey();
	}
}

void testColourImageAndKeepGreenCanal()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		//masoara timpul de procesare a imaginii
		double t = (double)getTickCount(); // Get the current time [s]

		Mat src = imread(fname, IMREAD_COLOR);
		int height = src.rows;
		int width = src.cols;
		Mat dst = Mat(height, width, CV_8UC3);
		// CV_8UC3 -> Accessing individual pixels in an 8 bits/pixel image
		// Inefficient way -> slow
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				Vec3b val = src.at<Vec3b>(i, j);
				val[0] = 0;
				val[1] = val[1];
				val[2] = 0;
				dst.at<Vec3b>(i, j) = val;
			}
		}

		// Get the current time again and compute the time difference [s]
		t = ((double)getTickCount() - t) / getTickFrequency();
		// Print (in the console window) the processing time in [ms] 
		printf("Time = %.3f [ms]\n", t * 1000);

		imshow("input image", src);
		imshow("color image and val green + 50", dst);
		waitKey();
	}
}

void testColourImageAndGreyscaleOn3Channels()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		//masoara timpul de procesare a imaginii
		double t = (double)getTickCount(); // Get the current time [s]

		Mat src = imread(fname, IMREAD_COLOR);
		int height = src.rows;
		int width = src.cols;
		Mat dstB = Mat(height, width, CV_8UC1);
		Mat dstG = Mat(height, width, CV_8UC1);
		Mat dstR = Mat(height, width, CV_8UC1);
		// CV_8UC3 -> Accessing individual pixels in an 8 bits/pixel image
		// Inefficient way -> slow
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				Vec3b val = src.at<Vec3b>(i, j);
				dstR.at<uchar>(i, j) = val[0];
				dstG.at<uchar>(i, j) = val[1];
				dstB.at<uchar>(i, j) = val[2];
			}
		}

		// Get the current time again and compute the time difference [s]
		t = ((double)getTickCount() - t) / getTickFrequency();
		// Print (in the console window) the processing time in [ms] 
		printf("Time = %.3f [ms]\n", t * 1000);

		imshow("input image", src);
		imshow("color image RED", dstR);
		imshow("color image GREEN", dstG);
		imshow("color image BLUE", dstB);
		waitKey();
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

int calcArea(Mat src) {
	int height = src.rows;
	int width = src.cols;

	int area = 0;
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
			area += (src.at<uchar>(i, j) == 0 ? 1 : 0); 

	return area;
}

void calcAreaWrapper() {
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_GRAYSCALE);

		imshow("input image", src);
		printf("The area of the object is %d pixels\n", calcArea(src));
		waitKey();
	}
}


void calcCentrulDeMasa(Mat src, int* r, int* c, int area) {
	int height = src.rows;
	int width = src.cols;
	
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++) {
			*r += (src.at<uchar>(i, j) == 0 ? i : 0);
			*c += (src.at<uchar>(i, j) == 0 ? j : 0);
		}

	*r /= area;
	*c /= area;
}

void calcCentrulDeMasaWrapper() {
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		int r = 0, c = 0;
		int area = calcArea(src);
		calcCentrulDeMasa(src, &r, &c, area);

		imshow("input image", src);
		printf("The center of the object is at coordinates (%d, %d)\n", r , c );
		waitKey();
	}
}

void calcAxaAlungire() {
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		int r = 0, c = 0;
		int area = calcArea(src);
		calcCentrulDeMasa(src, &r, &c, area);

		double s = 0, s1 = 0, s2 = 0;

		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++) {
				s += (src.at<uchar>(i, j) == 0 ? (i - r) * (j - c) : 0);
				s1 += (src.at<uchar>(i, j) == 0 ? (j - c) * (j - c) : 0);
				s2 += (src.at<uchar>(i, j) == 0 ? (i - r) * (i - r) : 0);
			}

		imshow("input image", src);
		printf("Axa de alungire la %f grade\n", 90*atan2(2*s, s1-s2)/PI);
		waitKey();
	}
}

int calcPerimeter(Mat src, int height, int width) {
	int p = 0;

	for (int i = 1; i < height - 1; i++)
		for (int j = 1; j < width - 1; j++) {
			if (src.at<uchar>(i, j) == 0 &&
				(src.at<uchar>(i - 1, j) != 0 ||
					src.at<uchar>(i, j + 1) != 0 ||
					src.at<uchar>(i + 1, j) != 0 ||
					src.at<uchar>(i, j - 1) != 0)) p++;
		}

	return p;
}

void calcPerimeterWrapper() {
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		
		imshow("input image", src);
		printf("Perimeter is %d\n", calcPerimeter(src, height, width));
		waitKey();
	}
}


void calcThinnessRatio() {
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;

		imshow("input image", src);
		printf("Thinness ratio is %f\n", 4*PI*((float)calcArea(src)/pow(calcPerimeter(src,height,width), 2)));
		waitKey();
	}
}

void calcAspectRatio() {
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;
		int rmin = height;
		int rmax = 0;
		int cmin = width;
		int cmax = 0;

		Mat dst = Mat(height, width, CV_8UC3);

		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++) {
				dst.at<Vec3b>(i, j) = Vec3b(src.at<uchar>(i, j), src.at<uchar>(i, j), src.at<uchar>(i, j));

				if (src.at<uchar>(i, j) == 0 && i < rmin) rmin = i;
				if (src.at<uchar>(i, j) == 0 && i > rmax) rmax = i;
				if (src.at<uchar>(i, j) == 0 && j > cmax) cmax = j;
				if (src.at<uchar>(i, j) == 0 && j < cmin) cmin = j;
			}


		for (int i = rmin; i <= rmax;i++) {
			dst.at<Vec3b>(i, cmin) = Vec3b(0, 0, 255);
			dst.at<Vec3b>(i, cmax) = Vec3b(0, 0, 255);
		}
			
		for (int j = cmin;j <= cmax;j++) {
			dst.at<Vec3b>(rmin, j) = Vec3b(0, 0, 255);
			dst.at<Vec3b>(rmax, j) = Vec3b(0, 0, 255);
		}
				

		imshow("input image", src);
		imshow("output image", dst);

		printf("Aspect ratio is %f\n", (float)(cmax-cmin+1)/(float)(rmax-rmin+1));
		waitKey();
	}
}

void projections() {
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src = imread(fname, IMREAD_GRAYSCALE);
		int height = src.rows;
		int width = src.cols;

		Mat dstX = Mat(height, width, CV_8UC1);
		Mat dstY = Mat(height, width, CV_8UC1);

		int last = 0;

		for (int i = 0; i < height; i++) {
			last = 0;
			for (int j = 0; j < width; j++)
				if (src.at<uchar>(i, j) == 0)
				{
					dstY.at<uchar>(i, last) = src.at<uchar>(i, j);
					last++;
				} 
		}

		//last = height;
		for (int j = 0; j < width; j++) {
			last = height - 1;
			for (int i = 0; i < height; i++)
				if (src.at<uchar>(i, j) == 0)
				{
					dstX.at<uchar>(last, j) = src.at<uchar>(i, j);
					last--;
				}
		}
			

		imshow("input image", src);
		imshow("On Y axis", dstY);
		imshow("On X axis", dstX);

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


Mat detectFinderPatternsAndColor(Mat binImg) {
	int height = binImg.rows;
	int width = binImg.cols;
	Mat dst;
	cvtColor(binImg, dst, COLOR_GRAY2BGR);

	struct Candidate {
		Point pos;
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
					float maxErr = unit * 0.5f;

					// verifica proportia pe orizontala
					if (abs(counter[0] - unit) < maxErr && abs(counter[1] - unit) < maxErr &&
						abs(counter[2] - unit * 3) < maxErr * 3 && abs(counter[3] - unit) < maxErr &&
						abs(counter[4] - unit) < maxErr) {

						// calculeaza centrul pe orizontala
						int centerX = j - counter[4] - counter[3] - counter[2] / 2;

						// realizeaza centrarea pe verticala
						int up = i, down = i;
						while (isInside(binImg, up - 1, centerX) && binImg.at<uchar>(up - 1, centerX) == 0) up--;
						while (isInside(binImg, down + 1, centerX) && binImg.at<uchar>(down + 1, centerX) == 0) down++;
						int correctedCenterY = (up + down) / 2;

						// verifica proportia pe verticala
						int vCounter[5] = { 0, 0, 0, 0, 0 };
						int y = correctedCenterY;

						// numara pixelii in sus
						while (y >= 0 && binImg.at<uchar>(y, centerX) == 0) { vCounter[2]++; y--; }
						while (y >= 0 && binImg.at<uchar>(y, centerX) != 0) { vCounter[1]++; y--; }
						while (y >= 0 && binImg.at<uchar>(y, centerX) == 0) { vCounter[0]++; y--; }

						// numara pixelii in jos
						y = correctedCenterY + 1;
						while (y < height && binImg.at<uchar>(y, centerX) == 0) { vCounter[2]++; y++; }
						while (y < height && binImg.at<uchar>(y, centerX) != 0) { vCounter[3]++; y++; }
						while (y < height && binImg.at<uchar>(y, centerX) == 0) { vCounter[4]++; y++; }

						int vTotal = vCounter[0] + vCounter[1] + vCounter[2] + vCounter[3] + vCounter[4];
						float vUnit = (float)vTotal / 7.0f;
						float vMaxErr = vUnit * 0.5f;

						// accepta candidatul daca proportia 1:1:3:1:1 este valida si pe verticala
						if (abs(vCounter[0] - vUnit) < vMaxErr && abs(vCounter[1] - vUnit) < vMaxErr &&
							abs(vCounter[2] - vUnit * 3) < vMaxErr * 3 && abs(vCounter[3] - vUnit) < vMaxErr &&
							abs(vCounter[4] - vUnit) < vMaxErr &&
							abs(unit - vUnit) < unit * 0.5f) { // verifica daca dimensiunea verticala este similara cu cea orizontala

							// evita inregistrarea multipla a aceluiasi punct
							bool duplicate = false;
							for (const auto& c : candidates) {
								if (norm(c.pos - Point(centerX, correctedCenterY)) < unit * 3) duplicate = true;
							}
							if (!duplicate) {
								candidates.push_back({ Point(centerX, correctedCenterY), (unit + vUnit) / 2.0f });
							}
						}
					}
					// reseteaza contoarele pentru continuarea scanarii
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

	// filtrare geometrica: cauta 3 puncte care formeaza un triunghi dreptunghic isoscel
	std::vector<Candidate> validQR;
	if (candidates.size() >= 3) {
		for (size_t i = 0; i < candidates.size(); i++) {
			for (size_t j = i + 1; j < candidates.size(); j++) {
				for (size_t k = j + 1; k < candidates.size(); k++) {

					// calculeaza distantele dintre cele 3 puncte
					float d1 = norm(candidates[i].pos - candidates[j].pos);
					float d2 = norm(candidates[i].pos - candidates[k].pos);
					float d3 = norm(candidates[j].pos - candidates[k].pos);

					// sorteaza distantele pentru a gasi ipotenuza
					float dists[3] = { d1, d2, d3 };
					std::sort(dists, dists + 3);

					float cateta1 = dists[0];
					float cateta2 = dists[1];
					float ipotenuza = dists[2];

					// verifica daca laturile mici sunt aproximativ egale in lungime
					if (abs(cateta1 - cateta2) < 0.3 * cateta2) {

						// aplica teorema lui pitagora
						float pitagora = cateta1 * cateta1 + cateta2 * cateta2;
						float c2 = ipotenuza * ipotenuza;

						// accepta grupul daca respecta teorema cu o toleranta de perspectiva
						if (abs(pitagora - c2) < 0.3 * c2) {
							validQR.push_back(candidates[i]);
							validQR.push_back(candidates[j]);
							validQR.push_back(candidates[k]);
							break;
						}
					}
				}
				if (!validQR.empty()) break;
			}
			if (!validQR.empty()) break;
		}
	}

	candidates = validQR;

	if (candidates.size() == 3) {
		for (const auto& c : candidates) {
			circle(dst, c.pos, (int)(c.unit * 2), Scalar(0, 0, 255), 3);
		}
		printf("Detectate corect cele 3 puncte principale.\n");
		imshow("DST: Puncte de Control Filtrate", dst);
	}
	else {
		printf("Nu a fost detectat niciun cod QR valid in imagine.\n");
		imshow("DST: Puncte de Control Filtrate", dst);
	}

	return dst;
}

int main() 
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_FATAL);
    projectPath = _wgetcwd(0, 0);

	system("cls");
	destroyAllWindows();
	printf("Selecteaza imaginea:\n");

	Mat ogImg = openImage();
	printf("Imaginea a fost incarcata! Apasa o tasta...\n");
	waitKey();

	Mat greyImg = image2Gray(ogImg);
	printf("Step 1: Conversie la grayscale realizata! Apasa o tasta...\n");
	waitKey();

	printf("Step 2: Binarizare realizata! Apasa o tasta...\n");
	Mat binImg = binarizare(greyImg);
	waitKey();


	printf("Step 3: Detectare colturi realizata! Apasa o tasta...\n");
	Mat cornerImg = detectFinderPatternsAndColor(binImg);
	waitKey();
		

	
	return 0;
}
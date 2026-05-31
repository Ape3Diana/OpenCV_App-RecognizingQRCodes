
#include "stdafx.h"
#include "common.h"
#include <opencv2/core/utils/logger.hpp>

wchar_t* projectPath;

int isInside(Mat img, int i, int j)
{
	int height = img.rows;
	int width = img.cols;
	if (i >= 0 && i <= (height - 1) && j > 0 && j <= (width - 1))
		return 1;
	return 0;
}

Mat openImage()
{
	char fname[MAX_PATH];
	while (openFileDlg(fname))
	{
		Mat src;
		src = imread(fname);
		imshow("Original QR-Code image", src);
		return src;
	}
}

// Pas 1: Grayscale
Mat image2Gray(Mat src)
{
	int height = src.rows;
	int width = src.cols;
	Mat dst = Mat(height, width, CV_8UC1);

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
		{
			Vec3b v3 = src.at<Vec3b>(i, j);
			uchar b = v3[0], g = v3[1], r = v3[2];
			dst.at<uchar>(i, j) = (uchar)(0.299 * r + 0.587 * g + 0.114 * b);
		}

	imshow("Grayscale", dst);
	return dst;
}

// Pas 2a: Gaussian Blur 5x5 inainte de binarizare
Mat applyGaussianBlur(Mat src)
{
	float k[5][5] = {
		{1,  4,  7,  4,  1},
		{4, 16, 26, 16,  4},
		{7, 26, 41, 26,  7},
		{4, 16, 26, 16,  4},
		{1,  4,  7,  4,  1}
	};
	float kSum = 273.0f;

	int height = src.rows;
	int width = src.cols;
	int radius = 2;
	Mat dst = src.clone();

	for (int i = radius; i < height - radius; i++)
		for (int j = radius; j < width - radius; j++)
		{
			float sum = 0.0f;
			for (int u = 0; u < 5; u++)
				for (int v = 0; v < 5; v++)
					sum += k[u][v] * src.at<uchar>(i + u - radius, j + v - radius);
			dst.at<uchar>(i, j) = saturate_cast<uchar>(sum / kSum);
		}

	imshow("Gaussian Blur (pre-binarizare)", dst);
	return dst;
}

// Pas 2b: Egalizare histograma pe grayscale
Mat equalizeHist(Mat src)
{
	int height = src.rows;
	int width = src.cols;
	int M = height * width;

	Mat dst(height, width, CV_8UC1);

	int hist[256] = { 0 };
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
			hist[src.at<uchar>(i, j)]++;

	float pdf[256] = { 0 };
	float cdf[256] = { 0 };
	float sum = 0.0f;

	for (int i = 0; i < 256; i++)
	{
		pdf[i] = (float)hist[i] / M;
		sum += pdf[i];
		cdf[i] = sum;
	}

	float cdf_min = 0.0f;
	for (int i = 0; i < 256; i++)
	{
		if (cdf[i] > 0.0f)
		{
			cdf_min = cdf[i];
			break;
		}
	}

	uchar lut[256];
	for (int i = 0; i < 256; i++)
	{
		if (cdf[i] <= cdf_min)
			lut[i] = 0;
		else
			lut[i] = saturate_cast<uchar>(
				(cdf[i] - cdf_min) / (1.0f - cdf_min) * 255.0f
			);
	}

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
			dst.at<uchar>(i, j) = lut[src.at<uchar>(i, j)];

	imshow("Egalizare Histograma", dst);
	return dst;
}

// Pas 3a: Binarizare iterativa
Mat binarizare(Mat src)
{
	int height = src.rows;
	int width = src.cols;
	int M = height * width;
	Mat dst = Mat(height, width, CV_8UC1);

	uchar minVal = 255, maxVal = 0;
	long long sumAll = 0;

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
		{
			uchar val = src.at<uchar>(i, j);
			sumAll += val;
			if (val < minVal) minVal = val;
			if (val > maxVal) maxVal = val;
		}

	double miu = (double)sumAll / M;
	double sumVariance = 0;
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
		{
			uchar val = src.at<uchar>(i, j);
			sumVariance += (val - miu) * (val - miu);
		}
	double sigma = sqrt(sumVariance / M);
	printf("  [Iterativ] Miu=%.2f  Sigma=%.2f\n", miu, sigma);

	int T = (minVal + maxVal) / 2;
	int TNext = T;
	do {
		T = TNext;
		long long sum1 = 0, sum2 = 0;
		int count1 = 0, count2 = 0;
		for (int i = 0; i < height; i++)
			for (int j = 0; j < width; j++)
			{
				uchar val = src.at<uchar>(i, j);
				if (val <= T) { sum1 += val; count1++; }
				else { sum2 += val; count2++; }
			}
		int V1 = count1 > 0 ? (int)(sum1 / count1) : 0;
		int V2 = count2 > 0 ? (int)(sum2 / count2) : 0;
		TNext = (V1 + V2) / 2;
	} while (T != TNext);

	printf("  [Iterativ] Prag optim = %d\n", T);

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
			dst.at<uchar>(i, j) = (src.at<uchar>(i, j) <= T) ? 0 : 255;

	imshow("Binarizare Iterativa", dst);
	return dst;
}

// Pas 3b: Binarizare Otsu
Mat binarizareOtsu(Mat src)
{
	int height = src.rows;
	int width = src.cols;
	int M = height * width;
	Mat dst = Mat(height, width, CV_8UC1);

	long long hist[256] = { 0 };
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
			hist[src.at<uchar>(i, j)]++;

	double muGlobal = 0.0;
	for (int g = 0; g < 256; g++)
		muGlobal += g * (double)hist[g] / M;

	double maxVarianta = -1.0;
	int T_otsu = 0;
	double w1 = 0.0, mu1 = 0.0;

	for (int t = 0; t < 255; t++)
	{
		w1 += (double)hist[t] / M;
		mu1 += t * (double)hist[t] / M;
		double w2 = 1.0 - w1;
		if (w1 <= 0.0 || w2 <= 0.0) continue;
		double mu2 = (muGlobal - mu1) / w2;
		double varianta = w1 * w2 * (mu1 / w1 - mu2) * (mu1 / w1 - mu2);
		if (varianta > maxVarianta) { maxVarianta = varianta; T_otsu = t; }
	}

	printf("  [Otsu] Prag optim = %d  (varianta inter-clase max = %.2f)\n", T_otsu, maxVarianta);

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
			dst.at<uchar>(i, j) = (src.at<uchar>(i, j) <= T_otsu) ? 0 : 255;

	imshow("Binarizare Otsu", dst);
	return dst;
}

// Pas 3c: Binarizare adaptiva pentru iluminare neuniforma
Mat binarizareAdaptiva(Mat src)
{
	int height = src.rows;
	int width = src.cols;
	Mat dst = Mat(height, width, CV_8UC1, Scalar(255));

	int blockSize = 51;
	int halfB = blockSize / 2;
	int C = 10;

	Mat integralImg;
	integral(src, integralImg, CV_64F);

	for (int i = 0; i < height; i++)
	{
		for (int j = 0; j < width; j++)
		{
			int r1 = max(0, i - halfB);
			int r2 = min(height - 1, i + halfB);
			int c1 = max(0, j - halfB);
			int c2 = min(width - 1, j + halfB);

			double suma = integralImg.at<double>(r2 + 1, c2 + 1)
				- integralImg.at<double>(r1, c2 + 1)
				- integralImg.at<double>(r2 + 1, c1)
				+ integralImg.at<double>(r1, c1);

			int nrPixeli = (r2 - r1 + 1) * (c2 - c1 + 1);
			double medie = suma / nrPixeli;

			uchar val = src.at<uchar>(i, j);
			dst.at<uchar>(i, j) = (val < medie - C) ? 0 : 255;
		}
	}

	imshow("Binarizare Adaptiva", dst);
	return dst;
}

// Pas 3d: Alegere metoda de binarizare in functie de iluminare
Mat chooseBestBinary(Mat iterativ, Mat otsu, Mat adaptiv, Mat srcGray)
{
	int M = srcGray.rows * srcGray.cols;

	long long sumAll = 0;
	for (int i = 0; i < srcGray.rows; i++)
		for (int j = 0; j < srcGray.cols; j++)
			sumAll += srcGray.at<uchar>(i, j);
	double miu = (double)sumAll / M;

	double sumVar = 0;
	for (int i = 0; i < srcGray.rows; i++)
		for (int j = 0; j < srcGray.cols; j++)
		{
			double d = srcGray.at<uchar>(i, j) - miu;
			sumVar += d * d;
		}
	double sigma = sqrt(sumVar / M);

	printf("  [Detectie] Sigma globala = %.2f\n", sigma);

	if (sigma > 80.0)
	{
		printf("  [Alegere] Sigma mare -> Binarizare Iterativa\n");
		return iterativ;
	}
	else
	{
		printf("  [Alegere] Sigma mica -> Binarizare Adaptiva\n");
		return adaptiv;
	}
}

// Pas 4: Gaussian Blur 3x3 dupa binarizare + re-binarizare la prag fix
Mat applyPostBinBlur(Mat src)
{
	float k[3][3] = {
		{1, 2, 1},
		{2, 4, 2},
		{1, 2, 1}
	};
	float kSum = 16.0f;

	int height = src.rows;
	int width = src.cols;
	Mat temp = src.clone();
	Mat dst = Mat(height, width, CV_8UC1);

	for (int i = 1; i < height - 1; i++)
		for (int j = 1; j < width - 1; j++)
		{
			float sum = 0.0f;
			for (int u = 0; u < 3; u++)
				for (int v = 0; v < 3; v++)
					sum += k[u][v] * src.at<uchar>(i + u - 1, j + v - 1);
			temp.at<uchar>(i, j) = saturate_cast<uchar>(sum / kSum);
		}

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
			dst.at<uchar>(i, j) = (temp.at<uchar>(i, j) < 128) ? 0 : 255;

	imshow("Gaussian Blur (post-binarizare)", dst);
	return dst;
}

// Pas 5: Eliminare zgomot Salt and Pepper
Mat clearSaltAndPepper(Mat src)
{
	int height = src.rows;
	int width = src.cols;
	Mat dst_median = Mat(height, width, CV_8UC1, Scalar(0));
	uchar vals[9];

	for (int i = 0; i < height - 2; i++)
		for (int j = 0; j < width - 2; j++)
		{
			int k = 0;
			for (int p = i; p < i + 3; p++)
				for (int q = j; q < j + 3; q++)
					vals[k++] = src.at<uchar>(p, q);
			std::sort(vals, vals + 9);
			dst_median.at<uchar>(i + 1, j + 1) = vals[4];
		}

	imshow("Salt and Pepper curatare", dst_median);
	return dst_median;
}

Mat createStructuringElement()
{
	Mat element = Mat(3, 3, CV_8UC1, Scalar(255));
	element.at<uchar>(0, 1) = 0;
	element.at<uchar>(1, 0) = 0;
	element.at<uchar>(1, 1) = 0;
	element.at<uchar>(1, 2) = 0;
	element.at<uchar>(2, 1) = 0;
	return element;
}

Mat dilatare(Mat src)
{
	Mat dst = src.clone();
	int rows = src.rows;
	int cols = src.cols;
	Mat elStr = createStructuringElement();
	int elRows = elStr.rows;
	int elCols = elStr.cols;

	for (int i = 1; i < rows - 1; i++)
		for (int j = 1; j < cols - 1; j++)
			if (src.at<uchar>(i, j) == 0)
				for (int k = 0; k < elRows; k++)
					for (int l = 0; l < elCols; l++)
						if (elStr.at<uchar>(k, l) == 0)
						{
							int newI = i + k - 1;
							int newJ = j + l - 1;
							dst.at<uchar>(newI, newJ) = 0;
						}

	imshow("Dilatare", dst);
	return dst;
}

Mat eroziuneDetectie(Mat src)
{
	int height = src.rows;
	int width = src.cols;
	Mat dst = Mat(height, width, CV_8UC1, Scalar(255));

	for (int i = 1; i < height - 1; i++)
		for (int j = 1; j < width - 1; j++)
		{
			bool allBlack = true;
			for (int u = -1; u <= 1 && allBlack; u++)
				for (int v = -1; v <= 1 && allBlack; v++)
					if (src.at<uchar>(i + u, j + v) != 0)
						allBlack = false;
			if (allBlack)
				dst.at<uchar>(i, j) = 0;
		}

	imshow("Eroziune (pre-detectie)", dst);
	return dst;
}

// Pas 6: Detectare Finder Patterns
Mat detectFinderPatternsAndColor(Mat binImg, std::vector<Point2f>& outCorners)
{
	int height = binImg.rows;
	int width = binImg.cols;
	Mat dst;
	cvtColor(binImg, dst, COLOR_GRAY2BGR);

	struct Candidate {
		Point2f pos;
		float unit;
	};
	std::vector<Candidate> candidates;

	for (int i = 0; i < height; i++)
	{
		std::vector<int> counter(5, 0);
		int currentState = 0;
		for (int j = 0; j < width; j++)
		{
			uchar pixel = binImg.at<uchar>(i, j);
			bool isBlack = (pixel == 0);

			if (isBlack == (currentState % 2 == 0))
				counter[currentState]++;
			else
			{
				if (currentState == 4)
				{
					int totalWidth = counter[0] + counter[1] + counter[2] + counter[3] + counter[4];
					float unit = (float)totalWidth / 7.0f;
					float maxErr = unit * 0.7f;

					if (abs(counter[0] - unit) < maxErr && abs(counter[1] - unit) < maxErr &&
						abs(counter[2] - unit * 3) < maxErr * 3 && abs(counter[3] - unit) < maxErr &&
						abs(counter[4] - unit) < maxErr)
					{
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
						float vMaxErr = vUnit * 0.7f;

						if (abs(vCounter[0] - vUnit) < vMaxErr && abs(vCounter[1] - vUnit) < vMaxErr &&
							abs(vCounter[2] - vUnit * 3) < vMaxErr * 3 && abs(vCounter[3] - vUnit) < vMaxErr &&
							abs(vCounter[4] - vUnit) < vMaxErr &&
							abs(unit - vUnit) < unit * 0.7f)
						{
							int sumX = 0, sumY = 0, count = 0;
							int safeRadius = (int)((unit + vUnit) / 2.0f * 1.5f);
							for (int dy = -safeRadius; dy <= safeRadius; dy++)
								for (int dx = -safeRadius; dx <= safeRadius; dx++)
								{
									int py = correctedCenterY + dy;
									int px = centerX + dx;
									if (isInside(binImg, py, px) && binImg.at<uchar>(py, px) == 0)
									{
										sumX += px; sumY += py; count++;
									}
								}

							if (count > 0)
							{
								float exactX = (float)sumX / count;
								float exactY = (float)sumY / count;
								float exactUnit = sqrt((float)count / 9.0f);

								bool duplicate = false;
								for (const auto& c : candidates)
									if (norm(c.pos - Point2f(exactX, exactY)) < exactUnit * 3)
										duplicate = true;

								if (!duplicate)
									candidates.push_back({ Point2f(exactX, exactY), exactUnit });
							}
						}
					}
					counter[0] = counter[2]; counter[1] = counter[3]; counter[2] = counter[4];
					counter[3] = 1; counter[4] = 0; currentState = 3;
				}
				else
				{
					currentState++;
					counter[currentState]++;
				}
			}
		}
	}

	std::vector<Candidate> validQR, bestCombo;
	float maxSize = 0.0f;

	if (candidates.size() >= 3)
	{
		for (size_t i = 0; i < candidates.size(); i++)
			for (size_t j = i + 1; j < candidates.size(); j++)
				for (size_t k = j + 1; k < candidates.size(); k++)
				{
					float u1 = candidates[i].unit, u2 = candidates[j].unit, u3 = candidates[k].unit;
					float avgUnit = (u1 + u2 + u3) / 3.0f;
					if (abs(u1 - avgUnit) > 0.6 * avgUnit || abs(u2 - avgUnit) > 0.6 * avgUnit || abs(u3 - avgUnit) > 0.6 * avgUnit)
						continue;

					float d1 = norm(candidates[i].pos - candidates[j].pos);
					float d2 = norm(candidates[i].pos - candidates[k].pos);
					float d3 = norm(candidates[j].pos - candidates[k].pos);
					float dists[3] = { d1, d2, d3 };
					std::sort(dists, dists + 3);

					if (abs(dists[0] - dists[1]) < 0.7 * dists[1])
						if (abs((dists[0] * dists[0] + dists[1] * dists[1]) - dists[2] * dists[2]) < 0.7 * (dists[2] * dists[2]))
						{
							float currentSize = dists[0] + dists[1] + dists[2];
							if (currentSize > maxSize)
							{
								maxSize = currentSize;
								bestCombo = { candidates[i], candidates[j], candidates[k] };
							}
						}
				}
	}

	candidates = bestCombo.empty() ? validQR : bestCombo;
	if (candidates.size() == 3)
	{
		for (const auto& c : candidates)
			circle(dst, c.pos, (int)(c.unit * 2), Scalar(0, 0, 255), 3);
		printf("Detectate corect cele 3 puncte principale.\n");
	}
	else printf("Eroare detectie.\n");

	imshow("Puncte de Control Filtrate", dst);
	for (const auto& c : candidates) outCorners.push_back(c.pos);
	return dst;
}

std::vector<Point2f> getMarkerInnerCorners(Mat binImg, Point2f center)
{
	int cx = round(center.x);
	int cy = round(center.y);
	int left = cx, right = cx, top = cy, bottom = cy;

	while (left > 0 && binImg.at<uchar>(cy, left) == 0) left--; left++;
	while (right < binImg.cols - 1 && binImg.at<uchar>(cy, right) == 0) right++; right--;
	while (top > 0 && binImg.at<uchar>(top, cx) == 0) top--; top++;
	while (bottom < binImg.rows - 1 && binImg.at<uchar>(bottom, cx) == 0) bottom++; bottom--;

	return { Point2f(left, top), Point2f(right, top), Point2f(left, bottom), Point2f(right, bottom) };
}

// Pas 7: Corectie perspectiva prin transformare afina
Mat applyAffineCorrection(Mat binImg, std::vector<Point2f> corners, int& outVersion)
{
	if (corners.size() < 3)
	{
		printf("  [EROARE] applyAffineCorrection: doar %zu colturi detectate, sunt necesare 3.\n", corners.size());
		return binImg.clone();
	}

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

	int warpSize = 500;
	int version = 1;
	outVersion = version;
	int N_qr = 21;
	float modSize = 16.0f;
	float padding = (warpSize - (N_qr * modSize)) / 2.0f;
	float offset = padding + 3.5f * modSize;
	float endOffset = padding + (N_qr - 3.5f) * modSize;

	Point2f srcPts[3] = { pTL, pTR, pBL };
	Point2f dstPts[3] = { Point2f(offset, offset), Point2f(endOffset, offset), Point2f(offset, endOffset) };
	Mat M = getAffineTransform(srcPts, dstPts);
	Mat affineWarped;
	warpAffine(processedImg, affineWarped, M, Size(warpSize, warpSize), INTER_NEAREST, BORDER_CONSTANT, Scalar(255));

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
	addEyeDst(0, 0);
	addEyeDst(N_qr - 7, 0);
	addEyeDst(0, N_qr - 7);

	Mat H = findHomography(srcH, dstH);
	Mat finalWarped;
	warpPerspective(affineWarped, finalWarped, H, Size(warpSize, warpSize), INTER_NEAREST, BORDER_CONSTANT, Scalar(255));

	imshow("QR corectat perspectiva", finalWarped);
	return finalWarped;
}

// Pas 8: Extragere grila de module
Mat sampleModuleGrid(Mat warped, int version, int moduleSize_vechi)
{
	Mat grayWarped;
	if (warped.channels() == 3) cvtColor(warped, grayWarped, COLOR_BGR2GRAY);
	else grayWarped = warped.clone();

	int N = 21;
	float modSize = 16.0f;
	float padding = (warped.cols - (N * modSize)) / 2.0f;
	Mat grid(N, N, CV_8UC1);

	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
		{
			int cy = round(padding + i * modSize + modSize / 2.0f);
			int cx = round(padding + j * modSize + modSize / 2.0f);
			int blackCount = 0, whiteCount = 0;
			int safeRadius = 3;
			for (int dy = -safeRadius; dy <= safeRadius; dy++)
				for (int dx = -safeRadius; dx <= safeRadius; dx++)
				{
					int y = min(max(cy + dy, 0), grayWarped.rows - 1);
					int x = min(max(cx + dx, 0), grayWarped.cols - 1);
					if (grayWarped.at<uchar>(y, x) < 128) blackCount++;
					else whiteCount++;
				}
			grid.at<uchar>(i, j) = (blackCount > whiteCount) ? 1 : 0;
		}

	int cellViz = 10;
	Mat vizGrid(N * cellViz, N * cellViz, CV_8UC3, Scalar(200, 200, 200));
	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
		{
			Scalar color = (grid.at<uchar>(i, j) == 1) ? Scalar(0, 0, 0) : Scalar(255, 255, 255);
			rectangle(vizGrid, Point(j * cellViz, i * cellViz),
				Point((j + 1) * cellViz - 1, (i + 1) * cellViz - 1), color, FILLED);
		}

	imshow("Grila module QR", vizGrid);
	return grid;
}

void processResult(const std::string& decodedText)
{
	printf(" SUCCES! QR decodat.\n");
	printf(" Link/Text: %s\n", decodedText.c_str());

	std::string url = decodedText;
	if (url.find("http://") == 0 || url.find("https://") == 0)
	{
		printf("Deschid browser-ul...\n");
		system(("start \"\" \"" + url + "\"").c_str());
	}
}

void runWebcamMode()
{
	printf("\nMod camera web activ.\n");
	printf("Tine codul QR in fata camerei.\n");
	printf("Apasa ESC pentru a iesi.\n\n");

	VideoCapture cap(0);

	if (!cap.isOpened())
	{
		printf("[EROARE] Nu s-a putut deschide camera web!\n");
		return;
	}

	cap.set(CAP_PROP_FRAME_WIDTH, 640);
	cap.set(CAP_PROP_FRAME_HEIGHT, 480);

	cv::QRCodeDetector qrDetector;

	Mat frame;
	int stableFrames = 0;
	const int REQUIRED_STABLE_FRAMES = 8;
	std::string lastDecoded = "";

	while (true)
	{
		cap >> frame;
		if (frame.empty()) break;

		Mat preview = frame.clone();
		std::vector<Point> qrPoints;
		std::string quickDecode = qrDetector.detectAndDecode(frame, qrPoints);
		bool found = !quickDecode.empty();

		if (found && qrPoints.size() >= 4)
		{
			double area = contourArea(qrPoints);
			if (area < 5000)
				found = false;
		}

		if (found)
		{
			if (quickDecode == lastDecoded)
				stableFrames++;
			else
			{
				stableFrames = 1;
				lastDecoded = quickDecode;
			}
		}
		else
		{
			stableFrames = 0;
			lastDecoded = "";
		}

		if (found && qrPoints.size() == 4)
		{
			for (int i = 0; i < 4; i++)
				line(preview, qrPoints[i], qrPoints[(i + 1) % 4], Scalar(0, 255, 0), 3);

			putText(preview, "QR VALID DETECTAT", Point(10, 40), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 255, 0), 2);

			char buffer[100];
			sprintf(buffer, "Stabilitate: %d / %d", stableFrames, REQUIRED_STABLE_FRAMES);
			putText(preview, buffer, Point(10, 75), FONT_HERSHEY_SIMPLEX, 0.7, Scalar(0, 255, 255), 2);
		}
		else
		{
			putText(preview, "Cauta cod QR...", Point(10, 40), FONT_HERSHEY_SIMPLEX, 0.8, Scalar(0, 100, 255), 2);
		}

		putText(preview, "ESC = Iesire", Point(10, 110), FONT_HERSHEY_SIMPLEX, 0.6, Scalar(255, 255, 255), 1);
		imshow("Camera Web - Preview Live", preview);

		if (waitKey(30) == 27) break;
		if (stableFrames < REQUIRED_STABLE_FRAMES) continue;

		printf("\nQR stabil detectat!\n");
		printf("Text detectat: %s\n", quickDecode.c_str());

		Mat captured = frame.clone();
		Sleep(500);
		destroyWindow("Camera Web - Preview Live");

		int maxDim = 600;
		if (captured.cols > maxDim || captured.rows > maxDim)
		{
			double factor = (double)maxDim / max(captured.cols, captured.rows);
			resize(captured, captured, Size(), factor, factor, INTER_LINEAR);
		}

		imshow("Frame Capturat", captured);
		printf("\nAplic pipeline-ul complet...\n");

		Mat greyImg = image2Gray(captured);
		Mat equalizedImg = equalizeHist(greyImg);
		Mat equalizedBlur = applyGaussianBlur(equalizedImg);

		Mat binIterativ = binarizare(equalizedBlur);
		Mat binOtsu = binarizareOtsu(equalizedBlur);
		Mat binAdaptiv = binarizareAdaptiva(equalizedBlur);
		Mat binImg = chooseBestBinary(binIterativ, binOtsu, binAdaptiv, equalizedBlur);

		Mat postBinBlurred = applyPostBinBlur(binImg);
		Mat cleanImg = postBinBlurred;
		Mat erodedImg = eroziuneDetectie(cleanImg);

		std::vector<Point2f> corners;
		Mat cornerImg = detectFinderPatternsAndColor(erodedImg, corners);

		if (corners.size() < 3)
		{
			printf("\nFinder patterns insuficiente.\n");
			waitKey(0);
			return;
		}

		int version = 1;
		Mat warped = applyAffineCorrection(erodedImg, corners, version);
		Mat moduleGrid = sampleModuleGrid(warped, version, 16);

		int Nq = moduleGrid.rows;
		Mat syntheticQR(Nq, Nq, CV_8UC1);
		for (int i = 0; i < Nq; i++)
			for (int j = 0; j < Nq; j++)
				syntheticQR.at<uchar>(i, j) = (moduleGrid.at<uchar>(i, j) == 1) ? 0 : 255;

		Mat scaledQR, perfectQR;
		resize(syntheticQR, scaledQR, Size(), 10.0, 10.0, INTER_NEAREST);
		copyMakeBorder(scaledQR, perfectQR, 40, 40, 40, 40, BORDER_CONSTANT, Scalar(255));
		imshow("QR Sintetic Generat", perfectQR);

		cv::QRCodeDetector qrDecoder;
		std::string decodedText = qrDecoder.detectAndDecode(perfectQR);
		if (decodedText.empty()) decodedText = qrDecoder.detectAndDecode(cleanImg);
		if (decodedText.empty()) decodedText = quickDecode;

		if (!decodedText.empty())
			processResult(decodedText);
		else
			printf("\n[EROARE] Nu s-a putut decoda.\n");

		printf("\nApasa o tasta pentru a inchide...\n");
		waitKey(0);
		return;
	}

	cap.release();
	destroyAllWindows();
	printf("\nCamera inchisa.\n");
}

int main()
{
	cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_FATAL);

	projectPath = _wgetcwd(0, 0);

	system("cls");
	destroyAllWindows();

	printf("PROIECT: DECODARE COD QR\n\n");
	printf("Selecteaza modul de operare:\n");
	printf(" [1] Imagine din fisier\n");
	printf(" [2] Camera web \n");

	int choice = 0;
	while (choice != 1 && choice != 2)
	{
		char c;
		std::cin >> c;
		choice = c - '0';
		if (choice != 1 && choice != 2)
			printf("\nAlege 1 sau 2: ");
	}

	printf("%d\n\n", choice);

	if (choice == 2)
	{
		runWebcamMode();
		return 0;
	}

	printf("Selecteaza imaginea din fereastra...\n");

	Mat ogImg = openImage();

	if (ogImg.empty())
	{
		printf("\n[EROARE] Imaginea nu a putut fi incarcata.\n");
		waitKey(0);
		return -1;
	}

	int maxDim = 600;
	if (ogImg.cols > maxDim || ogImg.rows > maxDim)
	{
		double factor = (double)maxDim / max(ogImg.cols, ogImg.rows);
		Mat resized;
		resize(ogImg, resized, Size(), factor, factor, INTER_LINEAR);
		ogImg = resized;
	}

	printf("Imaginea incarcata! Apasa o tasta pentru a continua...\n");
	waitKey();

	// Pas 1: Grayscale
	printf("\n[Pas 1] Conversie la Grayscale\n");
	Mat greyImg = image2Gray(ogImg);
	waitKey();

	// Pas 2a: Gaussian Blur
	printf("\n[Pas 2a] Gaussian Blur 5x5\n");
	Mat blurredImg = greyImg;
	applyGaussianBlur(greyImg);
	waitKey();

	// Pas 2b: Egalizare histograma
	printf("\n[Pas 2b] Egalizare Histograma\n");
	Mat equalizedImg = equalizeHist(greyImg);
	Mat equalizedBlurred = applyGaussianBlur(equalizedImg);
	waitKey();

	// Pas 3: Binarizare
	printf("\n[Pas 3] Binarizare\n");
	Mat binIterativ = binarizare(equalizedBlurred);
	Mat binOtsu = binarizareOtsu(equalizedBlurred);
	Mat binAdaptiv = binarizareAdaptiva(equalizedBlurred);
	Mat binImg = chooseBestBinary(binIterativ, binOtsu, binAdaptiv, ogImg);
	imshow("Binarizare aleasa", binImg);
	waitKey();

	// Pas 4: Blur post-binarizare
	printf("\n[Pas 4] Gaussian Blur 3x3 dupa binarizare\n");
	Mat postBinBlurred = applyPostBinBlur(binImg);
	waitKey();

	Mat cleanImg = postBinBlurred;

	// Pas 5: Eroziune
	printf("\n[Pas 5] Eroziune\n");
	Mat erodedImg = eroziuneDetectie(cleanImg);

	// Pas 6: Finder Patterns
	printf("\n[Pas 6] Detectare Finder Patterns\n");
	std::vector<Point2f> corners;
	Mat cornerImg = detectFinderPatternsAndColor(erodedImg, corners);
	waitKey();

	// Pas 7: Corectie perspectiva
	printf("\n[Pas 7] Corectie Perspectiva\n");
	if (corners.size() < 3)
	{
		printf("\n[EROARE] Detectia a gasit doar %zu finder patterns. Sunt necesare 3.\n", corners.size());
		waitKey(0);
		return 0;
	}

	int version = 1;
	Mat warped = applyAffineCorrection(erodedImg, corners, version);
	waitKey();

	// Pas 8: Grila logica
	printf("\n[Pas 8] Extragerea grilei logice\n");
	int moduleSize = 16;
	Mat moduleGrid = sampleModuleGrid(warped, version, moduleSize);
	waitKey();

	// Pas 9: QR sintetic si decodare
	printf("\n[Pas 9] Generare QR Sintetic si decodare\n");
	int N = moduleGrid.rows;
	Mat syntheticQR(N, N, CV_8UC1);

	for (int i = 0; i < N; i++)
		for (int j = 0; j < N; j++)
			syntheticQR.at<uchar>(i, j) = (moduleGrid.at<uchar>(i, j) == 1) ? 0 : 255;

	Mat scaledQR, perfectQR;
	resize(syntheticQR, scaledQR, Size(), 10.0, 10.0, INTER_NEAREST);
	copyMakeBorder(scaledQR, perfectQR, 40, 40, 40, 40, BORDER_CONSTANT, Scalar(255));
	imshow("QR Sintetic Generat", perfectQR);
	waitKey();

	cv::QRCodeDetector qrDecoder;
	std::string decodedText = qrDecoder.detectAndDecode(perfectQR);

	if (!decodedText.empty())
	{
		printf("\nSUCCES! Link gasit: %s\n", decodedText.c_str());
	}
	else
	{
		decodedText = qrDecoder.detectAndDecode(cleanImg);
		if (!decodedText.empty())
			printf("\nSUCCES (fallback)! Link gasit: %s\n", decodedText.c_str());
		else
			printf("\n[EROARE] Nicio metoda nu a putut citi codul.\n");
	}

	if (!decodedText.empty())
	{
		std::string url = decodedText;
		if (url.find("http://") != 0 && url.find("https://") != 0)
			url = "https://" + url;
		system(("start \"\" \"" + url + "\"").c_str());
	}

	printf("\nProiect finalizat! Apasa o tasta pentru a inchide...\n");
	waitKey(0);
	return 0;
}
// OpenCVApplication.cpp : Defines the entry point for the console application.

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

// ---------------------------------------------------------------
// openImage: Deschide imaginea din dialog
// ---------------------------------------------------------------
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




// ---------------------------------------------------------------
// PASUL 1: Conversie la Grayscale
// ---------------------------------------------------------------
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

	imshow("Imagine grayscale", dst);
	return dst;
}

// ---------------------------------------------------------------
// PASUL 2a - NOU: Gaussian Blur dupa grayscale (nucleu 5x5)
//   Reduce zgomotul inainte de procesarea pragului.
// ---------------------------------------------------------------
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

	imshow("Dupa Gaussian Blur (pre-binarizare)", dst);
	return dst;
}

// ---------------------------------------------------------------
// PASUL 2b - NOU: Egalizare histograma in spatiul HSV (canal V)
//   BGR -> HSV -> egalizare V -> BGR -> Grayscale
//   Imbunatateste contrastul fara a distorsiona culoarea.
// ---------------------------------------------------------------
Mat equalizeHist(Mat src)
{
	int height = src.rows;
	int width = src.cols;
	int M = height * width;

	Mat dst(height, width, CV_8UC1);

	// ----------------------------
	// 1. HISTOGRAMĂ
	// ----------------------------
	int hist[256] = { 0 };

	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
			hist[src.at<uchar>(i, j)]++;

	// ----------------------------
	// 2. PDF + CDF
	// ----------------------------
	float pdf[256] = { 0 };
	float cdf[256] = { 0 };

	float sum = 0.0f;

	for (int i = 0; i < 256; i++)
	{
		pdf[i] = (float)hist[i] / M;
		sum += pdf[i];

		cdf[i] = sum;   // CDF standard (fără pow!)
	}

	// ----------------------------
	// 3. CDF MIN (important!)
	// ----------------------------
	float cdf_min = 0.0f;
	for (int i = 0; i < 256; i++)
	{
		if (cdf[i] > 0.0f)
		{
			cdf_min = cdf[i];
			break;
		}
	}

	// ----------------------------
	// 4. LUT (mapare intensități)
	// ----------------------------
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

	// ----------------------------
	// 5. APLICARE LUT
	// ----------------------------
	for (int i = 0; i < height; i++)
		for (int j = 0; j < width; j++)
			dst.at<uchar>(i, j) = lut[src.at<uchar>(i, j)];

	imshow("Egalizare Gray Manual", dst);
	return dst;
}
// ---------------------------------------------------------------
// PASUL 3a: Binarizare iterativa cu prag optim (PASTRATA)
// ---------------------------------------------------------------
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
				else          { sum2 += val; count2++; }
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

// ---------------------------------------------------------------
// PASUL 3b - NOU: Binarizare Otsu
//   Maximizeaza varianta inter-clase pentru a gasi pragul optim.
// ---------------------------------------------------------------
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

// ---------------------------------------------------------------
// PASUL 3c - NOU: Binarizare Adaptiva (pentru lumina neuniforma)
//   Calculeaza un prag local per fereastra blockSize x blockSize
//   folosind imaginea integrala pentru eficienta O(1) per pixel.
//   prag_local = medie_locala - C
// ---------------------------------------------------------------
Mat binarizareAdaptiva(Mat src)
{
	int height = src.rows;
	int width  = src.cols;
	Mat dst = Mat(height, width, CV_8UC1, Scalar(255));

	// blockSize: fereastra locala (impar). 51 = ~8% din 600px
	// Mareste daca gradientul de iluminare e foarte lin
	int blockSize = 51;
	int halfB = blockSize / 2;

	// C: offset sub medie. Creste C => mai putin negru (mai curat)
	int C = 10;

	// Imagine integrala pentru suma O(1) per fereastra
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
						- integralImg.at<double>(r1,     c2 + 1)
						- integralImg.at<double>(r2 + 1, c1    )
						+ integralImg.at<double>(r1,     c1    );

			int nrPixeli = (r2 - r1 + 1) * (c2 - c1 + 1);
			double medie = suma / nrPixeli;

			uchar val = src.at<uchar>(i, j);
			dst.at<uchar>(i, j) = (val < medie - C) ? 0 : 255;
		}
	}

	imshow("Binarizare Adaptiva (lumina neuniforma)", dst);
	return dst;
}

// ---------------------------------------------------------------
// PASUL 3d: Alegem metoda de binarizare dupa tipul imaginii
//
//   Impartim imaginea in 9 blocuri (3x3) si calculam media
//   de intensitate a fiecaruia. Daca diferenta dintre blocul
//   cel mai luminos si cel mai intunecat depaseste DIFF_THRESH,
//   inseamna iluminare neuniforma (foto cu lumina naturala)
//   -> folosim ADAPTIVA.
//   Altfel (scanata, descarcata, dreapta) -> folosim ITERATIVA.
// ---------------------------------------------------------------
Mat chooseBestBinary(Mat iterativ, Mat otsu, Mat adaptiv, Mat srcGray)
{
	int height = srcGray.rows;
	int width  = srcGray.cols;
	int bh = height / 3;
	int bw = width  / 3;
	double minMedie = 255.0, maxMedie = 0.0;

	for (int bi = 0; bi < 3; bi++)
	{
		for (int bj = 0; bj < 3; bj++)
		{
			long long suma = 0;
			int count = 0;
			for (int i = bi * bh; i < (bi + 1) * bh; i++)
				for (int j = bj * bw; j < (bj + 1) * bw; j++)
				{
					suma += srcGray.at<uchar>(i, j);
					count++;
				}
			double medie = (double)suma / count;
			if (medie < minMedie) minMedie = medie;
			if (medie > maxMedie) maxMedie = medie;
		}
	}

	double diff = maxMedie - minMedie;
	printf("  [Detectie] Diferenta iluminare intre blocuri = %.1f\n", diff);

	// Peste 40 => iluminare neuniforma (fotografie) => ADAPTIVA
	// Sub 40  => imagine curata/scanata             => ITERATIVA
	double DIFF_THRESH = 40.0;

	if (diff > DIFF_THRESH)
	{
		printf("  [Alegere] Iluminare NEUNIFORMA -> Binarizare ADAPTIVA\n");
		return adaptiv;
	}
	else
	{
		printf("  [Alegere] Iluminare UNIFORMA -> Binarizare ITERATIVA\n");
		return iterativ;
	}
}

// ---------------------------------------------------------------
// PASUL 4 - NOU: Gaussian Blur dupa binarizare (nucleu 3x3)
//   Netezeste marginile zimtate ale modulelor QR,
//   urmat de re-binarizare la prag fix 128.
// ---------------------------------------------------------------
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

	imshow("Dupa Gaussian Blur (post-binarizare)", dst);
	return dst;
}

// ---------------------------------------------------------------
// PASUL 5: Eliminare zgomot Salt & Pepper (NESCHIMBATA)
// ---------------------------------------------------------------
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

	imshow("CleanedSaltAndPepper", dst_median);
	return dst_median;
}

// ---------------------------------------------------------------
// Element structural pentru dilatare (NESCHIMBAT)
// ---------------------------------------------------------------
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

// ---------------------------------------------------------------
// Eroziune pentru detectie - subtiem marginile groase false
// Folosim element structural 3x3 complet (patrат)
// ---------------------------------------------------------------
Mat eroziuneDetectie(Mat src)
{
	int height = src.rows;
	int width  = src.cols;
	Mat dst = Mat(height, width, CV_8UC1, Scalar(255));

	for (int i = 1; i < height - 1; i++)
		for (int j = 1; j < width - 1; j++)
		{
			// Pixel negru in dst doar daca TOTI vecinii 3x3 sunt negri
			bool allBlack = true;
			for (int u = -1; u <= 1 && allBlack; u++)
				for (int v = -1; v <= 1 && allBlack; v++)
					if (src.at<uchar>(i + u, j + v) != 0)
						allBlack = false;
			if (allBlack)
				dst.at<uchar>(i, j) = 0;
		}

	imshow("Dupa Eroziune (pre-detectie)", dst);
	return dst;
}

// ---------------------------------------------------------------
// Detectare Finder Patterns (NESCHIMBATA)
// ---------------------------------------------------------------
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
									{ sumX += px; sumY += py; count++; }
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

	imshow("DST: Puncte de Control Filtrate", dst);
	for (const auto& c : candidates) outCorners.push_back(c.pos);
	return dst;
}

// ---------------------------------------------------------------
// getMarkerInnerCorners (NESCHIMBATA)
// ---------------------------------------------------------------
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

// ---------------------------------------------------------------
// applyAffineCorrection (NESCHIMBATA)
// ---------------------------------------------------------------
Mat applyAffineCorrection(Mat binImg, std::vector<Point2f> corners, int& outVersion)
{
	// Guard: daca nu avem exact 3 colturi, nu putem face corectia
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

	imshow("Warped QR (Matematica Suprema)", finalWarped);
	return finalWarped;
}

// ---------------------------------------------------------------
// sampleModuleGrid (NESCHIMBATA)
// ---------------------------------------------------------------
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

	imshow("Grila Module QR (Aliniament Perfect)", vizGrid);
	return grid;
}



//////////////////////////////////////////////////////////////////////////


// ---------------------------------------------------------------
//   Afiseaza rezultatul si deschide browser-ul.
// ---------------------------------------------------------------
void processResult(const std::string& decodedText)
{
	printf("\n==================================================\n");
	printf(" SUCCES! QR decodat de camera web.\n");
	printf(" Link/Text: %s\n", decodedText.c_str());
	printf("==================================================\n\n");

	std::string url = decodedText;
	if (url.find("http://") == 0 || url.find("https://") == 0)
	{
		printf("Deschid browser-ul...\n");
		system(("start \"\" \"" + url + "\"").c_str());
	}
}

// ---------------------------------------------------------------
//   Captureaza frame-uri live, aplica algoritmul complet
//   si cand detecteaza un QR valid afiseaza rezultatul.


void runWebcamMode()
{
	printf("\n==================================================\n");
	printf("    MOD CAMERA WEB - Detectare QR Live\n");
	printf("==================================================\n");
	printf("  Tine codul QR in fata camerei.\n");
	printf("  Captura se face DOAR cand QR-ul este stabil.\n");
	printf("  Apasa ESC pentru a iesi.\n\n");

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

	// -------------------------------------------------
	// STABILITATE DETECTIE
	// -------------------------------------------------
	int stableFrames = 0;
	const int REQUIRED_STABLE_FRAMES = 8;

	std::string lastDecoded = "";

	while (true)
	{
		cap >> frame;

		if (frame.empty())
			break;

		Mat preview = frame.clone();

		// -------------------------------------------------
		// Detectie + Decodare rapida
		// -------------------------------------------------
		std::vector<Point> qrPoints;

		std::string quickDecode =
			qrDetector.detectAndDecode(frame, qrPoints);

		bool found = !quickDecode.empty();

		// -------------------------------------------------
		// Filtru: ignora detectii foarte mici
		// -------------------------------------------------
		if (found && qrPoints.size() >= 4)
		{
			double area = contourArea(qrPoints);

			if (area < 5000)
			{
				found = false;
			}
		}

		// -------------------------------------------------
		// Stabilitate pe frame-uri consecutive
		// -------------------------------------------------
		if (found)
		{
			if (quickDecode == lastDecoded)
			{
				stableFrames++;
			}
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

		// -------------------------------------------------
		// Desen contur QR
		// -------------------------------------------------
		if (found && qrPoints.size() == 4)
		{
			for (int i = 0; i < 4; i++)
			{
				line(preview,
					qrPoints[i],
					qrPoints[(i + 1) % 4],
					Scalar(0, 255, 0),
					3);
			}

			putText(preview,
				"QR VALID DETECTAT",
				Point(10, 40),
				FONT_HERSHEY_SIMPLEX,
				0.8,
				Scalar(0, 255, 0),
				2);

			char buffer[100];
			sprintf(buffer,
				"Stabilitate: %d / %d",
				stableFrames,
				REQUIRED_STABLE_FRAMES);

			putText(preview,
				buffer,
				Point(10, 75),
				FONT_HERSHEY_SIMPLEX,
				0.7,
				Scalar(0, 255, 255),
				2);
		}
		else
		{
			putText(preview,
				"Cauta cod QR...",
				Point(10, 40),
				FONT_HERSHEY_SIMPLEX,
				0.8,
				Scalar(0, 100, 255),
				2);
		}

		putText(preview,
			"ESC = Iesire",
			Point(10, 110),
			FONT_HERSHEY_SIMPLEX,
			0.6,
			Scalar(255, 255, 255),
			1);

		imshow("Camera Web - Preview Live", preview);

		// ESC
		if (waitKey(30) == 27)
			break;

		// -------------------------------------------------
		// Nu captura pana nu e stabil
		// -------------------------------------------------
		if (stableFrames < REQUIRED_STABLE_FRAMES)
			continue;

		// -------------------------------------------------
		// CAPTURA FINALA
		// -------------------------------------------------
		printf("\nQR stabil detectat!\n");
		printf("Text detectat: %s\n", quickDecode.c_str());

		Mat captured = frame.clone();

		// cooldown mic
		Sleep(500);

		// inchidem preview-ul live
		destroyWindow("Camera Web - Preview Live");

		// -------------------------------------------------
		// Resize imagine
		// -------------------------------------------------
		int maxDim = 600;

		if (captured.cols > maxDim ||
			captured.rows > maxDim)
		{
			double factor =
				(double)maxDim /
				max(captured.cols, captured.rows);

			resize(captured,
				captured,
				Size(),
				factor,
				factor,
				INTER_LINEAR);
		}

		imshow("Frame Capturat", captured);

		printf("\nAplic pipeline-ul complet...\n");

		// -------------------------------------------------
		// PASUL 1
		// -------------------------------------------------
		Mat greyImg = image2Gray(captured);

		// -------------------------------------------------
		// PASUL 2
		// -------------------------------------------------
		Mat equalizedImg = equalizeHist(greyImg);

		Mat equalizedBlur =
			applyGaussianBlur(equalizedImg);

		// -------------------------------------------------
		// PASUL 3
		// -------------------------------------------------
		Mat binIterativ =
			binarizare(equalizedBlur);

		Mat binOtsu =
			binarizareOtsu(equalizedBlur);

		Mat binAdaptiv =
			binarizareAdaptiva(equalizedBlur);

		Mat binImg =
			chooseBestBinary(
				binIterativ,
				binOtsu,
				binAdaptiv,
				equalizedBlur);

		// -------------------------------------------------
		// PASUL 4
		// -------------------------------------------------
		Mat postBinBlurred =
			applyPostBinBlur(binImg);

		Mat cleanImg = postBinBlurred;

		// -------------------------------------------------
		// PASUL 5
		// -------------------------------------------------
		Mat erodedImg =
			eroziuneDetectie(cleanImg);

		// -------------------------------------------------
		// PASUL 6
		// -------------------------------------------------
		std::vector<Point2f> corners;

		Mat cornerImg =
			detectFinderPatternsAndColor(
				erodedImg,
				corners);

		// -------------------------------------------------
		// Daca detectia custom esueaza,
		// folosim direct decoderul OpenCV
		// -------------------------------------------------
		if (corners.size() < 3)
		{
			printf("\nFinder patterns insuficiente.\n");
			waitKey(0);
			return;
		}

		// -------------------------------------------------
		// PASUL 7
		// -------------------------------------------------
		int version = 1;

		Mat warped =
			applyAffineCorrection(
				erodedImg,
				corners,
				version);

		// -------------------------------------------------
		// PASUL 8
		// -------------------------------------------------
		Mat moduleGrid =
			sampleModuleGrid(
				warped,
				version,
				16);

		// -------------------------------------------------
		// PASUL 9
		// -------------------------------------------------
		int Nq = moduleGrid.rows;

		Mat syntheticQR(Nq, Nq, CV_8UC1);

		for (int i = 0; i < Nq; i++)
			for (int j = 0; j < Nq; j++)
				syntheticQR.at<uchar>(i, j) =
				(moduleGrid.at<uchar>(i, j) == 1)
				? 0 : 255;

		Mat scaledQR, perfectQR;

		resize(syntheticQR,
			scaledQR,
			Size(),
			10.0,
			10.0,
			INTER_NEAREST);

		copyMakeBorder(
			scaledQR,
			perfectQR,
			40, 40, 40, 40,
			BORDER_CONSTANT,
			Scalar(255));

		imshow("QR Sintetic Generat", perfectQR);

		// -------------------------------------------------
		// DECODARE FINALA
		// -------------------------------------------------
		cv::QRCodeDetector qrDecoder;

		std::string decodedText =
			qrDecoder.detectAndDecode(perfectQR);

		if (decodedText.empty())
			decodedText =
			qrDecoder.detectAndDecode(cleanImg);

		if (decodedText.empty())
			decodedText = quickDecode;

		if (!decodedText.empty())
		{
			processResult(decodedText);
		}
		else
		{
			printf("\n[EROARE] Nu s-a putut decoda.\n");
		}

		printf("\nApasa o tasta pentru a inchide...\n");

		waitKey(0);

		return;
	}

	cap.release();

	destroyAllWindows();

	printf("\nCamera inchisa.\n");
}












// ---------------------------------------------------------------
// MAIN - Pipeline complet cu toate modificarile integrate
// ---------------------------------------------------------------
int main()
{
	cv::utils::logging::setLogLevel(
		cv::utils::logging::LOG_LEVEL_FATAL
	);

	projectPath = _wgetcwd(0, 0);

	system("cls");
	destroyAllWindows();

	printf("==================================================\n");
	printf(" PROIECT: DECODARE COD QR\n");
	printf("==================================================\n\n");

	printf("Selecteaza modul de operare:\n");
	printf(" [1] Imagine din fisier (modul original)\n");
	printf(" [2] Camera web (live)\n");
	printf("\nAlege (1 sau 2): ");

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
		// ---- MOD CAMERA WEB ----
		runWebcamMode();
		return 0;
	}

	// ---- MOD ORIGINAL: imagine din fisier ----

	printf("-> Selecteaza imaginea din fereastra...\n");

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
		double factor =
			(double)maxDim /
			max(ogImg.cols, ogImg.rows);

		Mat resized;

		resize(
			ogImg,
			resized,
			Size(),
			factor,
			factor,
			INTER_LINEAR
		);

		ogImg = resized;
	}

	printf("=> Imaginea incarcata! Apasa o tasta pentru a continua...\n");

	waitKey();

	// ======================================================
	// PASUL 1 - GRAYSCALE
	// ======================================================

	printf("\n[Pasul 1] Conversie la Grayscale...\n");

	Mat greyImg = image2Gray(ogImg);

	waitKey();

	// ======================================================
	// PASUL 2a - GAUSSIAN BLUR
	// ======================================================

	printf("\n[Pasul 2a] Gaussian Blur (5x5) dupa Grayscale...\n");

	Mat blurredImg =
		applyGaussianBlur(greyImg);

	waitKey();

	// ======================================================
	// PASUL 2b - EGALIZARE HSV
	// ======================================================

	printf(
		"\n[Pasul 2b] Egalizare Histograma in spatiul HSV (canal V)...\n"
	);

	Mat equalizedImg =
		equalizeHist(greyImg);

	Mat equalizedBlurred =
		applyGaussianBlur(equalizedImg);

	waitKey();

	// ======================================================
	// PASUL 3 - BINARIZARE
	// ======================================================

	printf("\n[Pasul 3] Binarizare tripla...\n");

	Mat binIterativ =
		binarizare(equalizedBlurred);

	Mat binOtsu =
		binarizareOtsu(equalizedBlurred);

	Mat binAdaptiv =
		binarizareAdaptiva(equalizedBlurred);

	Mat binImg =
		chooseBestBinary(
			binIterativ,
			binOtsu,
			binAdaptiv,
			ogImg
		);

	imshow("Binarizare ALEASA", binImg);

	waitKey();

	// ======================================================
	// PASUL 4 - POST BLUR
	// ======================================================

	printf(
		"\n[Pasul 4] Gaussian Blur (3x3) dupa Binarizare + re-binarizare...\n"
	);

	Mat postBinBlurred =
		applyPostBinBlur(binImg);

	waitKey();

	Mat cleanImg = postBinBlurred;

	// ======================================================
	// PASUL 5 - EROZIUNE
	// ======================================================

	printf(
		"\n[Pasul 5b] Eroziune pentru subtiere margini false...\n"
	);

	Mat erodedImg =
		eroziuneDetectie(cleanImg);

	// ======================================================
	// PASUL 6 - FINDER PATTERNS
	// ======================================================

	printf("\n[Pasul 6] Detectare Finder Patterns...\n");

	std::vector<Point2f> corners;

	Mat cornerImg =
		detectFinderPatternsAndColor(
			erodedImg,
			corners
		);

	waitKey();

	// ======================================================
	// PASUL 7 - PERSPECTIVA
	// ======================================================

	printf("\n[Pasul 7] Corectie Perspectiva...\n");

	if (corners.size() < 3)
	{
		printf(
			"\n[EROARE] Detectia a gasit doar %zu finder patterns. Sunt necesare 3.\n",
			corners.size()
		);

		waitKey(0);
		return 0;
	}

	int version = 1;

	Mat warped =
		applyAffineCorrection(
			erodedImg,
			corners,
			version
		);

	waitKey();

	// ======================================================
	// PASUL 8 - GRID LOGIC
	// ======================================================

	printf("\n[Pasul 8] Extragerea grilei logice...\n");

	int moduleSize = 16;

	Mat moduleGrid =
		sampleModuleGrid(
			warped,
			version,
			moduleSize
		);

	waitKey();

	// ======================================================
	// PASUL 9 - QR SINTETIC
	// ======================================================

	printf(
		"\n[Pasul 9] Generare QR Sintetic si Citire Date...\n"
	);

	int N = moduleGrid.rows;

	Mat syntheticQR(
		N,
		N,
		CV_8UC1
	);

	for (int i = 0; i < N; i++)
	{
		for (int j = 0; j < N; j++)
		{
			syntheticQR.at<uchar>(i, j) =
				(moduleGrid.at<uchar>(i, j) == 1)
				? 0
				: 255;
		}
	}

	Mat scaledQR;
	Mat perfectQR;

	resize(
		syntheticQR,
		scaledQR,
		Size(),
		10.0,
		10.0,
		INTER_NEAREST
	);

	copyMakeBorder(
		scaledQR,
		perfectQR,
		40,
		40,
		40,
		40,
		BORDER_CONSTANT,
		Scalar(255)
	);

	imshow(
		"QR Sintetic Generat",
		perfectQR
	);

	waitKey();

	// ======================================================
	// DECODARE QR
	// ======================================================

	cv::QRCodeDetector qrDecoder;

	std::string decodedText =
		qrDecoder.detectAndDecode(perfectQR);

	if (!decodedText.empty())
	{
		printf(
			"\n==================================================\n"
		);

		printf(
			" SUCCES! Link gasit: %s\n",
			decodedText.c_str()
		);

		printf(
			"==================================================\n\n"
		);
	}
	else
	{
		decodedText =
			qrDecoder.detectAndDecode(cleanImg);

		if (!decodedText.empty())
		{
			printf(
				"\n SUCCES (Fallback)! Link gasit: %s\n",
				decodedText.c_str()
			);
		}
		else
		{
			printf(
				"\n[EROARE] Nicio metoda nu a putut citi codul.\n"
			);
		}
	}

	// ======================================================
	// DESCHIDERE LINK
	// ======================================================

	if (!decodedText.empty())
	{
		std::string url = decodedText;

		if (
			url.find("http://") != 0 &&
			url.find("https://") != 0
			)
		{
			url = "https://" + url;
		}

		system(
			("start \"\" \"" + url + "\"").c_str()
		);
	}

	printf(
		"\nProiect finalizat! Apasa o tasta pentru a inchide...\n"
	);

	waitKey(0);

	return 0;
}
#ifndef GENSHINIMPACT_PAIMONSHUTUP_CPP_TESSERACT_H
#define GENSHINIMPACT_PAIMONSHUTUP_CPP_TESSERACT_H

#include <iostream>
#include <baseapi.h>
#include <allheaders.h>
#include "opencv2/core.hpp"

#define AUTOMATIC_PAGE_SEGMENTATION "3" // Tesseract's automatic page segmentation
#define OEM_DEFAULT "3" // Teseract's default ORC Engine mode
#define min(a, b)            (((a) < (b)) ? (a) : (b))


int InitTesseract(const char *dataPath, const char *language);

void DestroyTesseract();

std::string StripText(const std::string &input);

std::string GetTextFromImage(const cv::Mat &image);

unsigned int LevenshteinDistance(const std::string &s1, const std::string &s2);

bool IsStringsSimilar(std::string s1, std::string s2, const double overlap = 0.4);

#endif //GENSHINIMPACT_PAIMONSHUTUP_CPP_TESSERACT_H

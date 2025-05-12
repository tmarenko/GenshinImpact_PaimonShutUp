#ifndef GENSHINIMPACT_PAIMONSHUTUP_CPP_TESSERACT_H
#define GENSHINIMPACT_PAIMONSHUTUP_CPP_TESSERACT_H

#include <iostream>
#include <baseapi.h>
#include <allheaders.h>
#include "image.h"
#include <urlmon.h>
#include <algorithm>
#include <string>

#pragma comment(lib, "urlmon.lib")

#define AUTOMATIC_PAGE_SEGMENTATION "3" // Tesseract's automatic page segmentation
#define OEM_DEFAULT "3" // Teseract's default ORC Engine mode
#define min(a, b)            (((a) < (b)) ? (a) : (b))


int InitTesseract(const char *dataPath, const char *language);

void DestroyTesseract();

std::string StripText(const std::string &input);

std::string GetTextFromImage(const Image &image);

unsigned int LevenshteinDistance(const std::string &s1, const std::string &s2);

bool IsStringsSimilar(std::string s1, std::string s2, int maxDifference);

void DownloadTessdataFileIfNecessary(const std::string &language);

#endif //GENSHINIMPACT_PAIMONSHUTUP_CPP_TESSERACT_H

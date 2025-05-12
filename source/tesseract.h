#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <baseapi.h>
#include <allheaders.h>
#include <urlmon.h>
#include "image.h"

#pragma comment(lib, "urlmon.lib")

#define AUTOMATIC_PAGE_SEGMENTATION "3" // Tesseract's automatic page segmentation
#define OEM_DEFAULT "3" // Teseract's default ORC Engine mode


int InitTesseract(const char *dataPath, const char *language);

void DestroyTesseract();

std::string StripText(const std::string &input);

std::string GetTextFromImage(const Image &image);

unsigned int LevenshteinDistance(const std::string &s1, const std::string &s2);

bool IsStringsSimilar(std::string s1, std::string s2, int maxDifference);

void DownloadTessdataFileIfNecessary(const std::string &language);

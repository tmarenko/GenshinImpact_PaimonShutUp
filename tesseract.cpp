#include "tesseract.h"

tesseract::TessBaseAPI *tesseractApi;
const char *asciiLetters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";


int InitTesseract(const char* dataPath, const char* language) {
    tesseractApi = new tesseract::TessBaseAPI();
    if (tesseractApi->Init(dataPath, language)) {
        std::cout << "Could not initialize tesseract lib." << std::endl;
        return 1;
    }
    tesseractApi->SetVariable("load_system_dawg", "0");
    tesseractApi->SetVariable("load_freq_dawg", "0");
    tesseractApi->SetVariable("tessedit_oem_mode", OEM_DEFAULT);
    tesseractApi->SetVariable("debug_file", "/dev/null");

    std::cout << "Initialized Tesseract API." << std::endl;
    return 0;
}


void DestroyTesseract() {
    if (tesseractApi) {
        std::cout << "Destroying Tesseract lib." << std::endl;
        tesseractApi->End();
        delete tesseractApi;
    }
}


std::string StripText(const std::string &input) {
    if (input.empty())
        return input;
    auto start_it = input.begin();
    auto end_it = input.rbegin();
    while (std::isspace(*start_it))
        ++start_it;
    while (std::isspace(*end_it))
        ++end_it;
    return std::string(start_it, end_it.base());
}


std::string GetTextFromImage(const cv::Mat &image) {
    if (!tesseractApi) {
        std::cout << "Tesseract lib isn't initialized." << std::endl;
        return std::string();
    }
    char *tesseractOutText;
    tesseractApi->SetVariable("tessedit_char_whitelist", asciiLetters);
    tesseractApi->SetVariable("tessedit_pageseg_mode", AUTOMATIC_PAGE_SEGMENTATION);
    tesseractApi->SetImage(image.data, image.cols, image.rows, image.channels(), image.cols * image.channels());
    tesseractOutText = tesseractApi->GetUTF8Text();
    tesseractApi->Clear();
    tesseractApi->ClearAdaptiveClassifier();
    auto result = StripText(tesseractOutText);
    delete tesseractOutText;
    return result;
}


unsigned int LevenshteinDistance(const std::string &s1, const std::string &s2) {
    const std::size_t len1 = s1.size(), len2 = s2.size();
    std::vector<std::vector<unsigned int>> d(len1 + 1, std::vector<unsigned int>(len2 + 1));

    d[0][0] = 0;
    for (unsigned int i = 1; i <= len1; ++i) d[i][0] = i;
    for (unsigned int i = 1; i <= len2; ++i) d[0][i] = i;

    for (unsigned int i = 1; i <= len1; ++i)
        for (unsigned int j = 1; j <= len2; ++j)
            // note that std::min({arg1, arg2, arg3}) works only in C++11,
            // for C++98 use std::min(std::min(arg1, arg2), arg3)
            d[i][j] = min(min(d[i - 1][j] + 1, d[i][j - 1] + 1),
                          d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1));
    return d[len1][len2];
}


bool IsStringsSimilar(std::string s1, std::string s2, const double overlap) {
    std::transform(s1.begin(), s1.end(), s1.begin(), ::toupper);
    std::transform(s2.begin(), s2.end(), s2.begin(), ::toupper);
    double nonSimilarity = s1.length() > 0 ? (double) LevenshteinDistance(s1, s2) / s1.length() : 1;
    return nonSimilarity <= overlap;
}

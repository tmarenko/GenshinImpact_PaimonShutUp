#include "tesseract.h"

tesseract::TessBaseAPI *tesseractApi;
const char *tesseractDownloadUrl = "https://github.com/tesseract-ocr/tessdata/raw/3.04.00/";


int InitTesseract(const char *dataPath, const char *language) {
    tesseractApi = new tesseract::TessBaseAPI();
    if (tesseractApi->Init(dataPath, language)) {
        std::cout << "Could not initialize tesseract lib." << std::endl;
        delete tesseractApi;
        return 1;
    }
    tesseractApi->SetVariable("load_system_dawg", "0");
    tesseractApi->SetVariable("load_freq_dawg", "0");
    tesseractApi->SetVariable("tessedit_oem_mode", OEM_DEFAULT);
    tesseractApi->SetVariable("debug_file", "/dev/null");

    std::cout << "Initialized Tesseract API with '" << language << "' language." << std::endl;
    return 0;
}


void DestroyTesseract() {
    if (tesseractApi) {
        std::cout << "Destroying Tesseract API." << std::endl;
        tesseractApi->End();
        delete tesseractApi;
    }
}


std::string StripText(const std::string &input) {
    if (input.empty())
        return input;
    auto start_it = input.begin();
    auto end_it = input.rbegin();
    while (start_it != input.end() && (*start_it == ' ' || *start_it == '\n'))
        ++start_it;
    while (end_it != input.rend() && (*end_it == ' ' || *end_it == '\n'))
        ++end_it;
    if (start_it > end_it.base())
        return std::string();
    return std::string(start_it, end_it.base());
}


std::string GetTextFromImage(const Image &image) {
    if (!tesseractApi) {
        std::cout << "Tesseract lib isn't initialized." << std::endl;
        return std::string();
    }
    char *tesseractOutText;
    tesseractApi->SetVariable("tessedit_pageseg_mode", AUTOMATIC_PAGE_SEGMENTATION);
    tesseractApi->SetImage(image.data(), image.width(), image.height(), image.channels(), image.width() * image.channels());
    tesseractOutText = tesseractApi->GetUTF8Text();
    tesseractApi->Clear();
    tesseractApi->ClearAdaptiveClassifier();
    auto result = StripText(tesseractOutText);
    delete[] tesseractOutText;
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
            d[i][j] = std::min(std::min(d[i - 1][j] + 1, d[i][j - 1] + 1),
                          d[i - 1][j - 1] + (s1[i - 1] == s2[j - 1] ? 0 : 1));
    return d[len1][len2];
}


bool IsStringsSimilar(std::string s1, std::string s2, int maxDifference) {
    if (s1.length() == 0) {
        return false;
    }
    std::transform(s1.begin(), s1.end(), s1.begin(), ::toupper);
    std::transform(s2.begin(), s2.end(), s2.begin(), ::toupper);
    return LevenshteinDistance(s1, s2) <= maxDifference;
}


inline bool CheckFileExists(const std::string &name) {
    struct stat buffer{};
    return (stat(name.c_str(), &buffer) == 0);
}


void DownloadTessdataFileIfNecessary(const std::string &language) {
    std::string pathToLanguage = "tessdata/" + language + ".traineddata";
    if (CheckFileExists(pathToLanguage)) {
        std::cout << "Found Tesseract data for '" << language << "' language at " + pathToLanguage << std::endl;
        return;
    }
    std::string downloadUrl = tesseractDownloadUrl + language + ".traineddata";
    std::cout << "Downloading Tesseract data for '" << language << "' language from " + downloadUrl << std::endl;
    HRESULT hr = URLDownloadToFileA(nullptr, downloadUrl.c_str(), pathToLanguage.c_str(), 0, nullptr);
    if (hr == S_OK)
        std::cout << "Download complete. Saved to " + pathToLanguage << std::endl;
    else
        std::cout << "Error during download: " + std::to_string(hr) << std::endl;
}
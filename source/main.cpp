#include "opencv2/core.hpp"
#include "opencv2/imgproc.hpp"
#include "windows.h"
#include <iostream>
#include "audioclient.h"
#include "audiopolicy.h"
#include "mmdeviceapi.h"
#include "psapi.h"
#include <csignal>
#include "tesseract.h"
#include "config.h"
#include <chrono>
#include <thread>


typedef struct GenshinWindowInfo {
    std::wstring windowName = L"Genshin Impact";
    std::wstring windowClass = L"UnityWndClass";
    int maxOcrErrors = 1;
    int captureMode = 1;
    bool muteOverworld = true;
    HWND hwnd = nullptr;
    int width = 0;
    int height = 0;
    bool active = false;
} GenshinWindowInfo;

volatile sig_atomic_t stop;

void keyHand(int signum) {
    stop = 1;
}


std::map<std::string, cv::Scalar> dialoguePositions = {
        {"16:9_DEFAULT",    {0.463, 0.787, 0.537, 0.829}},
        {"16:9_OVERWORLD",  {0.465, 0.758, 0.533, 0.802}},
        {"16:10_DEFAULT",   {0.461, 0.809, 0.538, 0.841}},
        {"16:10_OVERWORLD", {0.462, 0.780, 0.538, 0.815}},
};

const cv::Scalar DIALOGUE_NAME_COLOR_RANGE_LOW = {0, 170, 230};   // BGR
const cv::Scalar DIALOGUE_NAME_COLOR_RANGE_HIGH = {10, 210, 255}; // BGR

cv::Mat frame;
HWND genshinWindow;
GenshinWindowInfo gwi;
std::wstring genshinExe = L"GenshinImpact.exe";


BOOL CALLBACK EnumWindowsFunc(HWND hwnd, LPARAM lParam) {
    auto *gwiParam = (GenshinWindowInfo *) lParam;
    WCHAR buf[1024]{};

    GetClassName(hwnd, buf, 100);
    if (!lstrcmp(buf, gwiParam->windowClass.c_str())) {
        GetWindowText(hwnd, buf, 100);
        if (!lstrcmp(buf, gwiParam->windowName.c_str())) {
            gwiParam->hwnd = hwnd;
            RECT windowRect;
            GetWindowRect(hwnd, &windowRect);
            gwiParam->width = windowRect.right - windowRect.left;
            gwiParam->height = windowRect.bottom - windowRect.top;
        }
    }
    return TRUE;
}


void FindGenshinWindow() {
    EnumWindows(EnumWindowsFunc, (LPARAM) (&gwi));
    genshinWindow = gwi.hwnd;

    WCHAR buff[1024]{};
    GetWindowText(genshinWindow, buff, 100);
    if (!lstrcmp(buff, gwi.windowName.c_str())) {
        if (gwi.hwnd && !gwi.active) {
            gwi.active = true;
            std::cout << "Ready for Paimon!" << std::endl;
        }
    } else {
        if (gwi.active) {
            gwi.active = false;
            gwi.hwnd = nullptr;
            std::cout << "Game was closed" << std::endl;
        }
    }
}


void GetFrame(int screenWidth, int screenHeight) {
    RECT clientRect;
    GetClientRect(genshinWindow, &clientRect);
    RECT windowRect;
    GetWindowRect(genshinWindow, &windowRect);
    int borderSize = ((windowRect.right - windowRect.left) - (clientRect.right - clientRect.left)) / 2;
    int titleBarSize = ((windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top)) - borderSize;
    screenHeight += titleBarSize;

    if (screenHeight <= 0 || screenWidth <= 0)
        return;
    frame.create(screenHeight, screenWidth, CV_8UC4);

    BITMAPINFOHEADER bi;
    bi.biSize = sizeof(BITMAPINFOHEADER);    //http://msdn.microsoft.com/en-us/library/windows/window/dd183402%28v=vs.85%29.aspx
    bi.biWidth = screenWidth;
    bi.biHeight = -screenHeight;  //this is the line that makes it draw upside down or not
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    auto hwndDC = GetWindowDC(genshinWindow);
    auto saveDC = CreateCompatibleDC(hwndDC);

    auto bitMap = CreateCompatibleBitmap(hwndDC, screenWidth, screenHeight);
    SelectObject(saveDC, bitMap);

    switch (gwi.captureMode) {
        case 0:
            BitBlt(saveDC, 0, 0, screenWidth, screenHeight, hwndDC, 0, 0, SRCCOPY);
            break;
        case 1:
            PrintWindow(genshinWindow, saveDC, PW_RENDERFULLCONTENT);
            break;
        default:
            std::cout << "Invalid capture mode, reverting to default." << std::endl;
            gwi.captureMode = 1;
            PrintWindow(genshinWindow, saveDC, PW_RENDERFULLCONTENT);
            break;
    }
    GetDIBits(saveDC, bitMap, 0, screenHeight - titleBarSize, frame.data, (BITMAPINFO *) &bi, DIB_RGB_COLORS);

    DeleteObject(bitMap);
    DeleteDC(saveDC);
    DeleteObject(hwndDC);
    ReleaseDC(genshinWindow, hwndDC);
    cv::cvtColor(frame, frame, cv::COLOR_RGBA2RGB);
}


std::string GetTextFromImageByRect(const cv::Mat &image, const cv::Rect& rect) {
    if (image.empty())
        return std::string();
    cv::Mat croppedImage = image(rect);
    cv::inRange(croppedImage, DIALOGUE_NAME_COLOR_RANGE_LOW, DIALOGUE_NAME_COLOR_RANGE_HIGH, croppedImage);
    return GetTextFromImage(croppedImage);
}


cv::Rect GetDialogueRect(const cv::Size &windowSize, const std::string &type) {
    double aspectRatio = static_cast<double>(windowSize.width) / windowSize.height;
    cv::Scalar dialoguePosition = dialoguePositions["16:9_" + type];

    if (std::abs(aspectRatio - 16.0 / 10.0) < 0.01) {
        dialoguePosition = dialoguePositions["16:10_" + type];
    }
    return {(int) (dialoguePosition.val[0] * windowSize.width),
            (int) (dialoguePosition.val[1] * windowSize.height),
            (int) (dialoguePosition.val[2] * windowSize.width) - (int) (dialoguePosition.val[0] * windowSize.width),
            (int) (dialoguePosition.val[3] * windowSize.height) - (int) (dialoguePosition.val[1] * windowSize.height)};
}

bool IsPaimonSpeaking(const std::string &paimonName) {
    GetFrame(gwi.width, gwi.height);
    if (frame.empty())
        return false;

    cv::Rect defaultDialoguePos = GetDialogueRect(frame.size(), "DEFAULT");
    std::string defaultDialogue = GetTextFromImageByRect(frame, defaultDialoguePos);

    if (gwi.muteOverworld){
        cv::Rect overworldDialoguePos = GetDialogueRect(frame.size(), "OVERWORLD");
        std::string overworldDialogue = GetTextFromImageByRect(frame, overworldDialoguePos);
        return IsStringsSimilar(defaultDialogue, paimonName, gwi.maxOcrErrors) ||
               IsStringsSimilar(overworldDialogue, paimonName, gwi.maxOcrErrors);
    }
    return IsStringsSimilar(defaultDialogue, paimonName, gwi.maxOcrErrors);
}


bool IsGenshinProcess(DWORD pid) {
    WCHAR buff[1024];
    HANDLE handle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (GetProcessImageFileName(handle, buff, sizeof(buff))) {
        CloseHandle(handle);
        return std::wstring(&buff[0]).find(genshinExe) != std::wstring::npos;
    }
    CloseHandle(handle);
    return false;
}


HRESULT SetMuteGenshin(BOOL bMute) {
    IMMDeviceEnumerator *m_pEnumerator;
    IMMDevice *pDevice;
    CoInitialize(NULL);

    HRESULT hr = E_FAIL;
    hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void **) &m_pEnumerator);

    if (FAILED(hr))
        return hr;

    hr = m_pEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &pDevice);
    m_pEnumerator->Release();
    if (FAILED(hr))
        return hr;

    IAudioSessionManager2 *pasm = NULL;
    hr = pDevice->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, (void **) &pasm);
    pDevice->Release();
    if (FAILED(hr))
        return hr;

    IAudioSessionEnumerator *audio_session_enumerator;
    if (SUCCEEDED(pasm->GetSessionEnumerator(&audio_session_enumerator))) {
        int count;
        if (SUCCEEDED(audio_session_enumerator->GetCount(&count))) {
            for (int i = 0; i < count; i++) {
                IAudioSessionControl *audio_session_control;
                IAudioSessionControl2 *audio_session_control2;

                if (SUCCEEDED(audio_session_enumerator->GetSession(i, &audio_session_control))) {
                    if (SUCCEEDED(audio_session_control->QueryInterface(__uuidof(IAudioSessionControl2), (void **) &audio_session_control2))) {
                        DWORD processId;
                        if (SUCCEEDED(audio_session_control2->GetProcessId(&processId))) {
                            if (IsGenshinProcess(processId)) {
                                ISimpleAudioVolume *pSAV;
                                hr = audio_session_control2->QueryInterface(__uuidof(ISimpleAudioVolume), (void **) &pSAV);
                                if (SUCCEEDED(hr)) {
                                    hr = pSAV->SetMute(bMute, NULL);
                                    pSAV->Release();
                                }
                            }
                            audio_session_control->Release();
                            audio_session_control2->Release();
                        }
                    }
                }
            }
            audio_session_enumerator->Release();
        }
    }
    pasm->Release();
    return hr;
}


int PaimonShutUp() {
    std::map<std::string, std::string> configMap = ParseConfig("settings.cfg");
    gwi.windowName = convertStringToWstring(configMap["genshin_" + configMap["language"]]);
    DownloadTessdataFileIfNecessary(configMap["language"]);
    if (InitTesseract(nullptr, configMap["language"].c_str()))
        return 1;

    gwi.maxOcrErrors = std::stoi(configMap["ocr_max_errors"]);
    gwi.muteOverworld = configMap["mute_overworld"] == "1";
    gwi.captureMode = std::stoi(configMap["capture_mode"]);
    std::string paimonName = configMap["paimon_" + configMap["language"]];
    std::cout << "Waiting for GenshinImpact.exe process." << std::endl;
    bool paimonWasHere = false;
    while (!stop) {
        FindGenshinWindow();
        if (!gwi.active)
            continue;
        bool isPaimonSpeaking = IsPaimonSpeaking(paimonName);
        if (isPaimonSpeaking && !paimonWasHere) {
            paimonWasHere = true;
            SetMuteGenshin(isPaimonSpeaking);
            std::cout << "Paimon, shut up!" << std::endl;
        }
        if (!isPaimonSpeaking && paimonWasHere) {
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
            paimonWasHere = false;
            SetMuteGenshin(isPaimonSpeaking);
            std::cout << "Unmuting the game." << std::endl;
        }
    }
    DestroyTesseract();
    SetMuteGenshin(false);
    return 0;
}


int main() {
    try {
        signal(SIGINT, keyHand);
        SetPriorityClass(GetCurrentProcess(), BELOW_NORMAL_PRIORITY_CLASS);
        return PaimonShutUp();
    }
    catch (const std::runtime_error &re) {
        std::cerr << "Runtime error: " << re.what() << std::endl;
    }
    catch (const std::exception &ex) {
        std::cerr << "Error occurred: " << ex.what() << std::endl;
    }
    catch (...) {
        std::cerr << "Unknown failure occurred. Possible memory corruption" << std::endl;
    }
    DestroyTesseract();
    system("pause");
    return 1;
}

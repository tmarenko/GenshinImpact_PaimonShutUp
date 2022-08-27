#include "opencv2/core.hpp"
#include "opencv2/photo.hpp"
#include "windows.h"
#include <iostream>
#include "audioclient.h"
#include "audiopolicy.h"
#include "mmdeviceapi.h"
#include "psapi.h"
#include <csignal>
#include "tesseract.h"
#include "config.h"


typedef struct GenshinWindowInfo {
    std::wstring windowName = L"Genshin Impact";
    std::wstring windowClass = L"UnityWndClass";
    HWND hwnd = nullptr;
    int width = 0;
    int height = 0;
    bool active = false;
} GenshinWindowInfo;

volatile sig_atomic_t stop;

void keyHand(int signum) {
    stop = 1;
}


const cv::Scalar DEFAULT_DIALOGUE_NAME_POS = {0.4631496915663028, 0.7872131016037641, 0.536658108769003, 0.8286028539255994};
const cv::Scalar OUT_DIALOGUE_NAME_POS = {0.4645968857034299, 0.7582338162046451, 0.5333868470215386, 0.8020998784944536};
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

    BitBlt(saveDC, 0, 0, screenWidth, screenHeight, hwndDC, 0, 0, SRCCOPY);
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


bool IsPaimonSpeaking(const std::string &paimonName) {
    GetFrame(gwi.width, gwi.height);
    if (frame.empty())
        return false;
    cv::Rect cropDefault = cv::Rect((int) (DEFAULT_DIALOGUE_NAME_POS.val[0] * frame.cols),
                                    (int) (DEFAULT_DIALOGUE_NAME_POS.val[1] * frame.rows),
                                    (int) (DEFAULT_DIALOGUE_NAME_POS.val[2] * frame.cols) - (int) (DEFAULT_DIALOGUE_NAME_POS.val[0] * frame.cols),
                                    (int) (DEFAULT_DIALOGUE_NAME_POS.val[3] * frame.rows) - (int) (DEFAULT_DIALOGUE_NAME_POS.val[1] * frame.rows));
    cv::Rect cropOut = cv::Rect((int) (OUT_DIALOGUE_NAME_POS.val[0] * frame.cols),
                                (int) (OUT_DIALOGUE_NAME_POS.val[1] * frame.rows),
                                (int) (OUT_DIALOGUE_NAME_POS.val[2] * frame.cols) - (int) (OUT_DIALOGUE_NAME_POS.val[0] * frame.cols),
                                (int) (OUT_DIALOGUE_NAME_POS.val[3] * frame.rows) - (int) (OUT_DIALOGUE_NAME_POS.val[1] * frame.rows));
    std::string defaultDialogue = GetTextFromImageByRect(frame, cropDefault);
    std::string outDialogue = GetTextFromImageByRect(frame, cropOut);
    return IsStringsSimilar(defaultDialogue, paimonName) || IsStringsSimilar(outDialogue, paimonName);
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
            std::cout << "Paimon, shut up!" << std::endl;
            SetMuteGenshin(isPaimonSpeaking);
        }
        if (!isPaimonSpeaking && paimonWasHere) {
            paimonWasHere = false;
            std::cout << "Unmuting the game." << std::endl;
            SetMuteGenshin(isPaimonSpeaking);
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

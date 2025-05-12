#include "image.h"
#include "windows.h"
#include <iostream>
#include "audio_session.h"
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
    DWORD processId = 0;
    AudioSession audioSession;
} GenshinWindowInfo;

volatile sig_atomic_t stop;

void keyHand(int signum) {
    stop = 1;
}


struct DialoguePosition {
    double x1, y1, x2, y2;
};

std::map<std::string, DialoguePosition> dialoguePositions = {
        {"16:9_DEFAULT",    {0.463, 0.787, 0.537, 0.829}},
        {"16:9_OVERWORLD",  {0.465, 0.758, 0.533, 0.802}},
        {"16:10_DEFAULT",   {0.461, 0.809, 0.538, 0.841}},
        {"16:10_OVERWORLD", {0.462, 0.780, 0.538, 0.815}},
};

const Color DIALOGUE_NAME_COLOR_RANGE_LOW(230, 170, 0);   // RGB
const Color DIALOGUE_NAME_COLOR_RANGE_HIGH(255, 210, 10); // RGB

Image frame;
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

    WCHAR buff[1024]{};
    GetWindowText(gwi.hwnd, buff, 100);
    if (!lstrcmp(buff, gwi.windowName.c_str())) {
        if (gwi.hwnd && !gwi.active) {
            DWORD processId;
            GetWindowThreadProcessId(gwi.hwnd, &processId);
            gwi.processId = processId;
            gwi.active = true;
            if (gwi.audioSession.initialize() && gwi.audioSession.findProcessSession(gwi.processId)) {
                std::cout << "Audio session initialized." << std::endl;
            }
            std::cout << "Ready for Paimon!" << std::endl;
        }
    } else {
        if (gwi.active) {
            gwi.active = false;
            gwi.hwnd = nullptr;
            gwi.processId = 0;
            gwi.audioSession.cleanup();
            std::cout << "Game was closed." << std::endl;
        }
    }
}


void GetFrame(int screenWidth, int screenHeight) {
    RECT clientRect;
    GetClientRect(gwi.hwnd, &clientRect);
    RECT windowRect;
    GetWindowRect(gwi.hwnd, &windowRect);
    int borderSize = ((windowRect.right - windowRect.left) - (clientRect.right - clientRect.left)) / 2;
    int titleBarSize = ((windowRect.bottom - windowRect.top) - (clientRect.bottom - clientRect.top)) - borderSize;
    screenHeight += titleBarSize;

    if (screenHeight <= 0 || screenWidth <= 0)
        return;

    if (frame.height() != screenHeight || frame.width() != screenWidth || frame.channels() != 4) {
        frame.create(screenHeight, screenWidth, 4);
    }

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

    auto hwndDC = GetWindowDC(gwi.hwnd);
    auto saveDC = CreateCompatibleDC(hwndDC);

    auto bitMap = CreateCompatibleBitmap(hwndDC, screenWidth, screenHeight);
    SelectObject(saveDC, bitMap);

    switch (gwi.captureMode) {
        case 0:
            BitBlt(saveDC, 0, 0, screenWidth, screenHeight, hwndDC, 0, 0, SRCCOPY);
            break;
        case 1:
            PrintWindow(gwi.hwnd, saveDC, PW_RENDERFULLCONTENT);
            break;
        default:
            std::cout << "Invalid capture mode, reverting to default." << std::endl;
            gwi.captureMode = 1;
            PrintWindow(gwi.hwnd, saveDC, PW_RENDERFULLCONTENT);
            break;
    }
    GetDIBits(saveDC, bitMap, 0, screenHeight - titleBarSize, frame.data(), (BITMAPINFO *) &bi, DIB_RGB_COLORS);

    DeleteObject(bitMap);
    DeleteDC(saveDC);
    ReleaseDC(gwi.hwnd, hwndDC);
}


std::string GetTextFromImageByRect(const Image &image, const Rect &rect) {
    if (image.empty())
        return std::string();
    Image thresholdImage;
    Image::cropAndThreshold(image, rect, DIALOGUE_NAME_COLOR_RANGE_LOW, DIALOGUE_NAME_COLOR_RANGE_HIGH, thresholdImage);
    return GetTextFromImage(thresholdImage);
}


Rect GetDialogueRect(const Size &windowSize, const std::string &type) {
    double aspectRatio = static_cast<double>(windowSize.width) / windowSize.height;
    auto dialoguePosition = dialoguePositions["16:9_" + type];

    if (std::abs(aspectRatio - 16.0 / 10.0) < 0.01) {
        dialoguePosition = dialoguePositions["16:10_" + type];
    }
    return Rect(
            (int) (dialoguePosition.x1 * windowSize.width),
            (int) (dialoguePosition.y1 * windowSize.height),
            (int) (dialoguePosition.x2 * windowSize.width) - (int) (dialoguePosition.x1 * windowSize.width),
            (int) (dialoguePosition.y2 * windowSize.height) - (int) (dialoguePosition.y1 * windowSize.height)
    );
}

bool IsPaimonSpeaking(const std::string &paimonName) {
    GetFrame(gwi.width, gwi.height);
    if (frame.empty())
        return false;

    Rect defaultDialoguePos = GetDialogueRect(frame.size(), "DEFAULT");
    std::string defaultDialogue = GetTextFromImageByRect(frame, defaultDialoguePos);

    if (gwi.muteOverworld) {
        Rect overworldDialoguePos = GetDialogueRect(frame.size(), "OVERWORLD");
        std::string overworldDialogue = GetTextFromImageByRect(frame, overworldDialoguePos);
        return IsStringsSimilar(defaultDialogue, paimonName, gwi.maxOcrErrors) ||
               IsStringsSimilar(overworldDialogue, paimonName, gwi.maxOcrErrors);
    }
    return IsStringsSimilar(defaultDialogue, paimonName, gwi.maxOcrErrors);
}


void SetMuteGenshin(bool bMute) {
    if (!gwi.audioSession.isSessionValid()) {
        if (!(gwi.audioSession.initialize() && gwi.audioSession.findProcessSession(gwi.processId))) {
            std::cout << "Failed to find Genshin Impact audio session." << std::endl;
            return;
        }
        std::cout << "Audio session reinitialized." << std::endl;
    }

    gwi.audioSession.setMute(bMute);
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
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }
    return 0;
}

BOOL WINAPI ExitHandler(DWORD ctrlType) {
    if (ctrlType == CTRL_CLOSE_EVENT || ctrlType == CTRL_SHUTDOWN_EVENT) {
        gwi.audioSession.setMute(false);
        DestroyTesseract();
        return TRUE;
    }
    return FALSE;
}

int main() {
    try {
        signal(SIGINT, keyHand);
        SetConsoleCtrlHandler(ExitHandler, TRUE);
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

#include "audio_session.h"
#include <stdexcept>
#include <utility>

AudioSession::AudioSession()
{
    comInitialized = SUCCEEDED(CoInitialize(nullptr));
    if (!comInitialized)
    {
        throw std::runtime_error("Failed to initialize COM");
    }
}

AudioSession::~AudioSession()
{
    setMute(false);
    cleanup();
    if (comInitialized)
    {
        CoUninitialize();
    }
}

void AudioSession::cleanup()
{
    pVolumeControl.reset();
    pAudioControl.reset();
    pSessionManager.reset();
    pDevice.reset();
    pEnumerator.reset();
    sessionValid = false;
}

bool AudioSession::initialize()
{
    cleanup();

    IMMDeviceEnumerator *enumTemp = nullptr;
    if (FAILED(CoCreateInstance(
        __uuidof(MMDeviceEnumerator),
        nullptr,
        CLSCTX_ALL,
        __uuidof(IMMDeviceEnumerator),
        reinterpret_cast<void **>(&enumTemp))))
    {
        std::cerr << "Failed to create MMDeviceEnumerator instance." << std::endl;
        return false;
    }
    pEnumerator.reset(enumTemp);

    return getDefaultAudioDevice();
}

bool AudioSession::getDefaultAudioDevice()
{
    if (!pEnumerator)
    {
        std::cerr << "Enumerator is not initialized." << std::endl;
        return false;
    }

    IMMDevice *deviceTemp = nullptr;

    if (FAILED(reinterpret_cast<IMMDeviceEnumerator *>(pEnumerator.get())->GetDefaultAudioEndpoint(eRender, eConsole, &deviceTemp)))
    {
        std::cerr << "Failed to get default audio endpoint." << std::endl;
        return false;
    }
    pDevice.reset(deviceTemp);

    IAudioSessionManager2 *sessionMgrTemp = nullptr;

    if (FAILED(reinterpret_cast<IMMDevice *>(pDevice.get())->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, nullptr, reinterpret_cast<void **>(&sessionMgrTemp))))
    {
        std::cerr << "Failed to activate audio session manager." << std::endl;
        return false;
    }
    pSessionManager.reset(sessionMgrTemp);

    return true;
}

bool AudioSession::findProcessSession(DWORD processId)
{
    if (!pSessionManager && !initialize())
    {
        std::cerr << "Failed to initialize audio session manager." << std::endl;
        return false;
    }

    IAudioSessionEnumerator *enumerator = nullptr;

    if (FAILED(reinterpret_cast<IAudioSessionManager2 *>(pSessionManager.get())->GetSessionEnumerator(&enumerator)))
    {
        std::cerr << "Failed to get session enumerator." << std::endl;
        return false;
    }

    std::unique_ptr<IAudioSessionEnumerator, ComDeleter> enumPtr(enumerator);
    int sessionCount;

    if (FAILED(enumerator->GetCount(&sessionCount)))
    {
        std::cerr << "Failed to get session count." << std::endl;
        return false;
    }

    for (int i = 0; i < sessionCount; i++)
    {
        IAudioSessionControl *sessionControl = nullptr;

        if (FAILED(enumerator->GetSession(i, &sessionControl)))
        {
            continue;
        }

        std::unique_ptr<IAudioSessionControl, ComDeleter> sessionPtr(sessionControl);
        IAudioSessionControl2 *sessionControl2 = nullptr;
        if (FAILED(sessionControl->QueryInterface(__uuidof(IAudioSessionControl2), reinterpret_cast<void **>(&sessionControl2))))
        {
            continue;
        }

        std::unique_ptr<IAudioSessionControl2, ComDeleter> session2Ptr(sessionControl2);
        DWORD sessionProcessId;
        if (SUCCEEDED(sessionControl2->GetProcessId(&sessionProcessId)) && sessionProcessId == processId)
        {
            ISimpleAudioVolume *volumeControl = nullptr;
            if (SUCCEEDED(sessionControl2->QueryInterface(__uuidof(ISimpleAudioVolume), reinterpret_cast<void **>(&volumeControl))))
            {
                pAudioControl = std::move(session2Ptr);
                pVolumeControl.reset(volumeControl);
                sessionValid = true;
                return true;
            }
        }
    }

    sessionValid = false;
    return false;
}

bool AudioSession::setMute(bool mute)
{
    if (!sessionValid || !pVolumeControl)
    {
        return false;
    }

    return SUCCEEDED(reinterpret_cast<ISimpleAudioVolume *>(pVolumeControl.get())->SetMute(mute, nullptr));
}

bool AudioSession::isSessionValid() const
{
    if (!sessionValid || !pAudioControl)
    {
        return false;
    }

    AudioSessionState state;
    return SUCCEEDED(reinterpret_cast<IAudioSessionControl2 *>(pAudioControl.get())->GetState(&state)) && state != AudioSessionStateExpired;
}

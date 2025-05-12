#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <windows.h>
#include <mmdeviceapi.h>
#include <audiopolicy.h>

class AudioSession
{
public:
    AudioSession();

    ~AudioSession();

    bool initialize();

    void cleanup();

    bool findProcessSession(DWORD processId);

    bool setMute(bool mute);

    bool isSessionValid() const;

private:
    bool getDefaultAudioDevice();

    bool comInitialized = false;

    // Smart pointers for COM interfaces
    struct ComDeleter
    {
        template <typename T>
        void operator()(T *ptr)
        {
            if (ptr)
                ptr->Release();
        }
    };

    using ComPtr = std::unique_ptr<IUnknown, ComDeleter>;

    ComPtr pEnumerator;     // IMMDeviceEnumerator
    ComPtr pDevice;         // IMMDevice
    ComPtr pSessionManager; // IAudioSessionManager2
    ComPtr pAudioControl;   // IAudioSessionControl2
    ComPtr pVolumeControl;  // ISimpleAudioVolume

    bool sessionValid = false;
};

#pragma once

#define WIN32_MAX_PATH MAX_PATH

struct win32_state
{
    void* gameMemoryBlock;
    size_t totalSize;
    HANDLE PlaybackHandle;
    HANDLE RecordingHandle;
    int InputRecordingIndex = 0;
    int InputPlaybackIndex = 0;
    const char* filename = "temp.rec";
    char EXEFileName[WIN32_MAX_PATH];
    char *OnePastLastEXEFileNameSlash;
};

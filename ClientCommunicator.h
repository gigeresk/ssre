#ifndef SSRE_CLIENTCOMMUNICATOR_H
#define SSRE_CLIENTCOMMUNICATOR_H

// Win32 shared-memory + semaphore channel to the Unity client (ENGINE mode only).

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

#include <cassert>
#include <cstring>
#include <string>

class SynchronizedUpscaleBuffer
{
private:
    HANDLE inputReady  = nullptr;
    HANDLE outputReady = nullptr;

    HANDLE inputFileMap = nullptr;
    long   inputBufferSize = 0;
    char*  inputMappedBuffer = nullptr;

    HANDLE outputFileMap = nullptr;
    long   outputBufferSize = 0;
    char*  outputMappedBuffer = nullptr;

    bool createdNewObjects = false; // help determine if I should clean up

    // Explicit *A functions: names are narrow strings regardless of whether
    // the project defines UNICODE (CMake's Visual Studio generators do not, but
    // the IDE's default project templates do).
    void OpenExisting(const std::string& inputBufferName,
                      const std::string& outputBufferName,
                      const std::string& inputSemName,
                      const std::string& outputSemName)
    {
        inputFileMap  = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, inputBufferName.c_str());
        outputFileMap = OpenFileMappingA(FILE_MAP_ALL_ACCESS, FALSE, outputBufferName.c_str());

        inputReady  = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, inputSemName.c_str());
        outputReady = OpenSemaphoreA(SEMAPHORE_ALL_ACCESS, FALSE, outputSemName.c_str());

        createdNewObjects = false;
    }

    void CreateNew(const std::string& inputBufferName,
                   const std::string& outputBufferName,
                   const std::string& inputSemName,
                   const std::string& outputSemName)
    {
        inputFileMap = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                          0, static_cast<DWORD>(inputBufferSize),
                                          inputBufferName.c_str());
        outputFileMap = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE,
                                           0, static_cast<DWORD>(outputBufferSize),
                                           outputBufferName.c_str());

        inputReady  = CreateSemaphoreA(nullptr, 0, 1, inputSemName.c_str());
        outputReady = CreateSemaphoreA(nullptr, 0, 1, outputSemName.c_str());
        createdNewObjects = true;
    }

public:
    SynchronizedUpscaleBuffer(const std::string& name, long inputSize, long outputSize)
    {
        const std::string inputBufferName  = name + "_inputbuffer";
        const std::string outputBufferName = name + "_outputbuffer";
        const std::string inputSemName     = name + "_inputsem";
        const std::string outputSemName    = name + "_outputsem";

        inputBufferSize  = inputSize;
        outputBufferSize = outputSize;

        OpenExisting(inputBufferName, outputBufferName, inputSemName, outputSemName);

        inputMappedBuffer = static_cast<char*>(
            MapViewOfFile(inputFileMap, FILE_MAP_ALL_ACCESS, 0, 0, inputBufferSize));
        outputMappedBuffer = static_cast<char*>(
            MapViewOfFile(outputFileMap, FILE_MAP_ALL_ACCESS, 0, 0, outputBufferSize));

        assert(inputMappedBuffer != nullptr);
        assert(outputMappedBuffer != nullptr);
        assert(outputReady != nullptr);
        assert(inputReady != nullptr);
    }

    SynchronizedUpscaleBuffer(const SynchronizedUpscaleBuffer&) = delete;
    SynchronizedUpscaleBuffer& operator=(const SynchronizedUpscaleBuffer&) = delete;

    ~SynchronizedUpscaleBuffer()
    {
        UnmapViewOfFile(inputMappedBuffer);
        UnmapViewOfFile(outputMappedBuffer);

        if (createdNewObjects)
        {
            CloseHandle(inputFileMap);
            CloseHandle(outputFileMap);
            CloseHandle(inputReady);
            CloseHandle(outputReady);
        }
    }

    void Write(const char* bytes, long numBytes)
    {
        std::memcpy(outputMappedBuffer, bytes, numBytes);
        ReleaseSemaphore(outputReady, 1, nullptr);
    }

    void Read(char* bytes, long numBytesToRead)
    {
        WaitForSingleObject(inputReady, INFINITE);
        std::memcpy(bytes, inputMappedBuffer, numBytesToRead);
    }
};

class ClientCommunicator
{
private:
    SynchronizedUpscaleBuffer buffer;

    long  outputBufferSize;

    char* inputBytes;
    long  inputBufferSize;

public:
    // Takes ownership of nnInputBytes (allocated with new[]).
    ClientCommunicator(long inputBufferSize, long outputBufferSize, char* nnInputBytes)
        : buffer("ssre", inputBufferSize, outputBufferSize),
          outputBufferSize(outputBufferSize),
          inputBytes(nnInputBytes),
          inputBufferSize(inputBufferSize)
    {
    }

    ClientCommunicator(const ClientCommunicator&) = delete;
    ClientCommunicator& operator=(const ClientCommunicator&) = delete;

    ~ClientCommunicator()
    {
        delete[] inputBytes;
    }

    // Blocking: wait for the client to submit a frame, then copy it out.
    void GetSRInputs()
    {
        buffer.Read(inputBytes, inputBufferSize);
    }

    // Non-blocking: publish the upscaled frame and signal the client.
    void ReturnSROutputs(const char* nnOutputBytes)
    {
        buffer.Write(nnOutputBytes, outputBufferSize);
    }
};

#endif // SSRE_CLIENTCOMMUNICATOR_H

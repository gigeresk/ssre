#ifndef __CLIENTCOMMUNICATOR_H
#define __CLIENTCOMMUNICATOR_H

#include <Windows.h>
#include <string>
#include <cassert>
#include <iostream>
using namespace std;

class SynchronizedUpscaleBuffer
{
private:
    HANDLE inputReady;
    HANDLE outputReady;

    HANDLE inputFileMap;
    long inputBufferSize;
    LPTCH inputMappedBuffer;

    HANDLE outputFileMap;
    long outputBufferSize;
    LPTCH outputMappedBuffer;

    bool createdNewObjects; // help determine if I should clean up

    void OpenExisting(
        string& inputBufferName,
        string& outputBufferName,
        string& inputSemName,
        string& outputSemName
    )
    {
        inputFileMap     = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, inputBufferName.c_str());
        outputFileMap    = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, outputBufferName.c_str());

        inputReady       = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, inputSemName.c_str());
        outputReady      = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, outputSemName.c_str());

        createdNewObjects = false;
    }

    void CreateNew(string& inputBufferName, string& outputBufferName, string& inputSemName, string& outputSemName)
    {
        inputFileMap = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            inputBufferSize,
            inputBufferName.c_str()
        );
        outputFileMap = CreateFileMappingA(
            INVALID_HANDLE_VALUE,
            NULL,
            PAGE_READWRITE,
            0,
            outputBufferSize,
            outputBufferName.c_str()
        );

        inputReady  = CreateSemaphoreA(NULL, 0, 1, inputSemName.c_str());
        outputReady = CreateSemaphoreA(NULL, 0, 1, outputSemName.c_str());
        createdNewObjects = true;
    }

public:
    SynchronizedUpscaleBuffer(string name, long inputSize, long outputSize)
    {
        string inputBufferName  = name + "_inputbuffer";
        string outputBufferName = name + "_outputbuffer";
        string inputSemName     = name + "_inputsem";
        string outputSemName    = name + "_outputsem";

        inputBufferSize  = inputSize;
        outputBufferSize = outputSize;

        OpenExisting(inputBufferName, outputBufferName, inputSemName, outputSemName);

        inputMappedBuffer = (LPTCH)MapViewOfFile(
            inputFileMap,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            inputBufferSize
        );

        outputMappedBuffer = (LPTCH)MapViewOfFile(
            outputFileMap,
            FILE_MAP_ALL_ACCESS,
            0,
            0,
            outputBufferSize
        );

        assert(inputMappedBuffer != NULL);
        assert(outputMappedBuffer != NULL);
        assert(outputReady != NULL);
        assert(inputReady != NULL);
    }

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

    void Write(LPTCH bytes, int numBytes)
    {
        memcpy(outputMappedBuffer, bytes, numBytes);
        ReleaseSemaphore(outputReady, 1, NULL);
    }

    void Read(LPTCH bytes, int numBytesToRead)
    {
        WaitForSingleObject(inputReady, INFINITE);
        memcpy(bytes, inputMappedBuffer, numBytesToRead);
    }
};

class ClientCommunicator
{
private:
    SynchronizedUpscaleBuffer buffer;

    LONG  outputBufferSize;

    LPTCH inputBytes;
    LONG  inputBufferSize;
public:
    ClientCommunicator(LONG inputBufferSize, LONG outputBufferSize, LPTCH nnInputBytes)
        : buffer("ssre", inputBufferSize, outputBufferSize)
    {
        inputBytes = nnInputBytes;
        this->inputBufferSize = inputBufferSize;
        this->outputBufferSize = outputBufferSize;
    }

    ~ClientCommunicator()
    {
        delete[] inputBytes;
    }

    // Blocking method: Submit data to NN and populate NN output buffer with received data
    void GetSRInputs()
    {
        buffer.Read(inputBytes, inputBufferSize);
    }

    // This method will not block: reason is if we read, we will need to write
    void ReturnSROutputs(LPTCH nnOutputBytes)
    {
        buffer.Write(nnOutputBytes, outputBufferSize);
    }
};

#endif // !__CLIENTCOMMUNICATOR_H
#ifndef __CLIENTCOMMUNICATOR_H
#define __CLIENTCOMMUNICATOR_H

#include <Windows.h>
#include <string>
#include <cassert>
#include <iostream>
using namespace std;

class MutexSharedBuffer
{
private:
    HANDLE fileMap;
    HANDLE dataRead;
    HANDLE dataWritten;
    LPTCH mappedBuffer;

    long bufferSize;

    bool createdNewObjects;

    void OpenExisting(string& bufferName, string& rSemName, string& wSemName)
    {
        fileMap     = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, bufferName.c_str());
        dataRead    = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, rSemName.c_str());
        dataWritten = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, wSemName.c_str());
        createdNewObjects = false;
    }

    void CreateNew(string& bufferName, string& rSemName, string& wSemName)
    {
        fileMap     = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, bufferSize, bufferName.c_str());
        dataRead    = CreateSemaphoreA(NULL, 1, 1, rSemName.c_str()); // wait for first w before r
        dataWritten = CreateSemaphoreA(NULL, 0, 1, wSemName.c_str()); // w right away
        createdNewObjects = true;
    }

public:
    MutexSharedBuffer(string name, long size)
    {
        bufferSize = size;
        string bufferName = name + "_buffer";
        string rSemName = name   + "_rsem";
        string wSemName = name   + "_wsem";

        OpenExisting(bufferName, rSemName, wSemName);

        mappedBuffer = (LPTCH)MapViewOfFile(fileMap, FILE_MAP_ALL_ACCESS, 0, 0, bufferSize);

        assert(fileMap      != NULL);
        assert(mappedBuffer != NULL);
        assert(dataWritten  != NULL);
        assert(dataRead     != NULL);

    }

    ~MutexSharedBuffer()
    {
        UnmapViewOfFile(mappedBuffer);
        if (createdNewObjects)
        {
            CloseHandle(fileMap);
            CloseHandle(dataRead);
            CloseHandle(dataWritten);
        }
    }

    void Write(LPTCH bytes, int numBytes)
    {
        WaitForSingleObject(dataRead, INFINITE);

        memcpy(mappedBuffer, bytes, numBytes);

        ReleaseSemaphore(dataWritten, 1, NULL);
    }

    void Read(LPTCH bytes, int numBytesToRead)
    {
        WaitForSingleObject(dataWritten, INFINITE);

        memcpy(bytes, mappedBuffer, numBytesToRead);

        ReleaseSemaphore(dataRead, 1, NULL);
    }
};

class ClientCommunicator
{
private:
    MutexSharedBuffer* inputSharedBuffer;
    MutexSharedBuffer* outputSharedBuffer;

    LONG  outputBufferSize;

    LPTCH inputBytes;
    LONG  inputBufferSize;
public:
    ClientCommunicator(LONG inputBufferSize, LONG outputBufferSize, LPTCH nnInputBytes)
    {
        inputBytes = nnInputBytes;
        this->inputBufferSize = inputBufferSize;
        this->outputBufferSize = outputBufferSize;

        
        inputSharedBuffer = new MutexSharedBuffer("ssreInput", inputBufferSize);
        outputSharedBuffer = new MutexSharedBuffer("ssreOutput", outputBufferSize);
    }

    ~ClientCommunicator()
    {
        delete[] inputBytes;
    }

    // Blocking method: Submit data to NN and populate NN output buffer with received data
    void GetSRInputs()
    {
        inputSharedBuffer->Read(inputBytes, inputBufferSize);
        
    }

    void ReturnSROutputs(LPTCH nnOutputBytes)
    {
        outputSharedBuffer->Write(nnOutputBytes, outputBufferSize);
    }
};

#endif // !__CLIENTCOMMUNICATOR_H
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
    HANDLE mutex;
    HANDLE newDataSem;
    LPTCH mappedBuffer;

    long bufferSize;

    void OpenExisting(string& mutexName, string& bufferName, string& semaphoreName)
    {
        fileMap = OpenFileMapping(FILE_MAP_ALL_ACCESS, FALSE, bufferName.c_str());
        mutex = OpenMutex(MUTEX_ALL_ACCESS, FALSE, mutexName.c_str());
        std::cout << GetLastError() << std::endl;
        newDataSem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, semaphoreName.c_str());
    }

    void CreateNew(string& mutexName, string& bufferName, string& semaphoreName)
    {
        fileMap = CreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, bufferSize, bufferName.c_str());
        mutex = CreateMutex(NULL, FALSE, mutexName.c_str());
        std::cout << GetLastError() << std::endl;
        newDataSem = CreateSemaphoreA(NULL, 0, 1, semaphoreName.c_str());
    }

public:
    MutexSharedBuffer(string name, long size)
    {
        bufferSize = size;
        string mutexName = name + "_mutex";
        string bufferName = name + "_buffer";
        string semaphoreName = name + "_sem";
        OpenExisting(mutexName, bufferName, semaphoreName);

        mappedBuffer = (LPTCH)MapViewOfFile(fileMap, FILE_MAP_ALL_ACCESS, 0, 0, bufferSize);

        assert(fileMap != NULL);
        assert(mappedBuffer != NULL);
        assert(mutex != NULL);
        assert(newDataSem != NULL);
    }

    ~MutexSharedBuffer()
    {
        UnmapViewOfFile(mappedBuffer);
        CloseHandle(fileMap);
        CloseHandle(mutex);
        CloseHandle(newDataSem);
    }

    void Write(LPTCH bytes, int numBytes)
    {
        WaitForSingleObject(mutex, INFINITE);

        memcpy(mappedBuffer, bytes, numBytes);

        ReleaseMutex(mutex);

        ReleaseSemaphore(newDataSem, 1, NULL);
    }

    void Read(LPTCH bytes, int numBytesToRead)
    {
        WaitForSingleObject(newDataSem, INFINITE);

        WaitForSingleObject(mutex, INFINITE);

        memcpy(bytes, mappedBuffer, numBytesToRead);

        ReleaseMutex(mutex);
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
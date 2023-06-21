#pragma once
#include <fstream>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <iostream>
#include <mutex>
#include <pthread.h>

#include "BlockQueue.h"

enum LogLevel {
	INFO,WARN,ERROR,FATAL
};

struct LogData {
	std::string Time, Str;
	int Level;
};

class Log
{
private:
	BlockQueue<LogData> LogQueue;
	pthread_t tWriterThread;
	std::ofstream LogFileStream;
	std::mutex LogQueueLock;
	std::string LogFileName;
	bool isWriteFile, isExit;
	std::string GetTime();
	int MoveOldLog(int index);
	static void* WriterThread(void* vParam);

public:
	Log();
	void OpenFile(std::string FileName);
	void Close();
	void WriteLog(int Level, std::string Log);
};
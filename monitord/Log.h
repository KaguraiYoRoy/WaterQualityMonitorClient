#pragma once
#include <fstream>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <iostream>

enum LogLevel {
	INFO,WARN,ERROR,FATAL
};

class Log
{
private:
	std::ofstream LogFileStream;
	std::string LogFileName;
	bool isWriteFile;
	std::string GetTime();
	int MoveOldLog(int index);

public:
	Log();
	~Log();
	void OpenFile(std::string FileName);
	void Close();
	void WriteLog(int Level, std::string Log);
};
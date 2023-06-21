#include "Log.h"

Log::Log() {
	isExit = false;
	isWriteFile = false;
	pthread_create(&tWriterThread, NULL, WriterThread, this);
}

std::string Log::GetTime()
{
	time_t timep;
	time(&timep);
	char tmp[64];
	strftime(tmp, sizeof(tmp), "%Y-%m-%d %H:%M:%S", localtime(&timep));
	return tmp;
}

int Log::MoveOldLog(int index) {
	char NameBuf[1024],TargetBuf[1024];
	int sum = 0;
	if(index == 0) {
		sprintf(NameBuf, "%s", LogFileName.c_str());
		sprintf(TargetBuf, "%s.001", LogFileName.c_str());
	}
	else {
		sprintf(NameBuf, "%s.%03d", LogFileName.c_str(), index);
		sprintf(TargetBuf, "%s.%03d", LogFileName.c_str(), index+1);
	}

	if (access(TargetBuf, F_OK) == 0)
		sum = MoveOldLog(index + 1);
	
	rename(NameBuf, TargetBuf);

	return sum + 1;
}

void* Log::WriterThread(void* vParam)
{
	LogData data;
	Log* pClass = (Log*)vParam;
	while (!pClass->isExit) {
		data = pClass->LogQueue.Take();
		char levelchar;
		char Buffer[1024];
		switch (data.Level) {
		case INFO: {
			levelchar = 'I';
			break;
		}
		case WARN: {
			levelchar = 'W';
			break;
		}
		case ERROR: {
			levelchar = 'E';
			break;
		}
		case FATAL: {
			levelchar = 'F';
			break;
		}
		}
		sprintf(Buffer, "[%s][%c]:%s\n", data.Time.c_str(), levelchar, data.Str.c_str());
		if (pClass->isWriteFile)pClass->LogFileStream << Buffer;
		std::cout << Buffer;
	}
	return nullptr;
}

void Log::OpenFile(std::string FileName) {
	int nExistLog = 0;
	isWriteFile = true;
	LogFileName = FileName;
	if(access(FileName.c_str(),F_OK) == 0)
		nExistLog = MoveOldLog(0);
	LogFileStream.open(FileName.c_str(), std::ios::out);
	if (nExistLog >= 10) {
		char buffer[64];
		sprintf(buffer, "%d log files already exist. Please clean up in time.", nExistLog);
		WriteLog(WARN, buffer);
	}
}

void Log::Close() {
	if(isWriteFile)
		LogFileStream.close();
	isExit = true;
	WriteLog(INFO, "Exit.");
	pthread_join(tWriterThread, nullptr);
}

void Log::WriteLog(int Level, std::string LogStr) {
	LogQueueLock.lock();
	LogQueue.Put({ GetTime(),LogStr,Level });
	LogQueueLock.unlock();
}

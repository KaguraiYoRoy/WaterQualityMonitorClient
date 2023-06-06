#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>

#include "Log.h"

Log Logger;

void* CronTimer(void*);
void* UploadTimer(void*);
void* CommandProcesser(void*);

int main() {
    Logger.OpenFile("monitor.log");
    Logger.WriteLog(INFO, "Water Quality MonitorDaemon Starting...");

    

    Logger.Close();
    return 0;
}

void* CronTimer(void*) {

    return 0;
}

void* UploadTimer(void*)
{
    return 0;
}

void* CommandProcesser(void*)
{
    return 0;
}

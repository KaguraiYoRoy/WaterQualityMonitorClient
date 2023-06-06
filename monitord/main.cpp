#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <wiringPi.h>
#include <wiringSerial.h>
#include <json/json.h>

#include "Log.h"
#include "BlockQueue.h"

Log Logger;
pthread_t tCronTimer, tUploadTimer, tCommandProcesser;

bool isExit;

static void SigHandler(int sig);
void* CronTimer(void*);
void* UploadTimer(void*);
void* CommandProcesser(void*);

int main() {
    Logger.OpenFile("monitor.log");
    Logger.WriteLog(INFO, "Water Quality MonitorDaemon starting...");

    signal(SIGINT, SigHandler);
    isExit = false;

    pthread_create(&tCronTimer, NULL, CronTimer, NULL);
    pthread_create(&tUploadTimer, NULL, UploadTimer, NULL);
    pthread_create(&tCommandProcesser, NULL, CommandProcesser, NULL);

    Logger.WriteLog(INFO, "Program exiting. Please wait...");

    Logger.Close();
    return 0;
}

static void SigHandler(int sig) {
    switch (sig)
    {
    case SIGINT: {
        isExit = true;
        break;
    }
    default:
        break;
    }
}

void* CronTimer(void*) {
    while (!isExit) {

        sleep(10);
    }
    return 0;
}

void* UploadTimer(void*) {
    while (!isExit) {

    }
    return 0;
}

void* CommandProcesser(void*) {
    while (!isExit) {

    }
    return 0;
}

#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>

#include <wiringPi.h>
#include <wiringSerial.h>
#include <curl/curl.h>
#include <json/json.h>

#include "Log.h"
#include "BlockQueue.h"
#include "clipp.h"

Log Logger;
pthread_t tCronTimer, tUploadTimer, tCommandProcesser;
std::string Configfile, Logfile;
bool isExit;

static void SigHandler(int sig);
void* CronTimer(void*);
void* UploadTimer(void*);
void* CommandProcesser(void*);

int main(int argc, char* argv[]) {
    Logfile = "";

    auto cli = (
        clipp::value("Config file", Configfile),
        clipp::option("-l", "--log") & clipp::value("log file", Logfile)
        );

    if (!clipp::parse(argc, argv, cli)) {
        std::cout << clipp::make_man_page(cli, argv[0]);
        return 0;
    }

    std::ifstream ConfigReaderStream;
    ConfigReaderStream.open(Configfile);
    Json::Reader JsonReader;
    Json::Value JsonConfigRoot;
    if (!JsonReader.parse(ConfigReaderStream, JsonConfigRoot)) {
        std::cout << "FATAL: Failed to load Config file!" << std::endl;
        return 0;
    }
        
    if (Logfile == (std::string)"") {
        if (JsonConfigRoot.isMember("LogFile")) {
            Logfile = JsonConfigRoot["LogFile"].asString();
        }
        else {
            std::cout << "FATAL: \"LogFile\" doesn't set in Config File!" << std::endl;
            return 0;
        }
    }

    Logger.OpenFile(Logfile);
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

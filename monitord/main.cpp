#include <cstdio>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <assert.h>

#include <wiringPi.h>
#include <wiringSerial.h>
#include <curl/curl.h>
#include <json/json.h>

#include "Log.h"
#include "BlockQueue.h"
#include "clipp.h"

Log Logger;
pthread_t tUploadTimer, tCommandProcesser;
std::string Configfile, Logfile;
std::string URLCron, URLUpload;
std::string Token, UA;
bool isExit;

static void SigHandler(int sig);
void* UploadTimer(void*);
void* CommandProcesser(void*);

size_t curl_default_callback(const char* ptr, size_t size, size_t nmemb, std::string* stream);

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

    UA = JsonConfigRoot.isMember("UA") ? JsonConfigRoot["UA"].asString() : "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:13.0) Gecko/20100101 Firefox/13.0.1 QualityMonitor";

    Logger.OpenFile(Logfile);
    Logger.WriteLog(INFO, "Water Quality MonitorDaemon starting...");

    signal(SIGINT, SigHandler);
    isExit = false;

    pthread_create(&tUploadTimer, NULL, UploadTimer, NULL);
    pthread_create(&tCommandProcesser, NULL, CommandProcesser, NULL);

    CURL* mCurl = curl_easy_init();
    CURLcode CurlRes;
    std::string szbuffer;
    curl_global_init(CURL_GLOBAL_ALL);
    curl_easy_setopt(mCurl, CURLOPT_USERAGENT, UA.c_str());
    curl_easy_setopt(mCurl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(mCurl, CURLOPT_CAINFO, "cacert.pem");
    curl_easy_setopt(mCurl, CURLOPT_MAXREDIRS, 5);
    curl_easy_setopt(mCurl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(mCurl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(mCurl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(mCurl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, curl_default_callback);
    curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, &szbuffer);

    while (!isExit) {

        sleep(10);
    }

    Logger.WriteLog(INFO, "Program exiting. Please wait...");

    pthread_join(tUploadTimer, nullptr);
    pthread_join(tCommandProcesser, nullptr);
    
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

size_t curl_default_callback(const char* ptr, size_t size, size_t nmemb, std::string* stream) {
    assert(stream != NULL);
    size_t len = size * nmemb;
    stream->append(ptr, len);
    return len;
}
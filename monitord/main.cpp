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

#include "DisplayOled.h"
#include "Log.h"
#include "clipp.h"

#define DEFAULT_UA          "Mozilla/5.0 (Windows NT 6.1; WOW64; rv:13.0) Gecko/20100101 Firefox/13.0.1 QualityMonitor"
#define DEFAULT_INTERVAL    1800
#define DEFAULT_USE_OLED    true

Log Logger;
DisplayOled Disp;
pthread_t tUploadTimer, tCommandProcesser;
std::string Configfile, Logfile;
std::string URLCron, URLUpload;
std::string Token, UA;
bool isExit, useOled;
int SerialFd;
int Interval;

_SENSORS_DATA SensorsData;

static void SigHandler(int sig);
void* UploadTimer(void*);
void* CommandProcesser(void*);
std::string ReadSerial(std::string puts);

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

    if (!JsonConfigRoot.isMember("Token")) {
        std::cout << "FATAL: \"Token\" doesn't set in Config File!\nHave you logged in yet?" << std::endl;
        return 0;
    }
    Token = JsonConfigRoot["Token"].asString();


    if (!JsonConfigRoot.isMember("CronURL")) {
        std::cout << "FATAL: \"CronURL\" doesn't set in Config File!\nHave you logged in yet?" << std::endl;
        return 0;
    }
    URLCron = JsonConfigRoot["CronURL"].asString();

    UA = JsonConfigRoot.isMember("UA") ? JsonConfigRoot["UA"].asString() : DEFAULT_UA;
    Interval = JsonConfigRoot.isMember("Interval") ? JsonConfigRoot["Interval"].asInt() : DEFAULT_INTERVAL;
    useOled = JsonConfigRoot.isMember("UseOled") ? JsonConfigRoot["UseOled"].asBool() : DEFAULT_USE_OLED;

    Logger.OpenFile(Logfile);
    Logger.WriteLog(INFO, "Water Quality MonitorDaemon starting...");

    signal(SIGINT, SigHandler);
    isExit = false;

    pthread_create(&tUploadTimer, NULL, UploadTimer, NULL);
    pthread_create(&tCommandProcesser, NULL, CommandProcesser, NULL);

    SerialFd = serialOpen("/dev/ttyS5", 115200);
    ReadSerial("g");
    if(useOled)
        Disp.Init("/dev/i2c-3");

    Json::Value JsonCronRoot;
    CURL* mCurl = curl_easy_init();
    CURLcode CurlRes;
    std::string szbuffer, strSerialData;
    char bufferPost[1024];
    bool isGetSensorsSuccess;
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
    curl_easy_setopt(mCurl, CURLOPT_URL, URLCron.c_str());
    curl_easy_setopt(mCurl, CURLOPT_POST, 1L);

    while (!isExit) {
        sleep(10);

        szbuffer = "";
        strSerialData = ReadSerial("g");
        isGetSensorsSuccess = true;

        if (!JsonReader.parse(strSerialData.c_str(), JsonCronRoot)) {
            Logger.WriteLog(ERROR, "Failed to get sensors data!");
            isGetSensorsSuccess = false;
        }
        else {
            Logger.WriteLog(INFO, "Read Sensors data:" + strSerialData);
            SensorsData.WaterTemp = JsonCronRoot["Values"]["WaterTemp"].asDouble();
            SensorsData.TDS = JsonCronRoot["Values"]["TDS"].asInt();
            SensorsData.LM35 = JsonCronRoot["Values"]["LM35"].asDouble();
            SensorsData.PH = JsonCronRoot["Values"]["PH"].asDouble();
            SensorsData.Turbidity = JsonCronRoot["Values"]["Turbidity"].asDouble();
            if (useOled)
                Disp.UpdateData(SensorsData);
        }

        sprintf(bufferPost, "token=%s&tasks=%d&temp=%f&sensors=%b", Token.c_str(), 0, SensorsData.LM35, isGetSensorsSuccess);
        curl_easy_setopt(mCurl, CURLOPT_POSTFIELDS, bufferPost);
        curl_easy_setopt(mCurl, CURLOPT_POSTFIELDSIZE, bufferPost);
        CurlRes = curl_easy_perform(mCurl);
        if (CurlRes != CURLE_OK) {
            Logger.WriteLog(ERROR, "Failed to get cron task!");
            continue;
        }
        if (!JsonReader.parse(szbuffer, JsonCronRoot)) {
            Logger.WriteLog(ERROR, "Remote information parse error!");
            continue;
        }
    }

    Logger.WriteLog(INFO, "Program exiting. Please wait...");

    pthread_join(tUploadTimer, nullptr);
    pthread_join(tCommandProcesser, nullptr);

    Logger.Close();
    if (useOled)
        Disp.Exit();

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
    int ElapsedTime = 0;
    char bufferPost[1024];
    std::string szbuffer;
    CURL* mCurl = curl_easy_init();
    CURLcode CurlRes;
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
    curl_easy_setopt(mCurl, CURLOPT_URL, URLUpload.c_str());
    curl_easy_setopt(mCurl, CURLOPT_POST, 1L);

    while (!isExit) {
        sleep(10);
        ElapsedTime += 10;
        if (ElapsedTime < Interval)
            continue;
        ElapsedTime = 0;
        sprintf(bufferPost, "token=%s&WaterTemp=%f&TDS=%d&LM35=%f&PH=%f&Turbidity=%f", Token.c_str(),
            SensorsData.WaterTemp,
            SensorsData.TDS,
            SensorsData.LM35,
            SensorsData.PH,
            SensorsData.Turbidity);

        curl_easy_setopt(mCurl, CURLOPT_POSTFIELDS, bufferPost);
        curl_easy_setopt(mCurl, CURLOPT_POSTFIELDSIZE, bufferPost);
        CurlRes = curl_easy_perform(mCurl);
        if (CurlRes != CURLE_OK) {
            Logger.WriteLog(ERROR, "Upload Failed!");
            continue;
        }
    }
    return 0;
}

void* CommandProcesser(void*) {
    while (!isExit) {
        sleep(10);
    }
    return 0;
}

size_t curl_default_callback(const char* ptr, size_t size, size_t nmemb, std::string* stream) {
    assert(stream != NULL);
    size_t len = size * nmemb;
    stream->append(ptr, len);
    return len;
}

std::string ReadSerial(std::string puts) {
    serialPuts(SerialFd, puts.c_str());
    std::string result;
    while (serialDataAvail(SerialFd)) {
        char ch = serialGetchar(SerialFd);
        if(ch != '\n')
            result += ch;
    }
        
    return result;
}
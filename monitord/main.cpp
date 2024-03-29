﻿#include "monitord.h"

#include "DisplayOled.h"
#include "Log.h"
#include "clipp.h"

Log Logger;
DisplayOled Disp;
CURL* mCurl;
pthread_t tUploadTimer, tCommandProcesser;
std::string Configfile, Logfile;
std::string URLCron, URLUpload;
std::string Token, UA;
std::mutex CurlLock;
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

    if (!JsonConfigRoot.isMember("UploadURL")) {
        std::cout << "FATAL: \"UploadURL\" doesn't set in Config File!\nHave you logged in yet?" << std::endl;
        return 0;
    }
    URLUpload = JsonConfigRoot["UploadURL"].asString();

    UA = JsonConfigRoot.isMember("UA") ? JsonConfigRoot["UA"].asString() : DEFAULT_UA;
    Interval = JsonConfigRoot.isMember("Interval") ? JsonConfigRoot["Interval"].asInt() : DEFAULT_INTERVAL;
    useOled = JsonConfigRoot.isMember("UseOled") ? JsonConfigRoot["UseOled"].asBool() : DEFAULT_USE_OLED;

    Logger.OpenFile(Logfile);
    Logger.WriteLog(INFO, "Water Quality MonitorDaemon starting...");

    std::ofstream writepid;
    writepid.open("pid", std::ios::out);
    writepid << getpid();
    writepid.close();

    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);
    isExit = false;

    pthread_create(&tUploadTimer, NULL, UploadTimer, NULL);
    pthread_create(&tCommandProcesser, NULL, CommandProcesser, NULL);

    wiringPiSetup();
    SerialFd = serialOpen("/dev/ttyS5", 115200);
    ReadSerial("g");
    if (useOled) {
        pinMode(DEFAULT_OLED_VCC, OUTPUT);
        digitalWrite(DEFAULT_OLED_VCC, HIGH);
        sleep(1);
        int ret = Disp.Init("/dev/i2c-3");
        if (!ret) {
            Logger.WriteLog(ERROR, "Failed to open display");
        }
    }

    Json::Value JsonCronRoot;
    CURLcode CurlRes;
    mCurl = curl_easy_init();
    std::string szbuffer, strSerialData;
    char bufferPost[1024];
    bool isGetSensorsSuccess;
    curl_global_init(CURL_GLOBAL_ALL);
    curl_easy_setopt(mCurl, CURLOPT_USERAGENT, UA.c_str());
    curl_easy_setopt(mCurl, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(mCurl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(mCurl, CURLOPT_MAXREDIRS, 5);
    curl_easy_setopt(mCurl, CURLOPT_FOLLOWLOCATION, 1);
    curl_easy_setopt(mCurl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(mCurl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(mCurl, CURLOPT_FAILONERROR, 1L);
    curl_easy_setopt(mCurl, CURLOPT_WRITEFUNCTION, curl_default_callback);
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
            SensorsData.BatVoltage = JsonCronRoot["Values"]["BatteryVoltage"].asDouble();
            if (useOled)
                Disp.UpdateData(SensorsData);
        } 

        sprintf(bufferPost, "token=%s&tasks=%d&temp=%f&sensors=%b&batvoltage=%f",
            Token.c_str(),
            0,
            SensorsData.LM35,
            isGetSensorsSuccess,
            SensorsData.BatVoltage);
        CurlLock.lock();
        curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, &szbuffer);
        curl_easy_setopt(mCurl, CURLOPT_URL, URLCron.c_str());
        curl_easy_setopt(mCurl, CURLOPT_POSTFIELDS, bufferPost);
        curl_easy_setopt(mCurl, CURLOPT_POSTFIELDSIZE, strlen(bufferPost));
        CurlRes = curl_easy_perform(mCurl);
        CurlLock.unlock();
        if (CurlRes != CURLE_OK) {
            Logger.WriteLog(ERROR, "Failed to get cron task!");
            continue;
        }
        if (!JsonReader.parse(szbuffer, JsonCronRoot)) {
            Logger.WriteLog(ERROR, "Remote information parse error!");
            continue;
        }
        if (!JsonCronRoot.isMember("result"))
            Logger.WriteLog(WARN, "Remote returned null.");
        else if (JsonCronRoot["result"].asInt() != 0)
            Logger.WriteLog(WARN, "Cron remote returned error:" + JsonCronRoot["msg"].asString());
        else Logger.WriteLog(INFO, "Cron result:" + JsonCronRoot["msg"].asString());
    }

    Logger.WriteLog(INFO, "Program exiting. Please wait...");

    unlink("pid");

    pthread_join(tUploadTimer, nullptr);
    pthread_join(tCommandProcesser, nullptr);
    if (useOled) {
        Disp.Exit();
        digitalWrite(DEFAULT_OLED_VCC, LOW);
    }

    Logger.Close();

    return 0;
}

static void SigHandler(int sig) {
    switch (sig)
    {
    case SIGTERM:
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
    CURLcode CurlRes;
    Json::Value JsonUploadRoot;
    Json::Reader JsonReader;

    while (!isExit) {
        sleep(10);
        ElapsedTime += 10;
        if (ElapsedTime < Interval)
            continue;
        ElapsedTime = 0;
        szbuffer = "";
        sprintf(bufferPost, "token=%s&WaterTemp=%f&TDS=%d&LM35=%f&PH=%f&Turbidity=%f", 
            Token.c_str(),
            SensorsData.WaterTemp,
            SensorsData.TDS,
            SensorsData.LM35,
            SensorsData.PH,
            SensorsData.Turbidity);
        CurlLock.lock();
        curl_easy_setopt(mCurl, CURLOPT_WRITEDATA, &szbuffer);
        curl_easy_setopt(mCurl, CURLOPT_URL, URLUpload.c_str());
        curl_easy_setopt(mCurl, CURLOPT_POSTFIELDS, bufferPost);
        curl_easy_setopt(mCurl, CURLOPT_POSTFIELDSIZE, strlen(bufferPost));
        CurlRes = curl_easy_perform(mCurl);
        CurlLock.unlock();
        if (CurlRes != CURLE_OK) {
            Logger.WriteLog(ERROR, "Upload Failed!");
            continue;
        }
        if (!JsonReader.parse(szbuffer, JsonUploadRoot) || !JsonUploadRoot.isMember("result")) 
            Logger.WriteLog(WARN, "No upload result.");
        else if (JsonUploadRoot["result"].asInt() != 0)
            Logger.WriteLog(ERROR, "Upload remote returned error:" + JsonUploadRoot["msg"].asString());
        else Logger.WriteLog(INFO, "Upload result:" + JsonUploadRoot["msg"].asString());

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
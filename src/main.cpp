#include <string>

#include "logger.h"

int main(int argc, char* argv[]) {

    logger::StartLogger();


    LOG_DBG << "this is some log message";
    LOG_INF << "this is some log message";
    LOG_WRN << "this is some log message";
    LOG_ERR << "this is some log message";


    logger::StopLoggerAsync();
    logger::WaitLoggerStop();

    return 0;
}


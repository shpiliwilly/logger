#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>


#include "logger.h"

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

namespace logger {

bool IsLevelEnabled(Level level) {
    // TODO
    return true;
}


//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

    // TODO
    // this is for draft impl. for now we will just use lock for 
    // protecting the queue, no complicated stuff.
    // also for now we write logs just to standart output

class Logger {

    Logger();
    ~Logger();

    static Logger m_instance;

public:

    static Logger& GetInstance() { return m_instance; }

    // start logging thread. no blocking. returns false if already started
    bool StartLoggerAsync();

    // stop logging thread. no blocking, returns false if failed to post stop msg
    bool StopLoggerAsync();

    // wait for logger to stop. caller get blocked untill the thread stopped
    void WaitLoggerStop();

    // returns true if logger's thread is up and running
    bool Running() const;

    // returns true if logger's thread is stopped
    bool Stopped() const;

    // 
    bool Log(const char* msg);

private:

    struct LogMsgBase;

    bool PostMsgImpl(LogMsgBase* msg);

    struct LogMsgBase { 
        virtual void Execute() = 0;
    };

    struct LogMsg : public LogMsgBase { 
        LogMsg(const char* msg) 
            : m_msg(new char[strlen(msg)])
        { 
            strcpy(m_msg.get(), msg);
        }
        virtual void Execute();
        std::unique_ptr<char> m_msg;
    };

    struct LogMsgStop : public LogMsgBase { 
        LogMsgStop() { }
        virtual void Execute();
    };

    friend class LogMsg;
    friend class LogMsgStop;

    const unsigned m_max_queue_size = 10000;

    std::queue<std::unique_ptr<LogMsgBase>> m_queue;
    std::mutex m_mux;
    std::condition_variable m_cond_var;
    std::unique_ptr<std::thread> m_thr;

    bool m_stopped;

    static void ThreadFuncS(void*);
    void ThreadFunc();

    void HandleLogMsg();
    void HandleLogMsgStop();
};

//////////////////////////////////////////////////////////


Logger::Logger() 
    : m_stopped(false)
{ }

Logger::~Logger() {
    if(!Stopped()) {
        WaitLoggerStop();
    }
}

bool Logger::StartLoggerAsync() {
    std::lock_guard<std::mutex> guard(m_mux);

    if(!Stopped())
        return false;

    m_thr = std::unique_ptr<std::thread>(new std::thread(ThreadFuncS, this));
    return true;
}

bool Logger::StopLoggerAsync() {
    return PostMsgImpl(new LogMsgStop());
}

void Logger::WaitLoggerStop() {
    if(Stopped())
        return;

    m_thr->join();
}

bool Logger::Running() const {
    return m_thr && m_thr->joinable(); 
}

bool Logger::Stopped() const {
    return !Running(); 
}

bool Logger::PostMsgImpl(LogMsgBase* msg) {
    {
        std::unique_lock<std::mutex> guard(m_mux);
        if(m_queue.size() >= m_max_queue_size) {
            return false;
        }
        m_queue.push(std::unique_ptr<LogMsgBase>(msg));
    }
    m_cond_var.notify_one();
    return true;
}

bool Logger::Log(const char* log_msg) {
    return PostMsgImpl(new LogMsg(log_msg));
}

void Logger::ThreadFuncS(void* this_ptr) {
    Logger* logger = static_cast<Logger*>(this_ptr);
    logger->ThreadFunc();
}

void Logger::ThreadFunc() {
    while(!m_stopped) {
        std::unique_ptr<LogMsgBase> log_msg = nullptr;
        {
            std::unique_lock<std::mutex> guard(m_mux);
            m_cond_var.wait(guard, [&]{ return !m_queue.empty(); } );
            log_msg = std::move(m_queue.front());
            m_queue.pop();
        }
        log_msg->Execute();
    }
}

void Logger::LogMsgStop::Execute() { 
    Logger::GetInstance().m_stopped = true;
}

void Logger::LogMsg::Execute() {
    std::cout << m_msg.get() << std::endl;
}

Logger Logger::m_instance;


void StartLogger() {
    Logger::GetInstance().StartLoggerAsync();
}

void StopLoggerAsync() {
    Logger::GetInstance().StopLoggerAsync();
}

void WaitLoggerStop() {
    Logger::GetInstance().WaitLoggerStop();
}

//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////

LogMsgBuilder::LogMsgBuilder(Level level)
    : m_buff(m_buff_prim)
    , m_curr_buff_size(m_prim_buff_size)
    , m_curr_offset(0)
    , m_using_prim_buff(true)
{
    memset(m_buff, 0, sizeof(m_buff));

    // TODO. this is shitty

    // WARNING! THIS CODE IS NOT THREAD SAFE
    // functions like localtime are not thread-safe, 
    // and you'd better use localtime_r instead

    timeval curTime;                                                 
    gettimeofday(&curTime, NULL);                                   
    int milli = curTime.tv_usec / 1000;                             

    unsigned written = strftime(m_buff, GetFreeBytes(), "%H:%M:%S", localtime(&curTime.tv_sec));   
    m_curr_offset += written;

    written = snprintf(m_buff + m_curr_offset, GetFreeBytes(), ":%03d %s : ", milli, GetLevelStr(level));                   
    m_curr_offset += written;
}

LogMsgBuilder::~LogMsgBuilder()
{
    Logger::GetInstance().Log(m_buff);
}

const char* LogMsgBuilder::GetLevelStr(Level level) {
    if(level == DBG) 
        return "DBG";
    else if(level == INF) 
        return "INF";
    else if(level == WRN) 
        return "WRN";
    else if(level == ERR) 
        return "ERR";
    else {
        //assert(false);
        return "";
    }
}

/////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////

void print_hex_byte(char* buff, char byte) {
    char tmp_buff[3];
    sprintf(tmp_buff, "%02X", byte);
    buff[0] = tmp_buff[0];
    buff[1] = tmp_buff[1];
}


///////////////////////////////////////////////////////////

// TODO

const unsigned padding_left = 4;
const unsigned padding_right = 4;
const unsigned padding_middle = 2;
const unsigned bytes_per_col = 4;

void print_hex_view(const char* buff, char* print_buff, unsigned row, unsigned buff_size) {

    //std::cout << "print_hex_view start" << std::endl;

    unsigned curr_index = row * 8;
    for(int i = 0; i < 2 * bytes_per_col; i++) {
        if(curr_index >= buff_size)
            break;
        //std::cout << "curr_index = " << curr_index << std::endl;

        print_hex_byte(print_buff, buff[curr_index]);

        if(i == bytes_per_col - 1) {
            print_buff += 4;
        } else {
            print_buff += 3;
        }

        curr_index++;
    }
    //std::cout << "print_hex_view exit" << std::endl;
}


void print_str_view(const char* buff, char* print_buff, unsigned row, unsigned buff_size) {

    unsigned curr_index = row * 8;
    for(int i = 0; i < 2 * bytes_per_col; i++) {
        if(curr_index >= buff_size)
            break;

        if(isprint(buff[curr_index]))
            *print_buff = buff[curr_index];

        print_buff++;
        curr_index++;

        if(i == bytes_per_col - 1) {
            memset(print_buff, ' ', padding_middle);
            print_buff += padding_middle;
        }
    }
}


std::unique_ptr<char[]> dmp(const char* buff, unsigned buff_size) {

    int rows_count = ceil(buff_size / 8.0);
    unsigned row_len = padding_left + padding_right + 2 * padding_middle
                + 2 * 2 * bytes_per_col + 2 * (bytes_per_col - 1) + 2 * bytes_per_col + 1;

    // +1 for the trailing null, +1 for '\n' at the front
    const unsigned print_buff_size = rows_count * row_len + 2;
    char* print_buff = new char[print_buff_size];
    print_buff[0] = '\n';
    memset(print_buff + 1, ' ', print_buff_size - 1);

    unsigned str_view_offset = row_len - 2 * bytes_per_col - 1 - padding_middle;

    //std::cout << "buff_size = " << buff_size << std::endl;
    //std::cout << "row_len = " << row_len << std::endl;
    //std::cout << "rows_count = " << rows_count << std::endl;
    //std::cout << "str_view_offset = " << str_view_offset << std::endl;

    for(int row = 0; row < rows_count; row++) {
        char* curr_row = print_buff + row * row_len + 1;
        print_hex_view(buff, curr_row + padding_left, row, buff_size);
        print_str_view(buff, curr_row + str_view_offset, row, buff_size);
        curr_row[row_len - 1] = '\n';
        if(row == rows_count - 1) {
            // the last row printed
            curr_row[row_len] = '\0';
        }
    }

    return std::move(std::unique_ptr<char[]>(print_buff));
}


}



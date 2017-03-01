#pragma once

#include <string>
#include <string.h>
#include <memory>


namespace logger {

    void StartLogger();
    void StopLoggerAsync();
    void WaitLoggerStop();

enum Level {
    DBG,
    INF,
    WRN,
    ERR
};

//////////////////////////////////////////////////////////////

class LogMsgBuilder {
private:

    const static unsigned m_prim_buff_size = 200;

    // primary buffer for building log messages
    char m_buff_prim[m_prim_buff_size];

    // pointer to the current buffer
    char* m_buff;

    // dynamic buffer is used if primary buffer was not big enough
    bool m_using_prim_buff;

    unsigned m_curr_offset;
    unsigned m_curr_buff_size;
    unsigned GetFreeBytes() const { return m_curr_buff_size - m_curr_offset; }

    const char* GetLevelStr(Level level);

    template <typename T>
    LogMsgBuilder& Helper(T val, const char* fmt) {
        unsigned bytes_free = GetFreeBytes();
        int written = snprintf(m_buff + m_curr_offset, bytes_free, fmt, val);

        if(written >= 0 && written < bytes_free) {
            m_curr_offset += written;
        } else {
            // curr buffer is not big enough - allocate bigger buffer, 
            // copy data from old buffer to the new one, set the new buffer 
            // as 'current', free old buffer and retry the last operation.
            unsigned new_buff_size = m_curr_buff_size * 2;
            char* new_buff = new char [new_buff_size];
            // no need to init the 'old' part
            memset(new_buff + m_curr_buff_size, 0, new_buff_size - m_curr_buff_size);
            memcpy(new_buff, m_buff, m_curr_offset);
            if(!m_using_prim_buff) {
                delete[] m_buff;
            }
            m_buff = new_buff;
            m_curr_buff_size = new_buff_size;
            m_using_prim_buff = false;
            return Helper(val, fmt);
        }

        return *this;
    }

public:

    LogMsgBuilder(Level level);
    ~LogMsgBuilder();

    LogMsgBuilder& operator<<(const std::unique_ptr<char[]>&& str) { return Helper(str.get(), "%s"); }
    LogMsgBuilder& operator<<(const char* str)          { return Helper(str, "%s"); }
    LogMsgBuilder& operator<<(const std::string& str)   { return operator<<(str.c_str()); }
    LogMsgBuilder& operator<<(char val)                 { return Helper(val, "%c"); }
    LogMsgBuilder& operator<<(unsigned char val)        { return Helper(val, "%c"); }
    LogMsgBuilder& operator<<(double val)               { return Helper(val, "%f"); }
    LogMsgBuilder& operator<<(bool val)                 { return Helper(val, "%d"); }
    LogMsgBuilder& operator<<(short val)                { return Helper(val, "%dh"); }
    LogMsgBuilder& operator<<(int val)                  { return Helper(val, "%d"); }
    LogMsgBuilder& operator<<(long val)                 { return Helper(val, "%dl"); }
    LogMsgBuilder& operator<<(long long val)            { return Helper(val, "%dll"); }
    LogMsgBuilder& operator<<(unsigned short val)       { return Helper(val, "%uh"); }
    LogMsgBuilder& operator<<(unsigned int val)         { return Helper(val, "%u"); }
    LogMsgBuilder& operator<<(unsigned long val)        { return Helper(val, "%ul"); }
    LogMsgBuilder& operator<<(unsigned long long val)   { return Helper(val, "%ull"); }
};

//////////////////////////////////////////////////////////////

bool IsLevelEnabled(Level level);


std::unique_ptr<char[]> dmp(const char* buff, unsigned buff_size);

}

#define LOG_MSG( level )                \
    if(!logger::IsLevelEnabled(level))     \
        ;                               \
    else                                \
        logger::LogMsgBuilder(level)

#define LOG_DBG LOG_MSG(logger::Level::DBG)
#define LOG_INF LOG_MSG(logger::Level::INF)
#define LOG_WRN LOG_MSG(logger::Level::WRN)
#define LOG_ERR LOG_MSG(logger::Level::ERR)


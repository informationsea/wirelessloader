/*- -*- c++ -*- */

#ifndef _SERIALPORT_H_
#define _SERIALPORT_H_

#ifdef __WIN32__
#include <windows.h>
#include <cstdlib>
typedef DWORD SIZE_t;
typedef long SSIZE_t;
typedef DWORD baud_t;
#define SERIAL_9600   CBR_9600
#define SERIAL_38400  CBR_38400
#define SERIAL_115200 CBR_115200
#else
#include <termios.h>
#include <unistd.h>
typedef ssize_t SSIZE_t;
typedef size_t SIZE_t;
typedef long baud_t;
#define SERIAL_9600   B9600
#define SERIAL_38400  B38400
#define SERIAL_115200 B115200
#endif

class serialport
{
private:
    const char *_path;
    baud_t _baudrate;

    bool _opened;
    bool _timeout_error;

    char _buf[1024];
    SIZE_t _buflen;

#ifdef __WIN32__
    HANDLE _hCom;
    DCB _newdcb, _olddcb;
#else
    int _fd;
    struct termios _newtio, _oldtio;
#endif

    SSIZE_t read_raw(long *timeout_milliseconds = NULL);

public:
    serialport(const char* path, baud_t baudrate);
    virtual ~serialport();

    bool open();
    bool is_opened() { return _opened; }
    SSIZE_t write(const void* buf, SIZE_t nbytes);

    bool writec(int ch);

    /**
     * @brief read bytes
     * @param buf read buffer
     * @param nbytes buffer size
     * @param timeout_milliseconds Timeout in milli seconds. negative to infinity timeout
     * @param blockmode block till buffer full
     */
    SSIZE_t read(void* buf, SIZE_t nbytes, long timeout_milliseconds = -1, bool blockmode = false);

    /**
     * @brief read one byte
     * @param timeout_milliseconds Timeout in milli seconds. negative to infinity timeout
     * @return return -1 if failed.
     */
    int readc(long timeout_milliseconds = -1);
    
    bool close();

    void perror(const char* message);
    bool is_timeout(void);
};

#endif /* _SERIALPORT_H_ */

#include "serialport.h"

#include <assert.h>
#include <stdint.h>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#ifndef __WIN32__

#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <paths.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/param.h>
#include <sys/select.h>
#include <sys/time.h>
#include <time.h>

#elif defined(__WIN32__) //Windows 32bit
#include <windows.h>
#include <winbase.h>
#endif

#include "debug.h"

using namespace std;

serialport::serialport(const char* path, baud_t baudrate) :
                _path(path), _baudrate(baudrate), _opened(false), _timeout_error(false)
{
#ifdef __WIN32__
    _hCom = NULL;
#else
    _fd = 0;
#endif
    memset(_buf, 0, sizeof(_buf));
    _buflen = 0;
}

serialport::~serialport()
{
    if (_opened) {
        close();
    }
}

bool serialport::open()
{
    _timeout_error = false;
#ifdef __WIN32__
    _hCom = CreateFile(_path, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (_hCom == INVALID_HANDLE_VALUE) {
        perror(_path);
        return false;
    }

    memset(&_newdcb, 0, sizeof(_newdcb));
    if (!GetCommState(_hCom, &_olddcb)) {
        perror("Cannot get COM state");
    }

    char build_str[128];
    snprintf(build_str, sizeof(build_str), "%s: baud=9600 parity=N data=8 stop=1", _path);
    BuildCommDCB(build_str, &_newdcb); // This function ignores COM1
    _newdcb.BaudRate = _baudrate;

    if (!SetCommState(_hCom, &_newdcb)) {
        perror("Cannot set COM state");
    }

    if (!PurgeComm(_hCom, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR)) {
        perror("Cannot purge");
        return false;
    }

    _opened = true;
    return true;
#else

    _fd = ::open(_path, O_RDWR | O_NOCTTY | O_NDELAY);
    if (_fd == -1) {
        /* Could not open the port.  */
        perror(_path);
        return false;
    }

    if (tcgetattr(_fd, &_oldtio)) {          /* Save original serial port settings */
        perror("serialport: original serialport settings");
        ::close(_fd);
        return false;
    }
    bzero(&_newtio, sizeof(_newtio));  /* Clear newtio */

    /* Hardware flow control, 8 bit data, no parity, ingnore modem, enable receiver */
    _newtio.c_cflag = CS8 | CLOCAL | CREAD | HUPCL;

    /* Ignore parity error */
    _newtio.c_iflag = IGNPAR | IGNBRK;

    /* Raw output. */
    _newtio.c_oflag = 0;

    /* No local flags */
    _newtio.c_lflag = 0;

    cfsetspeed(&_newtio, _baudrate); /* Set baud rate */

    tcflush(_fd, TCIFLUSH);            /* flush */
    tcsetattr(_fd, TCSANOW, &_newtio); /* new serialport settings now */

    _opened = true;
    return true;
#endif
}


SSIZE_t serialport::write(const void* buf, SIZE_t nbytes)
{
    _timeout_error = false;
#ifdef __WIN32__
    DWORD wrote_bytes;
#endif
    if (!_opened) return -1;
#ifdef __WIN32__
    if (WriteFile(_hCom, buf, nbytes, &wrote_bytes, NULL)) {
        return wrote_bytes;
    }
    return -1;
#else
    return ::write(_fd, buf, nbytes);
#endif
}

bool serialport::writec(int ch)
{
    char byte[1];
    byte[0] = ch;
    SSIZE_t wrote = write(byte, 1);
    return wrote == 1;
}

SSIZE_t serialport::read_raw(long *timeout_milliseconds)
{
    if (!_opened) return -1;
#ifdef __WIN32__
    DWORD start, mid, end;
    DWORD read_bytes = 0;
    COMMTIMEOUTS timeouts;
    BOOL success = false, pre_success;
    memset(&timeouts, 0, sizeof(timeouts));
    timeouts.ReadTotalTimeoutConstant = *timeout_milliseconds;
    SetCommTimeouts(_hCom, &timeouts);

    start = GetTickCount();

    pre_success = ReadFile(_hCom, _buf + _buflen, 1, &read_bytes, NULL);
    DEBUGF("Pre read %u %lu\n", pre_success, read_bytes);
    _buflen += read_bytes;

    if (pre_success && read_bytes) { // pre-read is done successfully.
        COMSTAT stat;
        DWORD error = 0;
        mid = GetTickCount();
        *timeout_milliseconds -= mid - start;
        timeouts.ReadTotalTimeoutConstant = *timeout_milliseconds;
        SetCommTimeouts(_hCom, &timeouts);

        if (ClearCommError(_hCom, &error, &stat)) {
            DWORD will_read_bytes = ((stat.cbInQue < (sizeof(_buf) - _buflen)) ? stat.cbInQue : (sizeof(_buf) - _buflen));
            if (will_read_bytes) { // read more bytes if remained.
                success = ReadFile(_hCom, _buf + _buflen, will_read_bytes, &read_bytes, NULL);
                DEBUGF("Full read %u %lu\n", success, read_bytes);
                if (success) {
                    _buflen += read_bytes;
                }
            } else {
                success = true;
            }
        }
    } else if (pre_success) {
        success = true;
    }

    end = GetTickCount();
    *timeout_milliseconds -= end - mid;

    if (success)
        return read_bytes;
    //DEBUGF("Failed to read\n");
    return -1;

#else
    struct timeval ti;
    struct timeval first, end;
    uint64_t first_time, end_time;
    SSIZE_t retvalue = 0;

    if (timeout_milliseconds) {
        ti.tv_sec = *timeout_milliseconds / 1000;
        ti.tv_usec = (*timeout_milliseconds % 1000) * 1000;
    }

    gettimeofday(&first, NULL);
    first_time = first.tv_sec*1000 + first.tv_usec/1000;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(_fd, &fds);
    if (select(_fd + 1, &fds, NULL, NULL, timeout_milliseconds == NULL ? NULL : &ti)
            >= 0) {
        if (FD_ISSET(_fd, &fds)) {
            ssize_t len = ::read(_fd, _buf + _buflen,
                    sizeof(_buf) - _buflen);
            if (len < 0) {
                perror("Cannot read serialport");
            }
            _buflen += len;
            retvalue = len;
            goto onend;
        }
        retvalue = 0;
        goto onend;
    }
    perror("Error at select");
    retvalue = -1;

    onend:
    gettimeofday(&end, NULL);
    end_time = end.tv_sec*1000 + end.tv_usec/1000;
    *timeout_milliseconds -= end_time - first_time;

    return retvalue;

#endif
}

SSIZE_t serialport::read(void* buf, SIZE_t nbytes, long timeout_milliseconds, bool blockmode)
{
    _timeout_error = false;
    while (_buflen == 0 || (blockmode && nbytes > _buflen)) {
        SSIZE_t ret = read_raw(&timeout_milliseconds);
        if (ret < 0)
            return ret;
        if (timeout_milliseconds <= 0 && ret == 0) {
            _timeout_error = true;
            return -1;
        }
    }
    //_buf[_buflen] = 0;
    //DEBUGF("on buffer %s\n", _buf);

    if (nbytes < _buflen) {
        memcpy(buf, _buf, nbytes);
        memmove(_buf, _buf + nbytes, _buflen - nbytes);
        _buflen -= nbytes;
        return nbytes;
    }

    memcpy(buf, _buf, _buflen);
    SIZE_t copied_len = _buflen;
    _buflen = 0;
    return copied_len;
}

int serialport::readc(long timeout_milliseconds)
{
    _timeout_error = false;
    while(_buflen == 0) {
        DEBUGF("Reading...%lu\n", timeout_milliseconds);
        SSIZE_t ret = read_raw(&timeout_milliseconds);
        if (ret < 0)
            return ret;
        if (timeout_milliseconds <= 0 && ret == 0) {
            _timeout_error = true;
            return -1;
        }
    }
    char ch = _buf[0];
    //_buf[_buflen] = 0;
    //DEBUGF("on buffer %s\n", _buf);
    memmove(_buf, _buf+1, _buflen - 1);
    _buflen -= 1;
    return ch;
}

bool serialport::close()
{
    _timeout_error = false;
    if (_opened) {
#ifdef __WIN32__
        if (CloseHandle(_hCom)) {
            return true;
        }
        perror(_path);
        return false;

#else
        if (tcsetattr(_fd, TCSANOW, &_oldtio)) { /* Revert settings */
            perror("Revert serial port settings");
        }
        _opened = false;
        if (::close(_fd)) {
            perror(_path);
            return false;
        }
#endif
        return true;
    }
    return false;
}


void serialport::perror(const char* message)
{
    if (_timeout_error) {
        fprintf(stderr, "%s : Timeout\n", message);
        return;
    }

#ifdef __WIN32__
    LPVOID lpMsgBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR) &lpMsgBuf, 0, NULL);


    fprintf(stderr, "%s : %s\n", message, (char *)lpMsgBuf);

    LocalFree(lpMsgBuf);
#else
    ::perror(message);
#endif
}

bool serialport::is_timeout(void)
{
    return _timeout_error;
}

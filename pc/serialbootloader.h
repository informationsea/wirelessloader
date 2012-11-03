#ifndef _SERIALBOOTLOADER_H_
#define _SERIALBOOTLOADER_H_

#include "writer.h"
#include "serialport.h"

class SerialBootloaderWrtier : public AVRWriter
{
private:
    const char* _port;
    uint32_t _speed;
    serialport _serialport;
    
    uint8_t _signature[3];
    uint8_t _fuse[3];
    uint8_t _lockbits;
    uint8_t _once_pagenum;
    AVRInfo *_avrinfo;

public:
    SerialBootloaderWrtier(const char* port, uint32_t speed);
    virtual ~SerialBootloaderWrtier();

    virtual bool open();
    virtual bool close();

    virtual bool get_signature_bytes(uint8_t buf[3]);
    virtual bool get_fuse_bytes(uint8_t buf[3]);
    virtual bool get_lock_bits(uint8_t* lock);

    virtual bool erase_program(void);
    virtual bool write_program(const AVRProgram *program);
    virtual bool read_program(AVRProgram *program);
    virtual bool verify_program(const AVRProgram *program);

    virtual void run(void);
private:
    bool write_program_helper(uint32_t addr);
};

#endif /* _SERIALBOOTLOADER_H_ */


/*
  Local Variables:
  mode: c++
  End:
*/

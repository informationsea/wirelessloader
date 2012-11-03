/*- -*- c++ -*- */

#ifndef _WRITER_H_
#define _WRITER_H_

#include <stdint.h>
#include <cstdlib>
#include <cstdio>

#include "avrinfo.h"

#define LENGTH(x) (sizeof(x)/sizeof(x[0]))

#define WRITER_MODE_NUMBER_OF_FUSE_BITS 3
#define MAXIMUM_AVR_FLASH 256*1024 // bytes

class AVRProgram
{
private:
    size_t _page_size;
    size_t _flash_size;
    uint8_t _buffer[MAXIMUM_AVR_FLASH];
public:
    AVRProgram();
    virtual ~AVRProgram();

    bool read(const char* ihex_path);
    const uint8_t* buffer() const { return _buffer; }
    bool set(uint8_t *new_program, uint32_t size);
    void write(FILE *stream) const;
};

class AVRWriter
{
public:
    AVRWriter();
    virtual ~AVRWriter();

    virtual bool open() = 0;
    virtual bool close() = 0;

    virtual bool get_signature_bytes(uint8_t buf[3]) = 0;
    virtual bool get_fuse_bytes(uint8_t buf[3]) = 0;
    virtual bool get_lock_bits(uint8_t* lock) = 0;

    virtual bool erase_program(void) = 0;
    virtual bool write_program(const AVRProgram *program) = 0;
    virtual bool read_program(AVRProgram *program) = 0;
    virtual bool verify_program(const AVRProgram *program) = 0;

    virtual void run(void) {}

    static AVRWriter *get_writer(const char* writer_type, const char* writer_port, uint32_t speed);

};


class WriterMode
{
private:
    bool _write_fuse[WRITER_MODE_NUMBER_OF_FUSE_BITS];
    uint8_t _fuse_new[WRITER_MODE_NUMBER_OF_FUSE_BITS];

    bool _read_fuse;

    bool _erase_program;
    bool _write_program;
    const char *_write_program_path;

    bool _read_program;
    bool _read_eeprom;

    bool _help;
    int _return_code;
    bool _verbose_mode;

    uint32_t _writer_speed;
    const char *_writer_type;
    const char *_writer_port;

    const char *_program_name;
    bool _run_after_do;

public:
    WriterMode();
    virtual ~WriterMode();

    enum fuse { LOW_FUSE, HIGH_FUSE, EXTENDED_FUSE };

    bool write_fuse(enum fuse fuse) const;
    uint8_t new_fuse(enum fuse fuse) const;
    bool read_fuse() const;
    bool read_eeprom() const;
    bool erase_program() const;
    bool read_program() const;
    const char* write_program() const;
    bool show_help() const;
    const char* writer_type() const;
    const char* writer_port() const;
    uint32_t writer_speed() const;
    bool verbose_mode() const;
    bool run_after_do() const;

    void parse_args(int argc, char* argv[]);
    void help() const;
    int return_code() const;
    void show_summary() const;
};


#endif /* _WRITER_H_ */

#include "writer.h"

#include <cstdio>
#include <cstring>

using namespace std;

#include "debug.h"

#ifndef _WIN32
#include <unistd.h>
#else
#include <memory.h>
#include "getopt.h"
#endif

/* --------- AVRWriter -----------*/
AVRWriter::AVRWriter() {}
AVRWriter:: ~AVRWriter() {}

#include "serialbootloader.h"

AVRWriter *AVRWriter::get_writer(const char* writer_type, const char* writer_port, uint32_t speed)
{
    if(strcmp(writer_type, "serialbootloader") == 0) {
        return new SerialBootloaderWrtier(writer_port, speed);
    }

    fprintf(stderr, "Unknown writer type : %s\n", writer_type);
    return 0;
}

#define LINE_BUFFER_SIZE 256

/* -------- AVRProgram -----------*/
AVRProgram::AVRProgram() : _page_size(256), _flash_size(64*1024) {
    memset(_buffer, 0xff, sizeof(_buffer));
}

AVRProgram::~AVRProgram()
{

}

bool AVRProgram::read(const char* ihex_path)
{
    FILE *ihex = fopen(ihex_path, "r");
    if (ihex == NULL) {
        perror(ihex_path);
        return false;
    }

    uint16_t high_address = 0;
    uint16_t segment_address = 0;
    uint8_t i;
    char linebuf[LINE_BUFFER_SIZE];
    SIZE_t line_index = 0;
    while(!feof(ihex) && fgets(linebuf, sizeof(linebuf), ihex) >= 0) {
        // printf("READ: %s", linebuf);
        line_index += 1;
        if (strlen(linebuf) == 0) // skip empty line
            continue;
        if (linebuf[0] != ':') {
            fprintf(stderr, "Invalid format at line %lu\n", line_index);
            goto onerror;
        }

        uint8_t data[LINE_BUFFER_SIZE];
        SIZE_t data_length = 0;
        uint32_t address = 0;
        uint32_t record_type = 0;
        int scanned = sscanf(linebuf+1, "%2lx%4x%2x", &data_length, &address, &record_type);
        if (scanned != 3) {
            fprintf(stderr, "Invalid format at line %lu\n", line_index);
            goto onerror;
        }
        DEBUGF("ADDR: %04x  TYPE: %02x  LENGTH: %02lu  ", address, record_type, data_length);

        uint8_t checksum = 0;
        for (i = 0; i < data_length+4+1; ++i) { // checksum
            uint32_t one = 0;
            scanned = sscanf(linebuf+1+i*2, "%2x", &one);
            checksum += (uint8_t)one;
        }
        if (checksum != 0) {
            fprintf(stderr, "Checksum failed %02x\n", checksum & 0xff);
            goto onerror;
        }

        for (i = 0; i < data_length; ++i) {
            uint32_t one;
            scanned = sscanf(linebuf+1+8+i*2, "%2x", &one);
            data[i] = one;
            DEBUGC("%02x", data[i]);
        }
        DEBUGC("\n");

        switch(record_type) {
        case 0:{
            uint32_t real_address = (high_address << 16) + (segment_address << 4) + address;
            if (real_address + data_length < sizeof(_buffer)) {
                memcpy(_buffer + real_address, data, data_length);
            } else {
                fprintf(stderr, "Address %x is out of range.\n", real_address);
            }
            break;
        }
        case 1:
            goto onend;
        case 2:
            segment_address= (data[0] << 8) | data[1];
            break;
        case 4:
            high_address = (data[0] << 8) | data[1];
            break;
        case 3:
        case 5:
            break;
        default:
            fprintf(stderr, "Error: Unknown record on %s\n", ihex_path);
            break;
        }
    }
    fprintf(stderr, "No end record\n");
    goto onerror;
    onend:

#if 0
    for (size_t i = 0; i < 16; i++) {
        for (size_t j = 0; j < 16; j++) {
            printf("%02x ", _buffer[i*16 + j]);
        }
        printf("\n");
    }
    printf("\n");
#endif

    fclose(ihex);
    return true;
    onerror:
    fclose(ihex);
    return false;
}


bool AVRProgram::set(uint8_t *new_program, uint32_t size)
{
    if (sizeof(_buffer) < size)
        return false;
    memset(_buffer, 0xff, sizeof(_buffer));
    memcpy(_buffer, new_program, size);
    return true;
}

#define IHEX_DATASIZE 32

void AVRProgram::write(FILE *stream) const
{
    SIZE_t addr, i;

    for (addr = 0; addr < MAXIMUM_AVR_FLASH; addr += IHEX_DATASIZE) {
        bool should_write = false;
        uint8_t checksum = 0;
        for (i = 0; i < IHEX_DATASIZE; i++) {
            if (_buffer[addr + i] != 0xff)
                should_write = true;
        }
        if (!should_write)
            continue;

        fprintf(stream, ":%02X%04lX%02X", IHEX_DATASIZE, addr, 0x00);

        for (i = 0; i < IHEX_DATASIZE; i++) {
            fprintf(stream, "%02X", _buffer[addr + i]);
        }

        checksum += IHEX_DATASIZE;
        for (i = 0; i < 4; ++i) {
            checksum += (addr >> (3-i)*8) & 0xff;
        }
        for (i = 0; i < IHEX_DATASIZE; i++) {
            checksum += _buffer[addr + i];
        }

        fprintf(stream, "%02X\n", (-checksum)&0xff);
    }
    fprintf(stream, ":00000001FF\n");
}

/* ------- WriteMode ------- */

WriterMode::WriterMode() :
                            _read_fuse(false), _erase_program(false), _write_program(false),
                            _write_program_path(0), _read_program(false), _read_eeprom(false),
                            _help(false), _return_code(0), _verbose_mode(false),
                            _writer_speed(38400),
                            _writer_type("serialbootloader"),
                            _writer_port("/dev/ttyUSB0"),
                            _run_after_do(false) {

    for (size_t i = 0; i < LENGTH(_write_fuse); ++i) {
        _write_fuse[i] = false;
    }

    for (size_t i = 0; i < LENGTH(_fuse_new); ++i) {
        _fuse_new[i] = 0xff;
    }
}
WriterMode::~WriterMode(){}

uint8_t WriterMode::new_fuse(enum fuse fuse) const {
    return _fuse_new[fuse];
}

bool WriterMode::write_fuse(enum fuse fuse) const {
    return _write_fuse[fuse];
}

bool WriterMode::read_fuse() const {
    return _read_fuse;
}

bool WriterMode::read_eeprom() const {
    return _read_eeprom;
}

bool WriterMode::erase_program() const {
    return _erase_program;
}

bool WriterMode::read_program() const {
    return _read_program;
}

const char* WriterMode::write_program() const {
    if (_write_program)
        return _write_program_path;
    return 0;
}

bool WriterMode::show_help() const {
    return _help;
}

const char* WriterMode::writer_type() const {
    return _writer_type;
}

const char* WriterMode::writer_port() const {
    return _writer_port;
}

uint32_t WriterMode::writer_speed() const
{
    return _writer_speed;
}

bool WriterMode::verbose_mode() const
{
    return _verbose_mode;
}


bool WriterMode::run_after_do() const
{
    return _run_after_do;
}

void WriterMode::parse_args(int argc, char* argv[])
{
    _program_name = argv[0];

    char ch;
    while ((ch = getopt(argc, argv, "h?f:r:p:s:veu")) != -1) {
        switch (ch) {
        case 'f':{
            switch(optarg[0]) {
            case 'l':
                _write_fuse[LOW_FUSE] = true;
                break;
            case 'h':
                _write_fuse[HIGH_FUSE] = true;
                break;
            case 'x':
                _write_fuse[EXTENDED_FUSE] = true;
                break;
            default:
                fprintf(stderr, "%s: illegal option -- f%s\n", argv[0], optarg);
                _help = true;
                _return_code = 1;
                return;
            }
            break;
        }
        case 'r': {
            if (strlen(optarg) != 1) {
                fprintf(stderr, "%s: illegal option -- r%s\n", argv[0], optarg);
                _help = true;
                _return_code = 1;
                return;
            }
            switch(optarg[0]) {
            case 'p':
                _read_program = true;
                break;
            case 'f':
                _read_fuse = true;
                break;
            case 'e':
                _read_eeprom = true;
                break;
            default:
                fprintf(stderr, "%s: illegal option -- r%s\n", argv[0], optarg);
                _help = true;
                _return_code = 1;
                return;
            }
            break;
        }
        case 'p':
            _writer_port = optarg;
            break;
        case 's':
            fprintf(stderr, "-s : Not implemented yet\n");
            break;
        case 'v':
            _verbose_mode = true;
            break;
        case 'e':
            _erase_program = true;
            break;
        case 'u':
            _run_after_do = true;
            break;
        case 'h':
        case '?':
            _help = true;
            return;
        default:
            _help = true;
            _return_code = 1;
            return;
        }
    }

    argc -= optind;
    argv += optind;

    if (_help || _read_program || _read_eeprom || _read_fuse ||
            _write_fuse[0] || _write_fuse[1] || _write_fuse[2])
        return;

    if (argc == 1) {
        _write_program = true;
        _write_program_path = argv[0];
        return;
    }

    fprintf(stderr, "No input file");
    _help = true;
    _return_code = 1;
}

void WriterMode::help() const
{
    fprintf(stderr, "%s [OPTION] [HexFile]\n\n", _program_name);
    fprintf(stderr, "-h  : show this help\n");
    fprintf(stderr, "-rp : read program\n");
    fprintf(stderr, "-rf : read fuse bits\n");
    fprintf(stderr, "-re : read eeprom\n");
    fprintf(stderr, "-fl : write low fuse bits\n");
    fprintf(stderr, "-fh : write high fuse bits\n");
    fprintf(stderr, "-fx : write extended fuse bits\n");
    fprintf(stderr, "-e  : erase program\n");
    fprintf(stderr, "-v  : verbose mode\n");
    fprintf(stderr, "-p  : writer port\n");
    fprintf(stderr, "-s  : write speed (baudrate)\n");
    fprintf(stderr, "-u  : run the program after writing\n");
    fprintf(stderr, "HexFile : write program\n");

}

int WriterMode::return_code() const
{
    return _return_code;
}

void WriterMode::show_summary() const
{
    printf("Show help     : %d\n", _help);
    printf("Read program  : %d\n", _read_program);
    printf("Read eeprom   : %d\n", _read_eeprom);
    printf("Erase program : %d\n", _erase_program);
    printf("Write program : %d\n", _write_program);
    if (_write_program)
        printf("                %s\n", _write_program_path);
    printf("Read fuse     : %d\n", _read_fuse);
    printf("Write fuse    : %d %d %d\n", _write_fuse[0], _write_fuse[1], _write_fuse[2]);
    printf("Writer Type   : %s\n", _writer_type);
    printf("Writer Port   : %s\n", _writer_port);
    printf("Writer Speed  : %u\n", _writer_speed);
    printf("Run after do  : %u\n", _run_after_do);
}


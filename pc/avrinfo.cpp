#include "avrinfo.h"
#include <string.h>

/* -------- AVRInfo --------*/
AVRInfo::AVRInfo(const char* avrname,
                 uint8_t signiture1,
                 uint8_t signiture2,
                 uint8_t signiture3,
                 uint16_t pagesize,
                 uint16_t pagenum)
{
    _avrname = avrname;
    _signiture[0] = signiture1;
    _signiture[1] = signiture2;
    _signiture[2] = signiture3;
    _pagesize = pagesize;
    _pagenum = pagenum;
}

AVRInfo::~AVRInfo(){}

const char* AVRInfo::name() const
{
    return _avrname;
}

const uint8_t* AVRInfo::signature() const
{
    return _signiture;
}

uint16_t AVRInfo::pagesize() const
{
    return _pagesize;
}

uint16_t AVRInfo::pagenum() const
{
    return _pagenum;
}

AVRInfo avr_information[] = {
    AVRInfo("atmega644p" , 0x1e, 0x96, 0x0a, 256, 256),
    AVRInfo("atmega324p" , 0x1e, 0x95, 0x08, 128, 256),
    AVRInfo("atmega164p" , 0x1e, 0x94, 0x0a, 128, 128),
    AVRInfo("atmega88p"  , 0x1e, 0x92, 0x0a, 64, 128),
    AVRInfo("atmega168p" , 0x1e, 0x94, 0x0b, 128, 128),
    AVRInfo("atmega328p" , 0x1e, 0x95, 0x0f, 128, 256)
};


AVRInfo *AVRInfo::get_info(const uint8_t signature[3])
{
    for (size_t i = 0; i < LENGTH(avr_information); ++i) {
        if (memcmp(signature, avr_information[i].signature(), 3) == 0) {
            return avr_information + i;
        }
    }
    return 0;
}


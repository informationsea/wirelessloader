/*- -*- c++ -*- */
#ifndef _AVRINFO_H_
#define _AVRINFO_H_

#include <stdint.h>

#ifndef LENGTH
#define LENGTH(x) (sizeof(x)/sizeof(x[0]))
#endif

class AVRInfo
{
private:
    const char* _avrname;
    uint8_t _signiture[3];
    uint16_t _pagesize;
    uint16_t _pagenum;
public:
    AVRInfo(const char* avrname,
            uint8_t signiture1,
            uint8_t signiture2,
            uint8_t signiture3,
            uint16_t pagesize, // bytes, not word
            uint16_t pagenum);
    virtual ~AVRInfo();

    const char* name() const;
    const uint8_t* signature() const;
    uint16_t pagesize() const;
    uint16_t pagenum() const;

    static AVRInfo *get_info(const uint8_t signature[3]);
};

extern AVRInfo avr_information[];

#endif /* _AVRINFO_H_ */

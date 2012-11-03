#include <cstdio>
#include <stdint.h>

#include "debug.h"
#include "serialport.h"
#include "writer.h"

int main(int argc, char *argv[])
{
    printf("AVR Writer 0.1  Copyright (C) 2012 Y.Okamura All Rights Reserved.\n");

    WriterMode writer_mode;
    writer_mode.parse_args(argc, argv);
    if (writer_mode.verbose_mode()) {
        writer_mode.show_summary();
        gdebug_to_stderr();
    }

    if (writer_mode.show_help()) {
        writer_mode.help();
        return writer_mode.return_code();
    }

    AVRWriter *writer = AVRWriter::get_writer(writer_mode.writer_type(),
            writer_mode.writer_port(),
            writer_mode.writer_speed());
    if (writer == 0) {
        fprintf(stderr, "Cannot get writer \"%s\"\n", writer_mode.writer_type());
        return 1;
    }

    if (!writer->open()) {
        fprintf(stderr, "Cannot open writer\n");
        return 1;
    }

    if (writer_mode.read_eeprom()) {
        fprintf(stderr, "\"Read EEPROM\" is not implemented yet.\n");
    }

    if (writer_mode.read_fuse()) {
        fprintf(stderr, "\"Read fuse bits\" is not implemented yet.\n");
    }

    if (writer_mode.write_fuse(WriterMode::LOW_FUSE)) {
        fprintf(stderr, "\"Write low fuse bit\" is not implemented yet.\n");
    }

    if (writer_mode.write_fuse(WriterMode::HIGH_FUSE)) {
        fprintf(stderr, "\"Write high fuse bit\" is not implemented yet.\n");
    }

    if (writer_mode.write_fuse(WriterMode::EXTENDED_FUSE)) {
        fprintf(stderr, "\"Write extended fuse bit\" is not implemented yet.\n");
    }

    if (writer_mode.read_program()) {
        AVRProgram program;
        fprintf(stderr, "Reading...\n");
        if (!writer->read_program(&program)) {
            writer->close();
            return 1;
        }
        program.write(stdout);
    }

    if (!writer_mode.write_program() && writer_mode.erase_program()) {
        fprintf(stderr, "Erasing...\n");
        if(!writer->erase_program()) {
            writer->close();
            return 1;
        }
    }

    if (writer_mode.write_program()) {
        AVRProgram program;
        if(!program.read(writer_mode.write_program())) {
            writer->close();
            return 1;
        }

        fprintf(stderr, "Erasing...\n");
        if(!writer->erase_program()) {
            writer->close();
            return 1;
        }


        fprintf(stderr, "Writing...\n");
        if(!writer->write_program(&program)) {
            writer->close();
            return 1;
        }

        fprintf(stderr, "Verifying...\n");
        if(!writer->verify_program(&program)) {
            writer->close();
            return 1;
        }
    }

    fprintf(stderr, "Done\n");

    if (writer_mode.run_after_do()) {
        writer->run();
    }

    writer->close();

    return 0;
}

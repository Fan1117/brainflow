#include "cyton_wifi.h"
#include "custom_cast.h"
#include "timestamp.h"

#define START_BYTE 0xA0
#define END_BYTE_STANDARD 0xC0
#define END_BYTE_ANALOG 0xC1
#define END_BYTE_MAX 0xC6


void CytonWifi::read_thread ()
{
    /*
        Byte 1: 0xA0
        Byte 2: Sample Number
        Bytes 3-5: Data value for EEG channel 1
        Bytes 6-8: Data value for EEG channel 2
        Bytes 9-11: Data value for EEG channel 3
        Bytes 12-14: Data value for EEG channel 4
        Bytes 15-17: Data value for EEG channel 5
        Bytes 18-20: Data value for EEG channel 6
        Bytes 21-23: Data value for EEG channel 6
        Bytes 24-26: Data value for EEG channel 8
        Aux Data Bytes 27-32: 6 bytes of data
        Byte 33: 0xCX where X is 0-F in hex
    */
    int res;
    unsigned char b[32];
    while (keep_alive)
    {
        // check start byte
        res = server_socket->recv (b, 1);
        if (res != 1)
        {
            continue;
        }
        if (b[0] != START_BYTE)
        {
            continue;
        }

        res = server_socket->recv (b, 32);
        if (res != 32)
        {
            continue;
        }

        double package[22] = {0.};
        // package num
        package[0] = (double)b[0];
        // eeg
        for (int i = 0; i < 8; i++)
        {
            package[i + 1] = eeg_scale * cast_24bit_to_int32 (b + 1 + 3 * i);
        }
        // end byte
        package[12] = (double)b[res - 1];
        // check end byte
        if (b[res - 1] == END_BYTE_STANDARD)
        {
            // accel
            package[9] = accel_scale * cast_16bit_to_int32 (b + 25);
            package[10] = accel_scale * cast_16bit_to_int32 (b + 27);
            package[11] = accel_scale * cast_16bit_to_int32 (b + 29);
        }
        else if (b[res - 1] == END_BYTE_ANALOG)
        {
            // analog
            package[19] = cast_16bit_to_int32 (b + 25);
            package[20] = cast_16bit_to_int32 (b + 27);
            package[21] = cast_16bit_to_int32 (b + 29);
        }
        else if ((b[res - 1] > END_BYTE_ANALOG) && (b[res - 1] <= END_BYTE_MAX))
        {
            // unprocessed bytes
            package[13] = (double)b[25];
            package[14] = (double)b[26];
            package[15] = (double)b[27];
            package[16] = (double)b[28];
            package[17] = (double)b[29];
            package[18] = (double)b[30];
        }
        else
        {
            safe_logger (spdlog::level::warn, "Wrong end byte, found {}", b[res - 1]);
            continue;
        }

        db->add_data (get_timestamp (), package);
    }
}
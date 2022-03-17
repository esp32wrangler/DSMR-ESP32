#include "utils.h"

// from https://github.com/fliphess/esp8266_p1meter

unsigned int CRC16(unsigned int crc, const unsigned char *buf, int len)
{
	for (int pos = 0; pos < len; pos++)
    {
		crc ^= (unsigned int)buf[pos];    // * XOR byte into least sig. byte of crc
                                          // * Loop over each bit
        for (int i = 8; i != 0; i--)
        {
            // * If the LSB is set
            if ((crc & 0x0001) != 0)
            {
                // * Shift right and XOR 0xA001
                crc >>= 1;
				crc ^= 0xA001;
			}
            // * Else LSB is not set
            else
                // * Just shift right
                crc >>= 1;
		}
	}
	return crc;
}

bool isOBIS(char c)
{
    return (c == '.' || c == ':' || c == '-' || (c >= '0' && c <= '9'));
}

bool isNumeric(const char * ptr)
{
    do
    {
        if (! ((*ptr >= '0' && *ptr <= '9') || *ptr == '.') )
        {
            return false;   
        }
    } while (*++ptr);
    return true;
}
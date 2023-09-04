/** @file
    Various utility functions for use by device drivers.

    Copyright (C) 2015 Tommy Vestermark

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

uint8_t crc4(uint8_t const message[], unsigned nBytes, uint8_t polynomial, uint8_t init)
{
  unsigned remainder = init << 4; // LSBs are unused
  unsigned poly = polynomial << 4;
  unsigned bit;

  while (nBytes--) {
    remainder ^= *message++;
    for (bit = 0; bit < 8; bit++) {
      if (remainder & 0x80) {
        remainder = (remainder << 1) ^ poly;
      } else {
        remainder = (remainder << 1);
      }
    }
  }
  return remainder >> 4 & 0x0f; // discard the LSBs
}

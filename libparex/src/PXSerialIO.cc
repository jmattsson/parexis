/*
 * Copyright (c) 2012 Johny Mattsson
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the copyright holders nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "PXSerialIO.h"
#include <cstring>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

namespace
{
using namespace ParEx;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-conversion"
bool
cfg_serial (int fd, speed_t br_key, bool only_7_bits, parity_t parity, bool two_stop_bits)
{
  struct termios tty;
  memset (&tty, 0, sizeof tty);
  if (tcgetattr (fd, &tty) != 0)
    return false;

  cfmakeraw (&tty);

  cfsetospeed (&tty, br_key);
  cfsetispeed (&tty, br_key);

  tty.c_cc[VMIN]  = 0;
  tty.c_cc[VTIME] = 0;

  tty.c_cflag &= ~CRTSCTS;
  tty.c_cflag |= CLOCAL | CREAD;

  if (parity == PARITY_EVEN)
    tty.c_cflag |= PARENB;
  else if (parity == PARITY_ODD)
    tty.c_cflag |= PARENB | PARODD;

  if (only_7_bits)
    tty.c_cflag &= ~CS8;

  if (two_stop_bits)
    tty.c_cflag |= CSTOPB;
  else
    tty.c_cflag &= ~CSTOPB;

  if (tcsetattr (fd, TCSANOW, &tty) != 0)
    return false;

  return true;
}
#pragma GCC diagnostic pop

} // anon

namespace ParEx
{

PXSerialIO::PXSerialIO (const std::string &dev, speed_t br_key, bool only_7_bits, parity_t par, bool two_stop_bits)
  : PXIO (-1),
    dev_ (dev),
    br_ (br_key), seven_ (only_7_bits), par_ (par), stops_ (two_stop_bits)
{
  fd_ = open ();
}


void
PXSerialIO::reopen ()
{
  int fd = open ();
  close (fd_);
  fd_ = fd;
}


int
PXSerialIO::open ()
{
  int fd = ::open (dev_.c_str (), O_RDWR);
  if (fd < 0 || !cfg_serial (fd, br_, seven_, par_, stops_))
    throw E_ERR ();
  return fd;
}

} // namespace

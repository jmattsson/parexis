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

#include "PXInterleavedPrinter.h"
#include <algorithm>
#include <sstream>

namespace ParEx
{

PXInterleavedPrinter::PXInterleavedPrinter (FILE *fil)
  : bufs_ (), out_ (fil)
{
  // Empty
}


void
PXInterleavedPrinter::add_channel (channel_id_t chan_id, std::shared_ptr<PXChannel> channel)
{
  chan_buf_t buf = { chan_id, channel, "" };
  bufs_.push_back (buf);
}


void
PXInterleavedPrinter::remove_channel (channel_id_t chan_id, std::shared_ptr<PXChannel> channel)
{
  // TODO: remove - the below comment no longer reflects reality
  // We don't remove channels since that would mess up the channel indexes
  // and yield very confusing output
  (void)chan_id;
  (void)channel;
}


PXInterleavedPrinter::chan_buf_t *
PXInterleavedPrinter::find_buf (channel_id_t chid)
{
  auto i = std::find_if (bufs_.begin (), bufs_.end (),
      [=](chan_buf_t &b) { return b.chid == chid; });

  return (i == bufs_.end ()) ? NULL : &*i;
}


void
PXInterleavedPrinter::out (channel_id_t chan_id, char c)
{
  chan_buf_t *buf = find_buf (chan_id);
  if (buf)
    buf->buffer += c;
}


void
PXInterleavedPrinter::matched (channel_id_t chan_id, const std::string &str)
{
  chan_buf_t *buf = find_buf (chan_id);
  if (buf)
  {
    std::string::size_type pos = buf->buffer.rfind (str);
    if (pos != std::string::npos)
    {
      buf->buffer.replace (pos, str.size (), hilight_match (str));
    }
    else {} // TODO
  }
}


void
PXInterleavedPrinter::timedout (channel_id_t chan_id, const std::string &expr, timeval_t timeout)
{
  chan_buf_t *buf = find_buf (chan_id);
  if (buf)
  {
    std::ostringstream oss;
    oss << "Timed out after " << timeout.tv_sec << "s waiting for '" << expr << "'";
    buf->buffer += hilight_timeout (oss.str ()) + "\n";
  }
}


void
PXInterleavedPrinter::flush ()
{
  for_each (bufs_.begin (), bufs_.end (),
    [&](chan_buf_t &b) {
      std::string::size_type pos = b.buffer.find ('\n');
      while (pos != std::string::npos)
      {
        std::string p = line_prefix ();
        fwrite (p.c_str (), p.size (), 1, out_);
        const std::string &name = b.channel->name ();
        fwrite (name.c_str (), name.size (), 1, out_);
        fwrite ("> ", 2, 1, out_);
        fwrite (b.buffer.c_str (), pos +1, 1, out_);
        fflush (out_);

        b.buffer = b.buffer.substr (pos +1);

        pos = b.buffer.find ('\n');
      }
    });
}


std::string
PXInterleavedPrinter::line_prefix () const
{
  time_t now = time (NULL);
  char buf[40];
  strftime (buf, sizeof (buf), "\033[1m[%T]\033[0m ", localtime (&now));
  return buf;
}


std::string
PXInterleavedPrinter::hilight_match (const std::string &str) const
{
  return "\033[34m" + str + "\033[0m";
}


std::string
PXInterleavedPrinter::hilight_timeout (const std::string &str) const
{
  return "\033[31m" + str + "\033[0m";
}

} // namespace

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

#ifndef _PXINTERLEAVEDPRINTER_H_
#define _PXINTERLEAVEDPRINTER_H_

#include "PXPrinter.h"
#include <cstdio>

namespace ParEx
{

class PXInterleavedPrinter : public PXPrinter
{
  public:
    explicit PXInterleavedPrinter (FILE *fil);
    ~PXInterleavedPrinter ();

    virtual void add_channel (channel_id_t chan_id, std::shared_ptr<PXChannel> channel);
    virtual void remove_channel (channel_id_t chan_id, std::shared_ptr<PXChannel> channel);
    
    virtual void out (channel_id_t chan_id, char c);
    virtual void matched (channel_id_t chan_id, const std::string &str);
    virtual void timedout (channel_id_t chan_id, const std::string &expr, timeval_t timeout);

    virtual void flush ();

  protected:
    virtual std::string line_prefix () const;
    virtual std::string hilight_match (const std::string &str) const;
    virtual std::string hilight_timeout (const std::string &str) const;

  private:
    PXInterleavedPrinter (const PXInterleavedPrinter &);
    PXInterleavedPrinter &operator = (const PXInterleavedPrinter &);

    typedef struct {
      channel_id_t chid;
      std::shared_ptr<PXChannel> channel;
      std::string buffer;
    } chan_buf_t;
    typedef std::vector<chan_buf_t> chan_vec_t;

    chan_buf_t *find_buf (channel_id_t chid);

    chan_vec_t bufs_;
    FILE *out_;
};

} // namespaec
#endif

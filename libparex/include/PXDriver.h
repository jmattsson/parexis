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

#ifndef _PXDRIVER_H_
#define _PXDRIVER_H_

#include "PXChannel.h"
#include "PXPrinter.h"
#include <memory>
#include <utility>
#include <vector>
#include <cstdint>

namespace ParEx
{

// opaque channel id
typedef uintptr_t channel_id_t;

class PXDriver
{
  public:
    explicit PXDriver (std::shared_ptr<PXPrinter> printer);

    channel_id_t addChannel (std::shared_ptr<PXChannel> chan);
    void         removeChannel (channel_id_t chan_id);

    void         waitForAll ();
    void         waitForOne (channel_id_t chan_id);
    channel_id_t waitForAny ();

    // Exception type for the waitXxx functions
    typedef struct {} TIMEOUT;

    typedef std::vector<std::shared_ptr<PXChannel> > channel_list_t;

  private:
    bool have_expectations () const;

    typedef std::pair<PXChannel *, expectation_t> expect_handle_t;
    expect_handle_t next_expect () const;

    bool check_expectations (channel_list_t &channels, channel_id_t *matched);

    std::shared_ptr<PXPrinter> printer_;
    channel_list_t channels_;
};

} // namespace
#endif

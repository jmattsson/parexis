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

#include "PXChannel.h"
#include <pcre.h>
#include <sys/time.h>

namespace ParEx
{

PXChannel::PXChannel (std::shared_ptr<PXIO> io, const std::string &chname)
  : io_ (io), exps_ (), name_ (chname), buffer_ (), last_match_ ()
{
  // Empty
}


void
PXChannel::add_expect(const std::string &expr, timeval_t timeout, exp_type_t et)
{
  timeval_t expiry;
  gettimeofday (&expiry, NULL);
  expiry += timeout;
  expectation_t exp (expr, timeout, expiry, NULL);
  if (et == PXPARALLEL || exps_.empty ())
  {
    expect_list_t el = { exp };
    exps_.push_back (el);
  }
  else
    exps_.back ().push_back (exp);
}


void
PXChannel::clear_expects ()
{
  expect_groups_t tmp;
  exps_.swap (tmp);
}


bool
PXChannel::expectation_met ()
{
  bool found = false;
  for (auto g = exps_.begin (); g != exps_.end () && !found; ++g)
  {
    for (auto e = g->begin (); e != g->end () && !found; ++e)
    {
      const char *err = NULL;
      int erroffset = 0;
      if (e->compiled_regex == NULL)
        e->compiled_regex =
          pcre_compile (
            e->expr.c_str (),
            PCRE_MULTILINE | PCRE_NEWLINE_ANY | PCRE_NO_AUTO_CAPTURE,
            &err, &erroffset, NULL
          );
      if (!e->compiled_regex)
      {
        fprintf(stderr, "invalid regex '%s' (at %d): %s\n",
          e->expr.c_str (), erroffset, err);
        throw E_REGEX ();
      }

      unsigned m[3];
      int num = pcre_exec (
        static_cast<pcre *>(e->compiled_regex), NULL,
        buffer_.c_str (), static_cast<int> (buffer_.size ()), 0,
        PCRE_NOTEMPTY | PCRE_NOTEOL,
        (int *)m,
        3);
      if (num == 1)
      {
        found = true;
        last_match_ = buffer_.substr (m[0], m[1]-m[0]);
        // consume used data and expectation
        buffer_ = buffer_.substr (m[1]);
        pcre_free (e->compiled_regex);
        g->erase (e);
        break;
      }
    }
  }
  if (found)
  {
    // look for empty lists, and if found clear all expectations on the
    // channel, as we just satisfied a full chain
    for (auto g = exps_.begin (); g != exps_.end (); ++g)
      if (g->empty ())
      {
        clear_expects ();
        break;
      }
  }
  return found;
}


} // namespace

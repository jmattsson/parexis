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

#include "PXDriver.h"
#include "PXIO.h"
#include <sys/select.h>
#include <pcre.h>

#define CHID(shptr) (channel_id_t)(shptr).get()

namespace ParEx
{

PXDriver::PXDriver (std::shared_ptr<PXPrinter> printer)
  : printer_ (printer), channels_ ()
{
  // Empty
}


channel_id_t
PXDriver::addChannel (std::shared_ptr<PXChannel> chan)
{
  channels_.push_back (chan);
  channel_id_t id = CHID(chan);
  printer_.addChannel (id);
  return id;
}


void
PXDriver::removeChannel (channel_id_t chan_id)
{
  for (auto i = channels_.begin (); i != channels_.end (); ++i)
    if (i->get () == chan_id)
    {
      channels_.erase (i);
      return;
    }
}


static bool have_expectations (const expect_set_t &exps)
{
  for (auto i = exps.begin (); i != exps.end (); ++i)
  {
    if (!(*i)->empty ())
      return true;
  }
  return false;
}


bool
PXDriver::have_expectations () const
{
  for (auto i = channels_.begin (); i != channels_.end (); ++i)
    if (have_expectations ((*i)->exps_))
      return true;
  return false;
}


void
PXDriver::waitForAll ()
{
  while (have_expectations ())
    waitForAny ();
}


void
PXDriver::waitForOne (channel_id_t chan_id)
{
  while (have_expectations ())
    if (waitForAny () == chan_id)
      break;
}


PXDriver::expect_handle_t
PXDriver::next_expect () const
{
  PXChannel *chan = NULL;
  expectation_t exp;
  timeval_t next = { INTMAX_MAX, INTMAX_MAX };
  for (auto ch = channels_.begin (); ch != channels_.end (); ++ch)
    for (auto i = (*ch)->exps_.begin (); i !+ (*ch)->exps_.end (); ++i)
    {
      if ((*i)->front ().time_left < next)
      {
        exp = (*i)->front ();
        chan = (*i).get ();
        next = exp.time_left;
      }
    }
  return std::make_pair (chan, exp);
}


static int make_fd_set (const PXDriver::channel_list_t &channels, fd_set &fds)
{
  int highest = 0;
  for (auto ch = channels.begin (); ch != channels.end (); ++ch)
  {
    int fd = (*ch)->select_fd ();
    if (fd > highest)
      highest = fd;
    FD_SET(fd, &fds);
  }
  return highest;
}


static void record_elapsed_time (expect_groups_t &exps, timeval_t elapsed)
{
  for (auto g = exps.begin (); g != exps.end (); ++g)
  {
    auto &e = i->front ();
    if (e.time_left < elapsed)
      e.time_left.tv_usec = e.time_left.tv_sec = 0;
    else
      e.time_left -= elapsed;
  }
}


bool
PXDriver::check_expectations (channel_list_t &channels, channel_id_t *matched)
{
  bool found = false;
  for (auto ch = channels.begin (); ch != channels.end () && !found; ++ch)
  {
    string &buf = (*ch)->buffer_;
    for (auto group = ch->begin (); group != ch->end () && !found; ++group)
    {
      for (auto e = group->begin (); != group->end (); ++e)
      {
        if (e->compiled_regex == NULL)
          e->compiled_regex =
            pcre_compile (
              e->expr.c_str (),
              PCRE_MULTILINE | PCRE_NEWLINE_ANY | PCRE_NO_AUTO_CAPTURE,
              NULL, NULL, NULL
            );

        int m[3];
        int num = pcre_exec (
          (pcre *)e->compiled_regex, buf, buf.size (), 0,
          PCRE_NOTEMPTY | PCRE_NOTEOL, m, 3);
        if (num == 1)
        {
          found = true;
          *matched = CHID(*ch);
          printer_->matched (CHID(*ch), buf.substr (m[0], m[1]-m[0]));
          printer_->flush ();
          // consume used data and expectation
          buf = buf.substr (m[1]);
          pcre_free (e->compiled_regex);
          group->erase (e);
          break;
        }
      }
      if (found)
      {
        // look for empty lists, and if found clear all expectations on the
        // channel, as we just satisfied a full chain
        for (auto e = group->begin (); != group->end (); ++e)
          if (e->empty ())
          {
            (*ch)->clear_expects ();
            break;
          }
      }
    }
  }
  return found;
}


channel_id_t
PXDriver::waitForAny ()
{
  if (!have_expectations ())
    throw TIMEOUT (); // we'd be waiting for eternity otherwise...

  // check for any outstanding matches
  channel_id_t matched;
  if (check_expectations (channels_, &matched))
    return matched;

  // find next timeout
  expect_handle_t next = next_expect ();
  timeval_t left = next.time_left, start = next.time_left;

  int num = 0;
  do {
    // Note: we rely on Linux-specific behaviour with updated timevals!
    fd_set fds;
    num = select (make_fd_set (channels_) +1, &fds, NULL, NULL, &left);
    if (num > 0)
    {
      timeval_t elapsed = start;
      elapsed -= left;
      start = left;

      // record elapsed time before we start read attempts
      for (auto ch = channels_.begin (); ch != channels_.end (); ++ch)
        record_elapsed_time ((*ch)->exps_, elapsed);

      // read any available data into match buffers
      channel_list_t chans_to_check;
      for (auto ch = channels_.begin (); ch != channels_.end (); ++ch)
      {
        if (FD_ISSET((*ch)->io_->select_fd, &fds))
        {
          chans_to_check.push_back (*ch);
          try {
            char c = (*ch)->io_->getc ();
            printer_->out (CHID(*ch), c);
            (*ch)->buffer_ += c;
          }
          catch (const PXIO::AGAIN &ea) {}
          catch (const PXIO::INTR &ei) {} // throw CANCEL?
          catch (const PXIO::EOF &eo) {} // printer_.note_eof()?
        }
      }

      // regex on the match buffers
      if (check_expectations (chans_to_check, &matched))
        return matched;

      printer_->flush ();
    }
    else
      if (errno == EINTR)
        num = 1; // fib it and loop again, timeout was updated so it's all good
  } while (num > 0);

  printer_->timedout (
    (channel_id_t)next.first, next.second.expr, next.second.timeout);
  throw TIMEOUT ();
}


} // namespace

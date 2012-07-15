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
#include "PXPrinter.h"
#include <sys/select.h>
#include <sys/time.h>
#include <pcre.h>

#define CHID(shptr) reinterpret_cast<channel_id_t>((shptr).get())

static void nowarn_FD_ZERO(fd_set &);
static void nowarn_FD_SET(int, fd_set &);
static bool nowarn_FD_ISSET(int, fd_set &);

namespace ParEx
{

PXDriver::PXDriver (std::shared_ptr<PXPrinter> printer)
  : printer_ (printer), channels_ ()
{
  // Empty
}


channel_id_t
PXDriver::add_channel (std::shared_ptr<PXChannel> chan)
{
  channels_.push_back (chan);
  channel_id_t id = CHID(chan);
  printer_->add_channel (id, chan);
  return id;
}


void
PXDriver::remove_channel (channel_id_t chan_id)
{
  for (auto i = channels_.begin (); i != channels_.end (); ++i)
    if (CHID(*i) == chan_id)
    {
      printer_->remove_channel (chan_id, *i);
      channels_.erase (i);
      return;
    }
}


static bool have_expectations (const expect_groups_t &exps)
{
  for (auto i = exps.begin (); i != exps.end (); ++i)
  {
    if (!i->empty ())
      return true;
  }
  return false;
}


bool
PXDriver::have_expectations () const
{
  for (auto i = channels_.begin (); i != channels_.end (); ++i)
    if (ParEx::have_expectations ((*i)->exps_))
      return true;
  return false;
}


void
PXDriver::wait_for_all ()
{
  while (have_expectations ())
    wait_for_any ();
}


void
PXDriver::wait_for_one (channel_id_t chan_id)
{
  while (have_expectations ())
    if (wait_for_any () == chan_id)
      break;
}


PXDriver::expect_handle_t
PXDriver::next_expect () const
{
  PXChannel *chan = NULL;
  expectation_t exp;
  timeval_t next = { INTMAX_MAX, INTMAX_MAX };
  for (auto ch = channels_.begin (); ch != channels_.end (); ++ch)
    for (auto i = (*ch)->exps_.begin (); i != (*ch)->exps_.end (); ++i)
    {
      if (i->front ().expiry < next)
      {
        exp = i->front ();
        chan = ch->get ();
        next = exp.expiry;
      }
    }
  return std::make_pair (chan, exp);
}


int
PXDriver::make_fd_set (const channel_list_t &channels, fd_set &fds) const
{
  int highest = 0;
  for (auto ch = channels.begin (); ch != channels.end (); ++ch)
  {
    int fd = (*ch)->io_->select_fd ();
    if (fd > highest)
      highest = fd;
    nowarn_FD_SET(fd, fds);
  }
  return highest;
}


bool
PXDriver::check_expectations (channel_list_t &channels, channel_id_t *matched)
{
  for (auto ch = channels.begin (); ch != channels.end (); ++ch)
  {
    if ((*ch)->expectation_met ())
    {
      *matched = CHID(*ch);
      printer_->matched (CHID(*ch), (*ch)->last_match ());
      return true;
    }
  }
  return false;
}


channel_id_t
PXDriver::wait_for_any ()
{
  if (!have_expectations ())
    throw TIMEOUT (); // we'd be waiting for eternity otherwise...

  // check for any outstanding matches
  channel_id_t matched;
  if (check_expectations (channels_, &matched))
    return matched;

  // find next timeout
  expect_handle_t next = next_expect ();

  int num = 0;
  timeval_t left;
  do {
    timeval_t now;
    gettimeofday (&now, NULL);
    left = next.second.expiry;
    if (now < next.second.expiry)
      left -= now;
    else
      left = { 0, 0 };

    fd_set fds;
    nowarn_FD_ZERO(fds);
    num = select (make_fd_set (channels_, fds) +1, &fds, NULL, NULL, &left);
    if (num > 0)
    {
      // read any available data into match buffers
      channel_list_t chans_to_check;
      for (auto ch = channels_.begin (); ch != channels_.end (); ++ch)
      {
        if (nowarn_FD_ISSET((*ch)->io_->select_fd (), fds))
        {
          chans_to_check.push_back (*ch);
          try {
            char c = (*ch)->io_->getc ();
            printer_->out (CHID(*ch), c);
            (*ch)->buffer_ += c;
          }
          catch (const PXIO::E_AGAIN &ea) {}
          catch (const PXIO::E_INTR &ei) {} // throw CANCEL?
          catch (const PXIO::E_EOF &eo) {} // printer_.note_eof()?
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
  } while (num > 0 && (left.tv_sec || left.tv_usec));

  printer_->timedout (
    (channel_id_t)next.first, next.second.expr, next.second.timeout);
  printer_->flush ();
  throw TIMEOUT ();
}


} // namespace

#pragma GCC diagnostic ignored "-Wsign-conversion"
static void nowarn_FD_ZERO(fd_set &fds)
{
  FD_ZERO(&fds);
}
static void nowarn_FD_SET(int fd, fd_set &fds)
{
  FD_SET(fd, &fds);
}
static bool nowarn_FD_ISSET(int fd, fd_set &fds)
{
  return FD_ISSET(fd, &fds);
}

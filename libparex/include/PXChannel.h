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

#ifndef _PXCHANNEL_H_
#define _PXCHANNEL_H_

#include <sys/times.h>
#include <list>
#include <string>
#include <memory>

namespace ParEx
{

typedef struct timeval timeval_t;

static inline bool operator < (const timeval_t &a, const timeval_t &b)
{
  if (a.tv_sec != b.tv_sec)
    return a.tv_sec < b.tv_sec;
  else
    return a.tv_usec < b.tv_usec;
}

static inline timeval_t &operator -= (timeval_t &a, const timeval_t &b)
{
  while (a.tv_usec < b.tv_usec)
  {
    --a.tv_sec;
    a.tv_usec += 1000000;
  }
  a.tv_sec -= b.tv_sec;
  a.tv_usec -= b.tv_usec;
  return a;
}

static inline timeval_t &operator += (timeval_t &a, const timeval_t &b)
{
  a.tv_sec += b.tv_sec;
  a.tv_usec += b.tv_usec;
  while (a.tv_usec > 1000000)
  {
    ++a.tv_sec;
    a.tv_usec -= 1000000;
  }
  return a;
}


class expectation_t
{
  public:
    std::string expr;
    timeval_t timeout;
    timeval_t expiry;
    void *compiled_regex;

    expectation_t ()
      : expr (), timeout (), expiry (), compiled_regex (NULL) {}
    expectation_t (const std::string &e, timeval_t t, timeval_t l, void *p)
      : expr (e), timeout (t), expiry (l), compiled_regex (p) {}
    expectation_t (const expectation_t &b)
      : expr (b.expr), timeout (b.timeout), expiry (b.expiry),
        compiled_regex (b.compiled_regex) {}
    expectation_t &operator = (const expectation_t &b)
    {
      expectation_t tmp (b);
      tmp.swap (*this);
      return *this;
    }
    expectation_t &swap (expectation_t &b)
    {
      expr.swap (b.expr);
      std::swap (timeout, b.timeout);
      std::swap (expiry, b.expiry);
      std::swap (compiled_regex, b.compiled_regex);
      return *this;
    }
};


typedef std::list<expectation_t> expect_list_t;

typedef std::list<expect_list_t> expect_groups_t;

typedef enum { PXSERIAL, PXPARALLEL } exp_type_t;

class PXDriver;
class PXIO;

class PXChannel
{
  public:
    PXChannel (std::shared_ptr<PXIO> io, const std::string &name);

    void add_expect (const std::string &expr, timeval_t timeout, exp_type_t et);
    void clear_expects ();

    void write (const std::string &str);

    const std::string &name () const { return name_; }
    const std::string &last_match () const { return last_match_; }

    // exception class for signalling a bad regex
    typedef struct {} E_REGEX;
  private:
    friend class PXDriver;

    bool expectation_met ();

    std::shared_ptr<PXIO> io_;
    expect_groups_t exps_;
    std::string name_;
    std::string buffer_;
    std::string last_match_;
};


} // namespace 
#endif

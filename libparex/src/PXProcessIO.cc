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

#include "PXProcessIO.h"
#include <stdexcept>
#include <cstring>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>

#include <iostream>

namespace
{

void reaper (int)
{
  int status;
  waitpid (-1, &status, WNOHANG);
}

} // anon

namespace ParEx
{

PXProcessIO::PXProcessIO (const argv_t &cmdline)
  : PXIO (-1), cmdline_ (cmdline),  child_ (-1)
{
  signal (SIGCHLD, reaper);
  do_open ();
}


PXProcessIO::~PXProcessIO ()
{
  do_close ();
}


void
PXProcessIO::reopen ()
{
  do_open ();
}


void
PXProcessIO::do_close ()
{
  if (child_ > 0)
    kill (child_, SIGTERM);
  child_ = -1;

  close (fd_);
  fd_ = -1;
}


void
PXProcessIO::do_open ()
{
  int mfd = open ("/dev/ptmx", O_RDWR | O_NONBLOCK | O_CLOEXEC);
  if (mfd < 0)
    throw PXIO::E_ERR ();
  
  char *devname = ptsname (mfd);
  if (grantpt (mfd) < 0 ||
      unlockpt (mfd) < 0 ||
      (devname = ptsname (mfd)) == NULL)
  {
    close (mfd);
    throw PXIO::E_ERR ();
  }

  pid_t npid = fork ();
  if (npid == -1)
  {
    close (mfd);
    throw PXIO::E_ERR ();
  }

  if (npid > 0) // parent
  {
    fd_ = mfd;
    child_ = npid;
  }
  else // child
  {
    close (mfd);

    // TODO: error-check and write useful string back to parent on failure

    setsid ();

    int sfd = open (devname, O_RDWR | O_CLOEXEC);

    dup2 (sfd, STDIN_FILENO);
    dup2 (sfd, STDOUT_FILENO);
    dup2 (sfd, STDERR_FILENO);
    // Note: original FDs closed on exec

    struct termios tios;
    tcgetattr (sfd, &tios);
    cfmakeraw (&tios);
    tcsetattr (sfd, TCSANOW, &tios);

    const size_t argc = cmdline_.size ();
    char *argv[argc +1];
    memset (argv, 0, sizeof (const char *) * (argc + 1));

    for (size_t i = 0; i < cmdline_.size (); ++i)
      argv[i] = const_cast<char *> (cmdline_[i].c_str ());

    execvp (argv[0], argv);
    exit (1); // We shouldn't get here...
  }
}

} // namespace

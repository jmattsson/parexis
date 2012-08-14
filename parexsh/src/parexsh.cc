#include "PXDriver.h"
#include "PXChannel.h"
#include "PXInterleavedPrinter.h"
#include "PXFileIO.h"
#include "PXSerialIO.h"
#include "PXProcessIO.h"
#include <cstdio>
#include <iostream>
#include <stdexcept>

#include <algorithm>

using namespace ParEx;

std::shared_ptr<PXPrinter> printer (new PXInterleavedPrinter (stderr));
PXDriver driver (printer);
std::vector<std::shared_ptr<PXChannel> > channels;
std::vector<channel_id_t> ids;

speed_t convert_speed (int spd)
{
  switch (spd)
  {
    case 1200: return B1200;
    case 2400: return B2400;
    case 4800: return B4800;
    case 9600: return B9600;
    case 19200: return B19200;
    case 38400: return B38400;
    case 57600: return B57600;
    case 115200: return B115200;
    case 230400: return B230400;
    default: return B0;
  }
}

void process_open_cmd (argv_t &argv)
{
  std::shared_ptr<PXIO> io;

  if (argv[1] == "file" && argv.size () == 4)
    io.reset (new PXFileIO (argv[3]));
  else if (argv[1] == "serial" && argv.size () == 6 && argv[5].size () == 3)
  {
    // serial <channel> <device> <speed> <[78][NOE][12]>
    io.reset (new PXSerialIO (
        argv[3],
        convert_speed (stoi (argv[4])),
        argv[5][0] == '7',
        argv[5][1] == 'O' ? PARITY_ODD : argv[5][1] == 'E' ? PARITY_EVEN : NO_PARITY,
        argv[5][2] == '2'));
  }
  else if (argv[1] == "process" && argv.size () > 3)
  {
    // process <channel> <cmd> [arg1 .. argN]
    argv_t proc (++ ++ ++argv.begin (), argv.end ()); // ignore first 3 args
    io.reset (new PXProcessIO (proc));
  }
  else
    throw std::invalid_argument ("bad args");

  std::shared_ptr<PXChannel> ch (new PXChannel (io, argv[2]));
  channels.push_back (ch);
  ids.push_back (driver.add_channel (ch));
  std::cout << ids.size () -1 << std::endl;
}

void process_expect (argv_t &argv, bool parallel)
{
  if (argv.size () != 4)
    throw std::invalid_argument ("bad args");

  channels.at (stoul (argv[1]))->add_expect (
    argv[2], { stol (argv[3]), 0 }, parallel ? PXPARALLEL : PXSERIAL);
}

void process_clear_expect (argv_t &argv)
{
  if (argv.size () != 2)
    throw std::invalid_argument ("bad args");
  channels.at (stoul (argv[1]))->clear_expects ();
}

void process_wait (argv_t &argv)
{
  if (argv.size () != 2)
    throw std::invalid_argument ("bad args");
  if (argv[1] == "all")
    driver.wait_for_all ();
  else if (argv[1] == "any")
    driver.wait_for_any ();
  else
  {
    driver.wait_for_one (ids.at (stoul (argv[1])));
    std::cout << channels.at (stoul (argv[1]))->last_match () << std::endl;
  }
}

void process_write (argv_t &argv)
{
  if (argv.size () == 3)
    channels.at (stoul (argv[1]))->write (argv[2]);
  else
    throw std::invalid_argument ("bad args");
}

char unescape (char c)
{
  switch (c)
  {
    case 'n': return '\n';
    case 'r': return '\r';
    case 't': return '\t';
    case 'b': return '\b';
    case 'a': return '\a';
    case 'f': return '\f';
    case 'v': return '\v';
    default: return c;
  }
}

argv_t split_line (std::string &line)
{
  argv_t argv;

  bool escaped = false, inquote = false, inspace = true;
  std::string arg;

  for (auto p = line.begin (); p != line.end (); ++p)
  {
    if (escaped)
    {
      escaped = false;
      arg += unescape (*p);
      continue;
    }
    if (*p == ' ' && !inquote && !escaped)
    {
      inspace = true;
      if (arg.size ())
        argv.push_back (arg);
      arg = "";
      continue;
    }
    if (*p == '"')
    {
      inquote = !inquote;
      continue;
    }
    if (inspace && !escaped && *p != ' ')
    {
      inspace = false;
      arg += *p;
      continue;
    }
    if (*p == '\\')
    {
      escaped = true;
      continue;
    }
    arg += *p;
  }
  if (arg.size ())
    argv.push_back (arg);

  if (escaped || inquote)
    throw std::invalid_argument ("malformed line");

  return argv;
}

int main (int argc, char *argv[])
{
  (void)argc;
  (void)argv;

  std::cout << "# ";
  std::string line;
  while (getline (std::cin, line))
  {
    typedef struct {} UNKNOWN;
    try
    {
      argv_t cmd_argv = split_line (line);
//std::for_each(cmd_argv.begin (), cmd_argv.end (), [](const std::string &s) { std::cerr << " :" << s << "\n"; });
      if (line.find ("open") == 0)
        process_open_cmd (cmd_argv);
      else if (line.find ("serexp") == 0)
        process_expect (cmd_argv, false);
      else if (line.find ("parexp") == 0)
        process_expect (cmd_argv, true);
      else if (line.find ("clearexp") == 0)
        process_clear_expect (cmd_argv);
      else if (line.find ("wait") == 0)
        process_wait (cmd_argv);
      else if (line.find ("write") == 0)
        process_write (cmd_argv);
      else if (line.find ("exit") == 0)
        break;
      else
        throw UNKNOWN ();
      std::cout << "ok\n" << std::flush;
    }
    catch (PXDriver::TIMEOUT&)
    {
      std::cout << "timeout\n" << std::flush;
    }
    catch (UNKNOWN&)
    {
      std::cout << "unknown\n" << std::flush;
    }
    catch (...)
    {
      std::cout << "error\n" << std::flush;
    }
    std::cout << "# ";
  }
  return 0;
}

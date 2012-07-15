#include "PXDriver.h"
#include "PXChannel.h"
#include "PXInterleavedPrinter.h"
#include "PXFileIO.h"
#include <cstdio>
#include <iostream>
#include <stdexcept>

#include <algorithm>

using namespace ParEx;

typedef std::vector<std::string> argv_t;

std::shared_ptr<PXPrinter> printer (new PXInterleavedPrinter (stderr));
PXDriver driver (printer);
std::vector<std::shared_ptr<PXChannel> > channels;
std::vector<channel_id_t> ids;

void process_open_cmd (argv_t &argv)
{
  if (argv[1] == "file" && argv.size () == 4)
  {
    std::shared_ptr<PXIO> io (new PXFileIO (argv[3]));
    std::shared_ptr<PXChannel> ch (new PXChannel (io, argv[2]));
    channels.push_back (ch);
    ids.push_back (driver.add_channel (ch));
    std::cout << ids.size () -1 << std::endl;
  }
  else
    throw std::invalid_argument ("bad args");
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

argv_t split_line (std::string &line)
{
  argv_t argv;
  bool in_quote = false;
  std::string::size_type last, cap, pos = line.find_first_not_of (' ');
  last = cap = pos;
  while ((pos = line.find_first_of (" \"", last)) != std::string::npos)
  {
    if (in_quote)
    {
      if (line[pos] == '\"')
      {
        in_quote = false;
        argv.push_back (line.substr (cap, pos - cap));
        ++pos;
      }
      else
      {
        last = pos + 1;
        continue; // space inside quotes, needs to be kept
      }
    }
    else
    {
      if (line[pos] == '\"')
      {
        in_quote = true;
        cap = last = pos + 1; // capture from character after "
        continue;
      }
      argv.push_back (line.substr (cap, pos - cap));
    }

    // skip to next interesting point
    cap = last = line.find_first_not_of (' ', pos);
  }
  if (in_quote)
    throw std::invalid_argument ("malformed line");

  if (last != std::string::npos && last != line.size ())
    argv.push_back (line.substr (last, pos - last));

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

/*
 * $Id: osutils.cxx,v 1.10 1994/06/25 11:55:15 robertj Exp $
 *
 * Portable Windows Library
 *
 * Operating System Classes Inline Function Definitions
 *
 * Copyright 1993 Equivalence
 *
 * $Log: osutils.cxx,v $
 * Revision 1.10  1994/06/25 11:55:15  robertj
 * Unix version synchronisation.
 *
 * Revision 1.9  1994/04/20  12:17:44  robertj
 * assert changes
 *
 * Revision 1.8  1994/04/01  14:05:06  robertj
 * Text file streams
 *
 * Revision 1.7  1994/03/07  07:47:00  robertj
 * Major upgrade
 *
 * Revision 1.6  1994/01/03  04:42:23  robertj
 * Mass changes to common container classes and interactors etc etc etc.
 *
 * Revision 1.5  1993/12/31  06:53:02  robertj
 * Made inlines optional for debugging purposes.
 *
 * Revision 1.4  1993/12/29  04:41:26  robertj
 * Mac port.
 *
 * Revision 1.3  1993/11/20  17:26:28  robertj
 * Removed separate osutil.h
 *
 * Revision 1.2  1993/08/31  03:38:02  robertj
 * G++ needs explicit casts for char * / void * interchange.
 *
 * Revision 1.1  1993/08/27  18:17:47  robertj
 * Initial revision
 *
 */

#include "ptlib.h"

#include <ctype.h>


///////////////////////////////////////////////////////////////////////////////
// PTimeInterval

#if defined(_PTIMEINTERVAL)

PTimeInterval::PTimeInterval(long millisecs,
                                 int seconds, int minutes, int hours, int days)
{
  milliseconds = millisecs+1000L*(seconds+60L*(minutes+60L*(hours+24L*days)));
}


PObject::Comparison PTimeInterval::Compare(const PObject & obj) const
{
  const PTimeInterval & other = (const PTimeInterval &)obj;
  return milliseconds < other.milliseconds ? LessThan :
         milliseconds > other.milliseconds ? GreaterThan : EqualTo;
}


ostream & PTimeInterval::PrintOn(ostream & strm) const
{
  return strm << milliseconds;
}


istream & PTimeInterval::ReadFrom(istream &strm)
{
  return strm >> milliseconds;
}


void PTimeInterval::SetInterval(long millisecs,
                                 int seconds, int minutes, int hours, int days)
{
  milliseconds = millisecs+1000L*(seconds+60L*(minutes+60L*(hours+24L*days)));
}


#endif


///////////////////////////////////////////////////////////////////////////////
// PTime

#if defined(_PTIME)

PObject::Comparison PTime::Compare(const PObject & obj) const
{
  const PTime & other = (const PTime &)obj;
  return theTime < other.theTime ? LessThan :
         theTime > other.theTime ? GreaterThan : EqualTo;
}


PTime::PTime(int second, int minute, int hour, int day, int month, int year)
{
  struct tm t;
  PAssert(second >= 0 && second <= 59, PInvalidParameter);
  t.tm_sec = second;
  PAssert(minute >= 0 && minute <= 59, PInvalidParameter);
  t.tm_min = minute;
  PAssert(hour >= 0 && hour <= 23, PInvalidParameter);
  t.tm_hour = hour;
  PAssert(day >= 1 && day <= 31, PInvalidParameter);
  t.tm_mday = day;
  PAssert(month >= 1 && month <= 12, PInvalidParameter);
  t.tm_mon = month-1;
  PAssert(year >= 1900, PInvalidParameter);
  t.tm_year = year-1900;
  theTime = mktime(&t);
  PAssert(theTime != -1, PInvalidParameter);
}


istream & PTime::ReadFrom(istream &strm)
{
  return strm;
}


PString PTime::AsString(TimeFormat format) const
{
  PString fmt;

  PString tsep = GetTimeSeparator();
  BOOL is12hour = GetTimeAMPM();

  switch (format) {
    case LongDateTime :
    case LongTime :
    case MediumDateTime :
    case ShortDateTime :
    case ShortTime :
      if (!is12hour)
        fmt = "h";

      fmt += "h" + tsep + "mm";

      switch (format) {
        case LongDateTime :
        case LongTime :
          fmt += tsep + "ss";
      }

      if (is12hour)
        fmt += "a";
  }

  switch (format) {
    case LongDateTime :
    case MediumDateTime :
    case ShortDateTime :
      fmt += ' ';
  }

  switch (format) {
    case LongDateTime :
    case LongDate :
      fmt += "wwww";
      switch (GetDateOrder()) {
        case MonthDayYear :
          fmt += "MMMM d, yyyy";
          break;
        case DayMonthYear :
          fmt += "d MMMM yyyy";
          break;
        case YearMonthDay :
          fmt += "yyyy MMMM d";
      }
      break;

    case MediumDateTime :
    case MediumDate :
      fmt += "www";
      switch (GetDateOrder()) {
        case MonthDayYear :
          fmt += "MMM d, yy";
          break;
        case DayMonthYear :
          fmt += "d MMM yy";
          break;
        case YearMonthDay :
          fmt += "yy MMM d";
      }

    case ShortDateTime :
    case ShortDate :
      PString dsep = GetDateSeparator();
      switch (GetDateOrder()) {
        case MonthDayYear :
          fmt = "MM" + dsep + "dd" + dsep + "yy";
          break;
        case DayMonthYear :
          fmt = "dd" + dsep + "MM" + dsep + "yy";
          break;
        case YearMonthDay :
          fmt = "yy" + dsep + "MM" + dsep + "dd";
      }
  }

  return AsString(fmt);
}


PString PTime::AsString(const char * format) const
{
  PAssert(format != NULL, PInvalidParameter);

  BOOL is12hour = strchr(format, 'a') != NULL;

  PString str;
  struct tm * t = localtime(&theTime);
  PINDEX repeatCount;

  while (*format != '\0') {
    repeatCount = 1;
    switch (*format) {
      case 'a' :
        while (*++format == 'a')
          ;
        if (t->tm_hour < 12)
          str += GetTimeAM();
        else
          str += GetTimePM();
        break;

      case 'h' :
        while (*++format == 'h')
          repeatCount++;
        str += psprintf("%0*u", repeatCount,
                                is12hour ? (t->tm_hour+11)%12+1 : t->tm_hour);
        break;

      case 'm' :
        while (*++format == 'm')
          repeatCount++;
        str += psprintf("%0*u", repeatCount, t->tm_min);
        break;

      case 's' :
        while (*++format == 's')
          repeatCount++;
        str += psprintf("%0*u", repeatCount, t->tm_sec);
        break;

      case 'w' :
        while (*++format == 'w')
          repeatCount++;
        str += GetDayName((Weekdays)t->tm_wday, repeatCount <= 3);
        break;

      case 'M' :
        while (*++format == 'M')
          repeatCount++;
        if (repeatCount < 3)
          str += psprintf("%0*u", repeatCount, t->tm_mon+1);
        else
          str += GetMonthName((Months)(t->tm_mon+1), repeatCount == 3);
        break;

      case 'd' :
        while (*++format == 'd')
          repeatCount++;
        str += psprintf("%0*u", repeatCount, t->tm_mday);
        break;

      case 'y' :
        while (*++format == 'y')
          repeatCount++;
        if (repeatCount < 3)
          str += psprintf("%02u", t->tm_year%100);
        else
          str += psprintf("%04u", t->tm_year+1900);
        break;

      default :
        str += *format++;
    }
  }

  return str;
}


#endif


///////////////////////////////////////////////////////////////////////////////
// PTimer

#if defined(_PTIMER)

PTimer::PTimer(long millisecs, int seconds, int minutes, int hours, int days)
  : PTimeInterval(millisecs, seconds, minutes, hours, days),
    owner(PProcess::Current()->GetTimerList())
{
  state = Stopped;
  inTimeout = FALSE;
}


PTimer::PTimer(const PTimer & timer)
  : PTimeInterval(timer), owner(timer.owner)
{
  state = Stopped;
  inTimeout = FALSE;
}


PTimer & PTimer::operator=(const PTimer & timer)
{
  PTimeInterval::operator=(timer);
  owner = timer.owner;
  state = Stopped;
  inTimeout = FALSE;
  return *this;
}


PTimer::~PTimer()
{
  PAssert(!inTimeout, "Timer destroyed in OnTimeout()");
  if (state == Running)
    owner->Remove(this);
}


PObject::Comparison PTimer::Compare(const PObject & obj) const
{
  const PTimer & other = (const PTimer &)obj;
  return targetTime < other.targetTime ? LessThan :
         targetTime > other.targetTime ? GreaterThan : EqualTo;
}


void PTimer::Start(BOOL once)
{
  if (state == Running && !inTimeout)
    owner->Remove(this);
  oneshot = once;
  targetTime = Tick() + *this;
  if (!inTimeout)
    owner->Append(this);
  state = Running;
}


void PTimer::Stop()
{
  if (state == Running && !inTimeout)
    owner->Remove(this);
  state = Stopped;
  targetTime = PMaxTimeInterval;
}


void PTimer::Pause()
{
  if (state == Running) {
    if (!inTimeout)
      owner->Remove(this);
    pauseLeft = targetTime - Tick();
    state = Paused;
  }
}


void PTimer::Resume()
{
  if (state == Paused) {
    targetTime = Tick() + pauseLeft;
    if (!inTimeout)
      owner->Append(this);
    state = Running;
  }
}


void PTimer::OnTimeout()
{
  // Empty callback function
}


PTimeInterval PTimerList::Process()
{
  PTimer * timer = (PTimer *)GetAt(0); // Get earliest timer value
  if (timer == NULL)
    return PMaxTimeInterval;

  PTimeInterval now = PTimer::Tick();
  if (now > timer->targetTime) {
    Remove(timer);
    timer->inTimeout = TRUE;
    if (timer->oneshot)
      timer->state = PTimer::Stopped;
    else
      timer->targetTime = now + *timer;
    timer->OnTimeout();
    timer->inTimeout = FALSE;
    if (timer->state == PTimer::Running)
      Append(timer);
  }

  timer = (PTimer *)GetAt(0); // Get new earliest timer value
  if (timer == NULL)
    return PMaxTimeInterval;

  now = PTimer::Tick();
  if (timer->targetTime <= now)
    return PTimeInterval(1);

  return timer->targetTime - now;
}


#endif


///////////////////////////////////////////////////////////////////////////////
// PChannel

#if defined(_PCHANNEL)

PChannelStreamBuffer::PChannelStreamBuffer(PChannel * chan)
  : channel(PAssertNULL(chan))
{
  setb(buffer, &buffer[sizeof(buffer)]);
  unbuffered(FALSE);
}


int PChannelStreamBuffer::overflow(int c)
{
  if (pbase() == NULL) {
    if (eback() == 0)
      setp(base(), ebuf());
    else {
      char * halfway = base()+(ebuf()-base())/2;
      setp(base(), halfway);
      setg(halfway, ebuf(), ebuf());
    }
  }

  int bufSize = out_waiting();
  if (bufSize != 0) {
    setp(pbase(), epptr());
    if (!channel->Write(pbase(), bufSize))
      return EOF;
  }

  if (c != EOF)
    if (!channel->Write(&c, 1))
      return EOF;

  return 0;
}


int PChannelStreamBuffer::underflow()
{
  if (eback() == NULL) {
    if (pbase() == 0)
      setg(base(), ebuf(), ebuf());
    else {
      char * halfway = base()+(ebuf()-base())/2;
      setp(base(), halfway);
      setg(halfway, ebuf(), ebuf());
    }
  }

  if (gptr() != egptr())
    return *gptr();

  if (!channel->Read(eback(), egptr() - eback()) ||
                                  channel->GetErrorCode() != PChannel::NoError)
    return EOF;

  PINDEX count = channel->GetLastReadCount();
  char * p = egptr() - count;
  memmove(p, eback(), count);
  setg(eback(), p, egptr());
  return *p;
}


int PChannelStreamBuffer::sync()
{
  int inAvail = in_avail();
  if (inAvail != 0) {
    setg(eback(), egptr(), egptr());
    if (channel->IsDescendant(PFile::Class()))
      ((PFile *)channel)->SetPosition(-inAvail, PFile::Current);
  }

  if (out_waiting() != 0)
    return overflow();

  return 0;
}


streampos PChannelStreamBuffer::seekoff(streamoff off, ios::seek_dir dir, int)
{
  sync();
  if (!channel->IsDescendant(PFile::Class()))
    return -1;
  ((PFile *)channel)->SetPosition(off, (PFile::FilePositionOrigin)dir);
  return ((PFile *)channel)->GetPosition();
}


PChannel::PChannel()
  : readTimeout(PMaxTimeInterval), writeTimeout(PMaxTimeInterval)
{
  osError = 0;
  lastError = NoError;
  lastReadCount = lastWriteCount = 0;
  init(new PChannelStreamBuffer(this));
}


void PChannel::DestroyContents()
{
  delete rdbuf();
  init(NULL);
}

void PChannel::CloneContents(const PChannel *)
{
  init(new PChannelStreamBuffer(this));
}

void PChannel::CopyContents(const PChannel & c)
{
  init(c.rdbuf());
  ((PChannelStreamBuffer*)rdbuf())->channel = this;
}


int PChannel::ReadChar()
{
  BYTE c;
  BOOL retVal = Read(&c, 1);
  return (retVal && lastReadCount == 1) ? c : -1;
}


PString PChannel::ReadString(PINDEX len)
{
  PString str;
  if (!Read(str.GetPointer(len), len))
    return PString();
  str.SetSize(lastReadCount+1);
  return str;
}


BOOL PChannel::ReadAsync(void * buf, PINDEX len)
{
  BOOL retVal = Read(buf, len);
  OnReadComplete(buf, lastReadCount);
  return retVal;
}


void PChannel::OnReadComplete(void *, PINDEX)
{
}


BOOL PChannel::WriteChar(int c)
{
  PAssert(c >= 0 && c < 256, PInvalidParameter);
  char buf = (char)c;
  return Write(&buf, 1);
}


BOOL PChannel::WriteAsync(void * buf, PINDEX len)
{
  BOOL retVal = Write(buf, len);
  OnWriteComplete(buf, lastWriteCount);
  return retVal;
}


void PChannel::OnWriteComplete(void *, PINDEX)
{
}


#endif


///////////////////////////////////////////////////////////////////////////////
// PDirectory

#if defined(_PDIRECTORY)

istream & PDirectory::ReadFrom(istream & strm)
{
  strm >> path;
  Construct();
  return strm;
}


#endif


///////////////////////////////////////////////////////////////////////////////
// PFile

#if defined(_PFILE)

BOOL PFile::Rename(const PString & newname)
{
  Close();

  if (!Rename(path, newname))
    return FALSE;
  path = newname;
  return TRUE;
}


BOOL PFile::Close()
{
  if (os_handle < 0) {
    osError = EBADF;
    lastError = NotOpen;
    return FALSE;
  }

  rdbuf()->sync();

  BOOL ok = _close(os_handle) == 0;

  osError = ok ? 0 : errno;
  switch (osError) {
    case 0 :
      lastError = NoError;
      break;
    case ENOSPC :
      lastError = DiskFull;
      break;
    case EACCES :
      lastError = AccessDenied;
      break;
    case ENOMEM :
      lastError = NoMemory;
      break;
    case EBADF :
      lastError = NotOpen;
      break;
    default :
      lastError = Miscellaneous;
  }

  os_handle = -1;
  return ok;
}


BOOL PFile::Read(void * buffer, PINDEX amount)
{
  rdbuf()->sync();

  lastReadCount = _read(GetHandle(), buffer, amount);

  osError = errno;
  switch (osError) {
    case 0 :
      lastError = NoError;
      break;
    case EACCES :
      lastError = AccessDenied;
      break;
    case ENOMEM :
      lastError = NoMemory;
      break;
    case EBADF :
      lastError = NotOpen;
      break;
    default :
      lastError = Miscellaneous;
  }

  return lastReadCount > 0;
}


BOOL PFile::Write(const void * buffer, PINDEX amount)
{
  rdbuf()->sync();

  lastWriteCount = _write(GetHandle(), buffer, amount);

  osError = errno;
  switch (osError) {
    case 0 :
      lastError = NoError;
      break;
    case ENOSPC :
      lastError = DiskFull;
      break;
    case EACCES :
      lastError = AccessDenied;
      break;
    case ENOMEM :
      lastError = NoMemory;
      break;
    case EBADF :
      lastError = NotOpen;
      break;
    default :
      lastError = Miscellaneous;
  }

  return lastWriteCount >= amount;
}


BOOL PFile::Open(const PFilePath & name, OpenMode  mode, int opts)
{
  SetFilePath(name);
  return Open(mode, opts);
}


off_t PFile::GetLength() const
{
  off_t pos = _lseek(GetHandle(), 0, SEEK_CUR);
  off_t len = _lseek(GetHandle(), 0, SEEK_END);
  PAssert(_lseek(GetHandle(), pos, SEEK_SET) == pos, POperatingSystemError);
  return len;
}


BOOL PFile::SetPosition(long pos, FilePositionOrigin origin)
{
  return _lseek(GetHandle(), pos, origin) == pos;
}


BOOL PFile::Copy(const PString & oldname, const PString & newname)
{
  PFile oldfile(oldname, ReadOnly);
  if (!oldfile.IsOpen())
    return FALSE;

  PFile newfile(newname, WriteOnly, Create|Truncate);
  if (!newfile.IsOpen())
    return FALSE;

  PCharArray buffer(10000);

  off_t amount = oldfile.GetLength();
  while (amount > 10000) {
    if (!oldfile.Read(buffer.GetPointer(), 10000))
      return FALSE;
    if (!newfile.Write((const char *)buffer, 10000))
      return FALSE;
    amount -= 10000;
  }

  if (!oldfile.Read(buffer.GetPointer(), (int)amount))
    return FALSE;
  if (!oldfile.Write((const char *)buffer, (int)amount))
    return FALSE;

  return newfile.Close();
}


#endif


///////////////////////////////////////////////////////////////////////////////
// PStructuredFile

#if defined(_PSTRUCTUREDFILE)

BOOL PStructuredFile::Read(void * buffer)
{
  return PFile::Read(buffer, structureSize);
}
      

BOOL PStructuredFile::Write(void * buffer)
{
  return PFile::Write(buffer, structureSize);
}


#endif


///////////////////////////////////////////////////////////////////////////////
// PSerialChannel

#ifdef _PSERIALCHANNEL

PSerialChannel::PSerialChannel()
{
  Construct();
}


PSerialChannel::PSerialChannel(const PString & port, DWORD speed, BYTE data,
       Parity parity, BYTE stop, FlowControl inputFlow, FlowControl outputFlow)
{
  Construct();
  Open(port, speed, data, parity, stop, inputFlow, outputFlow);
}


PSerialChannel::PSerialChannel(PConfig & cfg)
{
  Construct();
  Open(cfg);
}


void PSerialChannel::CloneContents(const PSerialChannel *)
{
  PAssertAlways("Cannot clone serial channel");
}


void PSerialChannel::DestroyContents()
{
  Close();
  PChannel::DestroyContents();
}


static const char PortName[] = "PortName";
static const char PortSpeed[] = "PortSpeed";
static const char PortDataBits[] = "PortDataBits";
static const char PortParity[] = "PortParity";
static const char PortStopBits[] = "PortStopBits";
static const char PortInputFlow[] = "PortInputFlow";
static const char PortOutputFlow[] = "PortOutputFlow";


BOOL PSerialChannel::Open(PConfig & cfg)
{
  return Open(cfg.GetString(PortName, PSerialChannel::GetPortNames()[0]),
              cfg.GetInteger(PortSpeed, 9600),
              (BYTE)cfg.GetInteger(PortDataBits, 8),
              (PSerialChannel::Parity)cfg.GetInteger(PortParity, 1),
              (BYTE)cfg.GetInteger(PortStopBits, 1),
              (PSerialChannel::FlowControl)cfg.GetInteger(PortInputFlow, 1),
              (PSerialChannel::FlowControl)cfg.GetInteger(PortOutputFlow, 1));
}


void PSerialChannel::SaveSettings(PConfig & cfg)
{
  cfg.SetString(PortName, GetName());
  cfg.SetInteger(PortSpeed, GetSpeed());
  cfg.SetInteger(PortDataBits, GetDataBits());
  cfg.SetInteger(PortParity, GetParity());
  cfg.SetInteger(PortStopBits, GetStopBits());
  cfg.SetInteger(PortInputFlow, GetInputFlowControl());
  cfg.SetInteger(PortOutputFlow, GetOutputFlowControl());
}


#endif


///////////////////////////////////////////////////////////////////////////////
// PModem

#ifdef _PMODEM

PModem::PModem()
{
  status = Unopened;
}


PModem::PModem(const PString & port, DWORD speed, BYTE data,
      Parity parity, BYTE stop, FlowControl inputFlow, FlowControl outputFlow)
  : PSerialChannel(port, speed, data, parity, stop, inputFlow, outputFlow)
{
  status = IsOpen() ? Uninitialised : Unopened;
}


PModem::PModem(PConfig & cfg)
{
  status = Open(cfg) ? Uninitialised : Unopened;
}


BOOL PModem::Close()
{
  status = Unopened;
  return PSerialChannel::Close();
}


BOOL PModem::Open(const PString & port, DWORD speed, BYTE data,
      Parity parity, BYTE stop, FlowControl inputFlow, FlowControl outputFlow)
{
  if (!PSerialChannel::Open(port,
                            speed, data, parity, stop, inputFlow, outputFlow))
    return FALSE;

  status = Uninitialised;
  return TRUE;
}


static const char ModemInit[] = "ModemInit";
static const char ModemDeinit[] = "ModemDeinit";
static const char ModemPreDial[] = "ModemPreDial";
static const char ModemPostDial[] = "ModemPostDial";
static const char ModemBusy[] = "ModemBusy";
static const char ModemNoCarrier[] = "ModemNoCarrier";
static const char ModemConnect[] = "ModemConnect";
static const char ModemHangUp[] = "ModemHangUp";

BOOL PModem::Open(PConfig & cfg)
{
  if (!PSerialChannel::Open(cfg))
    return FALSE;

  status = Uninitialised;

  initCmd = cfg.GetString(ModemInit, "ATZ\\r\\w2sOK\\w100m");
  deinitCmd = cfg.GetString(ModemDeinit, "\\d2s+++\\d2sATH0\\r");
  preDialCmd = cfg.GetString(ModemPreDial, "ATDT");
  postDialCmd = cfg.GetString(ModemPostDial, "\\r");
  busyReply = cfg.GetString(ModemBusy, "BUSY");
  noCarrierReply = cfg.GetString(ModemNoCarrier, "NO CARRIER");
  connectReply = cfg.GetString(ModemConnect, "CONNECT");
  hangUpCmd = cfg.GetString(ModemHangUp, "\\d2s+++\\d2sATH0\\r");
  return TRUE;
}


void PModem::SaveSettings(PConfig & cfg)
{
  PSerialChannel::SaveSettings(cfg);
  cfg.SetString(ModemInit, initCmd);
  cfg.SetString(ModemDeinit, deinitCmd);
  cfg.SetString(ModemPreDial, preDialCmd);
  cfg.SetString(ModemPostDial, postDialCmd);
  cfg.SetString(ModemBusy, busyReply);
  cfg.SetString(ModemNoCarrier, noCarrierReply);
  cfg.SetString(ModemConnect, connectReply);
  cfg.SetString(ModemHangUp, hangUpCmd);
}


static int HexDigit(char c)
{
  if (!isxdigit(c))
    return 0;

  int hex = c - '0';
  if (hex < 10)
    return hex;

  hex -= 'A' - '9' - 1;
  if (hex < 16)
    return hex;

  return hex - ('a' - 'A');
}


enum {
  NextCharEndOfString = -1,
  NextCharDelay = -2,
  NextCharSend = -3,
  NextCharWait = -4
};

static int GetNextChar(const PString & command,
                                    PINDEX & pos, PTimeInterval * time = NULL)
{
  int temp;

  if (command[pos] == '\0')
    return NextCharEndOfString;

  if (command[pos] != '\\')
    return command[pos++];

  switch (command[++pos]) {
    case '\0' :
      return NextCharEndOfString;

    case 'a' : // alert (ascii value 7)
      pos++;
      return 7;

    case 'b' : // backspace (ascii value 8)
      pos++;
      return 8;

    case 'f' : // formfeed (ascii value 12)
      pos++;
      return 12;

    case 'n' : // newline (ascii value 10)
      pos++;
      return 10;

    case 'r' : // return (ascii value 13)
      pos++;
      return 13;

    case 't' : // horizontal tab (ascii value 9)
      pos++;
      return 9;

    case 'v' : // vertical tab (ascii value 11)
      pos++;
      return 11;

    case 'x' : // followed by hh  where nn is hex number (ascii value 0xhh)
      if (isxdigit(command[++pos])) {
        temp = HexDigit(command[pos++]);
        if (isxdigit(command[pos]))
          temp += HexDigit(command[pos++]);
        return temp;
      }
      return command[pos];

    case 's' :
      pos++;
      return NextCharSend;

    case 'd' : // ns  delay for n seconds/milliseconds
    case 'w' :
      temp = command[pos] == 'd' ? NextCharDelay : NextCharWait;
      long milliseconds = 0;
      while (isdigit(command[++pos]))
        milliseconds = milliseconds*10 + command[pos] - '0';
      if (milliseconds <= 0)
        milliseconds = 1;
      if (command[pos] == 'm')
        pos++;
      else {
        milliseconds *= 1000;
        if (command[pos] == 's')
          pos++;
      }
      if (time != NULL)
        *time = milliseconds;
      return temp;
  }

  if (command[pos] < '0' || command[pos] > '7')
    return command[pos++];

  // octal number
  temp = command[pos++] - '0';
  if (command[pos] < '0' || command[pos] > '7')
    return temp;

  temp += command[pos++] - '0';
  if (command[pos] < '0' || command[pos] > '7')
    return temp;

  temp += command[pos++] - '0';
  return temp;
}


static BOOL ReceiveString(int nextChar,
                            const PString & reply, PINDEX & pos, PINDEX start)
{
  if (nextChar != GetNextChar(reply, pos)) {
    pos = start;
    return FALSE;
  }

  PINDEX dummyPos = pos;
  return GetNextChar(reply, dummyPos) < 0;
}


static int ReadCharWithTimeout(PModem & modem, PTimeInterval & timeout)
{
  modem.SetReadTimeout(timeout);
  PTimeInterval lastReadStart = PTimer::Tick();
  int c;
  if ((c = modem.ReadChar()) < 0) // Timeout or aborted
    return FALSE;
  timeout -= PTimer::Tick() - lastReadStart;
  return c;
}


BOOL PModem::SendString(const PString & command)
{
  int nextChar = NextCharSend;
  PINDEX sendPosition = 0;
  PTimeInterval timeout;
  SetWriteTimeout(10000);

  while (!CanRead()) { // not aborted
    nextChar = GetNextChar(command, sendPosition, &timeout);
    switch (nextChar) {
      default :
        if (!WriteChar(nextChar))
          return FALSE;
        break;

      case NextCharEndOfString :
        return TRUE;  // Success!!

      case NextCharSend :
        break;

      case NextCharDelay : // Delay in send
        PThread::Current()->Sleep(timeout);
        break;

      case NextCharWait : // Wait for reply
        PINDEX receivePosition = sendPosition;
        if (GetNextChar(command, receivePosition) < 0) {
          SetReadTimeout(timeout);
          while (ReadChar() >= 0)
            if (CanRead()) // aborted
              return FALSE;
        }
        else {
          receivePosition = sendPosition;
          do {
            if (CanRead()) // aborted
              return FALSE;
            if ((nextChar = ReadCharWithTimeout(*this, timeout)) < 0)
              return FALSE;
          } while (!ReceiveString(nextChar,
                                     command, receivePosition, sendPosition));
          nextChar = GetNextChar(command, receivePosition);
          sendPosition = receivePosition;
        }
    }
  }

  return FALSE;
}


BOOL PModem::CanInitialise() const
{
  switch (status) {
    case Unopened :
    case Initialising :
    case Dialling :
    case AwaitingResponse :
    case HangingUp :
    case Deinitialising :
    case SendingUserCommand :
      return FALSE;
  }
  return TRUE;
}


BOOL PModem::Initialise()
{
  if (CanInitialise()) {
    status = Initialising;
    if (SendString(initCmd)) {
      status = Initialised;
      return TRUE;
    }
    status = InitialiseFailed;
  }
  return FALSE;
}


BOOL PModem::CanDeinitialise() const
{
  switch (status) {
    case Unopened :
    case Initialising :
    case Dialling :
    case AwaitingResponse :
    case Connected :
    case HangingUp :
    case Deinitialising :
    case SendingUserCommand :
      return FALSE;
  }
  return TRUE;
}


BOOL PModem::Deinitialise()
{
  if (CanDeinitialise()) {
    status = Deinitialising;
    if (SendString(deinitCmd)) {
      status = Uninitialised;
      return TRUE;
    }
    status = DeinitialiseFailed;
  }
  return FALSE;
}


BOOL PModem::CanDial() const
{
  switch (status) {
    case Unopened :
    case Uninitialised :
    case Initialising :
    case InitialiseFailed :
    case Dialling :
    case AwaitingResponse :
    case Connected :
    case HangingUp :
    case Deinitialising :
    case DeinitialiseFailed :
    case SendingUserCommand :
      return FALSE;
  }
  return TRUE;
}


BOOL PModem::Dial(const PString & number)
{
  if (!CanDial())
    return FALSE;

  status = Dialling;
  if (!SendString(preDialCmd + "\\s" + number + postDialCmd)) {
    status = DialFailed;
    return FALSE;
  }

  status = AwaitingResponse;

  PTimeInterval timeout = 120000;
  PINDEX connectPosition = 0;
  PINDEX busyPosition = 0;
  PINDEX noCarrierPosition = 0;

  for (;;) {
    int nextChar;
    if ((nextChar = ReadCharWithTimeout(*this, timeout)) < 0)
      return FALSE;

    if (ReceiveString(nextChar, connectReply, connectPosition, 0))
      break;

    if (ReceiveString(nextChar, busyReply, busyPosition, 0)) {
      status = LineBusy;
      return FALSE;
    }

    if (ReceiveString(nextChar, noCarrierReply, noCarrierPosition, 0)) {
      status = NoCarrier;
      return FALSE;
    }
  }

  status = Connected;
  return TRUE;
}


BOOL PModem::CanHangUp() const
{
  switch (status) {
    case Unopened :
    case Uninitialised :
    case Initialising :
    case InitialiseFailed :
    case Dialling :
    case AwaitingResponse :
    case HangingUp :
    case Deinitialising :
    case SendingUserCommand :
      return FALSE;
  }
  return TRUE;
}


BOOL PModem::HangUp()
{
  if (CanHangUp()) {
    status = HangingUp;
    if (SendString(hangUpCmd)) {
      status = Initialised;
      return TRUE;
    }
    status = HangUpFailed;
  }
  return FALSE;
}


BOOL PModem::CanSendUser() const
{
  switch (status) {
    case Unopened :
    case Uninitialised :
    case Initialising :
    case InitialiseFailed :
    case Dialling :
    case AwaitingResponse :
    case HangingUp :
    case Deinitialising :
    case SendingUserCommand :
      return FALSE;
  }
  return TRUE;
}


BOOL PModem::SendUser(const PString & str)
{
  if (CanSendUser()) {
    Status oldStatus = status;
    status = SendingUserCommand;
    if (SendString(str)) {
      status = oldStatus;
      return TRUE;
    }
    status = oldStatus;
  }
  return FALSE;
}


void PModem::Abort()
{
  switch (status) {
    case Initialising :
      status = InitialiseFailed;
      break;
    case Dialling :
    case AwaitingResponse :
      status = DialFailed;
      break;
    case HangingUp :
      status = HangUpFailed;
      break;
    case Deinitialising :
      status = DeinitialiseFailed;
      break;
  }
}


BOOL PModem::CanRead() const
{
  switch (status) {
    case Unopened :
    case Initialising :
    case Dialling :
    case AwaitingResponse :
    case HangingUp :
    case Deinitialising :
    case SendingUserCommand :
      return FALSE;
  }
  return TRUE;
}


#endif


///////////////////////////////////////////////////////////////////////////////
// PConfig

#ifdef _PCONFIG

BOOL PConfig::GetBoolean(const char * section, const char * key, BOOL dflt)
{
  PString str = GetString(section, key, dflt ? "T" : "F").ToUpper();
  return str[0] == 'T' || str[0] == 'Y' || str.AsInteger() != 0;
}


void PConfig::SetBoolean(const char * section, const char * key, BOOL value)
{
  SetString(section, key, value ? "True" : "False");
}


long PConfig::GetInteger(const char * section, const char * key, long dflt)
{
  return GetString(section, key, psprintf("%li", dflt)).AsInteger();
}


void PConfig::SetInteger(const char * section, const char * key, long value)
{
  SetString(section, key, psprintf("%li", value));
}


double PConfig::GetReal(const char * section, const char * key, double dflt)
{
  return GetString(section, key, psprintf("%f", dflt)).AsReal();
}


void PConfig::SetReal(const char * section, const char * key, double value)
{
  SetString(section, key, psprintf("%f", value));
}


#endif

///////////////////////////////////////////////////////////////////////////////
// PArgList

#if defined(_PARGLIST)

PArgList::PArgList()
{
  arg_values = NULL;
  arg_count  = 0;
}


PArgList::PArgList(int theArgc, char ** theArgv, const char * theArgumentSpec)
{
  // get the program name and path
  SetArgs(theArgc, theArgv);

  if (theArgumentSpec != NULL)
    // we got an argument spec - so process them
    Parse(theArgumentSpec);
  else {
    // we have no argument spec, delay parsing the arguments until later
    arg_values = NULL;
    arg_count  = 0;
  }
}


PArgList::~PArgList()
{
  free(argumentSpec);
  free(optionCount);
  free(argumentList);
}


void PArgList::SetArgs(int argc, char ** argv)
{
  // save argv and and argc for later
  arg_values = argv;
  arg_count = argc;
  shift = 0;
}


void PArgList::Parse(const char * theArgumentSpec)
{
  char  c;
  char *p;
  int   l;

  // allocate and initialise storage
  argumentSpec = strdup (theArgumentSpec);
  l = strlen (argumentSpec);
  optionCount   = (PINDEX *) calloc (l, sizeof (PINDEX));
  argumentList = (char **) calloc (l, sizeof (char *));

  while (arg_count > 0 && arg_values[0][0] == '-') {
    while ((c = *++arg_values[0]) != 0) {
      if ((p = strchr (argumentSpec, c)) == NULL)
        UnknownOption (c);
      else {
        optionCount[p-argumentSpec]++;
        if (p[1] == ':') {
          if (*++(arg_values[0]))
            argumentList[p-argumentSpec] = arg_values[0];
          else {
            if (arg_count < 2) {
              optionCount[p-argumentSpec] = 0;
              MissingArgument (c);
            }
            else {
              --arg_count;
              argumentList [p-argumentSpec] = *++arg_values;
            }
          }
          break;
        }
      }
    }
    --arg_count;
    ++arg_values;
  }
}


PINDEX PArgList::GetOptionCount(char option) const
{
  char * p = strchr(argumentSpec, option);
  return (p == NULL ? 0 : optionCount[p-argumentSpec]);
}


PINDEX PArgList::GetOptionCount(const char * option) const
{
  // Future enhancement to have long option names
  return GetOptionCount(*option);
}


PString PArgList::GetOptionString(char option, const char * dflt) const
{
  char * p = strchr(argumentSpec, option);
  if (p != NULL)
    return argumentList[p-argumentSpec];

  if (dflt != NULL)
    return dflt;

  return PString();
}


PString PArgList::GetOptionString(const char * option, const char * dflt) const
{
  // Future enhancement to have long option names
  return GetOptionString(*option, dflt);
}


PString PArgList::GetParameter(PINDEX num) const
{
  if ((num+shift) < arg_count)
    return arg_values[num+shift];

  IllegalArgumentIndex(num+shift);
  return PString();
}


void PArgList::Shift(int sh) 
{
  if ((sh < 0) && (shift > 0))
    shift -= sh;
  else if ((sh > 0) && (shift < arg_count))
    shift += sh;
}


void PArgList::IllegalArgumentIndex(PINDEX idx) const
{
#ifdef _WINDLL
  idx = 1;
#else
  cerr << "attempt to access undefined argument at index "
       << idx << endl;
#endif
}
 

void PArgList::UnknownOption(char option) const
{
#ifdef _WINDLL
  option = ' ';
#else
  cerr << "unknown option \"" << option << "\"\n";
#endif
}


void PArgList::MissingArgument(char option) const
{
#ifdef _WINDLL
  option = ' ';
#else
  cerr << "option \"" << option << "\" requires argument\n";
#endif
}


#endif


///////////////////////////////////////////////////////////////////////////////
// PProcess

#if defined(_PPROCESS)

PProcess::~PProcess()
{
#ifdef PMEMORY_CHECK
  extern void PDumpMemoryLeaks();
  PDumpMemoryLeaks();
#endif
}

PObject::Comparison PProcess::Compare(const PObject & obj) const
{
  return executableName.Compare(((const PProcess &)obj).executableName);
}


void PProcess::PreInitialise(int argc, char ** argv)
{
  terminationValue = 0;

  arguments.SetArgs(argc-1, argv+1);

  executableFile = PString(argv[0]);
  executableName = executableFile.GetTitle().ToLower();

  InitialiseProcessThread();
}


void PProcess::Terminate()
{
#ifdef _WINDLL
  FatalExit(Termination());
#else
  exit(terminationValue);
#endif
}


void PProcess::SetTerminationValue(int value)
{
  terminationValue = value;
}


int PProcess::GetTerminationValue() const
{
  return terminationValue;
}



#endif


// End Of File ///////////////////////////////////////////////////////////////

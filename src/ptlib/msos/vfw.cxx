/*
 * vfw.cxx
 *
 * Classes to support streaming video input (grabbing) and output.
 *
 * Portable Windows Library
 *
 * Copyright (c) 1993-2000 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Portable Windows Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *                 Walter H Whitlock (twohives@nc.rr.com)
 */

#include <ptlib.h>

#if P_VIDEO

#define P_FORCE_STATIC_PLUGIN 1

#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>
#include <ptlib/pluginmgr.h>
#include <ptclib/delaychan.h>

#include <windowsx.h>


#if P_VFW_CAPTURE

#ifdef _MSC_VER
#pragma comment(lib, "vfw32.lib")
#pragma comment(lib, "gdi32.lib")
#endif


#include <vfw.h>

#ifdef __MINGW32__

#define VHDR_DONE       0x00000001
#define VHDR_KEYFRAME   0x00000008

extern "C" {
#ifdef _MSC_VER
BOOL VFWAPI capGetDriverDescriptionA (WORD wDriverIndex, LPSTR lpszName,
              int cbName, LPSTR lpszVer, int cbVer);
#endif
}

#endif // __MINGW32


#define PTraceModule() "VfW Grab"

/**This class defines a video input device.
 */
class PVideoInputDevice_VideoForWindows : public PVideoInputDevice
{
  PCLASSINFO(PVideoInputDevice_VideoForWindows, PVideoInputDevice);

  public:
    /** Create a new video input device.
     */
    PVideoInputDevice_VideoForWindows();

    /**Close the video input device on destruction.
      */
    ~PVideoInputDevice_VideoForWindows();


    /** Is the device a camera, and obtain video
     */
    static PStringArray GetInputDeviceNames();

    virtual PStringArray GetDeviceNames() const
      { return GetInputDeviceNames(); }

    /**Open the device given the device name.
      */
    virtual PBoolean Open(
      const PString & deviceName,   /// Device name to open
      PBoolean startImmediate = true    /// Immediately start device
    );

    /**Determine if the device is currently open.
      */
    virtual PBoolean IsOpen();

    /**Close the device.
      */
    virtual PBoolean Close();

    /**Retrieve a list of Device Capabilities
      */
    virtual bool GetDeviceCapabilities(
      Capabilities * /*caps*/         ///< List of supported capabilities
    );
    static PBoolean GetInputDeviceCapabilities(const PString & deviceName, Capabilities * capabilities);

    /**Start the video device I/O.
      */
    virtual PBoolean Start();

    /**Stop the video device I/O capture.
      */
    virtual PBoolean Stop();

    /**Determine if the video device I/O capture is in progress.
      */
    virtual PBoolean IsCapturing();

    /**Set the colour format to be used.
       Note that this function does not do any conversion. If it returns true
       then the video device does the colour format in native mode.

       To utilise an internal converter use the SetColourFormatConverter()
       function.

       Default behaviour sets the value of the colourFormat variable and then
       returns true.
    */
    virtual PBoolean SetColourFormat(
      const PString & colourFormat // New colour format for device.
    );

    /**Set the video frame rate to be used on the device.

       Default behaviour sets the value of the frameRate variable and then
       returns true.
    */
    virtual PBoolean SetFrameRate(
      unsigned rate  /// Frames  per second
    );

    /**Set the frame size to be used.

       Note that devices may not be able to produce the requested size, and
       this function will fail.  See SetFrameSizeConverter().

       Default behaviour sets the frameWidth and frameHeight variables and
       returns true.
    */
    virtual PBoolean SetFrameSize(
      unsigned width,   /// New width of frame
      unsigned height   /// New height of frame
    );

    /**Get the maximum frame size in bytes.

       Note a particular device may be able to provide variable length
       frames (eg motion JPEG) so will be the maximum size of all frames.
      */
    virtual PINDEX GetMaxFrameBytes();

    virtual bool SetCaptureMode(unsigned mode);
    virtual int GetCaptureMode() const { return m_useVideoMode; }


  protected:
    virtual bool InternalGetFrameData(BYTE * buffer, PINDEX & bytesReturned, bool & keyFrame, bool wait);
    PBoolean VerifyHardwareFrameSize(unsigned width, unsigned height);

    PDECLARE_NOTIFIER(PThread, PVideoInputDevice_VideoForWindows, HandleCapture);

    static LRESULT CALLBACK ErrorHandler(HWND hWnd, int id, LPCSTR err);
    LRESULT HandleError(int id, LPCSTR err);
    static LRESULT CALLBACK VideoHandler(HWND hWnd, LPVIDEOHDR vh);
    LRESULT HandleVideo(LPVIDEOHDR vh);
    PBoolean InitialiseCapture();

    PThread      * m_captureThread;
    PSyncPoint     m_threadStarted;

    HWND           m_hCaptureWindow;
    PDECLARE_MUTEX(m_operationMutex);

    PSyncPoint     m_frameAvailable;
    bool           m_useVideoMode;
    LPBYTE         m_lastFrameData;
    unsigned       m_lastFrameSize;
    PDECLARE_MUTEX(m_lastFrameMutex);
    bool           m_isCapturingNow;
    PAdaptiveDelay m_pacing;
};


///////////////////////////////////////////////////////////////////////////////

class PCapStatus : public CAPSTATUS
{
  public:
    PCapStatus(HWND hWnd);
    PBoolean IsOK()
       { return uiImageWidth != 0; }
};

///////////////////////////////////////////////////////////////////////////////

static struct FormatTableEntry { 
  const char * colourFormat; 
  WORD  bitCount;
  PBoolean  negHeight; // MS documentation suggests that negative height will request
                  // top down scan direction from video driver
                  // HOWEVER, not all drivers honor this request
  DWORD compression; 
} FormatTable[] = {
  { "BGR32",   32, TRUE,  BI_RGB },
  { "BGR24",   24, TRUE,  BI_RGB },
  { "Grey",     8, TRUE,  BI_RGB },
  { "Gray",     8, TRUE,  BI_RGB },

  { "RGB565",  16, TRUE,  BI_BITFIELDS },
  { "RGB555",  15, TRUE,  BI_BITFIELDS },

  // http://support.microsoft.com/support/kb/articles/q294/8/80.asp
  { PTLIB_VIDEO_YUV420P, 12, FALSE, mmioFOURCC('I','Y','U','V') },
  { "IYUV",    12, FALSE, mmioFOURCC('I','Y','U','V') }, // Synonym for IYUV
  { "I420",    12, FALSE, mmioFOURCC('I','4','2','0') }, // Synonym for IYUV
  { "YV12",    12, FALSE, mmioFOURCC('Y','V','1','2') }, // same as IYUV except that U and V planes are switched
  { "YUV422",  16, FALSE, mmioFOURCC('Y','U','Y','2') },
  { "YUY2",    16, FALSE, mmioFOURCC('Y','U','Y','2') },
  { "UYVY",    16, FALSE, mmioFOURCC('U','Y','V','Y') }, // Like YUY2 except for ordering
  { "YVYU",    16, FALSE, mmioFOURCC('Y','V','Y','U') }, // Like YUY2 except for ordering
  { "YVU9",    16, FALSE, mmioFOURCC('Y','V','U','9') },
  { "MJPEG",   12, FALSE, mmioFOURCC('M','J','P','G') },
  { NULL },
};


static struct {
    unsigned device_width, device_height;
} winTestResTable[] = {
    { 176, 144 },
    { 352, 288 },
    { 320, 240 },
    { 160, 120 },
    { 640, 480 },
    { 704, 576 },
    {1024, 768 },
};

///////////////////////////////////////////////////////////////////////////////

class PVideoDeviceBitmap : PBYTEArray
{
  public:
    // the following method is replaced by PVideoDeviceBitmap(HWND hWnd, WORD bpp)
    // PVideoDeviceBitmap(unsigned width, unsigned height, const PString & fmt);
    //returns object with gray color pallet if needed for 8 bpp formats
    PVideoDeviceBitmap(HWND hWnd, WORD bpp);
    // does not build color pallet
    PVideoDeviceBitmap(HWND hWnd); 
    // apply video format to capture device
    PBoolean ApplyFormat(HWND hWnd, const FormatTableEntry & formatTableEntry);

    BITMAPINFO * operator->() const 
    { return (BITMAPINFO *)GetPointer(); }
};

PVideoDeviceBitmap::PVideoDeviceBitmap(HWND hCaptureWindow)
{
  PINDEX sz = capGetVideoFormatSize(hCaptureWindow);
  SetSize(sz);
  if (!capGetVideoFormat(hCaptureWindow, GetPointer(), sz)) { 
    PTRACE(1, "capGetVideoFormat(hCaptureWindow) failed - " << ::GetLastError());
    SetSize(0);
    return;
  }
}

PVideoDeviceBitmap::PVideoDeviceBitmap(HWND hCaptureWindow, WORD bpp)
{
  PINDEX sz = capGetVideoFormatSize(hCaptureWindow);
  SetSize(sz);
  if (!capGetVideoFormat(hCaptureWindow, GetPointer(), sz)) { 
    PTRACE(1, "capGetVideoFormat(hCaptureWindow) failed - " << ::GetLastError());
    SetSize(0);
    return;
  }

  BITMAPINFO & bmi = *(BITMAPINFO*)GetPointer();
  if (8 == bpp && bpp != bmi.bmiHeader.biBitCount) {
    SetSize(sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256);
    RGBQUAD * bmiColors = bmi.bmiColors;
    for (int i = 0; i < 256; i++)
      bmiColors[i].rgbBlue  = bmiColors[i].rgbGreen = bmiColors[i].rgbRed = (BYTE)i;
  }
  bmi.bmiHeader.biBitCount = bpp;
}

PBoolean PVideoDeviceBitmap::ApplyFormat(HWND hWnd, const FormatTableEntry & formatTableEntry)
{
  // NB it is necessary to set the biSizeImage value appropriate to frame size
  // assume bmiHeader.biBitCount has already been set appropriatly for format
  BITMAPINFO & bmi = *(BITMAPINFO*)GetPointer();
  bmi.bmiHeader.biPlanes = 1;

  int height = bmi.bmiHeader.biHeight<0 ? -bmi.bmiHeader.biHeight : bmi.bmiHeader.biHeight;
  bmi.bmiHeader.biSizeImage = height*4*((bmi.bmiHeader.biBitCount * bmi.bmiHeader.biWidth + 31)/32);

  // set .biHeight according to .negHeight value
  if (formatTableEntry.negHeight)
    bmi.bmiHeader.biHeight = -height; 

#if PTRACING
  PTimeInterval startTime = PTimer::Tick();
#endif

  if (capSetVideoFormat(hWnd, GetPointer(), GetSize())) {
    PTRACE(3, "capSetVideoFormat succeeded: "
            << PString(formatTableEntry.colourFormat) << ' '
            << bmi.bmiHeader.biWidth << "x" << bmi.bmiHeader.biHeight
            << " sz=" << bmi.bmiHeader.biSizeImage << " time=" << (PTimer::Tick() - startTime));
    return true;
  }

  if (formatTableEntry.negHeight) {
    bmi.bmiHeader.biHeight = height; 
    if (capSetVideoFormat(hWnd, GetPointer(), GetSize())) {
      PTRACE(3, "capSetVideoFormat succeeded: "
              << PString(formatTableEntry.colourFormat) << ' '
              << bmi.bmiHeader.biWidth << "x" << bmi.bmiHeader.biHeight
              << " sz=" << bmi.bmiHeader.biSizeImage << " time=" << (PTimer::Tick() - startTime));
      return true;
    }
  }

  PTRACE(1, "capSetVideoFormat failed: "
          << (formatTableEntry.colourFormat != NULL ? formatTableEntry.colourFormat : "NO-COLOUR-FORMAT") << ' '
          << bmi.bmiHeader.biWidth << "x" << bmi.bmiHeader.biHeight
          << " sz=" << bmi.bmiHeader.biSizeImage << " time=" << (PTimer::Tick() - startTime)
          << " - lastError=" << ::GetLastError());
  return false;
}

///////////////////////////////////////////////////////////////////////////////

PCapStatus::PCapStatus(HWND hWnd)
{
  memset(this, 0, sizeof(*this));
  if (capGetStatus(hWnd, this, sizeof(*this)))
    return;

  PTRACE(1, "capGetStatus: failed - " << ::GetLastError());
}


///////////////////////////////////////////////////////////////////////////////
// PVideoInputDevice_VideoForWindows

PCREATE_VIDINPUT_PLUGIN(VideoForWindows);

PVideoInputDevice_VideoForWindows::PVideoInputDevice_VideoForWindows()
  : m_captureThread(NULL)
  , m_hCaptureWindow(NULL)
  , m_lastFrameSize(0)
  , m_isCapturingNow(false)
  , m_useVideoMode(false)
  , m_lastFrameData(NULL)
{
}


PVideoInputDevice_VideoForWindows::~PVideoInputDevice_VideoForWindows()
{
  delete m_lastFrameData;
  Close();
}


bool PVideoInputDevice_VideoForWindows::SetCaptureMode(unsigned mode)
{
  m_useVideoMode = mode != 0;

  // Do nothing if we are currently capturing (we don't support switching between picture- and video-mode during a capture).
  if(IsCapturing())
    return false;

  // Set the callback function for complete frames
  BOOL result;
  if (m_useVideoMode)
    result = capSetCallbackOnVideoStream(m_hCaptureWindow, VideoHandler);
  else
    result = capSetCallbackOnFrame(m_hCaptureWindow, VideoHandler);

  if (!result) {
    m_lastError = ::GetLastError();
    PTRACE(1, "Failed to set callback on VfW - " << m_lastError);
    return false;
  }

  CAPTUREPARMS parms;
  memset(&parms, 0, sizeof(parms));
  if (!capCaptureGetSetup(m_hCaptureWindow, &parms, sizeof(parms))) {
    m_lastError = ::GetLastError();
    PTRACE(1, "capCaptureGetSetup: failed - " << m_lastError);
    return false;
  }

  // For video mode we must tell VfW to work in a separate background thread, or our application will lock otherwise.
  if (m_useVideoMode) {
    parms.fYield = TRUE;
    parms.dwIndexSize = 324000;
  }
  else {
    parms.fYield = FALSE;
    parms.dwIndexSize = 0;
  }

  if (!capCaptureSetSetup(m_hCaptureWindow, &parms, sizeof(parms))) {
    m_lastError = ::GetLastError();
    PTRACE(1, "capCaptureSetSetup: failed - " << m_lastError);
    return false;
  }

  return true;
}


PBoolean PVideoInputDevice_VideoForWindows::Open(const PString & devName, PBoolean startImmediate)
{
  Close();

  m_operationMutex.Wait();

  m_deviceName = devName;

  m_captureThread = PThread::Create(PCREATE_NOTIFIER(HandleCapture), "VidIn");

  m_operationMutex.Signal();
  m_threadStarted.Wait();

  PWaitAndSignal mutex(m_operationMutex);

  if (m_hCaptureWindow == NULL) {
    delete m_captureThread;
    m_captureThread = NULL;
    return false;
  }

  if (startImmediate)
    return Start();

  return true;
}


PBoolean PVideoInputDevice_VideoForWindows::IsOpen() 
{
  return m_hCaptureWindow != NULL;
}


PBoolean PVideoInputDevice_VideoForWindows::Close()
{
  PWaitAndSignal mutex(m_operationMutex);

  if (!IsOpen())
    return false;
 
  Stop();

  ::PostThreadMessage(m_captureThread->GetThreadId(), WM_QUIT, 0, 0L);

  // Some brain dead drivers may hang so we provide a timeout.
  if (!PThread::WaitAndDelete(m_captureThread, 5000))
  {
      // Two things may happen if we are forced to terminate the capture thread:
      // 1. As the VIDCAP window is associated to that thread the OS itself will 
      //    close the window and release the driver
      // 2. the driver will not be released and we will not have video until we 
      //    terminate the process
      // Any of the two ios better than just hanging
      m_hCaptureWindow = NULL;
      PTRACE(1, "Capture thread failed to stop. Terminated");
  }

  return true;
}


PBoolean PVideoInputDevice_VideoForWindows::GetInputDeviceCapabilities(const PString & deviceName,
                                                                  Capabilities * capabilities)
{
  PVideoInputDevice_VideoForWindows instance;
  return instance.Open(deviceName, false) && instance.GetDeviceCapabilities(capabilities);
}


bool PVideoInputDevice_VideoForWindows::GetDeviceCapabilities(Capabilities * caps)
{
  for (PINDEX prefFormatIdx = 0; FormatTable[prefFormatIdx].colourFormat != NULL; prefFormatIdx++) {
    PVideoDeviceBitmap bi(m_hCaptureWindow, FormatTable[prefFormatIdx].bitCount); 
    bi->bmiHeader.biCompression = FormatTable[prefFormatIdx].compression;
    for (PINDEX prefResizeIdx = 0; prefResizeIdx < PARRAYSIZE(winTestResTable); prefResizeIdx++) {
      bi->bmiHeader.biWidth = winTestResTable[prefResizeIdx].device_width;
      bi->bmiHeader.biHeight = winTestResTable[prefResizeIdx].device_height;
      if (bi.ApplyFormat(m_hCaptureWindow, FormatTable[prefFormatIdx]) && caps != NULL) {
        PVideoFrameInfo frameInfo;
        frameInfo.SetFrameSize(winTestResTable[prefResizeIdx].device_width,
                               winTestResTable[prefResizeIdx].device_height);
        caps->m_frameSizes.push_back(frameInfo);
      }
    }
  }
  return !caps->m_frameSizes.empty();
}


PBoolean PVideoInputDevice_VideoForWindows::Start()
{
  PWaitAndSignal mutex(m_operationMutex);

  if (IsCapturing())
    return true;

  if (!m_useVideoMode) {
    m_isCapturingNow = true;
    return capGrabFrameNoStop(m_hCaptureWindow);
  }

  if (capCaptureSequenceNoFile(m_hCaptureWindow)) {
    PCapStatus status(m_hCaptureWindow);
    m_isCapturingNow = status.fCapturingNow;

    // As initializing the camera takes some time, and video-mode runs in a background thread, we need to wait for the first frame here.
    // Otherwise "InternalGetFrameData" might time-out.
    m_frameAvailable.Wait();
    return m_isCapturingNow;
  }

  m_lastError = ::GetLastError();
  PTRACE(1, "capCaptureSequenceNoFile: failed - " << m_lastError);
  return false;
}


PBoolean PVideoInputDevice_VideoForWindows::Stop()
{
  PWaitAndSignal mutex(m_operationMutex);

  if (!IsCapturing())
    return false;
  m_isCapturingNow = false;

  // If using the picture mode, we just need to wait for the very next frame ...
  if (!m_useVideoMode)
    return IsOpen() && m_frameAvailable.Wait(1000);

  // ... otherwise we need to explicitely stop capturing.
  if (capCaptureStop(m_hCaptureWindow))
    return true;

  m_lastError = ::GetLastError();
  PTRACE(1, "capCaptureStop: failed - " << m_lastError);
  return false;
}


PBoolean PVideoInputDevice_VideoForWindows::IsCapturing()
{
  return m_isCapturingNow;
}


PBoolean PVideoInputDevice_VideoForWindows::SetColourFormat(const PString & colourFmt)
{
  PWaitAndSignal mutex(m_operationMutex);

  if (!IsOpen())
    return PVideoDevice::SetColourFormat(colourFmt); // Not open yet, just set internal variables

  PBoolean running = IsCapturing();
  if (running)
    Stop();

  PString oldFormat = m_colourFormat;

  if (!PVideoDevice::SetColourFormat(colourFmt))
    return false;

  PINDEX i = 0;
  while (FormatTable[i].colourFormat != NULL && !(colourFmt *= FormatTable[i].colourFormat))
    i++;

  PVideoDeviceBitmap bi(m_hCaptureWindow, FormatTable[i].bitCount);

  if (FormatTable[i].colourFormat != NULL)
    bi->bmiHeader.biCompression = FormatTable[i].compression;
  else if (colourFmt.GetLength() == 4)
    bi->bmiHeader.biCompression = mmioFOURCC(colourFmt[0],colourFmt[1],colourFmt[2],colourFmt[3]);
  else {
    bi->bmiHeader.biCompression = 0xffffffff; // Indicate invalid colour format
    PVideoDevice::SetColourFormat(oldFormat);
    return false;
  }

  // set frame width and height
  bi->bmiHeader.biWidth = m_frameWidth;
  bi->bmiHeader.biHeight = m_frameHeight;
  if (!bi.ApplyFormat(m_hCaptureWindow, FormatTable[i])) {
    m_lastError = ::GetLastError();
    PVideoDevice::SetColourFormat(oldFormat);
    return false;
  }

  // Didn't do top down, tell everything we are up side down
  m_nativeVerticalFlip = FormatTable[i].negHeight && bi->bmiHeader.biHeight > 0;

  if (running)
    return Start();

  return true;
}


PBoolean PVideoInputDevice_VideoForWindows::SetFrameRate(unsigned rate)
{
  PWaitAndSignal mutex(m_operationMutex);

  if (!PVideoDevice::SetFrameRate(rate))
    return false;

  if (!IsOpen())
    return true; // Not open yet, just set internal variables

  PBoolean running = IsCapturing();
  if (running)
    Stop();

  CAPTUREPARMS parms;
  memset(&parms, 0, sizeof(parms));

  if (!capCaptureGetSetup(m_hCaptureWindow, &parms, sizeof(parms))) {
    m_lastError = ::GetLastError();
    PTRACE(1, "capCaptureGetSetup: failed - " << m_lastError);
    return false;
  }

  // keep current (default) framerate if 0==frameRate   
  if (0 != m_frameRate)
    parms.dwRequestMicroSecPerFrame = 1000000 / m_frameRate;

  parms.fMakeUserHitOKToCapture = false;
  parms.wPercentDropForError = 100;
  parms.fCaptureAudio = false;
  parms.fAbortLeftMouse = false;
  parms.fAbortRightMouse = false;
  parms.fLimitEnabled = false;

  if (!capCaptureSetSetup(m_hCaptureWindow, &parms, sizeof(parms))) {
    m_lastError = ::GetLastError();
    PTRACE(1, "capCaptureSetSetup: failed - " << m_lastError);
    return false;
  }
    
  if (running)
    return Start();

  return true;
}


PBoolean PVideoInputDevice_VideoForWindows::SetFrameSize(unsigned width, unsigned height)
{
  PWaitAndSignal mutex(m_operationMutex);

  if (!IsOpen())
    return PVideoDevice::SetFrameSize(width, height); // Not open yet, just set internal variables

  PBoolean running = IsCapturing();
  if (running)
    Stop();

  PVideoDeviceBitmap bi(m_hCaptureWindow); 
  PTRACE(5, "Changing frame size from "
         << bi->bmiHeader.biWidth << 'x' << bi->bmiHeader.biHeight << " to " << width << 'x' << height);

  PINDEX i = 0;
  while (FormatTable[i].colourFormat != NULL && !(m_colourFormat *= FormatTable[i].colourFormat))
    i++;

  bi->bmiHeader.biWidth = width;
  bi->bmiHeader.biHeight = height;
  if (!bi.ApplyFormat(m_hCaptureWindow, FormatTable[i])) {
    m_lastError = ::GetLastError();
    return false;
  }

  // Didn't do top down, tell everything we are up side down
  m_nativeVerticalFlip = FormatTable[i].negHeight && bi->bmiHeader.biHeight > 0;

  // verify that the driver really took the frame size
  if (!VerifyHardwareFrameSize(width, height)) 
    return false; 

  // frameHeight must be positive regardlesss of what the driver says
  if (0 > (int)height) 
    height = (unsigned)-(int)height;

  if (!PVideoDevice::SetFrameSize(width, height))
    return false;

  if (running)
    return Start();

  return true;
}


//return true if absolute value of height reported by driver 
//  is equal to absolute value of current frame height AND
//  width reported by driver is equal to current frame width
PBoolean PVideoInputDevice_VideoForWindows::VerifyHardwareFrameSize(unsigned width, unsigned height)
{
  PCapStatus status(m_hCaptureWindow);

  if (!status.IsOK())
    return false;

  if (width != status.uiImageWidth)
    return false;

  if (0 > (int)height)
    height = (unsigned)-(int)height;

  if (0 > (int)status.uiImageHeight)
    status.uiImageHeight = (unsigned)-(int)status.uiImageHeight;

  return (height == status.uiImageHeight);
}


PStringArray PVideoInputDevice_VideoForWindows::GetInputDeviceNames()
{
  PStringArray devices;

  for (WORD devId = 0; devId < 10; devId++) {
    char name[100];
    char version[200];
    if (capGetDriverDescription(devId, name, sizeof(name), version, sizeof(version)))
      devices.AppendString(name);
  }

  return devices;
}


PINDEX PVideoInputDevice_VideoForWindows::GetMaxFrameBytes()
{
  PWaitAndSignal mutex(m_operationMutex);

  if (!IsOpen())
    return 0;

  return GetMaxFrameBytesConverted(PVideoDeviceBitmap(m_hCaptureWindow)->bmiHeader.biSizeImage);
}


bool PVideoInputDevice_VideoForWindows::InternalGetFrameData(BYTE * buffer, PINDEX & bytesReturned, bool & keyFrame, bool wait)
{
  if (wait) {
    // Wait for frame to be available
    if (!m_frameAvailable.Wait(1000)) {
      PTRACE(1, "Timeout waiting for frame grab!");
      return false;
    }

    // Some camera drivers ignore the frame rate set in the CAPTUREPARMS structure,
    // so we have a fail safe delay here.
    m_pacing.Delay(1000 / GetFrameRate());
  }

  if (!m_frameAvailable.Wait(0)) {
    bytesReturned = 0;
    keyFrame = false;
    return true;
  }

  bool retval = false;

  m_lastFrameMutex.Wait();

  if (m_lastFrameData != NULL) {
    if (NULL != m_converter)
      retval = m_converter->Convert(m_lastFrameData, buffer, &bytesReturned);
    else {
      memcpy(buffer, m_lastFrameData, m_lastFrameSize);
      bytesReturned = m_lastFrameSize;
      retval = true;
    }
  }

  m_lastFrameMutex.Signal();

  if (!m_useVideoMode && m_isCapturingNow)
    capGrabFrameNoStop(m_hCaptureWindow);

  return retval;
}


LRESULT CALLBACK PVideoInputDevice_VideoForWindows::ErrorHandler(HWND hWnd, int id, LPCSTR err)
{
  if (hWnd == NULL)
    return false;

  return ((PVideoInputDevice_VideoForWindows *)capGetUserData(hWnd))->HandleError(id, err);
}


LRESULT PVideoInputDevice_VideoForWindows::HandleError(int PTRACE_PARAM(id), LPCSTR PTRACE_PARAM(err))
{
  PTRACE_IF(1, id != 0, "ErrorHandler: [id="<< id << "] " << err);
  return true;
}


LRESULT CALLBACK PVideoInputDevice_VideoForWindows::VideoHandler(HWND hWnd, LPVIDEOHDR vh)
{
  if (hWnd == NULL || capGetUserData(hWnd) == NULL)
    return false;

  return ((PVideoInputDevice_VideoForWindows *)capGetUserData(hWnd))->HandleVideo(vh);
}


LRESULT PVideoInputDevice_VideoForWindows::HandleVideo(LPVIDEOHDR vh)
{
  if ((vh->dwFlags&(VHDR_DONE|VHDR_KEYFRAME)) != 0) {
    m_lastFrameMutex.Wait();

    /**
    * As in video mode VfW captures in background, and hence might override the buffer of the current frame,
    * we must copy the frame's data into a separate buffer.
    */

    // If the size of the current frame is same as of the old ...
    //    -> ... simply copy the data of the new frame into the buffer ...
    if (m_lastFrameSize == vh->dwBytesUsed)
      memcpy(m_lastFrameData, vh->lpData, m_lastFrameSize);
    else {
      // ... otherwise delete the old buffer ...
      delete m_lastFrameData;

      // ... and allocate a new one.
      m_lastFrameSize = vh->dwBytesUsed;
      m_lastFrameData = new BYTE[m_lastFrameSize];

      memcpy(m_lastFrameData, vh->lpData, m_lastFrameSize);
    }

    m_lastFrameMutex.Signal();
    m_frameAvailable.Signal();
  }

  return true;
}


PBoolean PVideoInputDevice_VideoForWindows::InitialiseCapture()
{
  if ((m_hCaptureWindow = capCreateCaptureWindow("Capture Window",
                                                 WS_POPUP | WS_CAPTION,
                                                 CW_USEDEFAULT, CW_USEDEFAULT,
                                                 m_frameWidth + GetSystemMetrics(SM_CXFIXEDFRAME),
                                                 m_frameHeight + GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYFIXEDFRAME),
                                                 (HWND)0,
                                                 0)) == NULL) {
    m_lastError = ::GetLastError();
    PTRACE(1, "capCreateCaptureWindow failed - " << m_lastError);
    return false;
  }

  capSetCallbackOnError(m_hCaptureWindow, ErrorHandler);

  BOOL result = FALSE;
  if (m_useVideoMode)
    result = capSetCallbackOnVideoStream(m_hCaptureWindow, VideoHandler);
  else
    result = capSetCallbackOnFrame(m_hCaptureWindow, VideoHandler);

  if (!result) {
    m_lastError = ::GetLastError();
    PTRACE(1, "Failed to set callback on VfW - " << m_lastError);
    return false;
  }

  WORD devId;

#if PTRACING
  if (PTrace::CanTrace(4)) { // list available video capture drivers
    ostream & trace = PTRACE_BEGIN(5);
    trace << "Enumerating available video capture drivers:\n";
    for (devId = 0; devId < 10; devId++) { 
      char name[100];
      char version[200];
      if (capGetDriverDescription(devId, name, sizeof(name), version, sizeof(version)) ) 
        trace << "  Video device[" << devId << "] = " << name << ", " << version << '\n';
    }
    trace << PTrace::End;
  }
#endif

  if (m_deviceName.GetLength() == 1 && isdigit(m_deviceName[0]))
    devId = (WORD)(m_deviceName[0] - '0');
  else {
    for (devId = 0; devId < 10; devId++) {
      char name[100];
      char version[200];
      if (capGetDriverDescription(devId, name, sizeof(name), version, sizeof(version)) &&
          (m_deviceName *= name))
        break;
    }
  }

  capSetUserData(m_hCaptureWindow, this);

  // Use first driver available.
  if (!capDriverConnect(m_hCaptureWindow, devId)) {
    m_lastError = ::GetLastError();
    PTRACE(1, "capDriverConnect failed - " << m_lastError);
    return false;
  }

  CAPDRIVERCAPS driverCaps;
  memset(&driverCaps, 0, sizeof(driverCaps));
  if (!capDriverGetCaps(m_hCaptureWindow, &driverCaps, sizeof(driverCaps))) {
    m_lastError = ::GetLastError();
    PTRACE(1, "capGetDriverCaps failed - " << m_lastError);
    return false;
  }

  PTRACE(6, "Enumerating CAPDRIVERCAPS values:\n"
            "  driverCaps.wDeviceIndex           = " << driverCaps.wDeviceIndex        << "\n"
            "  driverCaps.fHasOverlay            = " << driverCaps.fHasOverlay         << "\n"
            "  driverCaps.fHasDlgVideoSource     = " << driverCaps.fHasDlgVideoSource  << "\n"
            "  driverCaps.fHasDlgVideoFormat     = " << driverCaps.fHasDlgVideoFormat  << "\n"
            "  driverCaps.fHasDlgVideoDisplay    = " << driverCaps.fHasDlgVideoDisplay << "\n"
            "  driverCaps.fCaptureInitialized    = " << driverCaps.fCaptureInitialized << "\n"
            "  driverCaps.fDriverSuppliesPalettes= " << driverCaps.fDriverSuppliesPalettes);
  
/*
  if (driverCaps.fHasOverlay)
    capOverlay(hCaptureWindow, true);
  else {
    capPreviewRate(hCaptureWindow, 66);
    capPreview(hCaptureWindow, true);
  }
*/
   
  capPreview(m_hCaptureWindow, false);

#if PTRACING
  if (PTrace::CanTrace(6)) {
    // Display log for every format set
    for (PINDEX prefFormatIdx = 0; FormatTable[prefFormatIdx].colourFormat != NULL; prefFormatIdx++) {
      PVideoDeviceBitmap bi(m_hCaptureWindow, FormatTable[prefFormatIdx].bitCount); 
      bi->bmiHeader.biCompression = FormatTable[prefFormatIdx].compression;
      for (PINDEX prefResizeIdx = 0; prefResizeIdx < PARRAYSIZE(winTestResTable); prefResizeIdx++) {
        bi->bmiHeader.biWidth = winTestResTable[prefResizeIdx].device_width;
        bi->bmiHeader.biHeight = winTestResTable[prefResizeIdx].device_height;
        bi.ApplyFormat(m_hCaptureWindow, FormatTable[prefFormatIdx]);
      }
    }
  }
#endif
  
  return SetFrameRate(m_frameRate) && SetColourFormatConverter(m_colourFormat.IsEmpty() ? PVideoFrameInfo::YUV420P() : m_colourFormat);
}


void PVideoInputDevice_VideoForWindows::HandleCapture(PThread &, P_INT_PTR)
{
  PBoolean initSucceeded = InitialiseCapture();

  if (initSucceeded) {
    m_threadStarted.Signal();

    MSG msg;
    while (::GetMessage(&msg, NULL, 0, 0))
      ::DispatchMessage(&msg);
  }

  PTRACE(5, "Disconnecting driver");
  capDriverDisconnect(m_hCaptureWindow);
  capSetUserData(m_hCaptureWindow, NULL);

  capSetCallbackOnError(m_hCaptureWindow, NULL);
  capSetCallbackOnVideoStream(m_hCaptureWindow, NULL);

  PTRACE(5, "Destroying VIDCAP window");
  DestroyWindow(m_hCaptureWindow);
  m_hCaptureWindow = NULL;

  // Signal the other thread we have completed, even if have error
  if (!initSucceeded)
    m_threadStarted.Signal();
}

#endif // P_VFW_CAPTURE


///////////////////////////////////////////////////////////////////////////////
// PVideoOutputDevice_Window

/**This class defines a video output device for RGB in a frame store.
 */
class PVideoOutputDevice_Window : public PVideoOutputDeviceRGB
{
  PCLASSINFO(PVideoOutputDevice_Window, PVideoOutputDeviceRGB);

  public:
    /** Create a new video output device.
     */
    PVideoOutputDevice_Window();

    /** Destroy a video output device.
     */
    ~PVideoOutputDevice_Window();

    /**Open the device given the device name.
      */
    virtual PBoolean Open(
      const PString & deviceName,   /// Device name (filename base) to open
      PBoolean startImmediate = true    /// Immediately start device
    );

    /**Determine if the device is currently open.
      */
    virtual PBoolean IsOpen();

    /**Close the device.
      */
    virtual PBoolean Close();

    /**Start the video device I/O display.
      */
    virtual PBoolean Start();

    /**Stop the video device I/O display.
      */
    virtual PBoolean Stop();

    /**Get a list of all of the devices available.
      */
    static PStringArray GetOutputDeviceNames();

    /**Get a list of all of the devices available.
      */
    virtual PStringArray GetDeviceNames() const
    { return GetOutputDeviceNames(); }

    enum SizeMode {
      NormalSize,
      HalfSize,
      DoubleSize,
      FullScreen,
      FixedSize,
      NumSizeModes
    };

    /**Get the number of video channels available on the device.
    */
    virtual int GetNumChannels() { return NumSizeModes; }

    /**Get the names of video channels available on the device.
    */
    virtual PStringArray GetChannelNames();

    /**Set the video channel to be used on the device.
       The channel number is an integer from 0 to GetNumChannels()-1. The
       special value of -1 will find the first working channel number.
    */
    virtual PBoolean SetChannel(
      int channelNumber  ///< New channel number for device.
    );

    /**Get the video channel to be used on the device.
    */
    virtual int GetChannel() const { return m_sizeMode; }

    /**Set the colour format to be used.
       Note that this function does not do any conversion. If it returns true
       then the video device does the colour format in native mode.

       To utilise an internal converter use the SetColourFormatConverter()
       function.

       Default behaviour sets the value of the colourFormat variable and then
       returns true.
    */
    virtual PBoolean SetColourFormat(
      const PString & colourFormat // New colour format for device.
    );

    /**Get the video conversion vertical flip state.
       Default action is to return false.
     */
    virtual PBoolean GetVFlipState();

    /**Set the video conversion vertical flip state.
       Default action is to return false.
     */
    virtual PBoolean SetVFlipState(
      PBoolean newVFlipState    /// New vertical flip state
    );

    /**Set the frame size to be used.

       Note that devices may not be able to produce the requested size, and
       this function will fail.  See SetFrameSizeConverter().

       Default behaviour sets the frameWidth and frameHeight variables and
       returns true.
    */
    virtual PBoolean SetFrameSize(
      unsigned width,   /// New width of frame
      unsigned height   /// New height of frame
    );

    /**Set a section of the output frame buffer.
      */
    virtual PBoolean FrameComplete();

    /**Get the position of the output device, where relevant. For devices such as
       files, this always returns zeros. For devices such as Windows, this is the
       position of the window on the screen.
      */
    virtual PBoolean GetPosition(
      int & x,  // X position of device surface
      int & y   // Y position of device surface
    ) const;

    /**Set the position of the output device, where relevant. For devices such as
       files, this does nothing. For devices such as Windows, this sets the
       position of the window on the screen.
       
       Returns: TRUE if the position can be set.
      */
    virtual bool SetPosition(
      int x,  // X position of device surface
      int y   // Y position of device surface
    );

    LRESULT WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam);

  protected:
    PDECLARE_NOTIFIER(PThread, PVideoOutputDevice_Window, HandleDisplay);
    static VOID CALLBACK CreateProc(_In_ HWND, _In_ UINT, _In_ UINT_PTR, _In_ DWORD);
    void CreateDisplayWindow();
    void Draw(HDC hDC);

    HWND       m_hWnd;
    HWND       m_hParent;
    DWORD      m_dwStyle;
    DWORD      m_dwExStyle;
    COLORREF   m_bgColour;
    int        m_rotation;
    bool       m_mouseEnabled;
    bool       m_hidden;
    PThread  * m_thread;
    PDECLARE_MUTEX(m_openCloseMutex);
    PSyncPoint m_started;
    BITMAPINFO m_bitmap;
    bool       m_flipped;
    POINT      m_lastPosition;
    SIZE       m_fixedSize;
    SizeMode   m_sizeMode;
    bool       m_showInfo;
    COLORREF   m_infoColour;
    UINT       m_infoOptions;
    PString    m_freezeText;
    COLORREF   m_freezeColour;
    UINT       m_freezeOptions;
    UINT       m_freezeTimeMs;
    bool       m_receivedFrame;
    atomic<unsigned> m_frameCount;
    atomic<unsigned> m_observedFrameRate;
};


#define DEFAULT_STYLE (WS_POPUP|WS_BORDER|WS_SYSMENU|WS_CAPTION)
#define DEFAULT_TITLE "Video Output"

PCREATE_VIDOUTPUT_PLUGIN_EX(Window,

  virtual const char * GetFriendlyName() const
  {
    return "Microsoft Windows Video Output";
  }

  virtual bool ValidateDeviceName(const PString & deviceName, P_INT_PTR /*userData*/) const
  {
    return deviceName.NumCompare(P_MSWIN_VIDEO_PREFIX) == PObject::EqualTo;
  }
)


///////////////////////////////////////////////////////////////////////////////
// PVideoOutputDeviceRGB

#undef PTraceModule
#define PTraceModule() PPlugin_PVideoOutputDevice_Window::ServiceName()

PVideoOutputDevice_Window::PVideoOutputDevice_Window()
  : m_hWnd(NULL)
  , m_hParent(NULL)
  , m_dwStyle(DEFAULT_STYLE)
  , m_dwExStyle(0)
  , m_bgColour(0) // Black
  , m_rotation(0)
  , m_mouseEnabled(true)
  , m_hidden(false)
  , m_thread(NULL)
  , m_flipped(false)
  , m_sizeMode(NormalSize)
  , m_showInfo(false)
  , m_infoColour(RGB(192,192,192))
  , m_infoOptions(DT_TOP|DT_RIGHT|DT_SINGLELINE|DT_END_ELLIPSIS)
  , m_freezeText("Poor Network Connection")
  , m_freezeColour(RGB(128,128,128))
  , m_freezeOptions(DT_VCENTER|DT_CENTER|DT_SINGLELINE|DT_END_ELLIPSIS)
  , m_freezeTimeMs(1000)
  , m_receivedFrame(false)
  , m_frameCount(0)
  , m_observedFrameRate(0)
{
  m_lastPosition.x = 0;
  m_lastPosition.y = 0;
  m_fixedSize.cx = 0;
  m_fixedSize.cy = 0;

  m_bitmap.bmiHeader.biSize = sizeof(m_bitmap.bmiHeader);
  m_bitmap.bmiHeader.biWidth = m_frameWidth;
  m_bitmap.bmiHeader.biHeight = -(int)m_frameHeight;
  m_bitmap.bmiHeader.biPlanes = 1;
  m_bitmap.bmiHeader.biBitCount = 32;
  m_bitmap.bmiHeader.biCompression = BI_RGB;
  m_bitmap.bmiHeader.biXPelsPerMeter = 0;
  m_bitmap.bmiHeader.biYPelsPerMeter = 0;
  m_bitmap.bmiHeader.biClrImportant = 0;
  m_bitmap.bmiHeader.biClrUsed = 0;
  m_bitmap.bmiHeader.biSizeImage = m_frameStore.GetSize();
}


PVideoOutputDevice_Window::~PVideoOutputDevice_Window()
{
  Close();
}


PStringArray PVideoOutputDevice_Window::GetOutputDeviceNames()
{
  return psprintf(P_MSWIN_VIDEO_PREFIX" STYLE=0x%08X TITLE=\"%s\"", DEFAULT_STYLE, DEFAULT_TITLE);
}


VOID CALLBACK PVideoOutputDevice_Window::CreateProc(_In_  HWND, _In_  UINT, _In_  UINT_PTR idEvent, _In_  DWORD)
{
  ((PVideoOutputDevice_Window*)idEvent)->CreateDisplayWindow();
}


PBoolean PVideoOutputDevice_Window::Open(const PString & name, PBoolean startImmediate)
{
  if (name.NumCompare(P_MSWIN_VIDEO_PREFIX) != EqualTo)
    return false;

  Close();

  m_openCloseMutex.Wait();

  m_deviceName = name;

  m_hParent = (HWND)ParseDeviceNameTokenUnsigned("PARENT", 0);
  if (m_hParent != NULL && !::IsWindow(m_hParent)) {
    PTRACE(2, "Illegal parent window " << m_hParent << " specified.");
    return false;
  }

  m_dwStyle = (DWORD)ParseDeviceNameTokenUnsigned("STYLE", DEFAULT_STYLE);
  if ((m_dwStyle&(WS_POPUP|WS_CHILD)) == 0) {
    PTRACE(1, "Window must be WS_POPUP or WS_CHILD window.");
    return false;
  }

  // Have parsed out style & parent, see if legal combination
  if (m_hParent == NULL && (m_dwStyle&WS_POPUP) == 0) {
    PTRACE(1, "Must have parent for WS_CHILD window.");
    return false;
  }

  m_lastPosition.x = ParseDeviceNameTokenInt("X", CW_USEDEFAULT);
  m_lastPosition.y = ParseDeviceNameTokenInt("Y", CW_USEDEFAULT);
  m_fixedSize.cx   = ParseDeviceNameTokenInt("WIDTH", 0);
  m_fixedSize.cy   = ParseDeviceNameTokenInt("HEIGHT", 0);
  m_bgColour       = ParseDeviceNameTokenInt("BACKGROUND", 0);
  m_rotation       = ParseDeviceNameTokenInt("ROTATION", 0);

  m_mouseEnabled = m_deviceName.Find("NO-MOUSE") == P_MAX_INDEX;
  m_hidden = !startImmediate || m_deviceName.Find("HIDE") != P_MAX_INDEX;

  if (m_deviceName.Find("FULLSCREEN") != P_MAX_INDEX)
    m_sizeMode = FullScreen;
  else if (m_deviceName.Find("HALF") != P_MAX_INDEX)
    m_sizeMode = HalfSize;
  else if (m_deviceName.Find("DOUBLE") != P_MAX_INDEX)
    m_sizeMode = DoubleSize;
  else if (m_fixedSize.cx > 0 && m_fixedSize.cy > 0)
    m_sizeMode = FixedSize;
  m_channelNumber = m_sizeMode;

  if (m_lastPosition.x == CW_USEDEFAULT && m_lastPosition.y == CW_USEDEFAULT) {
    if (m_hParent != NULL) {
      RECT rect;
      GetWindowRect(m_hParent, &rect);
      m_lastPosition.x = (rect.right + rect.left - m_frameWidth)/2;
      m_lastPosition.y = (rect.bottom + rect.top - m_frameHeight)/2;
    }
    else {
      m_lastPosition.x = (GetSystemMetrics(SM_CXSCREEN) - m_frameWidth)/2;
      m_lastPosition.y = (GetSystemMetrics(SM_CYSCREEN) - m_frameHeight)/2;
    }
  }
  
  m_showInfo = m_deviceName.Find("SHOWINFO") != P_MAX_INDEX;
  m_infoColour = (COLORREF)ParseDeviceNameTokenUnsigned("INFOCOLOUR", m_infoColour);
  m_infoOptions = (UINT)ParseDeviceNameTokenUnsigned("INFOMODE", m_infoOptions);
  m_freezeText = ParseDeviceNameTokenString("FREEZETEXT", m_freezeText);
  m_freezeColour = (COLORREF)ParseDeviceNameTokenUnsigned("FREEZECOLOUR", m_freezeColour);
  m_freezeOptions = (UINT)ParseDeviceNameTokenUnsigned("FREEZEMODE", m_freezeOptions);
  m_freezeTimeMs = (UINT)ParseDeviceNameTokenUnsigned("FREEZETIME", m_freezeTimeMs);

  if (m_hParent != NULL) {
    if (GetWindowThreadProcessId(m_hParent, NULL) == GetCurrentThreadId())
      CreateDisplayWindow();
    else {
      // This is a sneaky way to get a callback in the context of the parent windows thread
      SetTimer(m_hParent, (UINT_PTR)this, USER_TIMER_MINIMUM, CreateProc);
      m_started.Wait();
      KillTimer(m_hParent, (UINT_PTR)this);
    }
    m_openCloseMutex.Signal();
  }
  else {
    m_thread = PThread::Create(PCREATE_NOTIFIER(HandleDisplay), "VidOut");
    m_openCloseMutex.Signal();
    m_started.Wait();
  }

  return startImmediate ? Start() : IsOpen();
}


PBoolean PVideoOutputDevice_Window::IsOpen()
{
  return m_hWnd != NULL;
}


PBoolean PVideoOutputDevice_Window::Close()
{
  PWaitAndSignal m(m_openCloseMutex);

  if (m_hWnd == NULL)
    return false;

  SendMessage(m_hWnd, WM_CLOSE, 0, 0);

  PThread::WaitAndDelete(m_thread, 3000);

  return true;
}


PBoolean PVideoOutputDevice_Window::Start()
{
  PWaitAndSignal m(m_openCloseMutex);

  if (m_hWnd == NULL)
    return false;
  
  ShowWindow(m_hWnd, SW_SHOW);
  return true;
}


PBoolean PVideoOutputDevice_Window::Stop()
{
  PWaitAndSignal m(m_openCloseMutex);

  if (m_hWnd != NULL)
    return ShowWindow(m_hWnd, SW_HIDE);

  return false;
}


PStringArray PVideoOutputDevice_Window::GetChannelNames()
{
  PStringArray names(NumSizeModes);
  names[NormalSize] = "Normal size";
  names[HalfSize] = "Half size";
  names[DoubleSize] = "Double size";
  names[FullScreen] = "Full screen";
  names[FixedSize] = "Fixed size";
  return names;
}


PBoolean PVideoOutputDevice_Window::SetChannel(int newChannelNumber)
{
  if (!PVideoOutputDeviceRGB::SetChannel(newChannelNumber))
    return false;

  if (m_hWnd != NULL) {
    UINT flags = m_hidden ? SWP_HIDEWINDOW : SWP_SHOWWINDOW;

    if (m_sizeMode == FullScreen) {
      if (m_channelNumber == FullScreen)
        return true;

      SetWindowLong(m_hWnd, GWL_STYLE, m_dwStyle);
      SetWindowLong(m_hWnd, GWL_EXSTYLE, m_dwExStyle);
      flags |= SWP_FRAMECHANGED;
    }

    RECT rect;
    rect.right = rect.left = m_lastPosition.x;
    rect.bottom = rect.top = m_lastPosition.y;
    bool adjust = true;
    bool swapForRotation = m_rotation%180 != 0;
    switch (m_channelNumber) {
      case NormalSize :
        rect.right = m_frameWidth;
        rect.bottom = m_frameHeight;
        break;

      case HalfSize :
        rect.right = m_frameWidth/2;
        rect.bottom = m_frameHeight/2;
        break;

      case DoubleSize :
        rect.right = m_frameWidth*2;
        rect.bottom = m_frameHeight*2;
        break;

      case FixedSize :
        rect.right = m_fixedSize.cx;
        rect.bottom = m_fixedSize.cy;
        swapForRotation = false;
        break;

      case FullScreen :
        HMONITOR hmon = MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        if (GetMonitorInfo(hmon, &mi)) {
          SetWindowLong(m_hWnd, GWL_STYLE, m_dwStyle & ~(WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_THICKFRAME));
          SetWindowLong(m_hWnd, GWL_EXSTYLE, m_dwExStyle & ~(WS_EX_DLGMODALFRAME|WS_EX_CLIENTEDGE|WS_EX_STATICEDGE|WS_EX_WINDOWEDGE));
          rect = mi.rcWork;
          flags |= SWP_FRAMECHANGED;
          adjust = false;
        }
        else {
          rect.left = 0;
          rect.top = 0;
          rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
          rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
        }
    }

    if (adjust) {
      if (swapForRotation)
        std::swap(rect.right, rect.bottom);
      rect.right += rect.left;
      rect.bottom += rect.top;
      AdjustWindowRectEx(&rect, m_dwStyle, false, m_dwExStyle);
    }

    PTRACE(4, "SetWindowPos: chan=" << m_channelNumber
           << " pos=" << rect.left << 'x' << rect.top
           << " size=" << rect.right-rect.left << 'x' << rect.bottom-rect.top);
    ::SetWindowPos(m_hWnd, HWND_TOP, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, flags);
  }

  m_sizeMode = (SizeMode)m_channelNumber;
  return true;
}


PBoolean PVideoOutputDevice_Window::SetColourFormat(const PString & colourFormat)
{
  PWaitAndSignal lock(m_mutex);

  if (((colourFormat *= "BGR24") || (colourFormat *= "BGR32")) &&
                PVideoOutputDeviceRGB::SetColourFormat(colourFormat)) {
    m_bitmap.bmiHeader.biBitCount = (WORD)(m_bytesPerPixel*8);
    m_bitmap.bmiHeader.biSizeImage = m_frameStore.GetSize();
    return true;
  }

  return false;
}


PBoolean PVideoOutputDevice_Window::GetVFlipState()
{
  return m_flipped;
}


PBoolean PVideoOutputDevice_Window::SetVFlipState(PBoolean newVFlip)
{
  m_flipped = newVFlip;
  m_bitmap.bmiHeader.biHeight = m_flipped ? m_frameHeight : -(int)m_frameHeight;
  return true;
}


PBoolean PVideoOutputDevice_Window::SetFrameSize(unsigned width, unsigned height)
{
  {
    PWaitAndSignal lock(m_mutex);

    if (width == m_frameWidth && height == m_frameHeight)
      return true;

    if (!PVideoOutputDeviceRGB::SetFrameSize(width, height))
      return false;

    m_bitmap.bmiHeader.biWidth = m_frameWidth;
    m_bitmap.bmiHeader.biHeight = m_flipped ? m_frameHeight : -(int)m_frameHeight;
    m_bitmap.bmiHeader.biSizeImage = m_frameStore.GetSize();
  }

  // Must be outside of mutex
  SetChannel(GetChannel());

  return true;
}


PBoolean PVideoOutputDevice_Window::FrameComplete()
{
  if (m_hidden)
    m_hidden = !ShowWindow(m_hWnd, SW_SHOW);

  PWaitAndSignal lock(m_mutex);

  if (m_hWnd == NULL)
    return false;

  m_receivedFrame = true;
  ++m_frameCount;

  HDC hDC = GetDC(m_hWnd);
  Draw(hDC);
  ReleaseDC(m_hWnd, hDC);

  return true;
}


PBoolean PVideoOutputDevice_Window::GetPosition(int & x, int & y) const
{
  x = m_lastPosition.x;
  y = m_lastPosition.y;
  return true;
}


bool PVideoOutputDevice_Window::SetPosition(int x, int y)
{
  if (m_hWnd == NULL)
    return false;

  return ::SetWindowPos(m_hWnd, HWND_TOP, x, y, 0, 0, SWP_NOSIZE|SWP_NOACTIVATE);
}


static void Rotate(PBYTEArray & frameStore, int frameWidth, int frameHeight, unsigned bytesPerPixel, unsigned angle)
{
  PBYTEArray destination(frameStore.GetSize());

  if (bytesPerPixel == 4) {
    // Split out the BGR32 version for faster operation
    const uint32_t * src = (const uint32_t *)frameStore.GetPointer();
    uint32_t * dst = (uint32_t *)destination.GetPointer();
    switch (angle) {
      case -90 :
        dst += frameWidth*frameHeight;
        for (int y = frameHeight; y > 0; --y) {
          uint32_t * tempY = dst - y;
          for (int x = frameWidth; x > 0; --x) {
            *tempY = *src++;
            tempY -= frameHeight;
          }
        }
        break;

      case 90 :
        for (int y = frameHeight-1; y >= 0; --y) {
          uint32_t * tempY = dst + y;
          for (int x = frameWidth; x > 0; --x) {
            *tempY = *src++;
            tempY += frameHeight;
          }
        }
        break;

      case 180 :
        dst += frameWidth*frameHeight;
        for (int y = frameHeight; y > 0; --y) {
          for (int x = frameWidth; x > 0; --x)
            *--dst = *src++;
        }
        break;
    }
  }
  else {
    const BYTE * src = frameStore;
    BYTE * dst = destination.GetPointer();
    switch (angle) {
      case -90 :
        dst += frameWidth*frameHeight*bytesPerPixel;
        for (int y = frameHeight; y > 0; --y) {
          BYTE * tempY = dst - y*bytesPerPixel;
          for (int x = frameWidth; x > 0; --x) {
            for (unsigned i = 0; i < bytesPerPixel; ++i)
              tempY[i] = *src++;
            tempY -= frameHeight*bytesPerPixel;
          }
        }
        break;

      case 90 :
        for (int y = frameHeight-1; y >= 0; --y) {
          BYTE * tempY = dst + y*bytesPerPixel;
          for (int x = frameWidth; x > 0; --x) {
            for (unsigned i = 0; i < bytesPerPixel; ++i)
              tempY[i] = *src++;
            tempY += frameHeight*bytesPerPixel;
          }
        }
        break;

      case 180 :
        dst += frameWidth*frameHeight*bytesPerPixel;
        for (int y = frameHeight; y > 0; --y) {
          for (int x = frameWidth; x > 0; --x)
            for (unsigned i = 0; i < bytesPerPixel; ++i)
              *--dst = *src++;
        }
        break;
    }
  }

  frameStore = destination;
}


void PVideoOutputDevice_Window::Draw(HDC hDC)
{
  RECT rect;
  GetClientRect(m_hWnd, &rect);

  HBRUSH brush = m_bgColour < 0x1000000 ? CreateSolidBrush(m_bgColour) : NULL;

  if (m_frameStore.IsEmpty()) {
    if (brush != NULL)
      FillRect(hDC, &rect, brush);
  }
  else {
    int imageWidth = m_frameWidth;
    int imageHeight = m_frameHeight;
    if (m_rotation != 0) {
      Rotate(m_frameStore, m_frameWidth, m_frameHeight, m_bytesPerPixel, m_rotation);
      if (m_rotation != 180) {
        imageWidth = m_frameHeight;
        imageHeight = m_frameWidth;
      }
    }

    m_bitmap.bmiHeader.biWidth = imageWidth;
    m_bitmap.bmiHeader.biHeight = m_flipped ? imageHeight : -imageHeight;

    int result;
    if (imageWidth == rect.right && imageHeight == rect.bottom)
      result = SetDIBitsToDevice(hDC,
                                 0, 0, imageWidth, imageHeight,
                                 0, 0, 0, imageHeight,
                                 m_frameStore.GetPointer(), &m_bitmap, DIB_RGB_COLORS);
    else {
      int frameAspect = 1000*imageWidth/imageHeight;
      int windowAspect = 1000*rect.right/rect.bottom;
      int x,y,w,h;
      if (frameAspect < windowAspect) {
        w = imageWidth*rect.bottom/imageHeight;
        h = rect.bottom;
        x = (rect.right - w)/2;
        y = 0;
        if (brush != NULL) {
          RECT r;
          r.left = 0; r.top = 0; r.right = x; r.bottom = rect.bottom;
          FillRect(hDC, &r, brush);
          r.left = rect.right-x; r.right = rect.right;
          FillRect(hDC, &r, brush);
        }
      }
      else if (frameAspect > windowAspect) {
        w = rect.right;
        h = imageHeight*rect.right/imageWidth;
        x = 0;
        y = (rect.bottom - h)/2;
        if (brush != NULL) {
          RECT r;
          r.left = 0; r.top = 0; r.right = rect.right; r.bottom = y;
          FillRect(hDC, &r, brush);
          r.top = rect.bottom - y; r.bottom = rect.bottom;
          FillRect(hDC, &r, brush);
        }
      }
      else {
        x = y = 0;
        w = rect.right;
        h = rect.bottom;
      }

      SetStretchBltMode(hDC, STRETCH_DELETESCANS);
      result = StretchDIBits(hDC,
                             x, y, w, h,
                             0, 0, imageWidth, imageHeight,
                             m_frameStore.GetPointer(), &m_bitmap, DIB_RGB_COLORS, SRCCOPY);
    }

    if (result != imageHeight) {
      m_lastError = ::GetLastError();
      PTRACE(2, "Drawing image failed: resolution=" << rect.right << 'x' << rect.bottom
             << ", bitmap=" << imageWidth << 'x' << imageHeight
             << ", size=" << m_bitmap.bmiHeader.biSizeImage << ", result=" << result << ", error=" << m_lastError);
    }
  }

  if (brush != NULL)
    DeleteObject(brush);

  if (m_showInfo) {
    PStringStream strm;
    strm << " Video: " << m_frameWidth << 'x' << m_frameHeight << " @ " << m_observedFrameRate << "fps";
    if (m_frameWidth != (unsigned)rect.right && m_frameHeight != (unsigned)rect.bottom)
      strm << " Window: " << rect.right << 'x' << rect.bottom;
    SetTextColor(hDC, m_infoColour);
    SetBkMode(hDC, TRANSPARENT);
    rect.left += 8;
    rect.right -= 8;
    DrawText(hDC, strm, strm.GetLength(), &rect, m_infoOptions);
  }

  if (m_receivedFrame && m_observedFrameRate == 0 && !m_freezeText.IsEmpty()) {
    SetTextColor(hDC, m_freezeColour);
    SetBkMode(hDC, TRANSPARENT);
    rect.left += 8;
    rect.right -= 8;
    DrawText(hDC, m_freezeText, m_freezeText.GetLength(), &rect, m_freezeOptions);
  }
}


static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  if (uMsg == WM_CREATE)
    SetWindowLongPtr(hWnd, 0, (LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);

  PVideoOutputDevice_Window * vodw = (PVideoOutputDevice_Window *)GetWindowLongPtr(hWnd, 0);
  if (vodw != NULL)
    return vodw->WndProc(uMsg, wParam, lParam);

  return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


void PVideoOutputDevice_Window::CreateDisplayWindow()
{
  static char const wndClassName[] = "PVideoOutputDevice_Window";

  static bool needRegistration = true;
  if (needRegistration) {
    needRegistration = false;

    WNDCLASS wndClass;
    memset(&wndClass, 0, sizeof(wndClass));
    wndClass.style = CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS|CS_PARENTDC;
    wndClass.lpszClassName = wndClassName;
    wndClass.lpszClassName = wndClassName;
    wndClass.lpfnWndProc = ::WndProc;
    wndClass.hbrBackground = GetStockBrush(HOLLOW_BRUSH);
    wndClass.cbWndExtra = sizeof(this);
    PAssertOS(RegisterClass(&wndClass));
  }

  PVarString title = ParseDeviceNameTokenString("TITLE", DEFAULT_TITLE);
  if ((m_hWnd = CreateWindow(wndClassName,
                             title, 
                             m_dwStyle,
                             m_lastPosition.x , m_lastPosition.y, m_frameWidth, m_frameHeight,
                             m_hParent, NULL, GetModuleHandle(NULL), this)) != NULL) {
    m_dwExStyle = GetWindowLong(m_hWnd, GWL_EXSTYLE);
    SetChannel(GetChannel());
  }

  SetTimer(m_hWnd, 12345, m_freezeTimeMs, NULL);

  m_started.Signal();
}


void PVideoOutputDevice_Window::HandleDisplay(PThread &, P_INT_PTR)
{
  CreateDisplayWindow();

  if (m_hWnd != NULL) {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
      DispatchMessage(&msg);
  }
}


LRESULT PVideoOutputDevice_Window::WndProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  PWaitAndSignal lock(m_mutex);

  switch (uMsg) {
    case WM_PAINT :
      {
        PAINTSTRUCT paint;
        HDC hDC = BeginPaint(m_hWnd, &paint);
        Draw(hDC);
        EndPaint(m_hWnd, &paint);
        break;
      }

    case WM_TIMER :
      {
        unsigned rate = m_frameCount.exchange(0);
        m_observedFrameRate.exchange(rate);
        if (rate == 0) {
          PTRACE(3, "Zero frame rate detected");
          InvalidateRect(m_hWnd, NULL, false);
        }
        break;
      }

    case WM_MOVE :
      if (m_hWnd != NULL && m_sizeMode != FullScreen) {
#if 0
        RECT rect;
        rect.left = rect.right = GET_X_LPARAM(lParam);
        rect.top = rect.bottom = GET_Y_LPARAM(lParam);
        ::AdjustWindowRectEx(&rect, m_dwStyle, false, m_dwExStyle);
        m_lastPosition.x = rect.left;
        m_lastPosition.y = rect.top;
#else
        m_lastPosition.x = GET_X_LPARAM(lParam);
        m_lastPosition.y = GET_Y_LPARAM(lParam);
#endif
        PTRACE(4, "Moved to " << m_lastPosition.x << 'x' << m_lastPosition.y);
      }
      break;

    case WM_LBUTTONDOWN :
      if (m_mouseEnabled)
        PostMessage(m_hWnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
      break;

    case WM_LBUTTONDBLCLK :
      if (m_mouseEnabled) {
        switch (m_sizeMode) {
          default :
            SetChannel(NormalSize);
            break;

          case NormalSize :
            SetChannel(DoubleSize);
            break;

          case FixedSize :
            if (m_fixedSize.cx < 10000 && m_fixedSize.cy < 10000) {
              m_fixedSize.cx *= 2;
              m_fixedSize.cy *= 2;
              SetChannel(FixedSize);
            }
        }
      }
      break;

    case WM_RBUTTONDBLCLK :
      if (m_mouseEnabled) {
        switch (m_sizeMode) {
          default :
            SetChannel(NormalSize);
            break;

          case NormalSize :
            SetChannel(HalfSize);
            break;

          case FixedSize :
            if (m_fixedSize.cx > 64 && m_fixedSize.cy > 36 && (m_fixedSize.cx&1) == 0 && (m_fixedSize.cy&1) == 0) {
              m_fixedSize.cx /= 2;
              m_fixedSize.cy /= 2;
              SetChannel(FixedSize);
            }
        }
      }
      break;

    case WM_RBUTTONDOWN :
      if (m_mouseEnabled) {
        HMENU hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_ENABLED|MF_STRING|(m_sizeMode == NormalSize ? MF_CHECKED : 0), 100+NormalSize, "Normal size");
        AppendMenu(hMenu, MF_ENABLED|MF_STRING|(m_sizeMode == FullScreen ? MF_CHECKED : 0), 100+FullScreen, "Full screen");
        AppendMenu(hMenu, MF_ENABLED|MF_STRING|(m_sizeMode == HalfSize   ? MF_CHECKED : 0), 100+HalfSize,   "Half size"  );
        AppendMenu(hMenu, MF_ENABLED|MF_STRING|(m_sizeMode == DoubleSize ? MF_CHECKED : 0), 100+DoubleSize, "Double size");
        AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
        AppendMenu(hMenu, MF_ENABLED|MF_STRING|(m_showInfo ? MF_CHECKED : 0), 10, "Show Info");

        POINT pt;
        pt.x = GET_X_LPARAM(lParam);
        pt.y = GET_Y_LPARAM(lParam);
        ClientToScreen(m_hWnd, &pt);
        int menu = TrackPopupMenu(hMenu, TPM_RIGHTBUTTON|TPM_RETURNCMD, pt.x, pt.y, 0, m_hWnd, NULL);
        if (menu >= 100)
          SetChannel(menu-100);
        else {
          m_showInfo = !m_showInfo;
          InvalidateRect(m_hWnd, NULL, false);
        }
        DestroyMenu(hMenu);
      }
      break;

    case WM_CLOSE :
      DestroyWindow(m_hWnd);
      m_hWnd = NULL;
      break;

    case WM_DESTROY:
      if (m_thread != NULL)
        PostThreadMessage(GetCurrentThreadId(), WM_QUIT, 0, 0);
      break;
  }
  return DefWindowProc(m_hWnd, uMsg, wParam, lParam);
}

#endif // P_VIDEO



// End Of File ///////////////////////////////////////////////////////////////

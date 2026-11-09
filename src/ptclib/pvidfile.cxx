/*
 * pvidfile.cxx
 *
 * Video file implementation
 *
 * Portable Windows Library
 *
 * Copyright (C) 2004 Post Increment
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
 * The Initial Developer of the Original Code is
 * Craig Southeren <craigs@postincrement.com>
 *
 * All Rights Reserved.
 *
 * Contributor(s): ______________________________________.
 */

#ifdef P_USE_PRAGMA
#pragma implementation "pvidfile.h"
#endif

#include <ptlib.h>

#if P_VIDEO
#if P_VIDFILE

#include <ptclib/pvidfile.h>
#include <ptlib/videoio.h>
#include <ptlib/vconvert.h>

#if P_FFMPEG
  extern "C" {
    P_PUSH_MSVC_WARNINGS(4244)
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
    #include <libavutil/imgutils.h>
    #include <libswscale/swscale.h>
    P_POP_MSVC_WARNINGS()
  };
#endif


#define PTraceModule() "VidFile"


///////////////////////////////////////////////////////////////////////////////

PVideoFile::PVideoFile()
  : m_fixedFrameSize(false)
  , m_fixedFrameRate(false)
  , m_headerOffset(0)
  , m_frameHeaderLen(0)
{
  m_frameBytes = m_videoInfo.CalculateFrameBytes();
}


bool PVideoFile::SetFrameSizeFromFilename(const PString & fn)
{
  static PRegularExpression res("_(sqcif|qcif|cif|cif4|cif16|HD[0-9]+|[0-9]+p|[0-9]+x[0-9]+)[^a-z0-9]",
                                PRegularExpression::Extended|PRegularExpression::IgnoreCase);
  PINDEX pos, len;
  if (!fn.FindRegEx(res, pos, len) || !m_videoInfo.Parse(fn.Mid(pos + 1, len - 2)))
    return false;

  m_frameBytes = m_videoInfo.CalculateFrameBytes();
  return true;
}


bool PVideoFile::SetFPSFromFilename(const PString & fn)
{
  static PRegularExpression fps("_[0-9]+fps[^a-z]",
                                PRegularExpression::Extended|PRegularExpression::IgnoreCase);

  PINDEX pos;
  if ((pos = fn.FindRegEx(fps)) == P_MAX_INDEX)
    return false;

  m_fixedFrameRate = m_videoInfo.SetFrameRate(fn.Mid(pos + 1).AsUnsigned());
  return m_fixedFrameRate;
}


PBoolean PVideoFile::WriteFrame(const void * frame)
{
  return Write(frame, m_frameBytes);
}


PBoolean PVideoFile::ReadFrame(void * frame)
{
  if (Read(frame, m_frameBytes) && (GetLastReadCount() == m_frameBytes))
    return true;

#if PTRACING
  if (GetErrorCode(PFile::LastReadError) != PFile::NoError)
    PTRACE(2, "Error reading file \"" << GetFilePath() << "\" - " << GetErrorText(PFile::LastReadError));
  else
    PTRACE(4, "End of file \"" << GetFilePath() << '"');
#endif

  return false;
}


off_t PVideoFile::GetLength() const
{
  off_t len = PFile::GetLength();
  return len < m_headerOffset ? 0 : ((len - m_headerOffset)/(m_frameBytes+m_frameHeaderLen));
}


PBoolean PVideoFile::SetLength(off_t len)
{
  return PFile::SetLength(len*(m_frameBytes + m_frameHeaderLen) + m_headerOffset);
}


off_t PVideoFile::GetPosition() const
{
  off_t pos = PFile::GetPosition();
  return pos < m_headerOffset ? 0 : ((pos - m_headerOffset)/(m_frameBytes+m_frameHeaderLen));
}


PBoolean PVideoFile::SetPosition(off_t pos, PFile::FilePositionOrigin origin)
{
  pos *= m_frameBytes+m_frameHeaderLen;
  if (origin == PFile::Start)
    pos += m_headerOffset;

  return PFile::SetPosition(pos, origin);
}


PBoolean PVideoFile::SetFrameSize(unsigned width, unsigned height)
{
  if (m_videoInfo.GetFrameWidth() == width && m_videoInfo.GetFrameHeight() == height)
    return true;

  if (m_fixedFrameSize)
    return false;

  if (!m_videoInfo.SetFrameSize(width, height))
    return false;

  m_frameBytes = m_videoInfo.CalculateFrameBytes();
  return m_frameBytes > 0;
}


PBoolean PVideoFile::SetFrameRate(unsigned rate)
{
  if (m_videoInfo.GetFrameRate() == rate)
    return true;

  if (m_fixedFrameRate)
    return false;
  
  return m_videoInfo.SetFrameRate(rate);
}


///////////////////////////////////////////////////////////////////////////////

PFACTORY_CREATE(PVideoFileFactory, PYUVFile, ".yuv");
PFACTORY_SYNONYM(PVideoFileFactory, PYUVFile, y4m, ".y4m");

PYUVFile::PYUVFile()
  : m_y4mMode(false)
{
}


static PString ReadPrintable(PFile & file)
{
  PString str;

  int ch;
  while ((ch = file.ReadChar()) >= ' ' && ch < 0x7f)
    str += (char)ch;

  return str;
}


bool PYUVFile::InternalOpen(OpenMode mode, OpenOptions opts, PFileInfo::Permissions permissions)
{
  SetFrameSizeFromFilename(GetFilePath());
  SetFPSFromFilename(GetFilePath());

  if (!PVideoFile::InternalOpen(mode, opts, permissions))
    return false;

  m_y4mMode = GetFilePath().GetType() *= ".y4m";
  m_fixedFrameSize = !m_y4mMode;

  if (m_y4mMode) {
    PString info = ReadPrintable(*this);

    PStringArray params = info.Tokenise(" \t", false); // Really is juts a space, but be forgiving
    if (params.IsEmpty() || params[0] != "YUV4MPEG2") {
      PTRACE(2, "Invalid file format, does not start with YUV4MPEG2");
      SetErrorValues(Miscellaneous, EINVAL);
      return false;
    }

    unsigned frameWidth = 0, frameHeight = 0, frameRate = 0, sarWidth = 0, sarHeight = 0;
    for (PINDEX i = 1; i < params.GetSize(); ++i) {
      PString param = params[i].ToUpper();
      switch (param[0]) {
        case 'W' :
          frameWidth = param.Mid(1).AsUnsigned();
          m_fixedFrameSize = true;
          break;

        case 'H' :
          frameHeight = param.Mid(1).AsUnsigned();
          m_fixedFrameSize = true;
          break;

        case 'F' :
          {
            unsigned denom = param.Mid(param.Find(':')+1).AsUnsigned();
            frameRate = (param.Mid(1).AsUnsigned()+denom/2)/denom;
            m_fixedFrameRate = true;
          }
          break;

        case 'I' :
          if (param[1] != 'P') {
            PTRACE(2, "Interlace modes are not supported");
            return false;
          }
          break;

        case 'A' :
          sarWidth = param.Mid(1).AsUnsigned();
          sarHeight = param.Mid(param.Find(':')+1).AsUnsigned();
          break;

        case 'C' :
          if (param == "C420")
            m_videoInfo.SetColourFormat(PVideoFrameInfo::YUV420P());
          else if (param == "C422")
            m_videoInfo.SetColourFormat("YUV422P");
          else {
            PTRACE(2, "Interlace modes are not supported");
            return false;
          }
          break;
      }
    }

    PTRACE(4, "y4m \"" << info << '"');
    m_headerOffset = PFile::GetPosition();
    m_videoInfo.SetFrameSize(frameWidth, frameHeight);
    m_videoInfo.SetFrameRate(frameRate);
    m_videoInfo.SetFrameSar(sarWidth, sarHeight);
    m_frameBytes = m_videoInfo.CalculateFrameBytes();
  }

  return true;
}


PBoolean PYUVFile::WriteFrame(const void * frame)
{
  if (m_y4mMode) {
    if (PFile::GetPosition() > 0)
      WriteString("FRAME\n");
    else {
      *this << "YUV4MPEG2 W" << m_videoInfo.GetFrameWidth() << " H" << m_videoInfo.GetFrameHeight() << " F" << m_videoInfo.GetFrameRate() << ":1 Ip";
      if (m_videoInfo.GetSarWidth() > 0 && m_videoInfo.GetSarHeight() > 0)
        *this << " A" << m_videoInfo.GetSarWidth() << ':' << m_videoInfo.GetSarHeight();
      if (m_videoInfo.GetColourFormat() == "YUV422P")
        *this << " C422";
      *this << endl;
      m_headerOffset = PFile::GetPosition();
    }
  }

  return Write(frame, m_frameBytes);
}


PBoolean PYUVFile::ReadFrame(void * frame)
{
  if (m_y4mMode) {
    PString info = ReadPrintable(*this);
    if (m_frameHeaderLen == 0)
      m_frameHeaderLen = PFile::GetPosition() - m_headerOffset;
    if (info.NumCompare("FRAME") != EqualTo) {
      PTRACE(2, "Invalid frame header in y4m file");
      return false;
    }
    PTRACE(6, "y4m \"" << info << '"');
  }

  return PVideoFile::ReadFrame(frame);
}


///////////////////////////////////////////////////////////////////////////////

class PStillImageVideoFile : public PVideoFile
{
    PCLASSINFO(PStillImageVideoFile, PVideoFile);
  protected:
    PBYTEArray m_pixelData;

  public:
    PStillImageVideoFile()
    {
      m_fixedFrameSize = true;
    }

    virtual bool IsStillImage() const override
    {
      return true;
    }
    
    ~PStillImageVideoFile()
    {
      Close();
    }

    virtual bool Close() override
    { 
      m_pixelData.SetSize(0);
      return true;
    }

    virtual PBoolean IsOpen() const override
    {
      // Pixel data is what later reads return. The file handle has to count
      // as open too, or header parsing fails with EBADF before the picture
      // has been decoded.
      return !m_pixelData.IsEmpty() || PFile::IsOpen();
    }

    virtual off_t GetLength() const override
    {
      return 1;
    }


    virtual off_t GetPosition() const override
    {
      return 0;
    }


    virtual bool SetPosition(off_t pos, PFile::FilePositionOrigin) override
    {
      return pos == 0;
    }

    virtual PBoolean WriteFrame(const void *) override
    {
      return false;
    }

    virtual PBoolean ReadFrame(void * frame) override
    {
      if (!IsOpen())
        return false;

      memcpy(frame, m_pixelData, m_frameBytes);
      return true;
    }

};


///////////////////////////////////////////////////////////////////////////////

#if P_FFMPEG

/**A still image decoded with FFmpeg (BMP, JPEG, PNG, GIF, TIFF, …).
  */
class PFfmpegImageFile : public PStillImageVideoFile
{
    PCLASSINFO(PFfmpegImageFile, PStillImageVideoFile);

  protected:
    virtual bool InternalOpen(OpenMode mode, OpenOptions, PFileInfo::Permissions) override
    {
      if (mode != PFile::ReadOnly) {
        SetErrorValues(Miscellaneous, EINVAL);
        return false;
      }

      AVFormatContext * format = NULL;
      if (avformat_open_input(&format, m_path, NULL, NULL) < 0) {
        PTRACE(2, "FFmpeg could not open image " << m_path);
        SetErrorValues(Miscellaneous, EINVAL);
        return false;
      }

      if (avformat_find_stream_info(format, NULL) < 0) {
        PTRACE(2, "FFmpeg could not find stream info in " << m_path);
        avformat_close_input(&format);
        SetErrorValues(Miscellaneous, EINVAL);
        return false;
      }

      int streamIndex = av_find_best_stream(format, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
      if (streamIndex < 0) {
        PTRACE(2, "FFmpeg found no video stream in " << m_path);
        avformat_close_input(&format);
        SetErrorValues(Miscellaneous, EINVAL);
        return false;
      }

      AVStream * stream = format->streams[streamIndex];
      const AVCodec * codec = avcodec_find_decoder(stream->codecpar->codec_id);
      AVCodecContext * codecCtx = codec != NULL ? avcodec_alloc_context3(codec) : NULL;
      if (codecCtx == NULL ||
          avcodec_parameters_to_context(codecCtx, stream->codecpar) < 0 ||
          avcodec_open2(codecCtx, codec, NULL) < 0) {
        PTRACE(2, "FFmpeg could not open decoder for " << m_path);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&format);
        SetErrorValues(Miscellaneous, EINVAL);
        return false;
      }

      AVPacket * packet = av_packet_alloc();
      AVFrame * frame = av_frame_alloc();
      bool gotFrame = false;
      while (packet != NULL && frame != NULL && av_read_frame(format, packet) >= 0) {
        if (packet->stream_index == streamIndex &&
            avcodec_send_packet(codecCtx, packet) >= 0 &&
            avcodec_receive_frame(codecCtx, frame) >= 0) {
          gotFrame = true;
          break;
        }
        av_packet_unref(packet);
      }

      if (!gotFrame) {
        PTRACE(2, "FFmpeg could not decode a frame from " << m_path);
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&format);
        SetErrorValues(Miscellaneous, EINVAL);
        return false;
      }

      unsigned width = (unsigned)frame->width;
      unsigned height = (unsigned)frame->height;
      m_videoInfo.SetFrameSize(width, height);
      m_frameBytes = m_videoInfo.CalculateFrameBytes();

      PBYTEArray rgb;
      if (!rgb.SetSize(width * height * 4)) {
        av_frame_free(&frame);
        av_packet_free(&packet);
        avcodec_free_context(&codecCtx);
        avformat_close_input(&format);
        return false;
      }

      SwsContext * sws = sws_getContext(width, height, (AVPixelFormat)frame->format,
                                       width, height, AV_PIX_FMT_RGBA,
                                       SWS_BILINEAR, NULL, NULL, NULL);
      uint8_t * dstSlice[4] = { rgb.GetPointer(), NULL, NULL, NULL };
      int dstStride[4] = { (int)(width * 4), 0, 0, 0 };
      bool scaled = sws != NULL &&
                    sws_scale(sws, frame->data, frame->linesize, 0, (int)height,
                              dstSlice, dstStride) > 0;
      sws_freeContext(sws);
      av_frame_free(&frame);
      av_packet_free(&packet);
      avcodec_free_context(&codecCtx);
      avformat_close_input(&format);

      if (!scaled) {
        PTRACE(2, "FFmpeg could not convert pixels from " << m_path);
        SetErrorValues(Miscellaneous, EINVAL);
        return false;
      }

      PColourConverter * converter = PColourConverter::Create(PVideoFrameInfo(width, height, "RGB32"), m_videoInfo);
      if (converter == NULL) {
        PTRACE(2, "Could not create colour converter from RGB32 to " << m_videoInfo);
        SetErrorValues(Miscellaneous, EINVAL);
        return false;
      }

      bool converted = m_pixelData.SetSize(m_frameBytes) && converter->Convert(rgb, m_pixelData.GetPointer());
      delete converter;
      PTRACE_IF(2, !converted, "Could not do colour conversion from RGB32 to " << m_videoInfo);
      return converted;
    }
};

static const char * PFfmpegImageExtensions[] = {
  ".bmp",
  ".dib",
  ".jpg",
  ".jpeg",
  ".gif",
  ".png",
  ".tif",
  ".tiff",
  NULL
};

PFACTORY_CREATE(PVideoFileFactory, PFfmpegImageFile,        PFfmpegImageExtensions[0]);
PFACTORY_SYNONYM(PVideoFileFactory, PFfmpegImageFile, dib,  PFfmpegImageExtensions[1]);
PFACTORY_SYNONYM(PVideoFileFactory, PFfmpegImageFile, jpg,  PFfmpegImageExtensions[2]);
PFACTORY_SYNONYM(PVideoFileFactory, PFfmpegImageFile, jpeg, PFfmpegImageExtensions[3]);
PFACTORY_SYNONYM(PVideoFileFactory, PFfmpegImageFile, gif,  PFfmpegImageExtensions[4]);
PFACTORY_SYNONYM(PVideoFileFactory, PFfmpegImageFile, png,  PFfmpegImageExtensions[5]);
PFACTORY_SYNONYM(PVideoFileFactory, PFfmpegImageFile, tif,  PFfmpegImageExtensions[6]);
PFACTORY_SYNONYM(PVideoFileFactory, PFfmpegImageFile, tiff, PFfmpegImageExtensions[7]);

#else // P_FFMPEG

///////////////////////////////////////////////////////////////////////////////

/**A file containing a single image, which is repeatedly output.
  */
class PBMPFile : public PStillImageVideoFile
{
    PCLASSINFO(PBMPFile, PStillImageVideoFile);

  protected:
    virtual bool InternalOpen(OpenMode mode, OpenOptions opts, PFileInfo::Permissions permissions) override
    {
      if (mode != PFile::ReadOnly) {
        SetErrorValues(Miscellaneous, EINVAL);
        return false;
      }

      if (!PVideoFile::InternalOpen(mode, opts, permissions))
        return false;

    #pragma pack(1)
      struct {
        PUInt16l m_FileType;     /* File type, always 4D42h ("BM") */
        PUInt32l m_FileSize;     /* Size of the file in bytes */
        PUInt16l m_Reserved1;    /* Always 0 */
        PUInt16l m_Reserved2;    /* Always 0 */
        PUInt32l m_BitmapOffset; /* Starting position of image data in bytes */
      } fileHeader;
    #pragma pack()

      if (!Read(&fileHeader, sizeof(fileHeader)))
        return false;

      if (fileHeader.m_FileType != 0x4D42)
        return false;

    #pragma pack(1)
      struct {
        PUInt32l m_Size;            /* Size of this header in bytes */
        PInt32l  m_Width;           /* Image width in pixels */
        PInt32l  m_Height;          /* Image height in pixels */
        PUInt16l m_Planes;          /* Number of color planes */
        PUInt16l m_BitsPerPixel;    /* Number of bits per pixel */
        PUInt32l m_Compression;     /* Compression methods used */
        PUInt32l m_SizeOfBitmap;    /* Size of bitmap in bytes */
        PInt32l  m_HorzResolution;  /* Horizontal resolution in pixels per meter */
        PInt32l  m_VertResolution;  /* Vertical resolution in pixels per meter */
        PUInt32l m_ColorsUsed;      /* Number of colors in the image */
        PUInt32l m_ColorsImportant; /* Minimum number of important colors */
      } bitmapHeader;
    #pragma pack()

      if (!Read(&bitmapHeader.m_Size, sizeof(bitmapHeader.m_Size)))
        return false;
      if (!PFile::SetPosition(sizeof(fileHeader)))
        return false;
      if (!Read(&bitmapHeader, std::min((uint32_t)bitmapHeader.m_Size, (uint32_t)sizeof(bitmapHeader))))
        return false;
      if (bitmapHeader.m_Planes != 1)
        return false;
      if (bitmapHeader.m_BitsPerPixel != 24 && bitmapHeader.m_BitsPerPixel != 32)
        return false;
      if (bitmapHeader.m_Compression != 0)
        return false;

      m_headerOffset =bitmapHeader.m_Size + sizeof(fileHeader);
      if (!PFile::SetPosition(m_headerOffset))
        return false;

      m_videoInfo.SetFrameSize(bitmapHeader.m_Width, std::abs(bitmapHeader.m_Height));
      m_frameBytes = m_videoInfo.CalculateFrameBytes();

      PVideoFrameInfo rgb = m_videoInfo;
      rgb.SetColourFormat(bitmapHeader.m_BitsPerPixel == 24 ? "BGR24" : "BGR32");

      PBYTEArray temp;
      if (!temp.SetSize(rgb.CalculateFrameBytes()))
        return false;

      if (!Read(temp.GetPointer(), temp.GetSize()))
        return false;

      PColourConverter * converter = PColourConverter::Create(rgb, m_videoInfo);
      if (converter == NULL)
        return false;

      converter->SetVFlipState(bitmapHeader.m_Height > 0);

      bool converted = m_pixelData.SetSize(m_frameBytes) && converter->Convert(temp, m_pixelData.GetPointer());
      delete converter;
      return converted;
    }
};

PFACTORY_CREATE(PVideoFileFactory, PBMPFile, ".bmp");
PFACTORY_SYNONYM(PVideoFileFactory, PBMPFile, dib, ".dib");


///////////////////////////////////////////////////////////////////////////////

#if P_JPEG_DECODER

/**A file containing a JPEG image, which is repeatedly output.
  */
class PJPEGFile : public PStillImageVideoFile
{
    PCLASSINFO(PJPEGFile, PStillImageVideoFile);

  protected:
    virtual bool InternalOpen(OpenMode mode, OpenOptions opts, PFileInfo::Permissions permissions) override
    {
      if (mode != PFile::ReadOnly) {
        SetErrorValues(Miscellaneous, EINVAL);
        return false;
      }

      if (!PVideoFile::InternalOpen(mode, opts, permissions))
        return false;

      // Load from a plain file. This object's GetLength() is a frame count,
      // and IsOpen() stays false until the picture has been decoded.
      PFile raw;
      if (!raw.Open(GetFilePath(), PFile::ReadOnly, PFile::MustExist)) {
        SetErrorValues(raw.GetErrorCode(), raw.GetErrorNumber());
        return false;
      }

      PJPEGConverter decoder(PVideoFrameInfo(0, 0, "JPEG"), m_videoInfo);
      if (!decoder.Load(raw, m_pixelData))
        return false;

      decoder.GetDstFrameInfo(m_videoInfo);
      m_frameBytes = m_videoInfo.CalculateFrameBytes();
      return true;
    }
};

PFACTORY_CREATE(PVideoFileFactory, PJPEGFile, ".jpg");
PFACTORY_SYNONYM(PVideoFileFactory, PJPEGFile, jpeg, ".jpeg");


#endif  // P_JPEG_DECODER
#endif  // P_FFMPEG
#endif  // P_VIDFILE
#endif  // P_VIDEO

/*!
 * ffmpeg_demuxer.cc (https://github.com/SamsungDForum/NativePlayer)
 * Copyright 2016, Samsung Electronics Co., Ltd
 * Licensed under the MIT license
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * @author Jacob Tarasiewicz
 * @author Tomasz Borkowski
 */

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/log.h"
#include "libavutil/dict.h"
}


#include "ffmpeg_demuxer.h"
#include "common.h"

#include "convert_codecs.h"

using pp::AutoLock;
using pp::MessageLoop;
using std::unique_ptr;
using Samsung::NaClPlayer::Rational;
using Samsung::NaClPlayer::Size;
using Samsung::NaClPlayer::TimeTicks;

const uint8_t kPlayReadySystemId[] = {
    // "9a04f079-9840-4286-ab92-e65be0885f95";
    0x9a, 0x04, 0xf0, 0x79, 0x98, 0x40, 0x42, 0x86,
    0xab, 0x92, 0xe6, 0x5b, 0xe0, 0x88, 0x5f, 0x95,
};
const std::string kDRMInitDataType("cenc:pssh");

static const int kKidLength = 16;
static const size_t kErrorBufferSize = 1024;
static const uint32_t kBufferSize = 128 * 1024;
static const uint32_t kMicrosecondsPerSecond = 1000000;
static const TimeTicks kOneMicrosecond = 1.0 / kMicrosecondsPerSecond;
static const AVRational kMicrosBase = {1, kMicrosecondsPerSecond};

static const uint32_t kAnalyzeDuration = 10 * kMicrosecondsPerSecond;
static const uint32_t kAudioStreamProbeSize = 512;
static const uint32_t kVideoStreamProbeSize = 128 * 1024;

static const double kSegmentEps = 0.5;

static int s_demux_id = 0;

static TimeTicks ToTimeTicks(int64_t time_ticks, AVRational time_base) {
  int64_t us = av_rescale_q(time_ticks, time_base, kMicrosBase);
  return us * kOneMicrosecond;
}

static int AVIOReadOperation(void* opaque, uint8_t* buf, int buf_size) {
  FFMpegDemuxer* parser = reinterpret_cast<FFMpegDemuxer*>(opaque);
  int result = parser->Read(buf, buf_size);
  if (result < 0) {
    result = AVERROR(EIO);
  }
  return result;
}

template <size_t N, size_t M>
static bool SystemIdEqual(const uint8_t(&s0)[N], const uint8_t(&s1)[M]) {
  if (N != M) return false;
  for (size_t i = 0; i < N; ++i) {
    if (s0[i] != s1[i]) return false;
  }
  return true;
}

unique_ptr<StreamDemuxer> StreamDemuxer::Create(
    const pp::InstanceHandle& instance, Type type, InitMode init_mode) {
  switch (type) {
    case kAudio:
      return MakeUnique<FFMpegDemuxer>(instance, kAudioStreamProbeSize, type,
                                       init_mode);
    case kVideo:
      return MakeUnique<FFMpegDemuxer>(instance, kVideoStreamProbeSize, type,
                                       init_mode);
    default:
      LOG_ERROR("ERROR - not supported type of stream");
  }

  return nullptr;
}

FFMpegDemuxer::FFMpegDemuxer(const pp::InstanceHandle& instance,
                             uint32_t probe_size, Type type, InitMode init_mode)
    : stream_type_(type),
      audio_stream_idx_(-1),
      video_stream_idx_(-1),
      callback_factory_(this),
      format_context_(nullptr),
      io_context_(nullptr),
      context_opened_(false),
      streams_initialized_(false),
      end_of_file_(false),
      exited_(false),
      probe_size_(probe_size),
      timestamp_(0.0),
      has_packets_(false),
      init_mode_(init_mode),
      demux_id_(++s_demux_id) {
  LOG_DEBUG("parser: %p", this);
  audio_config_.demux_id = demux_id_;
  video_config_.demux_id = demux_id_;
}

FFMpegDemuxer::~FFMpegDemuxer() {
  LOG_DEBUG("");
  {
    std::unique_lock<std::mutex> lock(buffer_mutex_);
    exited_ = true;
  }
  buffer_condition_.notify_one();
  parser_thread_->join();
  av_freep(io_context_);
  avformat_free_context(format_context_);
  LOG_DEBUG("");
}

void FFMpegDemuxer::RedirectLoggingOutputs(void *ptr, int level, const char *fmt, va_list vargs )
{

	//	LOG_LIBAV(fmt, vargs);

}

bool FFMpegDemuxer::Init(const InitCallback& callback,
    MessageLoop callback_dispatcher) {
  LOG_DEBUG("Start, parser: %p", this);
  if (callback_dispatcher.is_null() || !callback) {
    LOG_ERROR("ERROR: callback is null or callback_dispatcher is invalid!");
    return false;
  }

  es_pkt_callback_ = callback;
  callback_dispatcher_ = callback_dispatcher;

  InitFFmpeg();

  av_log_set_level(AV_LOG_VERBOSE);
  av_log_set_callback(&FFMpegDemuxer::RedirectLoggingOutputs);


  const char *url = "http://192.168.1.210:9981/stream/channelid/295468956?profile=pass";
  AVFormatContext *s = NULL;

  int ret = avformat_open_input(&format_context_, url, NULL, NULL);
  if (ret < 0)
  {
	  LOG_DEBUG("Error conext %d", ret);
  }
  else
  {
	  LOG_DEBUG("Succ conext %d", ret);
  }

  //format_context_ = avformat_alloc_context();

  io_context_ = avio_alloc_context(
      reinterpret_cast<unsigned char*>(av_malloc(kBufferSize)), kBufferSize, 0,
      this, AVIOReadOperation, NULL, NULL);

  if (format_context_ == NULL || io_context_ == NULL) {
    LOG_ERROR("ERROR: failed to allocate avformat or avio context!");
    return false;
  }

  io_context_->seekable = 0;
  io_context_->write_flag = 0;

  // Change this value in case when clip is not well recognized by ffmpeg
  format_context_->probesize = probe_size_;
  format_context_->max_analyze_duration = kAnalyzeDuration;
  //format_context_->flags |= AVFMT_FLAG_CUSTOM_IO;
  //format_context_->pb = io_context_;

  LOG_INFO("ffmpeg probe size: %u", probe_size_);
  LOG_INFO("ffmpeg analyze duration: %d",
           format_context_->max_analyze_duration);
  LOG_INFO("done, format_context: %p, io_context: %p",
           format_context_, io_context_);

  LOG_INFO("Initialized");
  parser_thread_ = MakeUnique<std::thread>([this](){
    ParsingThreadFn();
  });
  DispatchCallback(kInitialized);

  return true;
}

void FFMpegDemuxer::Flush() {
  LOG_DEBUG("");
  DispatchCallback(kFlushed);
}

void FFMpegDemuxer::Parse(const std::vector<uint8_t>& data) {
  //LOG_DEBUG("parser: %p, data size: %d", this, data.size());
  bool signal_buffer = false;
  {
    std::unique_lock<std::mutex> lock(buffer_mutex_);
    if (data.empty()) {
      LOG_DEBUG("Signal EOF");
      end_of_file_ = true;
      signal_buffer = true;
    } else {
      buffer_.insert(buffer_.end(), data.begin(), data.end());
      signal_buffer = true;
      //LOG_DEBUG("parser: %p, Added buffer to parser.", this);
    }
  }
  if (signal_buffer)
    buffer_condition_.notify_one();
}

bool FFMpegDemuxer::SetAudioConfigListener(
    const std::function<void(const AudioConfig&)>& callback) {
  LOG_DEBUG("");
  if (callback) {
    audio_config_callback_ = callback;
    return true;
  } else {
    LOG_DEBUG("callback is null!");
    return false;
  }
}

bool FFMpegDemuxer::SetVideoConfigListener(
    const std::function<void(const VideoConfig&)>& callback) {
  LOG_DEBUG("");
  if (callback) {
    video_config_callback_ = callback;
    return true;
  } else {
    LOG_DEBUG("callback is null!");
    return false;
  }
}

bool FFMpegDemuxer::SetDRMInitDataListener(const DrmInitCallback& callback) {
  if (callback) {
    drm_init_data_callback_ = callback;
    return true;
  } else {
    LOG_DEBUG("callback is null!");
    return false;
  }
}

void FFMpegDemuxer::SetTimestamp(TimeTicks timestamp) {
  LOG_INFO("current timestamp: %f, new: %f", timestamp_, timestamp);
  timestamp_ = timestamp;
}

void FFMpegDemuxer::Close() {
  DispatchCallback(kClosed);
  LOG_DEBUG("");
}

void FFMpegDemuxer::ParsingThreadFn() {
  if (!streams_initialized_ && !InitStreamInfo()) {
    LOG_ERROR("Can't initialize demuxer");
    return;
  }

  AVPacket pkt;
  bool finished_parsing = false;


  while (!finished_parsing) {
    av_init_packet(&pkt);
    pkt.data = NULL;
    pkt.size = 0;
    Message packet_msg = kError;
    unique_ptr<ElementaryStreamPacket> es_pkt;
    int32_t ret = av_read_frame(format_context_, &pkt);
    if (ret < 0) {
      if (ret == AVERROR_EOF) {
        packet_msg = kEndOfStream;
        finished_parsing = true;
      } else if (ret != AVERROR_EXIT) {  // Unhandled error.
        char errbuff[kErrorBufferSize];
        int32_t strerror_ret = av_strerror(ret, errbuff, kErrorBufferSize);
        LOG_ERROR("%s av_read_frame error: %d [%s], av_strerror ret: %d",
                  stream_type_ == StreamDemuxer::kVideo ? "VIDEO" : "AUDIO",
                  ret, errbuff, strerror_ret);
        {
          std::unique_lock<std::mutex> lock(buffer_mutex_);
          if (exited_) finished_parsing = true;
        }
      } else {
        finished_parsing = true;
      }
    } else {
      //LOG_LIBAV("parser: %p, got packet with size: %d", this, pkt.size);
      if (pkt.stream_index == audio_stream_idx_) {
        packet_msg = kAudioPkt;
      } else if (pkt.stream_index == video_stream_idx_) {
        packet_msg = kVideoPkt;
      } else {
        packet_msg = kError;
        //LOG_LIBAV("Error! Packet stream index (%d) not recognized!",
        //          pkt.stream_index);
      }
      es_pkt = MakeESPacketFromAVPacket(&pkt);
    }

    if (packet_msg != kError) {
      auto es_pkt_callback = std::make_shared<EsPktCallbackData>(packet_msg,
          std::move(es_pkt));
      callback_dispatcher_.PostWork(callback_factory_.NewCallback(
          &FFMpegDemuxer::EsPktCallbackInDispatcherThread, es_pkt_callback));
    }

    av_packet_unref(&pkt);
  }

  LOG_DEBUG("Finished parsing data. buffer left: %d, parser: %p",
            buffer_.size(), this);
}

void FFMpegDemuxer::EsPktCallbackInDispatcherThread(int32_t,
    const std::shared_ptr<EsPktCallbackData>& data) {
  if (es_pkt_callback_) {
    es_pkt_callback_(
        std::get<kEsPktCallbackDataMessage>(*data),
        std::move(std::get<kEsPktCallbackDataPacket>(*data)));
  } else {
    LOG_ERROR("ERROR: es_pkt_callback_ is not initialized");
  }
}

void FFMpegDemuxer::CallbackInDispatcherThread(int32_t, Message msg) {
  (void)msg;  // suppress warning
  LOG_DEBUG("msg: %d", static_cast<int32_t>(msg));
}

void FFMpegDemuxer::DispatchCallback(Message msg) {
  LOG_DEBUG("");
  callback_dispatcher_.PostWork(callback_factory_.NewCallback(
      &FFMpegDemuxer::CallbackInDispatcherThread, msg));
}

void FFMpegDemuxer::CallbackConfigInDispatcherThread(int32_t, Type type) {
  LOG_DEBUG("type: %d", static_cast<int32_t>(type));
  switch (type) {
    case kAudio:
      if (audio_config_callback_) audio_config_callback_(audio_config_);
      break;
    case kVideo:
      if (video_config_callback_) {
        video_config_callback_(video_config_);
      }
      break;
    default:
      LOG_DEBUG("Unsupported type!");
  }
  LOG_DEBUG("");
}

int FFMpegDemuxer::Read(uint8_t* data, int size) {
  std::unique_lock<std::mutex> lock(buffer_mutex_);
  // Order in which conditions are processed below is important.
  // 1. Make sure buffer_ is empty before we can terminate this demuxer.
  //    Otherwise packet supply might be non-contiguous when changing
  //    representations.
  //    TODO(p.balut): However it might be a good idea to distinguish
  //      destroying demuxer upon seek, because seek destruction wouldn't
  //      need to wait for parsing to complete.
  // 2. EOF causes signalling End Of Stream. This must be done only after
  //    buffer_ is processed.
  // 3. See (1).
  buffer_condition_.wait(lock, [this]() {
    return end_of_file_ || !buffer_.empty() || exited_;
  });

  if (!buffer_.empty()) {
    size_t read_bytes = std::min(size, static_cast<int>(buffer_.size()));
    memcpy(data, buffer_.data(), read_bytes);
    buffer_.erase(buffer_.begin(), buffer_.begin() + read_bytes);
    return read_bytes;
  }

  if (end_of_file_)
    return AVERROR_EOF;

  if (exited_)
    return AVERROR(EIO);

  return AVERROR(EIO);
}

void FFMpegDemuxer::InitFFmpeg() {
  static bool is_initialized = false;
  if (!is_initialized) {
    LOG_INFO("avcodec_register_all() - start");
    av_register_all();
    avformat_network_init();
    is_initialized = true;
  }
}

bool FFMpegDemuxer::InitStreamInfo() {
  LOG_DEBUG("FFmpegStreamParser::InitStreamInfo");
  int ret;

  if (!context_opened_) {
    LOG_DEBUG("opening context = %p", format_context_);
    ret = avformat_open_input(&format_context_, NULL, NULL, NULL);
    if (ret < 0) {
      char errbuff[kErrorBufferSize];
      av_strerror(ret, errbuff, kErrorBufferSize);
      LOG_ERROR(
          "failed to open context"
          " ret %d %s",
          ret, errbuff);
      return false;
    }

    LOG_DEBUG("context opened");
    context_opened_ = true;
    streams_initialized_ = false;
  }

  if (!streams_initialized_ && init_mode_ != kSkipInitCodecData) {
    LOG_DEBUG("parsing stream info ctx = %p", format_context_);
    ret = avformat_find_stream_info(format_context_, NULL);
    LOG_DEBUG("find stream info ret %d", ret);
    if (ret < 0) {
      LOG_ERROR("ERROR - find stream info error, ret: %d", ret);
    }
    av_dump_format(format_context_, 0, NULL, 0);
  }
  UpdateContentProtectionConfig();

  audio_stream_idx_ =
      av_find_best_stream(format_context_, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
  if (audio_stream_idx_ >= 0 && init_mode_ != kSkipInitCodecData) {
    UpdateAudioConfig();
  }

  video_stream_idx_ =
      av_find_best_stream(format_context_, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
  if (video_stream_idx_ >= 0 && init_mode_ != kSkipInitCodecData) {
    UpdateVideoConfig();
  }

  LOG_DEBUG("Configs updated");
  if (!streams_initialized_) {
    streams_initialized_ = (audio_stream_idx_ >= 0 || video_stream_idx_ >= 0);
  }

  LOG_DEBUG("DONE, parser: %p, initialized: %d, audio: %d, video: %d", this,
            streams_initialized_, audio_stream_idx_ >= 0,
            video_stream_idx_ >= 0);

  return streams_initialized_;
}

void FFMpegDemuxer::UpdateAudioConfig() {
  LOG_DEBUG("audio index: %d", audio_stream_idx_);

  int channel_no;
  AVStream* s = format_context_->streams[audio_stream_idx_];
  LOG_DEBUG("audio ffmpeg duration: %lld %s", s->duration,
            s->duration == AV_NOPTS_VALUE ? "(AV_NOPTS_VALUE)" : "");

  AVSampleFormat sample_format =
      static_cast<AVSampleFormat>(s->codecpar->format);
  audio_config_.codec_type = ConvertAudioCodec(s->codecpar->codec_id);
  audio_config_.sample_format = ConvertSampleFormat(sample_format);
  if (s->codecpar->bits_per_coded_sample > 0) {
    audio_config_.bits_per_channel = s->codecpar->bits_per_coded_sample;
  } else {
    audio_config_.bits_per_channel = av_get_bytes_per_sample(sample_format) * 8;
    audio_config_.bits_per_channel /= s->codecpar->channels;
  }
  audio_config_.channel_layout =
      ConvertChannelLayout(s->codecpar->channel_layout, s->codecpar->channels);
  audio_config_.samples_per_second = s->codecpar->sample_rate;
  if (audio_config_.codec_type == Samsung::NaClPlayer::AUDIOCODEC_TYPE_AAC) {
    audio_config_.codec_profile =
        ConvertAACAudioCodecProfile(s->codecpar->profile);
    // this method read channel_no, and modify
    // audio_config_.samples_per_second too
    channel_no =
        PrepareAACHeader(s->codecpar->extradata, s->codecpar->extradata_size);
    // if we read channel no change channel_layout,
    // (else we sould break for AAC ;( )
    if (channel_no >= 0)
      audio_config_.channel_layout =
          ConvertChannelLayout(s->codecpar->channel_layout, channel_no);
  }

  if (s->codecpar->extradata_size > 0) {
    audio_config_.extra_data.assign(
        s->codecpar->extradata,
        s->codecpar->extradata + s->codecpar->extradata_size);
  }
  char fourcc[20];
  av_get_codec_tag_string(fourcc, sizeof(fourcc), s->codecpar->codec_tag);
  LOG_DEBUG(
      "audio configuration - codec: %d, profile: %d, codec_tag: (%s), "
      "sample_format: %d, bits_per_channel: %d, channel_layout: %d, "
      "samples_per_second: %d",
      audio_config_.codec_type, audio_config_.codec_profile, fourcc,
      audio_config_.sample_format, audio_config_.bits_per_channel,
      audio_config_.channel_layout, audio_config_.samples_per_second);

  callback_dispatcher_.PostWork(callback_factory_.NewCallback(
      &FFMpegDemuxer::CallbackConfigInDispatcherThread, kAudio));
  LOG_DEBUG("audio configuration updated");
}

void FFMpegDemuxer::UpdateVideoConfig() {
  LOG_DEBUG("video index: %d", video_stream_idx_);

  AVStream* s = format_context_->streams[video_stream_idx_];
  LOG_DEBUG("video ffmpeg duration: %lld %s", s->duration,
            s->duration == AV_NOPTS_VALUE ? "(AV_NOPTS_VALUE)" : "");

  video_config_.codec_type = ConvertVideoCodec(s->codecpar->codec_id);
  switch (video_config_.codec_type) {
    case Samsung::NaClPlayer::VIDEOCODEC_TYPE_VP8:
      video_config_.codec_profile =
          Samsung::NaClPlayer::VIDEOCODEC_PROFILE_VP8_MAIN;
      break;
    case Samsung::NaClPlayer::VIDEOCODEC_TYPE_VP9:
      video_config_.codec_profile =
          Samsung::NaClPlayer::VIDEOCODEC_PROFILE_VP9_MAIN;
      break;
    case Samsung::NaClPlayer::VIDEOCODEC_TYPE_H264:
      video_config_.codec_profile =
          ConvertH264VideoCodecProfile(s->codecpar->profile);
      break;
    case Samsung::NaClPlayer::VIDEOCODEC_TYPE_MPEG2:
      video_config_.codec_profile =
          ConvertMPEG2VideoCodecProfile(s->codecpar->profile);
      break;
    default:
      video_config_.codec_profile =
          Samsung::NaClPlayer::VIDEOCODEC_PROFILE_UNKNOWN;
  }

  video_config_.frame_format = ConvertVideoFrameFormat(s->codecpar->format);

  AVDictionaryEntry* webm_alpha =
      av_dict_get(s->metadata, "alpha_mode", NULL, 0);
  if (webm_alpha && !strcmp(webm_alpha->value, "1"))
    video_config_.frame_format = Samsung::NaClPlayer::VIDEOFRAME_FORMAT_YV12A;

  video_config_.size = Size(s->codecpar->width,
                            s->codecpar->height);

  LOG_DEBUG("r_frame_rate %d. %d#", s->r_frame_rate.num, s->r_frame_rate.den);
  video_config_.frame_rate = Rational(s->r_frame_rate.num, s->r_frame_rate.den);

  if (s->codecpar->extradata_size > 0) {
    video_config_.extra_data.assign(
        s->codecpar->extradata,
        s->codecpar->extradata + s->codecpar->extradata_size);
  }

  char fourcc[20];
  av_get_codec_tag_string(fourcc, sizeof(fourcc), s->codecpar->codec_tag);
  LOG_DEBUG(
      "video configuration - codec: %d, profile: %d, codec_tag: (%s), "
      "frame: %d, visible_rect: %d %d ",
      video_config_.codec_type, video_config_.codec_profile, fourcc,
      video_config_.frame_format, video_config_.size.width,
      video_config_.size.height);

  callback_dispatcher_.PostWork(callback_factory_.NewCallback(
      &FFMpegDemuxer::CallbackConfigInDispatcherThread, kVideo));
  LOG_DEBUG("video configuration updated");
}

int FFMpegDemuxer::PrepareAACHeader(const uint8_t* data, size_t length) {
  LOG_DEBUG("");
  if (!data || length < 2) {
    LOG_DEBUG("empty extra data, it's needed to read aac config");
    return -1;
  }

  int32_t samples_rate_index;
  samples_rate_index = (((*data) & 0x3) << 1) | (*(data + 1)) >> 7;

  uint8_t channel_config;
  if (samples_rate_index != 15) {
    channel_config = (*(data + 1) & 0x78) >> 3;
  } else {
    return -1;  // we don't support custom frequencies
  }

  // Extern from ffmpeg file: mpeg4audio.c
  extern const int avpriv_mpeg4audio_sample_rates[];
  audio_config_.samples_per_second =
      avpriv_mpeg4audio_sample_rates[samples_rate_index];

  LOG_DEBUG("prepared AAC (ADTS) header template");
  return channel_config;
}

void FFMpegDemuxer::UpdateContentProtectionConfig() {
  LOG_DEBUG("protection data count: %u",
            format_context_->protection_system_data_count);
  if (format_context_->protection_system_data_count <= 0) return;

  for (uint32_t i = 0; i < format_context_->protection_system_data_count; ++i) {
    AVProtectionSystemSpecificData& system_data =
        format_context_->protection_system_data[i];

    if (SystemIdEqual(system_data.system_id, kPlayReadySystemId)) {
      std::vector<uint8_t> init_data;
      init_data.insert(init_data.begin(), system_data.pssh_box,
                       system_data.pssh_box + system_data.pssh_box_size);
      LOG_DEBUG("Found PlayReady init data (pssh box)");

      callback_dispatcher_.PostWork(callback_factory_.NewCallback(
          &FFMpegDemuxer::DrmInitCallbackInDispatcherThread, kDRMInitDataType,
          init_data));
      return;
    }
  }

  LOG_DEBUG("Couldn't find PlayReady init data! App supports only PlayReady");
  return;
}

void FFMpegDemuxer::DrmInitCallbackInDispatcherThread(int32_t,
    const std::string& type, const std::vector<uint8_t>& init_data) {
  if (drm_init_data_callback_)
    drm_init_data_callback_(type, init_data);
  else
    LOG_ERROR("ERROR: drm_init_data_callback_ is not initialized!");
}

unique_ptr<ElementaryStreamPacket> FFMpegDemuxer::MakeESPacketFromAVPacket(
    AVPacket* pkt) {
  auto es_packet = MakeUnique<ElementaryStreamPacket>(pkt->data, pkt->size);
  es_packet->demux_id = demux_id_;

  AVStream* s = format_context_->streams[pkt->stream_index];

  es_packet->SetDuration(ToTimeTicks(pkt->duration, s->time_base));
  es_packet->SetKeyFrame(pkt->flags == 1);

  auto pts = ToTimeTicks(pkt->pts, s->time_base);
  auto dts = ToTimeTicks(pkt->dts, s->time_base);
  if (!has_packets_ && pts + kSegmentEps >= timestamp_) {
      LOG_DEBUG("Got properly timestamped packet. Zero timestamp variable");
      timestamp_ = 0;
  }
  has_packets_ = true;

  es_packet->SetPts(pts + timestamp_);
  es_packet->SetDts(dts + timestamp_);

  return es_packet;
}

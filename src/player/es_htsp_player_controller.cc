#include "es_htsp_player_controller.h"

#include <functional>
#include <limits>
#include <utility>

#include "nacl_player/media_codecs.h"
#include "nacl_player/media_common.h"
#include "ppapi/cpp/var_dictionary.h"
#include "nacl_player/error_codes.h"
#include "nacl_player/es_data_source.h"
#include "nacl_player/url_data_source.h"
#include "elementary_stream_packet.h"
#include "ffmpeg_demuxer.h"

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>

#include "stream_demuxer.h"
#include "common.h"

extern "C" {
#include <ppapi/c/pp_macros.h>
}

using Samsung::NaClPlayer::DRMType;
using Samsung::NaClPlayer::DRMType_Playready;
using Samsung::NaClPlayer::DRMType_Unknown;
using Samsung::NaClPlayer::ErrorCodes;
using Samsung::NaClPlayer::ESDataSource;
using Samsung::NaClPlayer::URLDataSource;
using Samsung::NaClPlayer::MediaDataSource;
using Samsung::NaClPlayer::MediaPlayer;
using Samsung::NaClPlayer::MediaPlayerState;
using Samsung::NaClPlayer::Rect;
using Samsung::NaClPlayer::TextTrackInfo;
using Samsung::NaClPlayer::TimeTicks;
using std::make_shared;
using std::placeholders::_1;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;
using Samsung::NaClPlayer::VideoElementaryStream;
using Samsung::NaClPlayer::VideoCodec_Profile;
using Samsung::NaClPlayer::ESPacket;
const int64_t kMainLoopDelay = 50;  // in milliseconds


class EsHtspPlayerController::Impl {
public:
 //template<typename RepType>
 static void InitializeStream(EsHtspPlayerController* thiz)
 {
 }
};

void EsHtspPlayerController::InitPlayer() {
  //LOG_INFO("Loading media from : [%s]", mpd_file_path.c_str());
  CleanPlayer();
  //demuxer_ = StreamDemuxer::Create(instance_handle_, demuxer_type, init_mode);

  demuxer_ = MakeUnique<FFMpegDemuxer>(instance_, 512, StreamDemuxer::kVideo, StreamDemuxer::kFullInitialization);
  //demuxer_ = StreamDemuxer::Create(instance_, StreamDemuxer::kVideo, StreamDemuxer::kFullInitialization);

  auto es_packet_callback = [this](StreamDemuxer::Message message,
      std::unique_ptr<ElementaryStreamPacket> packet) {
    OnEsPacket(message, std::move(packet));
  };

  demuxer_->Init(es_packet_callback, pp::MessageLoop::GetCurrent());

  demuxer_->SetVideoConfigListener([this](
        const VideoConfig& config) {
      OnVideoConfig(config);
    });

  player_ = make_shared<MediaPlayer>();
  listeners_.player_listener =
      make_shared<MediaPlayerListener>();
  listeners_.buffering_listener =
      make_shared<MediaBufferingListener>(shared_from_this());

  player_->SetMediaEventsListener(listeners_.player_listener);
  player_->SetBufferingListener(listeners_.buffering_listener);

  //video_stream->SetCodecExtraData(video_config.extra_data.size(),
  //                                &video_config.extra_data.front());


/*
  //elementary_stream_ =  std::static_pointer_cast<ElementaryStream>(video_stream);

  //elementary_stream_->App

  //es_data_source->AddStream(*video_stream, this);
*/
  //int32_t result = player_->AttachDataSource(*data_source_);

  int32_t ret = player_->SetDisplayRect(view_rect_);

  if (ret != ErrorCodes::Success) {
	  char str[1024];
	  sprintf(str, "Failed to set display rect [(%d - %d) (%d - %d)], code: %d", view_rect_.x(), view_rect_.y(), view_rect_.width(), view_rect_.height(), ret);
	  pp_Instance->PostMessage(
			  str
       );
	  free(str);
  }

  //InitializeSubtitles(subtitle, encoding);

  player_thread_ = MakeUnique<pp::SimpleThread>(instance_);
  player_thread_->Start();

  //if (!demuxer_->Init(es_packet_callback_, pp::MessageLoop::GetCurrent()))
	  //pp_Instance->PostMessage("Demuxer init fail");
  //Here init media provider
  //player_thread_->message_loop().PostWork(
      //cc_factory_.NewCallback(&EsDashPlayerController::InitializeHtsp,
                              //mpd_file_path));

  //Play();
  //init = 1;
}
void EsHtspPlayerController::InitializeSubtitles(const std::string& subtitle,
                                                 const std::string& encoding) {}

void EsHtspPlayerController::InitializeVideoStream(
    Samsung::NaClPlayer::DRMType drm_type) {
  Impl::InitializeStream(this);
}

void EsHtspPlayerController::InitializeAudioStream(
    Samsung::NaClPlayer::DRMType drm_type) {
  Impl::InitializeStream(this);
}

void EsHtspPlayerController::Seek(TimeTicks original_time) {

}

void EsHtspPlayerController::OnSeek(int32_t ret) {

}

void EsHtspPlayerController::OnChangeRepresentation(int32_t, StreamType type,
                                                     int32_t id) {

}

void EsHtspPlayerController::ChangeRepresentation(StreamType stream_type,
                                                  int32_t id) {
  //LOG_INFO("Changing rep type: %d -> %d", stream_type, id);
  player_thread_->message_loop().PostWork(cc_factory_.NewCallback(
      &EsHtspPlayerController::OnChangeRepresentation, stream_type, id));
}

void EsHtspPlayerController::UpdateStreamsBuffer(int32_t) {

}

void EsHtspPlayerController::SetViewRect(const Rect& view_rect) {
  view_rect_ = view_rect;
  if (!player_) return;

  char str[1024];
  sprintf(str, "Set view rect to %d, %d", view_rect_.width(), view_rect_.height());
  pp_Instance->PostMessage(
		  str
 );
  free(str);
pp_Instance->PostMessage(str);

  //LOG_DEBUG("Set view rect to %d, %d", view_rect_.width(), view_rect_.height());
  auto callback = WeakBind(&EsHtspPlayerController::OnSetDisplayRect,
      std::static_pointer_cast<EsHtspPlayerController>(
          shared_from_this()), _1);
  int32_t ret = player_->SetDisplayRect(view_rect_, callback);
  if (ret < ErrorCodes::CompletionPending)
  {
	  char str[1024];
	  sprintf(str, "SetDisplayRect result: %d", ret);
	  pp_Instance->PostMessage(
			  str
     );
	  free(str);
	pp_Instance->PostMessage(str);
	  //LOG_ERROR("SetDisplayRect result: %d", ret);
  }

}

void EsHtspPlayerController::PostTextTrackInfo() {
}

void EsHtspPlayerController::ChangeSubtitles(int32_t id) {
}

void EsHtspPlayerController::ChangeSubtitleVisibility() {
}

PlayerController::PlayerState EsHtspPlayerController::GetState() {
  return state_;
}

void EsHtspPlayerController::OnStreamConfigured(StreamType type) {
}

void EsHtspPlayerController::FinishStreamConfiguration() {
  //LOG_INFO("All streams configured, attaching data source.");
  // Audio and video stream should be initialized already.
  if (!player_) {
    //LOG_DEBUG("player_ is null!, quit function");
    return;
  }
  int32_t result = player_->AttachDataSource(*data_source_);

  if (result == ErrorCodes::Success && state_ != PlayerState::kError) {
    if (state_ == PlayerState::kUnitialized)
      state_ = PlayerState::kReady;
    //LOG_INFO("Data Source attached");
  } else {
    state_ = PlayerState::kError;
    //LOG_ERROR("Failed to AttachDataSource!");
  }
}

void EsHtspPlayerController::OnSetDisplayRect(int32_t ret) {
	  char str[1024];
	  sprintf(str, "SetDisplayRect result: %d", ret);
	  pp_Instance->PostMessage(
			  str
     );
	  free(str);
	pp_Instance->PostMessage(str);
  //LOG_DEBUG("SetDisplayRect result: %d", ret);
}

void EsHtspPlayerController::OnChangeSubtitles(int32_t, int32_t id) {
  int32_t ret = player_->SelectTrack(
      Samsung::NaClPlayer::ElementaryStreamType_Text, id);
  if (ret == ErrorCodes::Success) {
    //LOG_INFO("SelectTrack called successfully");
  } else {
    //LOG_ERROR("SelectTrack call failed, code: %d", ret);
  }
}

void EsHtspPlayerController::OnChangeSubVisibility(int32_t, bool show) {
  //if (show)
    //player_->SetSubtitleListener(listeners_.subtitle_listener);
  //else
    //player_->SetSubtitleListener(nullptr);
}

void EsHtspPlayerController::PerformWaitingOperations() {
  for (size_t i = 0; i < waiting_representation_changes_.size(); ++i) {
    if (waiting_representation_changes_[i]) {
      OnChangeRepresentation(PP_OK, static_cast<StreamType>(i),
                             *waiting_representation_changes_[i]);
      waiting_representation_changes_[i].reset(nullptr);
    }
  }
}

void EsHtspPlayerController::Play() {
  if (!player_) {
    //LOG_INFO("Play. player_ is null");
    return;
  }

  int32_t ret = player_->Play();
  if (ret == ErrorCodes::Success) {
   // LOG_INFO("Play called successfully");
    state_ = PlayerState::kPlaying;
  } else {
	  //pp_Instance->PostMessage("Failed play");
    //LOG_ERROR("Play call failed, code: %d", ret);
  }
}

void EsHtspPlayerController::Pause() {
  if (!player_) {
    //LOG_INFO("Pause. player_ is null");
    return;
  }

  int32_t ret = player_->Pause();
  if (ret == ErrorCodes::Success) {
    //LOG_INFO("Pause called successfully");
    state_ = PlayerState::kPaused;
  } else {
    //LOG_ERROR("Pause call failed, code: %d", ret);
  }
}

void EsHtspPlayerController::CleanPlayer() {
  //LOG_INFO("Cleaning player.");
  if (!player_) return;
  player_->SetMediaEventsListener(nullptr);
  player_->SetSubtitleListener(nullptr);
  player_->SetBufferingListener(nullptr);
  player_thread_.reset();
  data_source_.reset();
  //dash_parser_.reset();
  text_track_.reset();
  player_.reset();
  //packets_manager_.SetStream(StreamType::Audio, nullptr);
  //packets_manager_.SetStream(StreamType::Video, nullptr);
  //for (auto& stream : streams_)
    //stream.reset();
  state_ = PlayerState::kUnitialized;
  //video_representations_.clear();
  //audio_representations_.clear();
  //LOG_INFO("Finished closing.");
}


void EsHtspPlayerController::OnNeedData(int32_t bytes)
{
	  //pp_Instance->PostMessage("Player report need data");
}

void EsHtspPlayerController::OnEnoughData()
{
	//pp_Instance->PostMessage("Player report OnEnoughData");
}

void EsHtspPlayerController::OnSeekData(TimeTicks new_position)
{

}

void EsHtspPlayerController::Config(int32_t result, htsmsg_t* msg){
	pp_Instance->PostMessage("Set - Start");
	const void* bin = nullptr;
	  size_t binlen = 0;

	if ( htsmsg_get_bin(msg, "meta", &bin, &binlen))
	  {
		pp_Instance->PostMessage("Set - Error");
	    //Logger::Log(LogLevel::LEVEL_ERROR, "malformed muxpkt: 'stream'/'payload' missing");
	    return;
	  }

	//std::vector<uint8_t> v((uint8_t*)bin, (uint8_t*)bin + binlen);
	//packages.insert(packages.end(), v.begin(), v.end());
/*
	  data_source_ = std::make_shared<ESDataSource>();

	  auto video_stream = make_shared<VideoElementaryStream>();
	  data_source_->AddStream(*video_stream, this);


	  //auto video_stream =  std::static_pointer_cast<VideoElementaryStream>(elementary_stream_);
	  video_stream->SetVideoCodecType(Samsung::NaClPlayer::VIDEOCODEC_TYPE_H264);
	  //video_stream->SetVideoCodecProfile(Samsung::NaClPlayer::VIDEOCODEC_PROFILE_H264_HIGH10);
	  //video_stream->SetVideoFrameFormat(Samsung::NaClPlayer::VIDEOFRAME_FORMAT_YV12);
	  video_stream->SetVideoFrameSize(Samsung::NaClPlayer::Size(1920, 1080));
	  video_stream->SetFrameRate(Samsung::NaClPlayer::Rational(25, 1));
	  video_stream->SetCodecExtraData(binlen, &bin);


	  //data_source_->SetDuration(Samsung::NaClPlayer::TimeTicks(1));

	  auto result_init = video_stream->InitializeDone();
	  pp_Instance->PostMessage(result_init);

	  elementary_stream_ =  std::static_pointer_cast<ElementaryStream>(video_stream);

	  int32_t resulta = player_->AttachDataSource(*data_source_);

	  //Play();
	  pp_Instance->PostMessage(resulta);
*/
	  init = 1;
	  pp_Instance->PostMessage("Set Extra");
}



void EsHtspPlayerController::AddData(const std::string& method, htsmsg_t* msg)
{

	if(method == "subscriptionStart")
	{
		Config(0, msg);
		//htsmsg_t *msgs = htsmsg_copy(src);
		//player_thread_->message_loop().PostWork(
			//cc_factory_.NewCallback(
				//&EsHtspPlayerController::Config, msgs));
	}

		if(method == "muxpkt")
		{
			//htsmsg_t *msg = htsmsg_copy(src);
			if(init == 0)
			{
				pp_Instance->PostMessage("Packet before init");
				return;
			}

			Async(0, msg);
			//player_thread_->message_loop().PostWork(
				//cc_factory_.NewCallback(
					//&EsHtspPlayerController::Async, msg));
		}
}

void EsHtspPlayerController::Async(int32_t result, htsmsg_t* msgs){

	if(init == -1000)
	{
		return;
	}
	htsmsg_t *msg = htsmsg_copy(msgs);
	const void* bin = nullptr;
	  size_t binlen = 0;

	  uint32_t idx = 0;
	if (htsmsg_get_u32(msg, "stream", &idx) || htsmsg_get_bin(msg, "payload", &bin, &binlen))
	  {
	    //Logger::Log(LogLevel::LEVEL_ERROR, "malformed muxpkt: 'stream'/'payload' missing");
		pp_Instance->PostMessage("Malform");
	    return;
	  }

if(idx == 1)
{
	//uint8_t *charBuf = (uint8_t*)bin;
	init++;

	std::vector<uint8_t> v((uint8_t*)bin, (uint8_t*)bin + binlen);
	demuxer_->Parse(v);
	//packages.insert(packages.end(), v.begin(), v.end());
/*
	if(init > 100)
	{
		pp_Instance->PostMessage("Demux write");
		demuxer_->Parse(packages);
		init = -1000;
	}*/
	//void* bin2 = malloc (binlen);
	 //memcpy ( &bin2, bin, binlen );
	//pp_Instance->PostMessage("Index write");
	/*
	void *q = const_cast<void *>(bin);

	std::unique_ptr<ElementaryStreamPacket> es_packet = MakeUnique<ElementaryStreamPacket>((uint8_t*)q, binlen);

	double deleno = 1000000;

	//es_packet->buffer = bin;
	//es_packet->size = binlen;

	int64_t s64 = 0;
	htsmsg_get_s64(msg, "pts", &s64);

	double resultpts = (double)s64 / (double)deleno;
	pp_Instance->PostMessage("Pts:" +std::to_string(resultpts));
	//es_packet->pts = Samsung::NaClPlayer::TimeTicks(resultpts);
	es_packet->SetPts(Samsung::NaClPlayer::TimeTicks(resultpts));

	lastpts = resultpts;
	int64_t s642 = 0;
	htsmsg_get_s64(msg, "dts", &s642);

	double resultdts = (double)s642 / (double)deleno;
	pp_Instance->PostMessage("Dts:" +std::to_string(resultdts));
	//es_packet->dts = Samsung::NaClPlayer::TimeTicks(resultdts);
	es_packet->SetDts(Samsung::NaClPlayer::TimeTicks(resultdts));

	uint32_t u32 = 0;
	htsmsg_get_u32(msg, "duration", &u32);
	double resultduration = (double)u32 / (double)deleno;
	pp_Instance->PostMessage("Durr:" +std::to_string(resultduration));
	//es_packet->duration  = Samsung::NaClPlayer::TimeTicks(resultduration);
	es_packet->SetDuration(Samsung::NaClPlayer::TimeTicks(resultduration));
	uint32_t u32f = 0;
	if (!htsmsg_get_u32(msg, "frametype", &u32f))
	{
		if(u32f == 73)
		{
			//pp_Instance->PostMessage("Key");
			es_packet->SetKeyFrame(true);//is_key_frame = true;
		}
	}

	//pp_Instance->PostMessage("Dur - calc");
	duration = duration + ((double)u32 / deleno);
	//pp_Instance->PostMessage("Dur - set");
	data_source_->SetDuration(Samsung::NaClPlayer::TimeTicks(20));
	//pp_Instance->PostMessage("Pack - append");
	//pp::CompletionCallback callback = cc_factory_.NewCallback(&EsHtspPlayerController::LogPacket);
	//std::function<void(int32_t)> DelayFunction = std::function<void(uint32_t, &EsHtspPlayerController::LogPacket)>();
	auto resultapp = elementary_stream_->AppendPacket(es_packet->GetESPacket());
	if(resultapp != 0)
	{
		init = -1000;
		pp_Instance->PostMessage(resultapp);
	}
	if(init == 10)
	{	Play(); }
	//else
	//{
	//	init++;
	//}
	//pp_Instance->PostMessage("Succ"+ std::to_string(init));
*/
	}


}
void EsHtspPlayerController::OnVideoConfig(const VideoConfig& video_config)
{
	  data_source_ = std::make_shared<ESDataSource>();

	  auto video_stream = make_shared<VideoElementaryStream>();
	  data_source_->AddStream(*video_stream, this);


	  //auto video_stream =  std::static_pointer_cast<VideoElementaryStream>(elementary_stream_);
	  video_stream->SetVideoCodecType(video_config.codec_type);
	  video_stream->SetVideoCodecProfile(video_config.codec_profile);
	  video_stream->SetVideoFrameFormat(video_config.frame_format);
	  video_stream->SetVideoFrameSize(video_config.size);
	  video_stream->SetFrameRate(video_config.frame_rate);
	  video_stream->SetCodecExtraData(video_config.extra_data.size(), &video_config.extra_data.front());


	  //data_source_->SetDuration(Samsung::NaClPlayer::TimeTicks(1));

	  auto result_init = video_stream->InitializeDone();
	  pp_Instance->PostMessage(result_init);

	  elementary_stream_ =  std::static_pointer_cast<ElementaryStream>(video_stream);

	  int32_t resulta = player_->AttachDataSource(*data_source_);

	  //Play();
	  pp_Instance->PostMessage(resulta);
	  pp_Instance->PostMessage("Set");
	  //init = 1;

}

void EsHtspPlayerController::OnEsPacket(
    StreamDemuxer::Message message,
    std::unique_ptr<ElementaryStreamPacket> packet) {

	//pp_Instance->PostMessage("Es Packet");
	auto resultapp = elementary_stream_->AppendPacket(packet->GetESPacket());

	if(resultapp != 0)
	{
		pp_Instance->PostMessage(resultapp);
	}
	Play();
}

	void EsHtspPlayerController::LogPacket(int32_t result, Samsung::NaClPlayer::ESPacket* es_packet)
	{
		if(init == 1)
		{
			init = 2;
			return;
		}
		 auto resultapp = elementary_stream_->AppendPacket(*es_packet);
		pp_Instance->PostMessage(resultapp);
	}




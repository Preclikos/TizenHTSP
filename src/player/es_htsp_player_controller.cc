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
 #include <stdint.h>
#include  <cstring>
#include <string.h>
#include "ffmpeg_demuxer.h"
#include "common.h"
#include <iostream>
#include <list>
#include <iterator>

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
using Samsung::NaClPlayer::AudioElementaryStream;
using Samsung::NaClPlayer::VideoCodec_Profile;
using Samsung::NaClPlayer::ESPacket;
using Samsung::NaClPlayer::URLDataSource;
const int64_t kMainLoopDelay = 50;  // in milliseconds


class EsHtspPlayerController::Impl {
public:
 //template<typename RepType>
 static void InitializeStream(EsHtspPlayerController* thiz)
 {
 }
};


void EsHtspPlayerController::AttachSource()
{
	if(videoInit && audioInit)
	{
		int32_t resulta = player_->AttachDataSource(*data_source_);
		LOG_DEBUG("Add source to player result: %d", resulta);
	}
}

void EsHtspPlayerController::InitPlayer() {

  CleanPlayer();

  demuxer_ = StreamDemuxer::Create(instance_, StreamDemuxer::kVideo, StreamDemuxer::kFullInitialization);


  auto es_packet_ = [this](StreamDemuxer::Message message,
      std::unique_ptr<ElementaryStreamPacket> packet) {

	  	auto esPacket = packet->GetESPacket();


	  	if(message == StreamDemuxer::Message::kVideoPkt && videoInit) //Video Packet
	  	{
	  		auto resultapp = elementary_stream_video->AppendPacket(esPacket);

			if(resultapp != 0)
			{
				LOG_LIBAV("Video Packet append fail: %d", resultapp);
			}
	  	}

	  	if(message == StreamDemuxer::Message::kAudioPkt && audioInit) //Audio Packet
	  	{
	  		auto resultapp = elementary_stream_audio->AppendPacket(esPacket);

			if(resultapp != 0)
			{
				LOG_LIBAV("Audio Packet append fail: %d", resultapp);
			}
	  	}
  };



  demuxer_->SetVideoConfigListener([this](
        const VideoConfig& config) {

		auto video_stream = make_shared<VideoElementaryStream>();
		data_source_->AddStream(*video_stream, this);

		video_stream->SetVideoCodecType(config.codec_type);
		video_stream->SetVideoCodecProfile(config.codec_profile);
		video_stream->SetVideoFrameFormat(config.frame_format);
		video_stream->SetVideoFrameSize(config.size);
		video_stream->SetFrameRate(config.frame_rate);

	    video_stream->SetCodecExtraData(config.extra_data.size(),
	                                    &config.extra_data.front());


		auto result_init_video = video_stream->InitializeDone(Samsung::NaClPlayer::StreamInitializationMode_Full);
		//auto result_init_video = video_stream->InitializeDone();
		LOG_DEBUG("Video init result: %d", result_init_video);

		elementary_stream_video =  std::static_pointer_cast<ElementaryStream>(video_stream);

		videoInit = true;

	  	AttachSource();
    });

  demuxer_->SetAudioConfigListener([this](
        const AudioConfig& config) {
/*
		auto audio_stream = make_shared<AudioElementaryStream>();
		data_source_->AddStream(*audio_stream, this);

		audio_stream->SetAudioCodecType(config.codec_type);
		audio_stream->SetAudioCodecProfile(config.codec_profile);
		audio_stream->SetSampleFormat(config.sample_format);
		audio_stream->SetBitsPerChannel(config.bits_per_channel);
		audio_stream->SetSamplesPerSecond(config.samples_per_second);
		audio_stream->SetChannelLayout (config.channel_layout);

		audio_stream->SetCodecExtraData(config.extra_data.size(),
	                                    &config.extra_data.front());

		//auto result_init_audio = audio_stream->InitializeDone(Samsung::NaClPlayer::StreamInitializationMode_Full);
		auto result_init_audio = audio_stream->InitializeDone();
		LOG_DEBUG("Audio init result: %d", result_init_audio);

		elementary_stream_audio =  std::static_pointer_cast<ElementaryStream>(audio_stream);

		audioInit = true;

	  	AttachSource();
	  	*/
    });

  player_ = make_shared<MediaPlayer>();
  listeners_.player_listener =
      make_shared<MediaPlayerListener>();
  listeners_.buffering_listener =
      make_shared<MediaBufferingListener>(shared_from_this());

  player_->SetMediaEventsListener(listeners_.player_listener);
  player_->SetBufferingListener(listeners_.buffering_listener);


  int32_t ret = player_->SetDisplayRect(view_rect_);

  if (ret != ErrorCodes::Success) {
	  char str[1024];
	  sprintf(str, "Failed to set display rect [(%d - %d) (%d - %d)], code: %d", view_rect_.x(), view_rect_.y(), view_rect_.width(), view_rect_.height(), ret);
	  pp_Instance->PostMessage(
			  str
       );
	  free(str);
  }


  player_thread_ = MakeUnique<pp::SimpleThread>(instance_);
  player_thread_->Start();

  data_source_ = std::make_shared<ESDataSource>();

  demuxer_->Init(es_packet_, pp::MessageLoop::GetCurrent());

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
	  //pp_Instance->PostMessage(ret);
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
	pp_Instance->PostMessage("Player report OnEnoughData");
}

void EsHtspPlayerController::OnSeekData(TimeTicks new_position)
{

}

void EsHtspPlayerController::Config(int32_t result, htsmsg_t* msg){
	pp_Instance->PostMessage("Set - Vdeo - Start");

	data_source_ = std::make_shared<ESDataSource>();

	auto video_stream = make_shared<VideoElementaryStream>();
	data_source_->AddStream(*video_stream, this);

	video_stream->SetVideoCodecType(Samsung::NaClPlayer::VIDEOCODEC_TYPE_H264);
	video_stream->SetVideoCodecProfile(Samsung::NaClPlayer::VIDEOCODEC_PROFILE_H264_HIGH10);
	video_stream->SetVideoFrameFormat(Samsung::NaClPlayer::VIDEOFRAME_FORMAT_YV12);
	video_stream->SetVideoFrameSize(Samsung::NaClPlayer::Size(1920, 1080));
	video_stream->SetFrameRate(Samsung::NaClPlayer::Rational(2, 50));


	//auto audio_stream = make_shared<AudioElementaryStream>();
	//data_source_->AddStream(*audio_stream, this);

	//audio_stream->SetAudioCodecType(Samsung::NaClPlayer::AUDIOCODEC_TYPE_AAC);
	//audio_stream->SetSampleFormat(Samsung::NaClPlayer::SAMPLEFORMAT_PLANARF32);
	//audio_stream->SetBitsPerChannel(16);
	//audio_stream->SetSamplesPerSecond(48000);
	//audio_stream->SetChannelLayout (Samsung::NaClPlayer::CHANNEL_LAYOUT_STEREO);


	//auto map = htsmsg_get_list(msg, "streams");// htsmsg_get_map(msg, "streams");


	if (!htsmsg_get_bin(msg, "meta", &metabin, &metabinlen))
	{
		LOG_DEBUG("Set - No extra");
		video_stream->SetCodecExtraData(metabinlen, metabin);
	}

	//auto result_init_audio = audio_stream->InitializeDone();
	//pp_Instance->PostMessage(result_init_audio);
	auto result_init_video = video_stream->InitializeDone();
	pp_Instance->PostMessage(result_init_video);



	int32_t resulta = player_->AttachDataSource(*data_source_);
	pp_Instance->PostMessage(resulta);

	elementary_stream_video =  std::static_pointer_cast<ElementaryStream>(video_stream);
	//elementary_stream_audio =  std::static_pointer_cast<ElementaryStream>(audio_stream);

	init = 1;
	LOG_DEBUG("Init done");
}

void EsHtspPlayerController::AddHttpData(const char* buffer, int32_t num_bytes)
{
	std::vector<uint8_t> raw_image(buffer, buffer + num_bytes);
	demuxer_->Parse(raw_image);
}

void EsHtspPlayerController::AddData(const std::string& method, htsmsg_t* msg)
{

	if(method == "subscriptionStart")
	{
		//Config(0, msg);
	}

		if(method == "muxpkt")
		{
			/*
			if(init == 0)
			{
				pp_Instance->PostMessage("Packet before init");
				return;
			}*/
			MuxPacket(msg);
		}
}



void EsHtspPlayerController::MuxPacket(htsmsg_t* msgs){
	htsmsg_t *msg = htsmsg_copy(msgs);
	const void* bin = nullptr;
	size_t binlen = 0;

	uint32_t idx = 0;
	if (htsmsg_get_u32(msg, "stream", &idx) || htsmsg_get_bin(msg, "payload", &bin, &binlen))
	{
		pp_Instance->PostMessage("Malform");
		return;
	}
	init++;
	if(idx == 1)
	{
		std::unique_ptr<ESPacket> es_packet = MakeUnique<ESPacket>();



		double deleno = 1000000;

		int64_t s64 = 0;
		htsmsg_get_s64(msg, "pts", &s64);

		double resultpts = (double)s64 / (double)deleno;
		//pp_Instance->PostMessage("pts:" +std::to_string(resultpts));


		int64_t s642 = 0;
		htsmsg_get_s64(msg, "dts", &s642);

		double resultdts = (double)s642 / (double)deleno;
		//pp_Instance->PostMessage("pts / dts:" +std::to_string(resultdts));

		es_packet->pts = Samsung::NaClPlayer::TimeTicks(resultpts);
		es_packet->dts = Samsung::NaClPlayer::TimeTicks(resultdts);

		uint32_t u32 = 0;
		htsmsg_get_u32(msg, "duration", &u32);
		double resultduration = (double)u32 / (double)deleno;
		//pp_Instance->PostMessage("Durr:" +std::to_string(resultduration));
		es_packet->duration = Samsung::NaClPlayer::TimeTicks(resultduration);

        bool isKey = false;

		uint32_t u32f = 0;
		if (!htsmsg_get_u32(msg, "frametype", &u32f))
		{
			if(u32f == 73)
			{
				es_packet->is_key_frame = true;
				isKey = true;
				//pp_Instance->PostMessage("Key");
			}
		}


		pp_Instance->PostMessage("Pass - " + std::to_string(init));
		const vector<uint8_t> vuc(static_cast<const uint8_t*>(bin), static_cast<const uint8_t*>(bin) + binlen);
		demuxer_->Parse(vuc);
/*


		es_packet->buffer = bin;
		es_packet->size = binlen;


		auto resultapp = elementary_stream_video->AppendPacket(*es_packet);

		if(resultapp != 0)
		{
			pp_Instance->PostMessage(std::to_string(resultapp) + "-" + std::to_string(init));
			pp_Instance->PostMessage("PTS:" +std::to_string(resultpts) +" DTS:" +std::to_string(resultdts) + " Key: "+std::to_string(isKey));
			breakRun = false;
		}
		else {
			pp_Instance->PostMessage("PTS:" +std::to_string(resultpts) +" DTS:" +std::to_string(resultdts) + " Key: "+std::to_string(isKey));
			pp_Instance->PostMessage("Video Pack - append -" + std::to_string(init));
		}
*/
	}


	if(idx == 3)
	{
		/*
		pp_Instance->PostMessage("Pass - " + std::to_string(init));
		const vector<uint8_t> vuc(static_cast<const uint8_t*>(bin), static_cast<const uint8_t*>(bin) + binlen);
		demuxer_->Parse(vuc);


		std::unique_ptr<ESPacket> es_packet = MakeUnique<ESPacket>();

		double deleno = 1000000;

		int64_t s64 = 0;
		htsmsg_get_s64(msg, "pts", &s64);

		double resultpts = (double)s64 / (double)deleno;
		pp_Instance->PostMessage("PTS:" +std::to_string(resultpts));
		es_packet->pts = Samsung::NaClPlayer::TimeTicks(resultpts);

		int64_t s642 = 0;
		htsmsg_get_s64(msg, "dts", &s642);

		double resultdts = (double)s642 / (double)deleno;
		//pp_Instance->PostMessage("pts / dts:" +std::to_string(resultdts));
		es_packet->dts = Samsung::NaClPlayer::TimeTicks(resultdts);

		uint32_t u32 = 0;
		htsmsg_get_u32(msg, "duration", &u32);
		double resultduration = (double)u32 / (double)deleno;
		//pp_Instance->PostMessage("Durr:" +std::to_string(resultduration));
		es_packet->duration = Samsung::NaClPlayer::TimeTicks(resultduration);

		es_packet->buffer = bin;
		es_packet->size = binlen;

		auto resultapp = elementary_stream_audio->AppendPacket(*es_packet);
		if(resultapp != 0)
		{
			pp_Instance->PostMessage(resultapp);
		}
		else {
			pp_Instance->PostMessage("Audio Pack - append");
		}*/
	}

	Play();

}







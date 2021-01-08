#include "es_htsp_player_controller.h"

#include <functional>
#include <limits>
#include <utility>

#include "ppapi/cpp/var_dictionary.h"
#include "nacl_player/error_codes.h"
#include "nacl_player/es_data_source.h"
#include "nacl_player/url_data_source.h"

#include "common.h"

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


  player_ = make_shared<MediaPlayer>();
  listeners_.player_listener =
      make_shared<MediaPlayerListener>();
  listeners_.buffering_listener =
      make_shared<MediaBufferingListener>(shared_from_this());

  player_->SetMediaEventsListener(listeners_.player_listener);
  player_->SetBufferingListener(listeners_.buffering_listener);
  int32_t ret = player_->SetDisplayRect(view_rect_);

  if (ret != ErrorCodes::Success) {
    //LOG_ERROR("Failed to set display rect [(%d - %d) (%d - %d)], code: %d",
       //view_rect_.x(), view_rect_.y(), view_rect_.width(), view_rect_.height(),
       //ret);
  }

  //InitializeSubtitles(subtitle, encoding);

  player_thread_ = MakeUnique<pp::SimpleThread>(instance_);
  player_thread_->Start();

  //Here init media provider
  //player_thread_->message_loop().PostWork(
  //    cc_factory_.NewCallback(&EsDashPlayerController::InitializeDash,
  //                            mpd_file_path));
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

  //LOG_DEBUG("Set view rect to %d, %d", view_rect_.width(), view_rect_.height());
  auto callback = WeakBind(&EsHtspPlayerController::OnSetDisplayRect,
      std::static_pointer_cast<EsHtspPlayerController>(
          shared_from_this()), _1);
  int32_t ret = player_->SetDisplayRect(view_rect_, callback);
  //if (ret < ErrorCodes::CompletionPending)
    //LOG_ERROR("SetDisplayRect result: %d", ret);
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
  player_->SetDRMListener(nullptr);
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

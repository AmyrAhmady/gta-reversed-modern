#include "StdInc.h"

#include "AECutsceneTrackManager.h"
#include "AEAudioHardware.h"
#include "CutSceneStreamsPC.h"

CAECutsceneTrackManager& AECutsceneTrackManager = *(CAECutsceneTrackManager*)0x8AE554;

void CAECutsceneTrackManager::InjectHooks() {
    using namespace ReversibleHooks;
    Install("CAECutsceneTrackManager", "Initialise", 0x5B9630, &CAECutsceneTrackManager::Initialise);
    Install("CAECutsceneTrackManager", "PlayPreloadedCutsceneTrack", 0x4DBE70, &CAECutsceneTrackManager::PlayPreloadedCutsceneTrack);
    Install("CAECutsceneTrackManager", "StopCutsceneTrack", 0x4DBE80, &CAECutsceneTrackManager::StopCutsceneTrack);
    Install("CAECutsceneTrackManager", "IsCutsceneTrackActive", 0x4DBEB0, &CAECutsceneTrackManager::IsCutsceneTrackActive);
    Install("CAECutsceneTrackManager", "GetCutsceneTrackStatus", 0x4DBEC0, &CAECutsceneTrackManager::GetCutsceneTrackStatus);
    Install("CAECutsceneTrackManager", "PauseTrack", 0x4DBF00, &CAECutsceneTrackManager::PauseTrack);
    Install("CAECutsceneTrackManager", "PreloadBeatTrack", 0x4DC130, &CAECutsceneTrackManager::PreloadBeatTrack);
    Install("CAECutsceneTrackManager", "Service", 0x4DBFB0, &CAECutsceneTrackManager::Service);
}

// 0x5B9630
bool CAECutsceneTrackManager::Initialise(int32 nAudioChannel) {
    m_nAudioChannel = nAudioChannel;
    m_bPlayRequest = false;
    m_bPaused = false;
    m_bIsUserTrack = false;
    _f14 = 0;
    _f18 = 0;
    m_nStatus = S8;
    m_nTrackId = -1;
    _f10 = -1;

    return true;
}

// 0x4DBE70
void CAECutsceneTrackManager::PlayPreloadedCutsceneTrack() {
    m_bPlayRequest = true;
    m_bPaused = false;
}

// 0x4DBE80
void CAECutsceneTrackManager::StopCutsceneTrack() {
    if (m_nStatus <= S3)
        m_nStatus = S4;

    m_bPlayRequest = false;
    m_bPaused = false;
}

// 0x4DBEB0
bool CAECutsceneTrackManager::IsCutsceneTrackActive() const {
    return m_nStatus != S8;
}

// 0x4DBEC0
uint8 CAECutsceneTrackManager::GetCutsceneTrackStatus() const {
    switch (m_nStatus) {
    case S0:
    case S1:
        return S1;
    case S2:
        return S2;
    case S3:
        return S3;
    case S4:
    case S5:
    case S6:
    case S7:
        return S4;
    default:
        return S0;
    }
}

// 0x4DBF00
void CAECutsceneTrackManager::PauseTrack(bool pause) {
    m_bPaused = pause;
}

// 0x4DBFB0
void CAECutsceneTrackManager::Service(int32 trackPlayTime) {
    m_nTrackPlayTime = trackPlayTime;
    static int32 previousPlayTime = trackPlayTime;
    m_nTrackLengthMs = AEAudioHardware.GetTrackLengthMs();
    m_nPlayingTrackId = AEAudioHardware.GetPlayingTrackID();

    switch (m_nStatus) {
    case S0:
        AEAudioHardware.PlayTrack(m_nTrackId, -1, _f14, m_bIsUserTrack, false, false);
        m_nStatus = S1;
        break;
    case S1:
        if (m_nTrackPlayTime == -2) {
            m_nStatus = S2;
        }
        break;
    case S2:
        if (m_bPlayRequest) {
            StartTrackPlayback();
            m_nStatus = S3;
            m_bPlayRequest = false;
        }
        break;
    case S3:
        CheckForPause();
        if (m_nTrackPlayTime == -6) {
            m_nStatus = S8;
        }
        break;
    case S4:
    case S6:
        AEAudioHardware.StopTrack();
        m_nStatus = S7;
        break;
    case S5:
        AEAudioHardware.StopSound(m_nAudioChannel, 0);
        m_nStatus = S6;
        break;
    case S7:
        if (m_nTrackPlayTime == -6) {
            m_nStatus = S8;
        } else if (m_nTrackPlayTime == -2 || m_nTrackPlayTime == -7) {
            AEAudioHardware.StopTrack();
        }
        break;
    default:
        return;
    }
}

// 0x4DBF10
void CAECutsceneTrackManager::StartTrackPlayback() const {
    AEAudioHardware.DisableBassEq();
    AEAudioHardware.SetChannelFlags(m_nAudioChannel, 0, 39);
    AEAudioHardware.SetChannelVolume(m_nAudioChannel, 0, -3.0f, 0);
    AEAudioHardware.StartTrackPlayback();
}

// 0x4DC0E0
void CAECutsceneTrackManager::PreloadCutsceneTrack(int16 trackId, bool wait) {
    m_nTrackId = trackId;
    _f14 = 0;
    m_nStatus = S0;
    if (wait) {
        do {
            auto trackPlayTime = AEAudioHardware.GetTrackPlayTime();
            Service(trackPlayTime);
            AEAudioHardware.Service();
        } while (m_nStatus != S2);
    }
}

// 0x4DC130
void CAECutsceneTrackManager::PreloadBeatTrack(int16 trackId, bool wait) {
    static int16 (&gBeatTrackLookup)[14] = *(int16(*)[14])0x8AE538;

    PreloadCutsceneTrack(gBeatTrackLookup[trackId], wait);
}

// 0x4DBF60
void CAECutsceneTrackManager::CheckForPause() const {
    if (CTimer::GetIsPaused() || m_bPaused) {
        AEAudioHardware.SetChannelFrequencyScalingFactor(m_nAudioChannel, 0, 0.0f);
    } else {
        AEAudioHardware.SetChannelFrequencyScalingFactor(m_nAudioChannel, 0, 1.0f);
    }
}

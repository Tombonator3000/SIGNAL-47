#pragma once
#include <atomic>
#include <cmath>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

// Original research model for Signal 47. Not recovered Greyhill source and not
// connected to Unreal. Animation, geometry, navigation and disk I/O are adapters.
namespace signal47::research {
enum class Phase { Closed, Warning, Opening, Open, Closing };
enum class Outcome { Pass, Waiting, Busy, Denied };
enum class EventKind { Rattle, AnimateOpen, AnimateClose, Opened, Closed, Cancelled, TimedOut };
struct Ticket { std::uint64_t Epoch; std::uint64_t Generation; };
struct Event { EventKind Kind; Ticket Token; std::uint64_t Actor; };
struct Config { double WarningSeconds = 1.2; double ForcedSeconds = 3.0; double MotionTimeout = 4.0; };
struct SavedDoor { std::uint32_t Schema = 1; std::string Id; bool Open = false; bool Locked = false; };

class DoorPolicy {
public:
    explicit DoorPolicy(std::string id, Config config = {}) : Id_(std::move(id)), Config_(config) {
        if (Id_.empty() || !Positive(config.WarningSeconds) || !Positive(config.ForcedSeconds)
            || !Positive(config.MotionTimeout)) throw std::invalid_argument("invalid door configuration");
    }
    DoorPolicy(const DoorPolicy&) = delete;
    DoorPolicy& operator=(const DoorPolicy&) = delete;
    DoorPolicy(DoorPolicy&&) = delete;
    DoorPolicy& operator=(DoorPolicy&&) = delete;
    Phase State() const { return Phase_; }
    bool PassageBlocked() const { return Phase_ != Phase::Open; }
    bool Locked() const { return Locked_; }
    Ticket Token() const { return {Epoch_, Generation_}; }
    std::optional<std::uint64_t> Owner() const { return Owner_; }
    const std::vector<Event>& Events() const { return Events_; }
    void ClearEvents() { Events_.clear(); }

    // A scene may permit forcing this door. It does not grant knowledge of a player.
    Outcome RequestIntrusion(std::uint64_t actor, bool intentStillValid, bool mayForce = false) {
        if (!actor || !intentStillValid) return Outcome::Denied;
        if (Phase_ == Phase::Open) return Outcome::Pass;
        if (Phase_ != Phase::Closed) return Owner_ == actor ? Outcome::Waiting : Outcome::Busy;
        if (Locked_ && !mayForce) return Outcome::Denied;
        ++Generation_;
        Owner_ = actor;
        Phase_ = Phase::Warning;
        Remaining_ = Locked_ ? Config_.ForcedSeconds : Config_.WarningSeconds;
        Emit(EventKind::Rattle);
        return Outcome::Waiting;
    }

    bool SetLocked(bool locked) {
        if (Phase_ == Phase::Open || Phase_ == Phase::Opening || Phase_ == Phase::Closing) return false;
        if (Phase_ == Phase::Warning) Cancel(*Owner_);
        Locked_ = locked;
        return true;
    }

    // The adapter must call this when local evidence/route no longer justifies entry.
    bool Cancel(std::uint64_t actor) {
        if (!Owner_ || *Owner_ != actor) return false;
        const auto previous = Phase_;
        ++Generation_; // invalidate every outstanding animation completion
        Emit(EventKind::Cancelled);
        Owner_.reset();
        if (previous == Phase::Opening) BeginMotion(Phase::Closing);
        else { Phase_ = Phase::Closed; Remaining_ = 0; }
        return true;
    }

    bool Close() {
        if (Phase_ != Phase::Open) return false;
        ++Generation_;
        Owner_.reset();
        BeginMotion(Phase::Closing);
        return true;
    }

    void Advance(double dt, bool paused = false) {
        if (!std::isfinite(dt) || dt < 0) throw std::invalid_argument("invalid delta time");
        if (paused || Phase_ == Phase::Closed || Phase_ == Phase::Open) return;
        Remaining_ -= dt;
        if (Remaining_ > 0) return;
        if (Phase_ == Phase::Warning) {
            Locked_ = false;
            BeginMotion(Phase::Opening);
        } else {
            // Unknown physical state: remain blocking and request a close animation.
            // Only a matching completion can confirm that the door is closed.
            ++Generation_;
            Emit(EventKind::TimedOut);
            Owner_.reset();
            BeginMotion(Phase::Closing);
        }
    }

    // Host confirms the physical mesh reached its endpoint. A timer alone never
    // makes an actor pass through a door; stale or duplicate completions are ignored.
    bool MotionComplete(Ticket ticket, bool paused = false) {
        // Host retains/replays completion after pause, or freezes animation itself.
        if (paused || ticket.Epoch != Epoch_ || ticket.Generation != Generation_
            || (Phase_ != Phase::Opening && Phase_ != Phase::Closing)) return false;
        const bool opened = Phase_ == Phase::Opening;
        Phase_ = opened ? Phase::Open : Phase::Closed;
        Remaining_ = 0;
        Emit(opened ? EventKind::Opened : EventKind::Closed);
        Owner_.reset();
        return true;
    }

    SavedDoor Capture() const {
        // Deliberate stable checkpoint policy: transient motion/warning loads closed.
        return {1, Id_, Phase_ == Phase::Open, Locked_};
    }
    void Restore(const SavedDoor& saved) {
        if (saved.Schema != 1 || saved.Id != Id_ || (saved.Open && saved.Locked))
            throw std::invalid_argument("incompatible door snapshot");
        ++Generation_;
        Phase_ = saved.Open ? Phase::Open : Phase::Closed;
        Locked_ = saved.Locked;
        Remaining_ = 0;
        Owner_.reset();
        Events_.clear(); // restoring must not replay sound or intrusion events
    }

private:
    static bool Positive(double x) { return std::isfinite(x) && x > 0; }
    void Emit(EventKind kind) { Events_.push_back({kind, Token(), Owner_.value_or(0)}); }
    void BeginMotion(Phase next) {
        Phase_ = next;
        Remaining_ = Config_.MotionTimeout;
        Emit(next == Phase::Opening ? EventKind::AnimateOpen : EventKind::AnimateClose);
    }
    std::string Id_;
    Config Config_;
    Phase Phase_ = Phase::Closed;
    bool Locked_ = false;
    double Remaining_ = 0;
    std::uint64_t Generation_ = 0;
    inline static std::atomic<std::uint64_t> NextEpoch_{0};
    const std::uint64_t Epoch_ = ++NextEpoch_;
    std::optional<std::uint64_t> Owner_;
    std::vector<Event> Events_;
};

// UI acknowledgement is driven by actual disk success, not by calling save.
enum class SaveNotice { CheckpointSaved, SaveFailed };
inline SaveNotice NoticeAfterDiskWrite(bool succeeded) {
    return succeeded ? SaveNotice::CheckpointSaved : SaveNotice::SaveFailed;
}
} // namespace signal47::research

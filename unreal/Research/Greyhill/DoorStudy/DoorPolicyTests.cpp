#include "DoorPolicy.hpp"
#include <functional>
#include <iostream>
#include <limits>
#include <stdexcept>
using namespace signal47::research;
void Check(bool ok, const char* message) { if (!ok) throw std::runtime_error(message); }
template<class F> void Rejects(F f) {
    bool rejected = false;
    try { f(); } catch (const std::invalid_argument&) { rejected = true; }
    Check(rejected, "expected invalid input to be rejected");
}
int main() {
    const std::vector<std::pair<const char*, std::function<void()>>> cases = {
        {"ordinary intrusion requires physical completion", [] {
            DoorPolicy d("annex-west");
            Check(d.RequestIntrusion(1, true) == Outcome::Waiting, "warning");
            d.Advance(1.2);
            Check(d.State() == Phase::Opening && d.PassageBlocked(), "still blocking during opening");
            Check(d.MotionComplete(d.Token()) && !d.PassageBlocked(), "physical endpoint releases passage");
        }},
        {"paused warning does not elapse", [] {
            DoorPolicy d("a"); d.RequestIntrusion(1, true); d.Advance(100, true);
            Check(d.State() == Phase::Warning, "pause consumed timer");
        }},
        {"duplicate request does not restart warning or replay sound", [] {
            DoorPolicy d("a"); d.RequestIntrusion(1, true); d.Advance(1);
            Check(d.RequestIntrusion(1, true) == Outcome::Waiting, "same actor");
            Check(d.Events().size() == 1, "duplicate rattle"); d.Advance(.21);
            Check(d.State() == Phase::Opening, "timer restarted");
        }},
        {"second actor cannot steal an active door", [] {
            DoorPolicy d("a"); d.RequestIntrusion(1, true);
            Check(d.RequestIntrusion(2, true) == Outcome::Busy && d.Owner() == 1, "ownership stolen");
            Check(!d.Cancel(2) && d.State() == Phase::Warning, "foreign cancellation");
        }},
        {"locked door requires explicit scene permission", [] {
            DoorPolicy d("a"); Check(d.SetLocked(true), "lock");
            Check(d.RequestIntrusion(1, true) == Outcome::Denied, "locked door ignored");
            Check(d.RequestIntrusion(1, true, true) == Outcome::Waiting, "force permitted");
            d.Advance(1.3); Check(d.State() == Phase::Warning, "force warning too short");
            d.Advance(1.71); Check(d.State() == Phase::Opening && !d.Locked(), "force transition");
        }},
        {"lost route cancels warning", [] {
            DoorPolicy d("a"); d.RequestIntrusion(1, true); Check(d.Cancel(1), "cancel");
            d.Advance(10); Check(d.State() == Phase::Closed && d.PassageBlocked(), "cancelled action fired");
        }},
        {"old animation completion cannot open a cancelled door", [] {
            DoorPolicy d("a"); d.RequestIntrusion(1, true); d.Advance(2);
            const auto old = d.Token(); d.Cancel(1);
            Check(d.State() == Phase::Closing && !d.MotionComplete(old), "stale callback accepted");
            Check(d.MotionComplete(d.Token()) && d.State() == Phase::Closed, "close endpoint");
        }},
        {"closing blocks passage immediately and rejects duplicates", [] {
            DoorPolicy d("a"); d.Restore({1, "a", true, false});
            Check(d.Close() && d.PassageBlocked(), "closing released passage");
            const auto ticket = d.Token(); Check(d.MotionComplete(ticket), "complete close");
            Check(!d.MotionComplete(ticket), "duplicate callback accepted");
        }},
        {"motion timeout never makes door passable", [] {
            DoorPolicy d("a"); d.RequestIntrusion(1, true); d.Advance(2); d.Advance(5);
            Check(d.State() == Phase::Closing && d.PassageBlocked(), "timeout teleported past obstacle");
            Check(!d.Owner(), "timeout retained owner");
        }},
        {"loading an interrupted interaction uses a stable checkpoint", [] {
            DoorPolicy d("a"); d.RequestIntrusion(1, true); d.Advance(2);
            const auto ticket = d.Token(); auto save = d.Capture(); d.Restore(save);
            Check(d.State() == Phase::Closed && !d.Owner() && d.Events().empty(), "transient state replayed");
            Check(!d.MotionComplete(ticket), "preload callback accepted");
        }},
        {"stable open and locked closed states survive restore", [] {
            DoorPolicy d("a"); d.Restore({1, "a", true, false});
            DoorPolicy loaded("a"); loaded.Restore(d.Capture()); Check(!loaded.PassageBlocked(), "open lost");
            d.Close(); d.MotionComplete(d.Token()); d.SetLocked(true); loaded.Restore(d.Capture());
            Check(loaded.Locked() && loaded.PassageBlocked(), "closed lock lost");
        }},
        {"snapshot identity and schema are validated", [] {
            DoorPolicy d("a"); Rejects([&] {d.Restore({1, "b", false, false});});
            Rejects([&] {d.Restore({2, "a", false, false});});
            Rejects([&] {d.Restore({1, "a", true, true});});
        }},
        {"invalid requests and timing fail explicitly", [] {
            DoorPolicy d("a"); Check(d.RequestIntrusion(0, true) == Outcome::Denied, "zero identity");
            Check(d.RequestIntrusion(1, false) == Outcome::Denied, "no local intent");
            Rejects([&] {d.Advance(-1);}); Rejects([&] {d.Advance(std::numeric_limits<double>::infinity());});
            Rejects([] {DoorPolicy invalid("a", {0, 1, 1});});
        }},
        {"completion is deferred during pause", [] {
            DoorPolicy d("a"); d.RequestIntrusion(1, true); d.Advance(2);
            auto ticket=d.Token(); Check(!d.MotionComplete(ticket,true), "completion during pause");
            Check(d.PassageBlocked() && d.MotionComplete(ticket,false), "completion not resumed");
        }},
        {"a previous object lifetime cannot finish a new motion", [] {
            DoorPolicy oldDoor("a"); oldDoor.RequestIntrusion(1,true); oldDoor.Advance(2);
            auto old=oldDoor.Token(); DoorPolicy newDoor("a"); newDoor.RequestIntrusion(1,true); newDoor.Advance(2);
            Check(!newDoor.MotionComplete(old) && newDoor.PassageBlocked(), "cross-instance ticket accepted");
        }},
        {"checkpoint acknowledgement reflects disk failure", [] {
            Check(NoticeAfterDiskWrite(false) == SaveNotice::SaveFailed, "false success acknowledgement");
            Check(NoticeAfterDiskWrite(true) == SaveNotice::CheckpointSaved, "lost success acknowledgement");
        }}
    };
    int passed = 0;
    for (const auto& [name, run] : cases) {
        try { run(); ++passed; std::cout << "PASS " << name << '\n'; }
        catch (const std::exception& e) { std::cerr << "FAIL " << name << ": " << e.what() << '\n'; return 1; }
    }
    std::cout << passed << "/" << cases.size() << " reference-model scenarios passed\n";
}

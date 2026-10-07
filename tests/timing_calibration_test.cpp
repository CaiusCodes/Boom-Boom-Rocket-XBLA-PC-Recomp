#include "src/timing_calibration.h"
#include <bit>
#include <iostream>
#include <limits>
#include <stdexcept>

static void Check(bool value,const char* label) {
  if(!value) throw std::runtime_error(label);
  std::cout << "PASS: " << label << '\n';
}
int main() {
  try {
    bbr::TimingCalibration calibration;
    for (int i=0;i<41;++i)
      Check(calibration.Index(calibration.Value(i))==i,"all native spinner values round-trip");
    Check(calibration.Value(-1)==-200 && calibration.Value(42)==200,"spinner index bounds");
    Check(calibration.Normalize(999)==200 && calibration.Normalize(-999)==-200,"config bounds");
    Check(calibration.Normalize(16)==20 && calibration.Normalize(-16)==-20,"signed 10ms steps");
    calibration.Begin(0);
    for (double value : {0.0,-0.0,1.0,1.0/3,1234.56789,std::numeric_limits<double>::infinity()})
      Check(std::bit_cast<uint64_t>(value)==std::bit_cast<uint64_t>(calibration.Adjust(value)),
            "zero offset preserves original timing bits");
    calibration.Begin(100);
    Check(calibration.Adjust(10)==double(float(9.9)),"positive offset delays judgement clock");
    Check(calibration.Adjust(0)==double(float(-0.1)),"early-song time is not clipped");
    Check(calibration.Adjust(std::numeric_limits<double>::infinity())==std::numeric_limits<double>::infinity(),
          "nonfinite native value preserved");
    calibration.Begin(-100);
    Check(calibration.Adjust(10)==double(float(10.1)),"negative offset advances judgement clock");
    // The real engine selects/grads against a two-frame interval and misses
    // against an absolute note deadline. All use the same adjusted history.
    auto grade_distance=[](double note,double previous,double now) {
      return note<std::min(previous,now) ? std::min(previous,now)-note
        : note>std::max(previous,now) ? note-std::max(previous,now) : 0.;
    };
    for (int offset : {-200,-100,0,100,200}) {
      calibration.Begin(offset);
      const double note=10., hit=note+offset/1000.;
      Check(grade_distance(note,calibration.Adjust(hit-.01),calibration.Adjust(hit+.01))==0,
            "accepted grading window moves by signed offset");
      Check(calibration.Adjust(hit-.02)<note && calibration.Adjust(hit+.02)>note,
            "miss deadline moves with the same clock");
      Check(calibration.session_ms==offset,"session value remains latched until next begin");
    }
  } catch(const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}

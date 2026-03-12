#include "Rtc.h"

extern "C" {
    bool RtcGetTime2C(TTime* time) {
        if (time == nullptr) return false;
        return RtcGetTime(*time);
    }
    

    bool RtcSetTime2C(const TTime* time) {
        if (time == nullptr) return false;
        return RtcSetTime(*time);
    }
}

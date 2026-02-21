//
// Created by amirf on 2/20/2026.
//

#include "safetynet.h"
namespace core {
    bool safetynet_check(safety_net& safetynet) {
        safetynet.curr_ops++;
        if (safetynet.curr_ops > safetynet.max_ops || safetynet.infinite_loop_detected) {
            safetynet.infinite_loop_detected = true;
            return false;
        }
        return true;
    }
    void safetynet_reset(safety_net& safetynet) {
        safetynet.infinite_loop_detected = false;
        safetynet.curr_ops = 0;
    }
}

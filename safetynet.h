//
// Created by amirf on 2/20/2026.
//

#ifndef FOP_PROJECT_G66_SAFETYNET_H
#define FOP_PROJECT_G66_SAFETYNET_H
namespace core {
    struct safety_net {
        int max_ops = 1000;
        int curr_ops = 0;
        bool infinite_loop_detected = false;
    };

    bool safetynet_check(safety_net& safetynet);
    void safetynet_reset(safety_net& safetynet);
}
#endif //FOP_PROJECT_G66_SAFETYNET_H

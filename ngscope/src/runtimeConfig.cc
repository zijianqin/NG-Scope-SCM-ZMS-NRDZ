#include "../hdr/runtimeConfig.h"

// void ngscope_config_t::puschGrantPush(puschGrant_t puschGrant) {
//     // std::lock_guard<std::mutex> lock(ngscope_config_mutex);
//     puschGrantVec.push_back(puschGrant);
//     std::cout << "Push back grant!" << std::endl;
// }


// puschGrant_t ngscope_config_t::puschGrantPop(uint32_t curtti) {
//     // std::lock_guard<std::mutex> lock(ngscope_config_mutex);
//     if (puschGrantVec.empty()) {
//         puschGrant_t emptyGrant = {.pusch_grant = {}, .target_tti = 20000};
//         return emptyGrant;
//     }
//     for (auto it = puschGrantVec.begin(); it != puschGrantVec.end(); it++) {
//         if ((it->target_tti + 10) % 10240 < curtti) {
//             puschGrantVec.erase(it);
//         }
//         if (it->target_tti == curtti) {
//             std::cout << "Got grant!" << std::endl;
//             return *it;
//         }
//     }
// }


asn1::rrc::sib_type2_s ngscope_config_t::getSib2() {
    std::lock_guard<std::mutex> lock(ngscope_config_mutex);
    return sib2;
}

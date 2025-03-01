#include "../hdr/ueDatabase.h"

void tempRNTI::push_tcRNTI(uint16_t t_crnti) {
    std::lock_guard<std::mutex> lock(tcrntiMutex);
    tcrntiVec.push_back(t_crnti);
}


std::vector<uint16_t> tempRNTI::get_tcRNTI() {
    std::lock_guard<std::mutex> lock(tcrntiMutex);
    return tcrntiVec;
}


void tempRNTI::delete_tcRNTI(uint16_t t_crnti) {
    std::lock_guard<std::mutex> lock(tcrntiMutex);
    for (std::vector<uint16_t>::iterator iter = tcrntiVec.begin(); iter != tcrntiVec.end(); iter++) {
        if (*iter == t_crnti) {
            tcrntiVec.erase(iter);
            std::cout << "Delete TC-RNTI: " << t_crnti << std::endl;
            break;
        }
    }
}


void ueDatabase::pushUE(uint16_t rnti) {
    std::lock_guard<std::mutex> lock(ueDatabaseMutex);
    auto it = ueTracker.find(rnti);
    if (it != ueTracker.end()) {
        it ->second = 600000;
    } else {
        ueTracker.insert(std::pair<uint16_t, int>(rnti, 600000));
    }
}


void ueDatabase::set_cfg_ded(srsran_ue_dl_cfg_t ue_dl_cfg, srsran_ue_ul_cfg_t ue_ul_cfg) {
    std::lock_guard<std::mutex> lock(ueDatabaseMutex);
    if (have_cfg) {

    } else {
        ue_dl_cfg_ded = ue_dl_cfg;
        ue_ul_cfg_ded = ue_ul_cfg;
        have_cfg = true;
    }
}


bool ueDatabase::get_cfg_ded(srsran_ue_dl_cfg_t* ue_dl_cfg, srsran_ue_ul_cfg_t* ue_ul_cfg) {
    std::lock_guard<std::mutex> lock(ueDatabaseMutex);
    if (!have_cfg) {
        return false;
    } else {
        *ue_dl_cfg = ue_dl_cfg_ded;
        *ue_ul_cfg = ue_ul_cfg_ded;
        return true;
    }
}


bool ueDatabase::updateUE(uint16_t rnti, bool have_dci) {
    std::lock_guard<std::mutex> lock(ueDatabaseMutex);
    auto it = ueTracker.find(rnti);
    if (it != ueTracker.end()) {
        if (have_dci) {
            it->second = 600000;
        } else {
            it->second = it->second - 1;
            if (it->second == 0) {
                ueTracker.erase(rnti);
            }
        }
        return true;
    } else {
        return false;
    }
}


bool ueDatabase::have_cfg_ded() {
    std::lock_guard<std::mutex> lock(ueDatabaseMutex);
    return have_cfg;
}


std::map<uint16_t, int> ueDatabase::getUETrack() {
    std::lock_guard<std::mutex> lock(ueDatabaseMutex);
    return ueTracker;
}
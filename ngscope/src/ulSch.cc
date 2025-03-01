#include "../hdr/ulSch.h"

void ULSch::pushULDCI(uint32_t tti, int tdd_config, uint16_t rnti, srsran_dci_ul_t dci_ul) {
	std::lock_guard<std::mutex> lock(ulschMutex);
	uint32_t tartti = getTargetTti(tti, tdd_config);
	ulDCIDatabase[tartti].push_back(std::make_pair(rnti, dci_ul));
}


void ULSch::pushULDCI(uint32_t tti, int tdd_config, uint16_t rnti, srsran_pusch_grant_t pusch_grant, srsran_dci_ul_t dci_ul, int nof_ack) {
      std::lock_guard<std::mutex> lock(ulschMutex);
      uint32_t tartti = getTargetTti(tti, tdd_config);
      DCIUL dciul = {.pusch_grant = pusch_grant,
                     .dci_ul = dci_ul,
                     .nof_ack = nof_ack};
      
      ulDCImap[tartti].push_back(std::make_pair(rnti, dciul));
}


uint32_t ULSch::getTargetTti(uint32_t curtti, int tdd_config) {
	int sf_idx = curtti % 10;
	if (tdd_config == -1) {
		return (curtti + 4) % 10240;
	} else {//TDD mode
        if (tdd_config == 0) {
            if (sf_idx == 0) {
                  return (curtti + 4) % 10240;
            } else if (sf_idx == 1) {
                  return (curtti + 6) % 10240;
            } else if (sf_idx == 5) {
                  return (curtti + 4) % 10240;
            } else if (sf_idx == 6) {
                  return (curtti + 6) % 10240;
            }
        } else if (tdd_config == 1) {
            if (sf_idx == 1) {
                  return (curtti + 6) % 10240;
            } else if (sf_idx == 4) {
                  return (curtti + 4) % 10240;
            } else if (sf_idx == 6) {
                  return (curtti + 6) % 10240;
            } else if (sf_idx == 9) {
                  return (curtti + 4) % 10240;
            } 
        } else if (tdd_config == 2) {
            if (sf_idx == 3) {
                  return (curtti + 4) % 10240;
            } else if (sf_idx == 8) {
                  return (curtti + 4) % 10240;
            } 
        } else {
            ERROR("Error tdd config!");
        }
    }
}


uint32_t ULSch::getTargetTtiAck(uint32_t curtti, int tdd_config) {
	int sf_idx = curtti % 10;
      if (tdd_config == -1) {
		return (curtti + 4) % 10240;
	} else { // TDD mode
        if (tdd_config == 0) {
            if (sf_idx == 0) {
                  return (curtti + 4) % 10240;
            } else if (sf_idx == 1) {
                  return (curtti + 6) % 10240;
            } else if (sf_idx == 5) {
                  return (curtti + 4) % 10240;
            } else if (sf_idx == 6) {
                  return (curtti + 6) % 10240;
            }
        } else if (tdd_config == 1) {
            if (sf_idx == 0) {
                  return (curtti + 7) % 10240;
            } else if (sf_idx == 1) {
                  return (curtti + 6) % 10240;
            } else if (sf_idx == 4) {
                  return (curtti + 4) % 10240;
            } else if (sf_idx == 5) {
                  return (curtti + 7) % 10240;
            } else if (sf_idx == 6) {
                  return (curtti + 6) % 10240;
            } else if (sf_idx == 9) {
                  return (curtti + 4) % 10240;
            } 
        } else if (tdd_config == 2) {
            if (sf_idx == 0) {
                  return (curtti + 7) % 10240;
            } else if (sf_idx == 1) {
                  return (curtti + 6) % 10240;
            } else if (sf_idx == 3) {
                  return (curtti + 4) % 10240;
            } else if (sf_idx == 4) {
                  return (curtti + 8) % 10240;
            } else if (sf_idx == 5) {
                  return (curtti + 7) % 10240;
            } else if (sf_idx == 6) {
                  return (curtti + 6) % 10240;
            } else if (sf_idx == 8) {
                  return (curtti + 4) % 10240;
            } else if (sf_idx == 9) {
                  return (curtti + 8) % 10240;
            } 
        } else {
            ERROR("Error tdd config!");
        }
      }
}


std::vector<std::pair<uint16_t, srsran_dci_ul_t>> ULSch::getUlDCIVec(uint32_t tti) {
	std::lock_guard<std::mutex> lock(ulschMutex);
	auto it = ulDCIDatabase.find(tti);
	if (it != ulDCIDatabase.end()) {
		return it->second;
	} else {
		return { };
	}
}


std::vector<std::pair<uint16_t, DCIUL>> ULSch::getULDCIstruct(uint32_t tti) {
      std::lock_guard<std::mutex> lock(ulschMutex);
      auto it = ulDCImap.find(tti);
      if (it != ulDCImap.end()) {
            return it->second;
      } else {
            return { };
      }
}


void ULSch::deleteDciUL(uint32_t tti) {
      std::lock_guard<std::mutex> lock(ulschMutex);
      auto it = ulDCImap.find(tti);
      if (it != ulDCImap.end()) {
            ulDCImap.erase(it);
      } else {

      }
}


void ULSch::addUlAckNum(uint32_t tti, int tdd_config, uint16_t rnti) {
      std::lock_guard<std::mutex> lock(ulAckMutex);
      int tartti = getTargetTtiAck(tti, tdd_config);
      ulACKmap[tartti].push_back(rnti);
}


int ULSch::getUlAckNum(uint32_t tti, uint16_t rnti) {
      std::lock_guard<std::mutex> lock(ulAckMutex);
      auto it = ulACKmap.find(tti);
      if (it != ulACKmap.end()) {
            return std::count(it->second.begin(), it->second.end(), rnti);
      } else {
            return 0;
      }
}
    
    
void ULSch::deleteUlAckNum(uint32_t tti) {
      std::lock_guard<std::mutex> lock(ulAckMutex);
      auto it = ulACKmap.find(tti);
      if (it != ulACKmap.end()) {
            ulACKmap.erase(it);
      }
}
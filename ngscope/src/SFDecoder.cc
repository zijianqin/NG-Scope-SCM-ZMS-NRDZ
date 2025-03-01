#include "../hdr/SFDecoder.h"

int ul_cqi_hl_get_subband_size(int nof_prb){
  if (nof_prb < 7) {
    ERROR("Error: nof_prb is invalid (< 7)");
    return SRSRAN_ERROR;
  } else if (nof_prb <= 26) {
    return 4;
  } else if (nof_prb <= 63) {
    return 6;
  } else if (nof_prb <= 110) {
    return 8;
  } else {
    ERROR("Error: nof_prb is invalid (> 110)");
    return SRSRAN_ERROR;
  }

}

int ul_sniffer_cqi_hl_get_no_subbands(int nof_prb)
{
  int hl_size = ul_cqi_hl_get_subband_size(nof_prb);
  if (hl_size > 0) {
    return (int)ceil((float)nof_prb / hl_size);
  } else {
    return 0;
  }
}


static const int tbs_table_32A[110] = 
    /* Implementation of Index 32A for uplink pusch, table Table 7.1.7.2.1-1 36.213 v15 */
    {904 , 1864 , 2792 , 3752 , 4584 , 5544 , 6456 , 7480 , 8248 , 9144 , 10296, 11064, 12216, 12960,
    14112, 14688, 15840, 16416, 17568, 18336, 19848, 20616, 21384, 22152, 22920, 24496, 25456,
    26416, 27376, 27376, 28336, 29296, 30576, 31704, 32856, 32856, 34008, 35160, 36696, 36696, 
    37888, 39232, 40576, 40576, 42368, 42368, 43816, 43816, 45352, 46888, 46888, 48936, 48936,
    51024, 51024, 52752, 52752, 52752, 55056, 55056, 57336, 57336, 59256, 59256, 59256, 61664,
    61664, 63776, 63776, 63776, 66592, 66592, 68808, 68808, 68808, 71112, 71112, 73712, 73712,
    73712, 75376, 76208, 76208, 78704, 78704, 78704, 81176, 81176, 81176, 84760, 84760, 84760,
    87936, 87936, 87936, 87936, 90816, 90816, 90816, 93800, 93800, 93800, 93800, 97896, 97896,
    97896, 97896, 101840, 101840, 101840};


int SFDecoder::run_ul_mode(SFWorker* curworker) {
    int ret = SRSRAN_ERROR;
    srsran_ue_dl_t* ue_dl = &curworker->ue_dl;
    srsran_ue_ul_t* ue_ul = &curworker->ue_ul;
    srsran_ue_dl_cfg_t* ue_dl_cfg = &curworker->ue_dl_cfg;
    srsran_dl_sf_cfg_t* dl_sf_cfg = &curworker->dl_sf_cfg;
    srsran_ul_sf_cfg_t* ul_sf_cfg = &curworker->ul_sf_cfg;
    cf_t** sf_buffer = curworker->sf_buffer;
    // srsran_ul_cfg_t* ul_cfg = &curworker->ul_cfg;
    srsran_ue_ul_cfg_t* ue_ul_cfg = &curworker->ue_ul_cfg;
    srsran_enb_ul_t* enb_ul = &curworker->enb_ul;
    srsran_cell_t cell = curworker->cell;
    // uint16_t rnti = ue_dl_cfg->cfg.pdsch.rnti;
    srsran_ul_cfg_t* ul_cfg = &ue_ul_cfg->ul_cfg;

    if (!curworker->isConfigured()) {
        asn1::rrc::sib_type1_s sib1 = sibs->getSIB1();
        asn1::rrc::sib_type2_s sib2 = sibs->getSIB2();
        asn1::rrc::rr_cfg_common_sib_s rrc_config = sib2.rr_cfg_common;

        if (curworker->cell.frame_type == SRSRAN_TDD) {
            dl_sf_cfg->tdd_config.sf_config = sib1.tdd_cfg.sf_assign.to_number();
            dl_sf_cfg->tdd_config.ss_config = sib1.tdd_cfg.special_sf_patterns.to_number();
            dl_sf_cfg->tdd_config.configured = true;
        } else {
            dl_sf_cfg->tdd_config.configured = false;
        }

        ul_cfg->dmrs.cyclic_shift = rrc_config.pusch_cfg_common.ul_ref_sigs_pusch.cyclic_shift;
        ul_cfg->dmrs.group_hopping_en = rrc_config.pusch_cfg_common.ul_ref_sigs_pusch.group_hop_enabled;
        ul_cfg->dmrs.sequence_hopping_en = rrc_config.pusch_cfg_common.ul_ref_sigs_pusch.seq_hop_enabled;
        ul_cfg->dmrs.delta_ss = rrc_config.pusch_cfg_common.ul_ref_sigs_pusch.group_assign_pusch;

        ul_cfg->hopping.hop_mode = srsran_pusch_hopping_cfg_t::SRSRAN_PUSCH_HOP_MODE_INTER_SF;
        ul_cfg->hopping.hopping_offset = rrc_config.pusch_cfg_common.pusch_cfg_basic.pusch_hop_offset;
        ul_cfg->hopping.n_sb = rrc_config.pusch_cfg_common.pusch_cfg_basic.n_sb;
        ul_cfg->hopping.n_rb_ho = rrc_config.pusch_cfg_common.pusch_cfg_basic.pusch_hop_offset;

        ul_cfg->pucch.delta_pucch_shift = rrc_config.pucch_cfg_common.delta_pucch_shift;
        ul_cfg->pucch.N_cs = rrc_config.pucch_cfg_common.ncs_an;
        ul_cfg->pucch.n_rb_2 = rrc_config.pucch_cfg_common.nrb_cqi;
        ul_cfg->pucch.N_pucch_1 = rrc_config.pucch_cfg_common.n1_pucch_an;
        ul_cfg->power_ctrl.delta_f_pucch[0] = sib2.rr_cfg_common.ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format1.to_number();
        ul_cfg->power_ctrl.delta_f_pucch[1] = sib2.rr_cfg_common.ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format1b.to_number();
        ul_cfg->power_ctrl.delta_f_pucch[2] = sib2.rr_cfg_common.ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format2.to_number();
        ul_cfg->power_ctrl.delta_f_pucch[3] = sib2.rr_cfg_common.ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format2a.to_number();
        ul_cfg->power_ctrl.delta_f_pucch[4] = sib2.rr_cfg_common.ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format2b.to_number();

        ul_cfg->pusch.enable_64qam = rrc_config.pusch_cfg_common.pusch_cfg_basic.enable64_qam;

        if (srsran_enb_ul_set_cell(enb_ul, cell, &ul_cfg->dmrs, &ul_cfg->srs)) {
            ERROR("Error initiating enb uplink cell");
        }

        curworker->setConfigured();
        std::cout << "Finish worker config!" << std::endl;
    }

    dl_sf_cfg->tti = tti;
    ul_sf_cfg->tti = tti;
    ul_sf_cfg->tdd_config = dl_sf_cfg->tdd_config;
    ul_sf_cfg->shortened = false;
    // ue_ul_cfg->ul_cfg.pucch.rnti = rnti;
    // ue_dl_cfg->cfg.tm = (srsran_tm_t)3;
    // enb_ul->pusch.ue_rnti = rnti;
    std::vector<srsran_pusch_grant_t> *puschGrantObtained = nullptr;
    srsran_dci_ul_t* puschDciObtained = nullptr;
    uint8_t* data[SRSRAN_MAX_CODEWORDS];
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        data[i] = srsran_vec_u8_malloc(2000 * 8);
    }
    bool acks[SRSRAN_MAX_CODEWORDS] = {false};
    uint16_t rnti = ue_dl_cfg->cfg.pdsch.rnti;
    srsran_pucch_res_t pucch_res = { };
    srsran_ue_dl_cfg_t* ue_dl_cfg_temp = nullptr;
    srsran_ue_ul_cfg_t* ue_ul_cfg_temp = nullptr;
    srsran_ue_dl_cfg_t* ue_dl_cfg_ded = &curworker->ue_dl_cfg_ded;
    srsran_ue_ul_cfg_t* ue_ul_cfg_ded = &curworker->ue_ul_cfg_ded;
    if (rnti == 0) {
        int retRach = sfDecodeRach(ue_dl, dl_sf_cfg, ue_dl_cfg, ue_ul_cfg);
        if (!curworker->have_cfg_ded) {
            if (!ueDataBase->get_cfg_ded(ue_dl_cfg_ded, ue_ul_cfg_ded)) {
                return 0;
            }
        }
    } else {
        if (!curworker->have_cfg_ded) {
            *ue_dl_cfg_ded = *ue_dl_cfg;
            *ue_ul_cfg_ded = *ue_ul_cfg;
            ue_ul_cfg_ded->ul_cfg.pusch.uci_offset.I_offset_ack = 5;
            ue_ul_cfg_ded->ul_cfg.pusch.uci_offset.I_offset_cqi = 6;
            ue_ul_cfg_ded->ul_cfg.pusch.uci_offset.I_offset_ri = 6;
            curworker->have_cfg_ded = true;
        }
    }

    if (curworker->cell.frame_type == SRSRAN_TDD) {
        if (srsran_sfidx_tdd_type(dl_sf_cfg->tdd_config, tti % 10) != SRSRAN_TDD_SF_U) {
            ret = getPuschGrantMultiUE(ue_dl, ue_ul, dl_sf_cfg, ul_sf_cfg, ue_dl_cfg_ded, ue_ul_cfg_ded);
        } else {
            ret = sfDecodePuschMultiUE(ue_ul, ue_ul_cfg_ded, enb_ul, ul_sf_cfg);
        }
    } else {
        ret = getPuschGrantMultiUE(ue_dl, ue_ul, dl_sf_cfg, ul_sf_cfg, ue_dl_cfg_ded, ue_ul_cfg_ded);
        ret = sfDecodePuschMultiUE(ue_ul, ue_ul_cfg_ded, enb_ul, ul_sf_cfg);
    }

    return ret;
}


int SFDecoder::getPuschGrant(srsran_ue_dl_t* ue_dl, srsran_ue_ul_t* ue_ul, srsran_dl_sf_cfg_t* dl_sf_cfg, srsran_ul_sf_cfg_t* ul_sf_cfg, srsran_ue_dl_cfg_t* ue_dl_cfg, srsran_ue_ul_cfg_t* ue_ul_cfg, srsran_dci_ul_t* dci_ul_ret) {
    int ret = SRSRAN_ERROR;
    int ret_dl = SRSRAN_ERROR;
    uint16_t rnti = ue_dl_cfg->cfg.pdsch.rnti;
    srsran_dci_dl_t dci_dl[SRSRAN_MAX_DCI_MSG];
    ZERO_OBJECT(dci_dl);
    srsran_dci_ul_t dci_ul[SRSRAN_MAX_DCI_MSG];
    ZERO_OBJECT(dci_ul);
    srsran_pusch_cfg_t* pusch_cfg = &ue_ul_cfg->ul_cfg.pusch;
    srsran_pdsch_res_t pdsch_res[SRSRAN_MAX_CODEWORDS];
    ZERO_OBJECT(pdsch_res);
    // std::cout << "pdsch tm mode: " << ue_dl_cfg->cfg.tm << std::endl;

    srsran_softbuffer_rx_t* rx_softbuffers;
    rx_softbuffers = new srsran_softbuffer_rx_t[SRSRAN_MAX_CODEWORDS];
    for (uint32_t i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        ue_dl_cfg->cfg.pdsch.softbuffers.rx[i] = &rx_softbuffers[i];
        srsran_softbuffer_rx_init(ue_dl_cfg->cfg.pdsch.softbuffers.rx[i], ue_dl->cell.nof_prb);
    }

    srsran_softbuffer_rx_t* rx_ul_softbuffers;
    rx_ul_softbuffers = new srsran_softbuffer_rx_t;
    ue_ul_cfg->ul_cfg.pusch.softbuffers.rx = rx_ul_softbuffers;
    srsran_softbuffer_rx_init(ue_ul_cfg->ul_cfg.pusch.softbuffers.rx, SRSRAN_MAX_PRB);

    uint8_t* data[SRSRAN_MAX_CODEWORDS];
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        data[i] = srsran_vec_u8_malloc(2000 * 8);
    }
    bool acks[SRSRAN_MAX_CODEWORDS] = {false};

    srsran_ue_dl_set_mi_auto(ue_dl);
    if (srsran_ue_dl_decode_fft_estimate(ue_dl, dl_sf_cfg, ue_dl_cfg) < 0) {
        return SRSRAN_ERROR;
    }

    ret_dl = srsran_ue_dl_find_dl_dci(ue_dl, dl_sf_cfg, ue_dl_cfg, rnti, dci_dl);
    if (ret_dl > 0) {
        char str[512];
            srsran_dci_dl_info(&dci_dl[0], str, 512);
            INFO("PDCCH DL: %s", str);
            printf("tddconfig: %d, SF: %d, RNTI = %d, PDCCH DL: %s\n", dl_sf_cfg->tdd_config.sf_config, tti % 10, rnti, str);
    }

    if (ret_dl < 0) {
       return ret;
    }

    ret = srsran_ue_dl_find_ul_dci(ue_dl, dl_sf_cfg, ue_dl_cfg, rnti, dci_ul);
    // std::cout << "ul ret = " << ret << std::endl;
    if (ret > 0) {
        *dci_ul_ret = dci_ul[0];
        for (int dci_idx = 0; dci_idx < ret; dci_idx++) {
            char str[512];
            srsran_dci_ul_info(&dci_ul[dci_idx], str, 512);
            INFO("PDCCH UL: %s", str);
            // printf("tddconfig: %d, SF: %d, PDCCH UL: %s\n", dl_sf_cfg->tdd_config.sf_config, tti % 10, str);
            
            // if (dci_ul[0].cqi_request == true) {
            //     pusch_cfg->uci_cfg.cqi.four_antenna_ports = false;
            //     pusch_cfg->uci_cfg.cqi.data_enable = true;
            //     pusch_cfg->uci_cfg.cqi.pmi_present = false;
            //     pusch_cfg->uci_cfg.cqi.rank_is_not_one = false;
            //     pusch_cfg->uci_cfg.cqi.ri_len = 1;
            // } else {
            //     pusch_cfg->uci_cfg.cqi.data_enable = false;
            //     pusch_cfg->uci_cfg.cqi.ri_len = 0;
            // }
            if (srsran_ue_ul_dci_to_pusch_grant(ue_ul, ul_sf_cfg, ue_ul_cfg, dci_ul, &pusch_cfg->grant)) {
                ERROR("Error unpacking DCI");
                // return SRSRAN_ERROR;
            } 
        }
        return ret;
    }

    if (ret_dl > 0) {
        ue_dl_cfg->cfg.pdsch.use_tbs_index_alt = false;
        if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[0], &ue_dl_cfg->cfg.pdsch.grant)) {
            ERROR("Error unpacking dl DCI!");
        }

        for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
            // if (ue_dl_cfg->cfg.pdsch.grant.tb[i].enabled) {
                // if (ue_dl_cfg->cfg.pdsch.grant.tb[i].rv < 0) {
                //     uint32_t sfn = dl_sf_cfg->tti / 10;
                //     uint32_t k = (sfn / 2) % 4;
                //     ue_dl_cfg->cfg.pdsch.grant.tb[i].rv = ((uint32_t)ceilf((float)1.5 * k)) % 4;
                // }
            //     srsran_softbuffer_rx_reset_tbs(ue_dl_cfg->cfg.pdsch.softbuffers.rx[i], (uint32_t)ue_dl_cfg->cfg.pdsch.grant.tb[i].tbs);
            // }
            srsran_softbuffer_rx_reset_tbs(ue_dl_cfg->cfg.pdsch.softbuffers.rx[i], (uint32_t)ue_dl_cfg->cfg.pdsch.grant.tb[i].tbs);
        }

        bool decode_enable = false;
        for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
            if (ue_dl_cfg->cfg.pdsch.grant.tb[tb].enabled) {
                decode_enable         = true;
                // pdsch_res[tb].payload = data[tb];
                pdsch_res[tb].payload = srsran_vec_u8_malloc(2000 * 8);
                srsran_vec_u8_zero(pdsch_res[tb].payload, 2000 * 8);
                pdsch_res[tb].crc     = false;
            }
        }
        if (decode_enable) {
            if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, &ue_dl_cfg->cfg.pdsch, pdsch_res)) {
              ERROR("ERROR: Decoding PDSCH");
              // ret = -1;
            } else if (pdsch_res->crc == false) {
                for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
                    // if (ue_dl_cfg->cfg.pdsch.grant.tb[i].enabled) {
                    //     if (ue_dl_cfg->cfg.pdsch.grant.tb[i].rv < 0) {
                    //         uint32_t sfn = dl_sf_cfg->tti / 10;
                    //         uint32_t k = (sfn / 2) % 4;
                    //         ue_dl_cfg->cfg.pdsch.grant.tb[i].rv = ((uint32_t)ceilf((float)1.5 * k)) % 4;
                    //     }
                    //     srsran_softbuffer_rx_reset_tbs(ue_dl_cfg->cfg.pdsch.softbuffers.rx[i], (uint32_t)ue_dl_cfg->cfg.pdsch.grant.tb[i].tbs);
                    // }
                    srsran_softbuffer_rx_reset_tbs(ue_dl_cfg->cfg.pdsch.softbuffers.rx[i], (uint32_t)ue_dl_cfg->cfg.pdsch.grant.tb[i].tbs);
                    // srsran_vec_u8_zero(pdsch_res[i].payload, 2000 * 8);
                }
                ue_dl_cfg->cfg.pdsch.use_tbs_index_alt = true;
                if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[0], &ue_dl_cfg->cfg.pdsch.grant)) {
                    ERROR("Error unpacking dl DCI!");
                }
                if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, &ue_dl_cfg->cfg.pdsch, pdsch_res)) {
                    ERROR("ERROR: Decoding PDSCH");
                }
            }
        }
        char str[512];
        srsran_pdsch_rx_info(&ue_dl_cfg->cfg.pdsch, pdsch_res, str, sizeof(str));
        printf("Decoded PDSCH: %s\n", str);

        if (pdsch_res->crc) {
            auto& mac_logger = srslog::fetch_basic_logger("MAC");
            srsran::sch_pdu mac_msg_dl(10, mac_logger);
            mac_msg_dl.init_rx(ue_dl_cfg->cfg.pdsch.grant.tb[0].nof_bits / 8, false);
            mac_msg_dl.parse_packet(pdsch_res->payload);
            while(mac_msg_dl.next()) {
                uint32_t buff_size_idx[4]   = {};
                uint32_t buff_size_bytes[4] = {};
                if (mac_msg_dl.get()) {
                    // std::cout << "Decoding Downlink PDU header, lcid = " << int(mac_msg_dl.get()->dl_sch_ce_type()) << ", nof_subheaders = " << mac_msg_dl.nof_subh() << std::endl;
                    switch (mac_msg_dl.get()->dl_sch_ce_type())
                    {
                    case srsran::dl_sch_lcid::CCCH:
                        break;
                    case srsran::dl_sch_lcid::RESERVED:
                        break;
                    case srsran::dl_sch_lcid::SCELL_ACTIVATION:
                        break;
                    case srsran::dl_sch_lcid::CON_RES_ID:
                        break;
                    case srsran::dl_sch_lcid::TA_CMD:
                        // std::cout << "It's timing advance command, " << mac_msg_dl.get()->get_ta_cmd() << std::endl;
                        break;
                    case srsran::dl_sch_lcid::DRX_CMD:
                        break;
                    case srsran::dl_sch_lcid::PADDING:
                        break;
                    
                    default:
                        break;
                    }
                }
            }
        }
        // std::cout << std::endl;
        for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
            if (ue_dl_cfg->cfg.pdsch.grant.tb[tb].enabled) {
                acks[tb] = pdsch_res[tb].crc;
            }
        }
    }
    return ret;
}


int SFDecoder::getPuschGrantMultiUE(srsran_ue_dl_t* ue_dl, srsran_ue_ul_t* ue_ul, srsran_dl_sf_cfg_t* dl_sf_cfg, srsran_ul_sf_cfg_t* ul_sf_cfg, srsran_ue_dl_cfg_t* ue_dl_cfg, srsran_ue_ul_cfg_t* ue_ul_cfg) {
    int ret = SRSRAN_ERROR;
    int ret_dl = SRSRAN_ERROR;
    int ret_ul = SRSRAN_ERROR;
    srsran_ue_dl_set_mi_auto(ue_dl);
    if (srsran_ue_dl_decode_fft_estimate(ue_dl, dl_sf_cfg, ue_dl_cfg) < 0) {
        return SRSRAN_ERROR;
    }
    uint16_t rnti = 0;
    uint8_t* data[SRSRAN_MAX_CODEWORDS];
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        data[i] = srsran_vec_u8_malloc(2000 * 8);
    }
    auto& mac_logger = srslog::fetch_basic_logger("MAC");
    
    std::map<uint16_t, int> ueTracker = ueDataBase->getUETrack();
    for (const auto& pair : ueTracker){
        rnti = pair.first;
        ue_dl_cfg->cfg.pdsch.rnti = rnti;
        // ue_dl_cfg->cfg.tm = (srsran_tm_t)1;
        
        srsran_dci_dl_t dci_dl[SRSRAN_MAX_DCI_MSG];
        ZERO_OBJECT(dci_dl);
        srsran_dci_ul_t dci_ul[SRSRAN_MAX_DCI_MSG];
        ZERO_OBJECT(dci_ul);
        srsran_pdsch_res_t pdsch_res[SRSRAN_MAX_CODEWORDS] = {};
        
        srsran_softbuffer_rx_t rx_softbuffers[SRSRAN_MAX_CODEWORDS];
        srsran_softbuffer_rx_t rx_ul_softbuffers;

        int nof_ack = 0;
        int nof_dlmsg = 0;
        int nof_ulmsg = 0;

        for (uint32_t j = 0; j < SRSRAN_MAX_CODEWORDS; j++) {
            ue_dl_cfg->cfg.pdsch.softbuffers.rx[j] = &rx_softbuffers[j];
            srsran_softbuffer_rx_init(ue_dl_cfg->cfg.pdsch.softbuffers.rx[j], ue_dl->cell.nof_prb);
        }
        ue_ul_cfg->ul_cfg.pusch.softbuffers.rx = &rx_ul_softbuffers;
        srsran_softbuffer_rx_init(ue_ul_cfg->ul_cfg.pusch.softbuffers.rx, SRSRAN_MAX_PRB);

        // ret = srsran_ue_dl_find_dl_ul_dci(ue_dl, dl_sf_cfg, ul_sf_cfg, ue_dl_cfg, ue_ul_cfg, rnti, dci_dl, dci_ul, &ue_dl_cfg->cfg.pdsch.grant, &ue_ul_cfg->ul_cfg.pusch.grant, &nof_dlmsg, &nof_ulmsg);

        // if (ret >= 0) {
        //     if (nof_dlmsg > 0) {
        //         char str[512];
        //         srsran_dci_dl_info(&dci_dl[0], str, 512); 
        //         std::cout << "tti = " << tti << ", DL DCI: " << str << std::endl;
        //         puschSche->addUlAckNum(tti, dl_sf_cfg->tdd_config.sf_config, rnti);
        //     }
        //     if (nof_ulmsg > 0) {
        //         char str[512];
        //         srsran_dci_ul_info(&dci_ul[0], str, 512);
        //         std::cout << "DCI0, tti = " << tti << ", " << str << std::endl;
        //         puschSche->pushULDCI(tti, dl_sf_cfg->tdd_config.sf_config, rnti, ue_ul_cfg->ul_cfg.pusch.grant, nof_ack);
        //     }
        // }

        // ueDataBase->updateUE(rnti, ret > 0);

        ret_dl = srsran_ue_dl_find_dl_dci(ue_dl, dl_sf_cfg, ue_dl_cfg, rnti, dci_dl);
        ueDataBase->updateUE(rnti, ret_dl > 0);
        
        // std::cout << "rnti = " << rnti << ", ret dl = " << ret_dl;
        if (ret_dl > 0) {
            puschSche->addUlAckNum(tti, dl_sf_cfg->tdd_config.sf_config, rnti);
            char str[512];
            srsran_dci_dl_info(&dci_dl[0], str, 512);
            INFO("PDCCH DL: %s", str);
            printf("tddconfig: %d, SF: %d, RNTI = %d, PDCCH DL: %s\n", dl_sf_cfg->tdd_config.sf_config, tti % 10, rnti, str);

            ue_dl_cfg->cfg.pdsch.use_tbs_index_alt = false;
            if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[0], &ue_dl_cfg->cfg.pdsch.grant)) {
                ERROR("Error unpacking dl DCI!");
            } else {
                nof_ack = ue_dl_cfg->cfg.pdsch.grant.nof_tb;
                if (dci_dl[0].format == SRSRAN_DCI_FORMAT1 || dci_dl[0].format == SRSRAN_DCI_FORMAT1A) {
                    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
                        srsran_softbuffer_rx_reset_tbs(ue_dl_cfg->cfg.pdsch.softbuffers.rx[i], (uint32_t)ue_dl_cfg->cfg.pdsch.grant.tb[i].tbs);
                    }
                    bool decode_enable = false;
                    for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
                        if (ue_dl_cfg->cfg.pdsch.grant.tb[tb].enabled) {
                            decode_enable         = true;
                            pdsch_res[tb].payload = data[tb];
                            pdsch_res[tb].crc     = false;
                        }
                    }
                    if (decode_enable) {
                        if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, &ue_dl_cfg->cfg.pdsch, pdsch_res)) {
                            ERROR("ERROR: Decoding PDSCH");
                            ret = -1;
                        }
                    }

                    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
                        if (ue_dl_cfg->cfg.pdsch.grant.tb[i].enabled && pdsch_res[i].crc) {
                            char str[512];
                            srsran_pdsch_rx_info(&ue_dl_cfg->cfg.pdsch, pdsch_res, str, sizeof(str));
                            printf("Decoded PDSCH: %s, tb = %d\n", str, i);

                            srsran::sch_pdu dlsch_pdu(10, mac_logger);
                            dlsch_pdu.init_rx(ue_dl_cfg->cfg.pdsch.grant.tb[i].tbs / 8, false);
                            dlsch_pdu.parse_packet(pdsch_res[i].payload);

                            asn1::rrc::dl_dcch_msg_s dl_dcch_msg;
                            asn1::json_writer dcchwriter;
                            asn1::json_writer cqiwriter;

                            while (dlsch_pdu.next()) {
                                if (dlsch_pdu.get()) {
                                    // std::cout << "lcid: " << dlsch_pdu.get()->get_sdu_lcid() << " ";
                                    switch (dlsch_pdu.get()->get_sdu_lcid())
                                    {
                                    case 0b00000:
                                    {
                                        // std::cout << "This is BCCH" << std::endl;
                                    }
                                    break;

                                    case 0b00001:
                                    {
                                        std::cout << "This is DCCH, subheader size: " << dlsch_pdu.get()->get_header_size(false) << std::endl;
                                        asn1::cbit_ref bref(dlsch_pdu.get()->get_sdu_ptr()+3, dlsch_pdu.get()->get_payload_size()-3);
                                        int asn1_result = dl_dcch_msg.unpack(bref);
                                        if (asn1_result == asn1::SRSASN_SUCCESS && dl_dcch_msg.msg.type() == asn1::rrc::dl_dcch_msg_type_c::types_opts::c1) {
                                          dl_dcch_msg.to_json(dcchwriter);
                                          std::cout << dcchwriter.to_string().c_str() << std::endl;
                                          std::cout << "cqi format: " << dl_dcch_msg.msg.c1().rrc_conn_recfg().crit_exts.c1().rrc_conn_recfg_r8().rr_cfg_ded.phys_cfg_ded.cqi_report_cfg.cqi_report_periodic.setup().cqi_format_ind_periodic.type().to_string() << std::endl;
                                          dl_dcch_msg.msg.c1().rrc_conn_recfg().crit_exts.c1().rrc_conn_recfg_r8().rr_cfg_ded.phys_cfg_ded.cqi_report_cfg.to_json(cqiwriter);
                                          std::cout << cqiwriter.to_string() << std::endl;
                                            // switch (dl_dcch_msg.msg.c1().type().value)
                                            // {
                                            // case asn1::rrc::dl_dcch_msg_type_c::c1_c_::types_opts::rrc_conn_recfg:
                                            //     std::cout << "This is RRC recfg" << std::endl;
                                            //     dl_dcch_msg.to_json(dcchwriter);
                                            //     std::cout << dcchwriter.to_string().c_str() << std::endl;
                                            //     break;
                                      
                                            // default:
                                            //     break;
                                            // }
                                        }
                                    }
                                        break;
                            
                                    default:
                                        break;
                                    }
                                }
                                std::cout << std::endl;
                            }
                        }
                    }
                }
            }
            // if ((pdsch_res[0].crc == false) && (pdsch_res[1].crc == false)) {
            //     ue_dl_cfg->cfg.pdsch.use_tbs_index_alt = true;
            //     if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[0], &ue_dl_cfg->cfg.pdsch.grant)) {
            //         ERROR("Error unpacking dl DCI!");
            //     } else {
            //         nof_ack = ue_dl_cfg->cfg.pdsch.grant.nof_tb;
            //         for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
            //             srsran_softbuffer_rx_reset_tbs(ue_dl_cfg->cfg.pdsch.softbuffers.rx[i], (uint32_t)ue_dl_cfg->cfg.pdsch.grant.tb[i].tbs);
            //         }
            //         bool decode_enable = false;
            //         for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
            //             if (ue_dl_cfg->cfg.pdsch.grant.tb[tb].enabled) {
            //                 decode_enable         = true;
            //                 pdsch_res[tb].payload = data[tb];
            //                 pdsch_res[tb].crc     = false;
            //             }
            //         }
            //         if (decode_enable) {
            //             if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, &ue_dl_cfg->cfg.pdsch, pdsch_res)) {
            //                 ERROR("ERROR: Decoding PDSCH");
            //                 ret = -1;
            //             }
            //         }
            //     }
            // }
            
        }
        if (ret_dl >= 0) {
            ret_ul = srsran_ue_dl_find_ul_dci(ue_dl, dl_sf_cfg, ue_dl_cfg, rnti, dci_ul);
            if (ret_ul > 0) {
                char str[512];
                srsran_dci_ul_info(&dci_ul[0], str, 512);
                INFO("PDCCH UL: %s", str);
                printf("RNTI: %d, PDCCH UL: %s\n", rnti, str);
                if (srsran_ue_ul_dci_to_pusch_grant(ue_ul, ul_sf_cfg, ue_ul_cfg, dci_ul, &ue_ul_cfg->ul_cfg.pusch.grant)) {
                    ERROR("Error unpacking DCI");
                } else {
                    // std::cout << "In tti = " << tti << ", PUSCH grant for tti = " << (tti + 4) % 10240 << ", freq hopping =  " << ue_ul_cfg->ul_cfg.pusch.grant.freq_hopping << ", nof prb = " << ue_ul_cfg->ul_cfg.pusch.grant.n_prb[0] << " " << ue_ul_cfg->ul_cfg.pusch.grant.n_prb[1] << ", enabled = " << ue_ul_cfg->ul_cfg.pusch.grant.tb.enabled << ", tbs = " << ue_ul_cfg->ul_cfg.pusch.grant.tb.tbs << std::endl;
                }
                // puschSche->pushULDCI(tti, dl_sf_cfg->tdd_config.sf_config, rnti, dci_ul[0]);
                // puschSche->pushULDCI(tti, -1, rnti, dci_ul[0], nof_ack);
                // if (dl_sf_cfg->tdd_config.configured) {
                    puschSche->pushULDCI(tti, dl_sf_cfg->tdd_config.sf_config, rnti, ue_ul_cfg->ul_cfg.pusch.grant, dci_ul[0], nof_ack);
                // } else {
                //     puschSche->pushULDCI(tti, -1, rnti, ue_ul_cfg->ul_cfg.pusch.grant, dci_ul[0], nof_ack);
                // }
            }
        }
    }
    return 0;
}


int SFDecoder::sfDecodePuschMultiUE(srsran_ue_ul_t* ue_ul, srsran_ue_ul_cfg_t* ue_ul_cfg, srsran_enb_ul_t* enb_ul, srsran_ul_sf_cfg_t* ul_sf_cfg) {
    int ret = SRSRAN_ERROR;
    srsran_dci_ul_t dci_ul;
    ZERO_OBJECT(dci_ul);
    srsran_pusch_res_t pusch_res;
    ZERO_OBJECT(pusch_res);
    srsran_pucch_res_t pucch_res;
    ZERO_OBJECT(pucch_res);
    pusch_res.data = srsran_vec_u8_malloc(2000 * 8);
    uint16_t rnti = 0;
    srsran_softbuffer_rx_t rx_ul_softbuffers;
    int N_offset_cqi = 7;
    int N_pd = 160;
    int M_ri = 2;
    int N_offset_ri = -80;
    ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.four_antenna_ports = false;
    ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.rank_is_not_one = false;
    ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.ri_len = 0;
    ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.data_enable = false;
        // ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.N = ul_sniffer_cqi_hl_get_no_subbands(enb_ul->cell.nof_prb);
        // ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.ri_len = 0;
    // if ((tti - N_offset_cqi) % N_pd == 0) {
    //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.data_enable = true;
    //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.pmi_present = true;
    //     std::cout << "have cqi this tti: " << tti << std::endl << std::endl;
    // } else {
    //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.data_enable = false;
    //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.pmi_present = false;
    // }
    // if ((tti - N_offset_cqi - N_offset_ri) % (N_pd * M_ri) == 0) {
    //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.ri_len = 2;
    //     std::cout << "have ri this tti: " << tti << std::endl << std::endl;
    // } else {
    //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.ri_len = 0;
    // }
    ue_ul_cfg->ul_cfg.pusch.softbuffers.rx = &rx_ul_softbuffers;
    ue_ul_cfg->ul_cfg.pusch.meas_evm_en = true;
    ue_ul_cfg->ul_cfg.pusch.meas_ta_en = true;
    ue_ul_cfg->ul_cfg.pusch.meas_epre_en = true;
    ue_ul_cfg->ul_cfg.srs.common_enabled = false;
    ue_ul_cfg->ul_cfg.srs.dedicated_enabled = false;
    ue_ul_cfg->ul_cfg.srs.configured = false;
    ue_ul_cfg->ul_cfg.pusch.grant.is_rar = false;
    ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.type = srsran_cqi_type_t::SRSRAN_CQI_TYPE_WIDEBAND;
    ue_ul_cfg->ul_cfg.pusch.max_nof_iterations = 200;
    // enb_ul->pusch.llr_is_8bit = true;
    srsran_softbuffer_rx_init(ue_ul_cfg->ul_cfg.pusch.softbuffers.rx, SRSRAN_MAX_PRB);

    enb_ul->fft.mbsfn_subframe = false;

    srsran_enb_ul_fft(enb_ul);
    computerPower(enb_ul->sf_symbols);

    std::vector<std::pair<uint16_t, DCIUL>> dciULVec = puschSche->getULDCIstruct(tti);
    // std::vector<std::pair<uint16_t, srsran_dci_ul_t>> dci_ul_vec = puschSche->getUlDCIVec(tti);
    // std::cout << "PUSCH Config: n-SB = " << ue_ul_cfg->ul_cfg.hopping.n_sb << ", hopping mode = " << ue_ul_cfg->ul_cfg.hopping.hop_mode << ", hopping offset = " << ue_ul_cfg->ul_cfg.hopping.hopping_offset << std::endl;

    auto& mac_logger = srslog::fetch_basic_logger("MAC");
    for (auto it = dciULVec.begin(); it != dciULVec.end(); it++) {
        rnti = it->first;
        ue_ul_cfg->ul_cfg.pusch.enable_64qam = false;
        ue_ul_cfg->ul_cfg.pusch.rnti = rnti;
        ue_ul_cfg->ul_cfg.pusch.uci_cfg.ack[0].nof_acks = puschSche->getUlAckNum(tti, rnti);
        ue_ul_cfg->ul_cfg.pusch.uci_cfg.ack[0].tdd_ack_M = 4;
        // std::cout << "nof ack = " << ue_ul_cfg->ul_cfg.pusch.uci_cfg.ack[0].nof_acks << ", tdd_ack_M = " << ue_ul_cfg->ul_cfg.pucch.uci_cfg.ack[0].tdd_ack_M << std::endl;
        // std::cout << "dci ul_idx = " << it->second.dci_ul.ul_idx << std::endl;

        ue_ul_cfg->ul_cfg.pucch.rnti = rnti;
        ue_ul_cfg->ul_cfg.pucch.uci_cfg.ack[0].tdd_ack_M = 4;
        ue_ul_cfg->ul_cfg.pucch.uci_cfg.ack[0].nof_acks = ue_ul_cfg->ul_cfg.pusch.uci_cfg.ack[0].nof_acks;

        dci_ul = it->second.dci_ul;

        
        // if (dci_ul.cqi_request) {
        //     std::cout << "cqi request" << std::endl;
        //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.four_antenna_ports = false;
        //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.data_enable = true;
        //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.pmi_present = false;
        //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.rank_is_not_one = false;
        //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.N = ul_sniffer_cqi_hl_get_no_subbands(enb_ul->cell.nof_prb);
        //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.ri_len = 1;
        // } else {
        //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.data_enable = false;
        //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.ri_len = 0;
        // }
        // std::cout << "cell id: " << enb_ul->pusch.cell.id << std::endl;
        if (ul_sf_cfg->tti - 4 >= 0) {
            ul_sf_cfg->tti = ul_sf_cfg->tti - 4;
        } else {
            ul_sf_cfg->tti = ul_sf_cfg->tti - 4 + 10240;
        }
        // std::cout << "SFDecoder, nof_prb = " << ue_ul->cell.nof_prb << std::endl;
        if (srsran_ue_ul_dci_to_pusch_grant(ue_ul, ul_sf_cfg, ue_ul_cfg, &it->second.dci_ul, &ue_ul_cfg->ul_cfg.pusch.grant)) {
            ERROR("Error unpacking DCI");
        } else {
            //test begins
                    // ue_ul_cfg->ul_cfg.pusch.grant.n_prb_tilde[0] = 5;
                    // ue_ul_cfg->ul_cfg.pusch.grant.n_prb_tilde[1] = 5;
                    // ue_ul_cfg->ul_cfg.pusch.grant.L_prb = 1;
            // test ends
            // srsran_ue_ul_pusch_hopping(ue_ul, ul_sf_cfg, ue_ul_cfg, &ue_ul_cfg->ul_cfg.pusch.grant);
            srsran_softbuffer_rx_reset_tbs(ue_ul_cfg->ul_cfg.pusch.softbuffers.rx, (uint32_t)ue_ul_cfg->ul_cfg.pusch.grant.tb.tbs);
            // ue_ul_cfg->ul_cfg.pusch.grant = it->second.pusch_grant;
            // srsran_vec_u8_zero(pusch_res.data, 2000 * 8);
            pusch_res.crc = false;
            // ue_ul_cfg->ul_cfg.pusch.max_nof_iterations = 10;
            ul_sf_cfg->tti = (ul_sf_cfg->tti + 4) % 10240;
            // std::cout << "In tti = " << tti << ", freq hopping =  " << ue_ul_cfg->ul_cfg.pusch.grant.freq_hopping << ", nof prb = " << ue_ul_cfg->ul_cfg.pusch.grant.n_prb[0] << " " << ue_ul_cfg->ul_cfg.pusch.grant.n_prb[1] << ", enabled = " << ue_ul_cfg->ul_cfg.pusch.grant.tb.enabled << ", tbs = " << ue_ul_cfg->ul_cfg.pusch.grant.tb.tbs << std::endl;

            // std::cout << "Test uci cfg: ackIdxOffset = " << ue_ul_cfg->ul_cfg.pusch.uci_offset.I_offset_ack << ", riIdxOffset = " << ue_ul_cfg->ul_cfg.pusch.uci_offset.I_offset_ri << ", cqiIdxOffset = " << ue_ul_cfg->ul_cfg.pusch.uci_offset.I_offset_cqi << std::endl;
            ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.data_enable = false;
            ue_ul_cfg->ul_cfg.pusch.meas_epre_en = true;
            ret = srsran_enb_ul_get_pusch(enb_ul, ul_sf_cfg, &ue_ul_cfg->ul_cfg.pusch, &pusch_res);
            // float tmp_power = 0.0f;
            // float sum_power = 0.0f;
            // float aver_power = 0.0f;
            // // float max_power = 0.0f;
            // // int max_power_idx = 0;
            // for (uint32_t rb_idx = 0; rb_idx < ue_ul_cfg->ul_cfg.pusch.grant.L_prb; rb_idx++) {
            //     tmp_power = rb_power.at(ue_ul_cfg->ul_cfg.pusch.grant.n_prb[0] + rb_idx);
            //     sum_power += tmp_power;
            // }
            // aver_power = tmp_power / ue_ul_cfg->ul_cfg.pusch.grant.L_prb;
            // std::cout << "tbs = " << ue_ul_cfg->ul_cfg.pusch.grant.tb.tbs << ", payload lenth: " << ue_ul_cfg->ul_cfg.pusch.grant.tb.nof_bits << std::endl;
            // if (pusch_res.crc == false) {
            //     srsran_softbuffer_rx_reset_tbs(ue_ul_cfg->ul_cfg.pusch.softbuffers.rx, (uint32_t)ue_ul_cfg->ul_cfg.pusch.grant.tb.tbs);
            //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.data_enable = true;
            //     ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.ri_len = 0;
            //     ret = srsran_enb_ul_get_pusch(enb_ul, ul_sf_cfg, &ue_ul_cfg->ul_cfg.pusch, &pusch_res);
            // }
            

            // if ((ue_ul_cfg->ul_cfg.pusch.grant.tb.enabled == false) && (ue_ul_cfg->ul_cfg.pusch.uci_cfg.ack[0].nof_acks > 0)) {
            //     ret = srsran_enb_ul_get_pucch(enb_ul, ul_sf_cfg, &ue_ul_cfg->ul_cfg.pucch, &pucch_res);
            // }

            // if (pusch_res.crc == false) {
            //     ue_ul_cfg->ul_cfg.pusch.enable_64qam = true;
            //     // pusch_res.data = srsran_vec_u8_malloc(2000 * 8);
            //     if (ul_sf_cfg->tti - 4 >= 0) {
            //         ul_sf_cfg->tti = ul_sf_cfg->tti - 4;
            //     } else {
            //         ul_sf_cfg->tti = ul_sf_cfg->tti - 4 + 10240;
            //     }
            //     if (srsran_ue_ul_dci_to_pusch_grant(ue_ul, ul_sf_cfg, ue_ul_cfg, &it->second.dci_ul, &ue_ul_cfg->ul_cfg.pusch.grant)) {
            //         ERROR("Error unpacking DCI");
            //     } else {
            //         

            //         ul_sf_cfg->tti = (ul_sf_cfg->tti + 4) % 10240;
            //         srsran_softbuffer_rx_reset_tbs(ue_ul_cfg->ul_cfg.pusch.softbuffers.rx, ue_ul_cfg->ul_cfg.pusch.grant.tb.tbs);
            //         // srsran_vec_u8_zero(pusch_res.data, 2000 * 8);
            //         ret = srsran_enb_ul_get_pusch(enb_ul, ul_sf_cfg, &ue_ul_cfg->ul_cfg.pusch, &pusch_res);

            //         // if (pusch_res.crc == false) {
            //         //     ue_ul_cfg->ul_cfg.pusch.enable_64qam = true;
            //         //     pusch_res.data = srsran_vec_u8_malloc(2000 * 8);
            //         //     if (ul_sf_cfg->tti - 4 >= 0) {
            //         //         ul_sf_cfg->tti = ul_sf_cfg->tti - 4;
            //         //     } else {
            //         //         ul_sf_cfg->tti = ul_sf_cfg->tti - 4 + 10240;
            //         //     }
            //         //     if (srsran_ue_ul_dci_to_pusch_grant_qam256_ngscope(ue_ul, ul_sf_cfg, ue_ul_cfg, &dci_ul, &ue_ul_cfg->ul_cfg.pusch.grant)) {
            //         //         ERROR("Error unpacking DCI");
            //         //     } else {
            //         //         ul_sf_cfg->tti = (ul_sf_cfg->tti + 4) % 10240;
            //         //         srsran_softbuffer_rx_reset_tbs(ue_ul_cfg->ul_cfg.pusch.softbuffers.rx, ue_ul_cfg->ul_cfg.pusch.grant.tb.tbs);
            //         //         srsran_vec_u8_zero(pusch_res.data, 2000 * 8);
            //         //         ret = srsran_enb_ul_get_pusch(enb_ul, ul_sf_cfg, &ue_ul_cfg->ul_cfg.pusch, &pusch_res);
            //         //     }
            //         // }
            //     }
            // }
        }
          std::cout << "pusch rb: " << ue_ul_cfg->ul_cfg.pusch.grant.n_prb_tilde[0] << "-" << ue_ul_cfg->ul_cfg.pusch.grant.n_prb_tilde[0]+ue_ul_cfg->ul_cfg.pusch.grant.L_prb-1 << std::endl;
            for (uint32_t prb_idx = 0; prb_idx < nof_prb; prb_idx++) {
                // std::cout << rb_power.at(prb_idx) << ", ";
                std::cout << "prb_idx: " << prb_idx << ", power: " << rb_power.at(prb_idx) << std::endl;
            }
            std::cout << std::endl;

        char str[512];
        srsran_pusch_rx_info(&ue_ul_cfg->ul_cfg.pusch, &pusch_res, &enb_ul->chest_res, str, sizeof(str));
        printf("PUSCH: %s, rsrp = %fdB\n\n", str, enb_ul->chest_res.rsrp_dBfs);
        if (pusch_res.crc) {
            char str[512];
            srsran_pusch_rx_info(&ue_ul_cfg->ul_cfg.pusch, &pusch_res, &enb_ul->chest_res, str, sizeof(str));
            printf("\n\nHaha Decoded PUSCH: %s, rsrp = %fdB\n\n", str, enb_ul->chest_res.rsrp_dBfs);
            // for (int i = 0; i < ue_ul_cfg->ul_cfg.pusch.grant.tb.tbs; i++) {
            //     printf("%d ", pusch_res.data[i]);
            // }
            std::cout << std::endl;
            srsran::sch_pdu ulsch_pdu(10, mac_logger);
            ulsch_pdu.init_rx(ue_ul_cfg->ul_cfg.pusch.grant.tb.tbs / 8, true);
            ulsch_pdu.parse_packet(pusch_res.data);
            

            while (ulsch_pdu.next()) {
                if (ulsch_pdu.get()) {
                    uint32_t buff_size_idx[4]   = {};
                    uint32_t buff_size_bytes[4] = {};
                    std::cout << "ulsch type: " << (int)ulsch_pdu.get()->ul_sch_ce_type() << std::endl;
                    switch (ulsch_pdu.get()->ul_sch_ce_type())
                    {
                    case srsran::ul_sch_lcid::TRUNC_BSR:
                      std::cout << "this is trunc bsr: " << ulsch_pdu.get()->get_bsr(buff_size_idx, buff_size_bytes) << std::endl;
                      break;
                    
                    case srsran::ul_sch_lcid::SHORT_BSR:
                      std::cout << "this is short bsr: " << ulsch_pdu.get()->get_bsr(buff_size_idx, buff_size_bytes) << std::endl;
                      break;
                    
                    case srsran::ul_sch_lcid::LONG_BSR:
                      std::cout << "this is long bsr: " << ulsch_pdu.get()->get_bsr(buff_size_idx, buff_size_bytes) << std::endl;
                      break;

                    case srsran::ul_sch_lcid::PADDING:
                      std::cout << "this is padding, padding length: " << ulsch_pdu.get()->get_payload_size() << std::endl;
                      break;

                    default:
                      break;
                    }
                }
            }
        }
    }

    puschSche->deleteDciUL(tti);
    puschSche->deleteUlAckNum(tti);
    free(pusch_res.data);
    srsran_softbuffer_rx_free(ue_ul_cfg->ul_cfg.pusch.softbuffers.rx);

    return 0;
}


int SFDecoder::findDCI(srsran_ue_dl_t* ue_dl, srsran_dl_sf_cfg_t* dl_sf_cfg, srsran_ue_dl_cfg_t* ue_dl_cfg, uint16_t rnti, srsran_dci_dl_t* dci_dl, srsran_dci_ul_t* dci_ul) {

}


int SFDecoder::sfDecodePusch(srsran_ue_ul_t* ue_ul, srsran_ue_ul_cfg_t* ue_ul_cfg, srsran_dci_ul_t dci_ul, srsran_enb_ul_t* enb_ul, srsran_ul_sf_cfg_t* ul_sf_cfg, uint32_t rnti) {
    int ret = SRSRAN_ERROR;
    srsran_pusch_res_t pusch_res;
    ZERO_OBJECT(pusch_res);
    pusch_res.data = srsran_vec_u8_malloc(2000 * 8);
    srsran_enb_ul_fft(enb_ul);
    ue_ul_cfg->ul_cfg.pusch.meas_evm_en = true;
    ue_ul_cfg->ul_cfg.pusch.meas_ta_en = false;
    if (dci_ul.cqi_request == true) {
        ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.four_antenna_ports = false;
        ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.data_enable = true;
        ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.pmi_present = false;
        ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.rank_is_not_one = false;
        ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.N = ul_sniffer_cqi_hl_get_no_subbands(enb_ul->cell.nof_prb);
        ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.ri_len = 1;
    } else {
        ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.data_enable = false;
        ue_ul_cfg->ul_cfg.pusch.uci_cfg.cqi.ri_len = 0;
    }
    ue_ul_cfg->ul_cfg.pusch.enable_64qam = false;
    if (srsran_ue_ul_dci_to_pusch_grant(ue_ul, ul_sf_cfg, ue_ul_cfg, &dci_ul, &ue_ul_cfg->ul_cfg.pusch.grant)) {
        ERROR("Error unpacking DCI");
    } else {
        srsran_softbuffer_rx_reset_tbs(ue_ul_cfg->ul_cfg.pusch.softbuffers.rx, ue_ul_cfg->ul_cfg.pusch.grant.tb.tbs);
        srsran_vec_u8_zero(pusch_res.data, 2000 * 8);
        pusch_res.crc = false;
        std::cout << "mcs_idx = " << ue_ul_cfg->ul_cfg.pusch.grant.tb.mcs_idx << std::endl;
        ret = srsran_enb_ul_get_pusch(enb_ul, ul_sf_cfg, &ue_ul_cfg->ul_cfg.pusch, &pusch_res);
        if (pusch_res.crc == false) {
            ue_ul_cfg->ul_cfg.pusch.enable_64qam = true;
            pusch_res.data = srsran_vec_u8_malloc(2000 * 8);
            if (srsran_ue_ul_dci_to_pusch_grant(ue_ul, ul_sf_cfg, ue_ul_cfg, &dci_ul, &ue_ul_cfg->ul_cfg.pusch.grant)) {
                ERROR("Error unpacking DCI");
            } else {
                srsran_softbuffer_rx_reset_tbs(ue_ul_cfg->ul_cfg.pusch.softbuffers.rx, ue_ul_cfg->ul_cfg.pusch.grant.tb.tbs);
                srsran_vec_u8_zero(pusch_res.data, 2000 * 8);
                ret = srsran_enb_ul_get_pusch(enb_ul, ul_sf_cfg, &ue_ul_cfg->ul_cfg.pusch, &pusch_res);
            } if (pusch_res.crc == false) {
                ue_ul_cfg->ul_cfg.pusch.enable_64qam = true;
                pusch_res.data = srsran_vec_u8_malloc(2000 * 8);
                if (srsran_ue_ul_dci_to_pusch_grant_qam256_ngscope(ue_ul, ul_sf_cfg, ue_ul_cfg, &dci_ul, &ue_ul_cfg->ul_cfg.pusch.grant)) {
                    ERROR("Error unpacking DCI");
                } else {
                    srsran_softbuffer_rx_reset_tbs(ue_ul_cfg->ul_cfg.pusch.softbuffers.rx, ue_ul_cfg->ul_cfg.pusch.grant.tb.tbs);
                    srsran_vec_u8_zero(pusch_res.data, 2000 * 8);
                    ret = srsran_enb_ul_get_pusch(enb_ul, ul_sf_cfg, &ue_ul_cfg->ul_cfg.pusch, &pusch_res);
                }
            }
        }

        if (pusch_res.crc == true) {
            char str[512];
            srsran_pusch_rx_info(&ue_ul_cfg->ul_cfg.pusch, &pusch_res, &enb_ul->chest_res, str, sizeof(str));
            printf("Haha Decoded PUSCH: %s\n\n", str);

            srsran::sch_pdu ulsch_pdu();
        }
    }
    return ret;
}


int SFDecoder::sfDecodePucch(srsran_ue_ul_t* ue_ul, srsran_ue_ul_cfg_t* ue_ul_cfg, srsran_enb_ul_t* enb_ul, srsran_ul_sf_cfg_t* ul_sf, uint16_t rnti) {
    ue_ul_cfg->ul_cfg.pucch.rnti = rnti;
    int ret = SRSRAN_ERROR;
    srsran_pucch_res_t pucch_res;
    ZERO_OBJECT(pucch_res);

    for (int i = 1; i <= 2; i++){
        ue_ul_cfg->ul_cfg.pucch.uci_cfg.ack[0].nof_acks = i;
        ue_ul_cfg->ul_cfg.pucch.uci_cfg.ack[1].nof_acks = 0;
        ue_ul_cfg->ul_cfg.pucch.uci_cfg.ack[2].nof_acks = 0;
        ue_ul_cfg->ul_cfg.pucch.uci_cfg.ack[3].nof_acks = 0;
        ue_ul_cfg->ul_cfg.pucch.uci_cfg.ack[4].nof_acks = 0;
        srsran_enb_ul_get_pucch(enb_ul, ul_sf, &ue_ul_cfg->ul_cfg.pucch, &pucch_res);
    }
    return ret;
}


int SFDecoder::sfDecodeRach(srsran_ue_dl_t* ue_dl, srsran_dl_sf_cfg_t* dl_sf_cfg, srsran_ue_dl_cfg_t* ue_dl_cfg, srsran_ue_ul_cfg_t* ue_ul_cfg) {
    int ret = SRSRAN_ERROR;
    uint16_t rnti;
    uint16_t c_rnti;
    uint8_t* data[SRSRAN_MAX_CODEWORDS];
    srsran_ue_dl_cfg_t temp_ue_dl_cfg = *ue_dl_cfg;
    srsran_ue_ul_cfg_t temp_ue_ul_cfg = *ue_ul_cfg;
    ue_dl_cfg->cfg.tm = (srsran_tm_t)1;
    // srsran_ue_ul_cfg_t temp_ue_ul_cfg = *ue_ul_cfg;
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        data[i] = srsran_vec_u8_malloc(2000 * 8);
    }
    bool acks[SRSRAN_MAX_CODEWORDS] = {false};
    
    srsran_pdsch_cfg_t pdsch_cfg = ue_dl_cfg->cfg.pdsch;

    // dl_sf_cfg->tti = tti;
    uint32_t mi_set_len = 1;
    srsran_ue_dl_set_mi_auto(ue_dl);

    ret = srsran_ue_dl_decode_fft_estimate(ue_dl, dl_sf_cfg, ue_dl_cfg);
    if (ret < 0) {
        return ret;
    } 
    for (rnti = SRSRAN_RARNTI_START; rnti <= SRSRAN_RARNTI_END; rnti++){
        srsran_dci_dl_t    dci_dl[SRSRAN_MAX_DCI_MSG] = {};
        srsran_pdsch_res_t pdsch_res[SRSRAN_MAX_CODEWORDS] = {};
        pdsch_cfg.rnti = rnti;
        ret = srsran_ue_dl_find_dl_dci_rarnti(ue_dl, dl_sf_cfg, ue_dl_cfg, rnti, dci_dl);
        if (ret > 0) {
            char str[512];
            srsran_dci_dl_info(&dci_dl[0], str, 512);
            // std::cout << "RACH Decoder found DCI: PDCCH: " << str << ", snr = " << ue_dl->chest_res.snr_db << " dB, ret = " << ret << std::endl;

            pdsch_cfg.use_tbs_index_alt = false;
            // Convert DCI message to DL grant
            if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[0], &pdsch_cfg.grant)) {
                // ERROR("Error unpacking DCI");
                ret = -1;
            } else {
                for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
                    srsran_softbuffer_rx_reset_tbs(pdsch_cfg.softbuffers.rx[i], (uint32_t)pdsch_cfg.grant.tb[i].tbs);
                }
                bool decode_enable = false;
                for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
                    if (pdsch_cfg.grant.tb[tb].enabled) {
                        decode_enable         = true;
                        pdsch_res[tb].payload = data[tb];
                        pdsch_res[tb].crc     = false;
                    }
                }
                if (decode_enable) {
                    if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, &pdsch_cfg, pdsch_res)) {
                        ERROR("ERROR: Decoding PDSCH");
                        ret = -1;
                    }
                }
            }
            // if ((pdsch_res[0].crc == false && pdsch_res[1].crc == false) || ret == -1) {
            //         pdsch_cfg.use_tbs_index_alt = true;
            //         if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[0], &pdsch_cfg.grant)) {
            //             ERROR("Error unpacking DCI");
            //         } else {
            //             for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
            //                 srsran_softbuffer_rx_reset_tbs(pdsch_cfg.softbuffers.rx[i], (uint32_t)pdsch_cfg.grant.tb[i].tbs);
            //             }
            //             bool decode_enable = false;
            //             for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
            //                 if (pdsch_cfg.grant.tb[tb].enabled) {
            //                     decode_enable         = true;
            //                     pdsch_res[tb].payload = data[tb];
            //                     pdsch_res[tb].crc     = false;
            //                 }
            //             }
            //             if (decode_enable) {
            //                 if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, &pdsch_cfg, pdsch_res)) {
            //                     ERROR("ERROR: Decoding PDSCH");
            //                     ret = -1;
            //                 }
            //             }
            //         }
            //     }

            srsran::rar_pdu rar_pdu_msg;
            uint16_t t_crnti = 0;
            for (int tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
                if (pdsch_res[tb].crc && pdsch_cfg.grant.tb[tb].tbs > 0) {
                    char str[512];
                    srsran_pdsch_rx_info(&pdsch_cfg, pdsch_res, str, sizeof(str));
                    printf("Rach Decoded PDSCH: %s, tb = %d\n", str, tb);
                    rar_pdu_msg.init_rx(pdsch_cfg.grant.tb[tb].tbs / 8, false);
                    if (rar_pdu_msg.parse_packet(pdsch_res[tb].payload) == SRSRAN_SUCCESS) {
                        std::cout << "Successfully parsing pdu, nof subheaders: " << rar_pdu_msg.nof_subh() << std::endl;
                        while (rar_pdu_msg.next()) {
                            // if (rar_pdu_msg.get()->has_rapid()) {
                            t_crnti = rar_pdu_msg.get()->get_temp_crnti();
                            std::cout << "r_crnti = " << t_crnti << std::endl;
                            if (t_crnti) {
                                std::cout << std::endl << "Found RACH response! TC-RNTI = " << t_crnti << ", ta cmd = " << rar_pdu_msg.get()->get_ta_cmd() << ", index = " << rar_pdu_msg.get()->has_rapid() << std::endl << std::endl;
                                tcrntiDatabase->push_tcRNTI(t_crnti);
                                break;
                            }
                        }
                    }
                }
            }
                // if (pdsch_res[0].crc == true) {
                //     char str[512];
                //     srsran_pdsch_rx_info(&pdsch_cfg, pdsch_res, str, sizeof(str));
                //     printf("Rach Decoded PDSCH tb0: %s\n", str);
                //     rar_pdu_msg.init_rx(pdsch_cfg.grant.tb[0].tbs / 8, false);
                //     ret = rar_pdu_msg.parse_packet(pdsch_res[0].payload);
                //     std::cout << "pdu parse ret = " << ret << std::endl;
                // } 
                // if (pdsch_res[1].crc == true) {
                //     char str[512];
                //     srsran_pdsch_rx_info(&pdsch_cfg, pdsch_res, str, sizeof(str));
                //     printf("Rach Decoded PDSCH tb1: %s\n", str);
                //     rar_pdu_msg.init_rx(pdsch_cfg.grant.tb[1].tbs / 8, false);
                //     ret = rar_pdu_msg.parse_packet(pdsch_res[1].payload);
                // }
                // if (ret == SRSRAN_SUCCESS){
                //     // if (rar_pdu_msg.parse_packet(pdsch_res->payload) == SRSRAN_SUCCESS) {
                //         while (rar_pdu_msg.next()) {
                //             // if (rar_pdu_msg.get()->has_rapid()) {
                //                 t_crnti = rar_pdu_msg.get()->get_temp_crnti();
                //                 if (t_crnti) {
                //                     std::cout << "Found RACH response! TC-RNTI = " << t_crnti << ", ta cmd = " << rar_pdu_msg.get()->get_ta_cmd() << ", index = " << rar_pdu_msg.get()->has_rapid() << std::endl;
                //                     tcrntiDatabase->push_tcRNTI(t_crnti);
                //                 }
                //             // }
                //         }
                //     // }
                //     std::cout << std::endl;
                // }
        }
    }

    std::vector<uint16_t> tcrntiVec = tcrntiDatabase->get_tcRNTI();
    asn1::json_writer phys_ded_json;
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        srsran_vec_u8_zero(data[i], 2000 * 8);
    }
    auto& mac_logger = srslog::fetch_basic_logger("MAC");

    for (std::vector<uint16_t>::iterator iter = tcrntiVec.begin(); iter != tcrntiVec.end(); iter++) {
        srsran_dci_dl_t dci_dl[SRSRAN_MAX_DCI_MSG] = {};
        srsran_dci_ul_t dci_ul[SRSRAN_MAX_DCI_MSG] = {};
        srsran_pdsch_res_t pdsch_res[SRSRAN_MAX_CODEWORDS] = {};
        rnti = *iter;
        pdsch_cfg.rnti = rnti;
        ret = srsran_ue_dl_find_dl_dci(ue_dl, dl_sf_cfg, ue_dl_cfg, rnti, dci_dl);
        // std::cout << "Trying for TC-RNTI: " << *iter << ", tm = " << ue_dl_cfg->cfg.tm << ", ret = " << ret << std::endl;

        if (ret > 0) {
            char str[512];
            srsran_dci_dl_info(&dci_dl[0], str, 512);
            // std::cout << "RACH Decoder found DCI for msg4: PDCCH: " << str << ", snr = " << ue_dl->chest_res.snr_db << " dB." << std::endl;

            // Convert DCI message to DL grant
            pdsch_cfg.use_tbs_index_alt = false;
            ue_dl_cfg->cfg.tm = (srsran_tm_t)0;
            if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[0], &pdsch_cfg.grant)) {
                // ERROR("Error unpacking DCI");
                tcrntiDatabase->delete_tcRNTI(rnti);
            } else {
                // std::cout << "nof_rx_ant: " << ue_dl->pdsch.nof_rx_antennas << std::endl;
                for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
                    if (pdsch_cfg.grant.tb[i].enabled) {
                        if (pdsch_cfg.grant.tb[i].rv < 0) {
                            uint32_t sfn = dl_sf_cfg->tti / 10;
                            uint32_t k = (sfn / 2) % 4;
                            pdsch_cfg.grant.tb[i].rv = ((uint32_t)ceilf((float)1.5 * k)) % 4;
                        }
                        srsran_softbuffer_rx_reset_tbs(pdsch_cfg.softbuffers.rx[i], (uint32_t)pdsch_cfg.grant.tb[i].tbs);
                    }
                }
                bool decode_enable = false;
                for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
                    if (pdsch_cfg.grant.tb[tb].enabled) {
                        decode_enable         = true;
                        pdsch_res[tb].payload = data[tb];
                        pdsch_res[tb].crc     = false;
                    }
                }

                if (decode_enable) {
                    if (dl_sf_cfg->sf_type == SRSRAN_SF_NORM) {
                        if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, &pdsch_cfg, pdsch_res)) {
                            ERROR("ERROR: Decoding PDSCH");
                            ret = -1;
                        }
                    } 
                }
            }

            // if (pdsch_res[0].crc == false && pdsch_res[1].crc == false) {
            //   pdsch_cfg.use_tbs_index_alt = true;
            //   if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[0], &pdsch_cfg.grant)) {
            //       ERROR("Error unpacking DCI");
            //       tcrntiDatabase->delete_tcRNTI(rnti);
            //   } else {
            //     // std::cout << "nof_rx_ant: " << ue_dl->pdsch.nof_rx_antennas << std::endl;
            //     for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
            //         if (pdsch_cfg.grant.tb[i].enabled) {
            //             if (pdsch_cfg.grant.tb[i].rv < 0) {
            //                 uint32_t sfn = dl_sf_cfg->tti / 10;
            //                 uint32_t k = (sfn / 2) % 4;
            //                 pdsch_cfg.grant.tb[i].rv = ((uint32_t)ceilf((float)1.5 * k)) % 4;
            //             }
            //             srsran_softbuffer_rx_reset_tbs(pdsch_cfg.softbuffers.rx[i], (uint32_t)pdsch_cfg.grant.tb[i].tbs);
            //         }
            //     }
            //     bool decode_enable = false;
            //     for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
            //         if (pdsch_cfg.grant.tb[tb].enabled) {
            //             decode_enable         = true;
            //             pdsch_res[tb].payload = data[tb];
            //             pdsch_res[tb].crc     = false;
            //         }
            //     }

            //     if (decode_enable) {
            //         if (dl_sf_cfg->sf_type == SRSRAN_SF_NORM) {
            //             if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, &pdsch_cfg, pdsch_res)) {
            //                 ERROR("ERROR: Decoding PDSCH");
            //                 ret = -1;
            //             }
            //         } 
            //     }
            //   }
            // }
            asn1::rrc::dl_ccch_msg_s dlcch_msg;
            srsran::sch_pdu msg4pdu(10, mac_logger);
            asn1::json_writer phywriter;

            // for (int tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
              if (pdsch_res[0].crc) {
                    char str[512];
                    srsran_pdsch_rx_info(&pdsch_cfg, pdsch_res, str, sizeof(str));
                    printf("Rach Decoded Msg4 PDSCH: %s\n", str);
                    int nof_bytes = 0;
                    for (int tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
                      if (pdsch_cfg.grant.tb[tb].enabled) {
                        nof_bytes = nof_bytes + pdsch_cfg.grant.tb[tb].nof_bits / 8;
                      }
                    }

                msg4pdu.init_rx(pdsch_cfg.grant.tb[0].tbs / 8, false);
                msg4pdu.parse_packet(pdsch_res[0].payload);
                int i = 0;
                bool find_ccch = false;
                uint32_t headersize = 0;
                uint32_t byteoffset = 0;
                uint32_t byteoffsettale = 0;
                while (msg4pdu.next()) {
                  std::cout << "pdsch size: " << pdsch_cfg.grant.tb[0].tbs / 8 << ", nof subheaders: " << msg4pdu.nof_subh() << ", pdu size: " << msg4pdu.get_pdu_len() << std::endl;
                  if (msg4pdu.get()) {
                    i++;
                    std::cout << "payload size: " << msg4pdu.get()->get_payload_size() << ", header size: " << msg4pdu.get()->get_header_size(i == msg4pdu.nof_subh()) << std::endl;
                    headersize += msg4pdu.get()->get_header_size(i == msg4pdu.nof_subh());
                    switch (msg4pdu.get()->dl_sch_ce_type()){
                      case srsran::dl_sch_lcid::CCCH:
                        {std::cout << "this is ccch, i = " << i << std::endl;
                        find_ccch = true;
                        uint8_t* sdu_ptr = msg4pdu.get()->get_sdu_ptr();
                        asn1::rrc::dl_ccch_msg_s dl_ccch_msg;
                        asn1::cbit_ref bref(sdu_ptr, msg4pdu.get()->get_payload_size());
                        int asn1_result = dl_ccch_msg.unpack(bref);
                        asn1::json_writer msg4writer;
                        asn1::json_writer cqiwriter;
                        if (asn1_result == asn1::SRSASN_SUCCESS && dl_ccch_msg.msg.type() == asn1::rrc::dl_ccch_msg_type_c::types_opts::c1) {
                          std::cout << "Msg4 decoded!" << std::endl;
                          switch (dl_ccch_msg.msg.c1().type().value)
                          {
                          case asn1::rrc::dl_ccch_msg_type_c::c1_c_::types::rrc_conn_reject:
                            {std::cout << "Unfortunately, this is a rrc reject for this tc-rnti: " << rnti << std::endl;
                            tcrntiDatabase->delete_tcRNTI(rnti);
                            }
                            break;

                          case asn1::rrc::dl_ccch_msg_type_c::c1_c_::types::rrc_conn_setup:
                            {std::cout << "Cool! This is a rrc setup! c-rnti: " << rnti << std::endl;
                            dl_ccch_msg.to_json(msg4writer);
                            std::cout << msg4writer.to_string().c_str() << std::endl;
                            tcrntiDatabase->delete_tcRNTI(rnti);
                            ueDataBase->pushUE(rnti);
                            if (!ueDataBase->have_cfg_ded()) {
                              temp_ue_dl_cfg.cfg.pdsch.rnti = rnti;
                              temp_ue_dl_cfg.cfg.pdsch.p_a = dl_ccch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.pdsch_cfg_ded.p_a.to_number();
                              temp_ue_dl_cfg.cfg.tm = (srsran_tm_t)(dl_ccch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.ant_info.explicit_value().tx_mode.to_number() - 1);
                              // std::cout << "Transmission mode: " << dl_ccch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.ant_info.explicit_value().tx_mode.to_string() << ", tm = " << temp_ue_dl_cfg.cfg.tm << std::endl;
                              std::cout << "Transmission mode: " << dl_ccch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.ant_info.explicit_value().tx_mode.to_string() << std::endl;
                              temp_ue_ul_cfg.ul_cfg.pusch.rnti = rnti;
                              temp_ue_ul_cfg.ul_cfg.pusch.uci_offset.I_offset_ack = dl_ccch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.pusch_cfg_ded.beta_offset_ack_idx;
                              temp_ue_ul_cfg.ul_cfg.pusch.uci_offset.I_offset_cqi = dl_ccch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.pusch_cfg_ded.beta_offset_cqi_idx;
                              temp_ue_ul_cfg.ul_cfg.pusch.uci_offset.I_offset_ri = dl_ccch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.pusch_cfg_ded.beta_offset_ri_idx;
                              temp_ue_ul_cfg.ul_cfg.pusch.uci_cfg.cqi.type; 
                              temp_ue_ul_cfg.ul_cfg.pucch.rnti = rnti;
                              std::cout << "cqi format: " << dl_ccch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.cqi_report_cfg.cqi_report_periodic.setup().cqi_format_ind_periodic.type().to_string() << std::endl;
                              dl_ccch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.cqi_report_cfg.cqi_report_periodic.to_json(cqiwriter);
                              std::cout << cqiwriter.to_string() << std::endl;
                              temp_ue_ul_cfg.ul_cfg.pucch.simul_cqi_ack = dl_ccch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.cqi_report_cfg.cqi_report_periodic.setup().simul_ack_nack_and_cqi;
                              temp_ue_ul_cfg.ul_cfg.pucch.tdd_ack_multiplex = (dl_ccch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.pucch_cfg_ded.tdd_ack_nack_feedback_mode == asn1::rrc::pucch_cfg_ded_s::tdd_ack_nack_feedback_mode_e_::mux);
                              ueDataBase->set_cfg_ded(temp_ue_dl_cfg, temp_ue_ul_cfg);
                            }
                            }
                            break;

                          case asn1::rrc::dl_ccch_msg_type_c::c1_c_::types::rrc_conn_reest_reject:
                            {std::cout << "Unfortunately, this is a rrc re-establishment reject for this tc-rnti: " << rnti << std::endl;
                            tcrntiDatabase->delete_tcRNTI(rnti);
                            }
                            break;

                          case asn1::rrc::dl_ccch_msg_type_c::c1_c_::types::rrc_conn_reest:
                            {std::cout << "Cool! This is a rrc re-establishment! c-rnti: " << rnti << std::endl;
                            tcrntiDatabase->delete_tcRNTI(rnti);
                            dl_ccch_msg.to_json(msg4writer);
                            std::cout << msg4writer.to_string().c_str() << std::endl;
                            tcrntiDatabase->delete_tcRNTI(rnti);
                            ueDataBase->pushUE(rnti);
                            // temp_ue_dl_cfg.cfg.pdsch.rnti = rnti;
                            // temp_ue_dl_cfg.cfg.pdsch.p_a = dl_ccch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded.pdsch_cfg_ded.p_a.to_number();
                            // temp_ue_dl_cfg.cfg.tm = (srsran_tm_t)(dl_ccch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded.ant_info.explicit_value().tx_mode.to_number()-1);
                            // temp_ue_ul_cfg.ul_cfg.pusch.rnti = rnti;
                            // temp_ue_ul_cfg.ul_cfg.pusch.uci_offset.I_offset_ack = dl_ccch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded.pusch_cfg_ded.beta_offset_ack_idx;
                            // temp_ue_ul_cfg.ul_cfg.pusch.uci_offset.I_offset_cqi = dl_ccch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded.pusch_cfg_ded.beta_offset_cqi_idx;
                            // temp_ue_ul_cfg.ul_cfg.pusch.uci_offset.I_offset_ri = dl_ccch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded.pusch_cfg_ded.beta_offset_ri_idx;
                            // temp_ue_ul_cfg.ul_cfg.pucch.rnti = rnti;
                            // temp_ue_ul_cfg.ul_cfg.pucch.simul_cqi_ack = dl_ccch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded.cqi_report_cfg.cqi_report_periodic.setup().simul_ack_nack_and_cqi;
                            // temp_ue_ul_cfg.ul_cfg.pucch.tdd_ack_multiplex = (dl_ccch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded.pucch_cfg_ded.tdd_ack_nack_feedback_mode == asn1::rrc::pucch_cfg_ded_s::tdd_ack_nack_feedback_mode_e_::mux);
                            // ueDataBase->pushUE(rnti, temp_ue_dl_cfg, temp_ue_ul_cfg);
                            // return 0;
                            }
                            break;
                          
                          default:
                            break;
                          }
                          
                        }
                      }
                      break;

                      default:
                        
                      break;
                    }
                    
                  }
                }
                if (!find_ccch) {
                  std::cout << "No ccch in this msg" << std::endl;
                }

                // asn1::cbit_ref bref(pdsch_res[tb].payload+byteoffset, pdsch_cfg.grant.tb[tb].tbs / 8 - headersize - byteoffsettale);
                // srsran_pusch_cfg_t pusch_cfg;
                // srsran_pucch_cfg_t pucch_cfg;
                // if ((dlcch_msg.unpack(bref) != asn1::SRSASN_SUCCESS) || (dlcch_msg.msg.type().value != asn1::rrc::dl_ccch_msg_type_c::types_opts::c1)) {
                //     ERROR("Failed to unpack dl_ccch msg!");
                // } else {
                //     std::cout << "Msg 4 Decoded." << std::endl << std::endl;
                //     asn1::rrc::dl_dcch_msg_type_c::c1_c_::types::rrc_conn_recfg; 
                //     switch (dlcch_msg.msg.c1().type().value){
                //         case asn1::rrc::dl_ccch_msg_type_c::c1_c_::types::rrc_conn_reject:
                //     {
                //         std::cout << "Unfortunately, this is a rrc reject for this tc-rnti: " << rnti << std::endl;
                //         tcrntiDatabase->delete_tcRNTI(rnti);
                //         break;
                //     }

                //         case asn1::rrc::dl_ccch_msg_type_c::c1_c_::types::rrc_conn_setup:
                //     {
                //         std::cout << "Cool! This is a rrc setup! c-rnti: " << rnti << std::endl;
                //         tcrntiDatabase->delete_tcRNTI(rnti);
                    
                //         temp_ue_dl_cfg.cfg.pdsch.rnti = rnti;
                //         asn1::rrc::rrc_conn_setup_s rrc_set_up =  dlcch_msg.msg.c1().rrc_conn_setup();
                //         // asn1::rrc::phys_cfg_ded_s phys_cfg_ded = dlcch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded;
                //         // phys_cfg_ded.to_json(phywriter);
                //         rrc_set_up.to_json(phywriter);
                //         std::cout << phywriter.to_string().c_str() << std::endl;
                //         pusch_cfg.uci_offset.I_offset_ack = rrc_set_up.crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.pusch_cfg_ded.beta_offset_ack_idx;
                //         pusch_cfg.uci_offset.I_offset_cqi = rrc_set_up.crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.pusch_cfg_ded.beta_offset_cqi_idx;
                //         pucch_cfg.I_sr = rrc_set_up.crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.sched_request_cfg.set_setup().sr_cfg_idx;
                //         // asn1::SRSASN_CODE err = dlcch_msg.unpack(dlcch_bref);
                //         // if (err == asn1::SRSASN_SUCCESS) {
                //         //     asn1::rrc::phys_cfg_ded_s phys_cfg_ded = dlcch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded;
                //         //     phys_cfg_ded.to_json(phys_ded_json);
                //         //     std::cout << phys_ded_json.to_string() << std::endl;
                //         // }
                //     // temp_ue_dl_cfg.cfg.tm = (srsran_tm_t)dlcch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.ant_info_r10.get()->explicit_value_r10().tx_mode_r10.to_number();
                    
                //         break;
                //     }

                //         case asn1::rrc::dl_ccch_msg_type_c::c1_c_::types::rrc_conn_reest_reject:
                //     {
                //         std::cout << "Unfortunately, this is a rrc re-establishment reject for this tc-rnti: " << rnti << std::endl;
                //         tcrntiDatabase->delete_tcRNTI(rnti);
                //         break;
                //     }

                //         case asn1::rrc::dl_ccch_msg_type_c::c1_c_::types::rrc_conn_reest:
                //     {
                //         std::cout << "Cool! This is a rrc re-establishment! c-rnti: " << rnti << std::endl;
                //         tcrntiDatabase->delete_tcRNTI(rnti);
                //         temp_ue_dl_cfg.cfg.pdsch.rnti = rnti;
                //         asn1::rrc::rrc_conn_reest_s rrc_reest = dlcch_msg.msg.c1().rrc_conn_reest();
                //         // asn1::rrc::phys_cfg_ded_s phys_cfg_ded = dlcch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded;
                //         // phys_cfg_ded.to_json(phywriter);
                //         // rrc_reest.to_json(phywriter);
                //         dlcch_msg.msg.to_json(phywriter);
                //         std::cout << phywriter.to_string().c_str() << std::endl;
                //     // asn1::rrc::phys_cfg_ded_s phys_cfg_ded = dlcch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded;
                //     // phys_cfg_ded.unpack(phys_cbref);
                //         // asn1::SRSASN_CODE err = dlcch_msg.unpack(dlcch_bref);
                //         // if (err == asn1::SRSASN_SUCCESS) {
                //         //     asn1::rrc::phys_cfg_ded_s phys_cfg_ded = dlcch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded;
                //         //     phys_cfg_ded.to_json(phys_ded_json);
                //         //     std::cout << phys_ded_json.to_string() << std::endl;
                //         // }
                //     // temp_ue_dl_cfg.cfg.tm = (srsran_tm_t)dlcch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded.
                //         break;
                //     }
            
                //     default:
                //     {
                //         break;
                //     }
                // }
                // }


                
                // asn1::cbit_ref msg4pdubref(msg4pdu.write_packet(),  )

                // asn1::cbit_ref dlcch_bref(pdsch_res[tb].payload, pdsch_cfg.grant.tb[tb].tbs / 8);
                // asn1::cbit_ref dldcch_bref(pdsch_res[tb].payload, pdsch_cfg.grant.tb[tb].tbs / 8);
                // asn1::SRSASN_CODE err = dlcch_msg.unpack(dlcch_bref);
                // asn1::SRSASN_CODE err1 = dldcch_msg.unpack(dldcch_bref);
                // if (err1 != SRSRAN_SUCCESS) {
                //     ERROR("Failed to unpack DL-DCCH message!");
                //     tcrntiDatabase->delete_tcRNTI(rnti);
                //     return 0;
                // } 

                // auto& mac_logger = srslog::fetch_basic_logger("MAC");

                // asn1::rrc::rrc_conn_setup_s rrc_setup = dlcch_msg.msg.c1().rrc_conn_setup();
                
                std::cout << std::endl;
              }
            // }
               
                
        }
        if (ret >= 0) {
            ret = srsran_ue_dl_find_ul_dci(ue_dl, dl_sf_cfg, ue_dl_cfg, rnti, dci_ul);
            if (ret > 0) {
                puschSche->pushULDCI(tti, dl_sf_cfg->tdd_config.sf_config, rnti, dci_ul[0]);
            }
        }
    }
    return 0;
}


int SFDecoder::execute(srsran_ue_dl_t* ue_dl, cf_t** sf_buffer, srsran_ue_dl_cfg_t* ue_dl_cfg, srsran_dl_sf_cfg_t* dl_sf_cfg, srsran_pdsch_cfg_t* pdsch_cfg) {
    int ret = 0;
    if (srsran_sfidx_tdd_type(dl_sf_cfg->tdd_config, sf_idx) == SRSRAN_TDD_SF_U) {
        return 0;
    }
    // std::cout << "In SFDecoder!" << std::endl;

    uint8_t* data[SRSRAN_MAX_CODEWORDS];
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        data[i] = srsran_vec_u8_malloc(2000 * 8);
    }
    bool acks[SRSRAN_MAX_CODEWORDS] = {false};
    dl_sf_cfg->tti = tti;

    ret = srsran_ue_dl_find_and_decode(ue_dl, dl_sf_cfg, ue_dl_cfg, pdsch_cfg, data, acks);
    if (ret) {
      std::cout << "ret = " << ret << ", RNTI = " << pdsch_cfg->rnti << std::endl;
    }

    // for (int i = 0x0001; i <= 0x0960; i++){
    //     pdsch_cfg->rnti = i;
    //     ret = srsran_ue_dl_find_and_decode(ue_dl, dl_sf_cfg, ue_dl_cfg, pdsch_cfg, data, acks);
    //     if (ret > 0) {
    //         std:: cout << "RNTI = " << i << std::endl;
    //     }
    // }
    // ngscope_config->ngscope_config_mutex.lock();
    // pdsch_cfg->rnti = ngscope_config->RNTI;
    // ngscope_config->ngscope_config_mutex.unlock();

    // ret = srsran_ue_dl_find_and_decode(ue_dl, dl_sf_cfg, ue_dl_cfg, pdsch_cfg, data, acks);
    // if (ret > 0) {
    //     // std::cout << "ret = " << ret << std::endl;
    //     std::cout << "MCS = " << pdsch_cfg->grant.tb[0].mcs_idx << std::endl;
    // }
    return ret;
    
    // std::cout << "Test sfdecoder, tti = " << tti << std::endl;
}


void SFDecoder::computerPower(const cf_t *sf_symbols) {
    std::fill(rb_power.begin(), rb_power.end(), 0);

    for (uint32_t j = 0; j < 14; j++) {
        for (uint32_t i = 0; i < nof_prb; i++) {
            rb_power[i] += srsran_vec_avg_power_cf(&sf_symbols[i*12+j*(12*nof_prb)], 12);
        }
    }

    max_power = -std::numeric_limits<float>::max();
    min_power = std::numeric_limits<float>::min();
    const float logDivBy = 10*(log10f(14));
    for (uint32_t i = 0; i < nof_prb; i++) {
        rb_power[i] = 10 * log10f(rb_power[i]) - logDivBy;
        if (rb_power[i] > max_power) {
            max_power = rb_power[i];
            max_power_idx = i;
        }
        if (rb_power[i] < min_power) {
            min_power = rb_power[i];
            min_power_idx = i;
        }
    }
}


int SFDecoder::execute_ul(srsran_cell_t cell, srsran_ue_dl_t* ue_dl, srsran_ue_ul_t* ue_ul, cf_t** sf_buffer, srsran_ue_dl_cfg_t* ue_dl_cfg, srsran_dl_sf_cfg_t* dl_sf_cfg, 
srsran_ul_sf_cfg_t *sf_ul, srsran_ue_ul_cfg_t *ue_ul_cfg, srsran_pdsch_cfg_t* pdsch_cfg, srsran_dl_cfg_t* dl_cfg, srsran_ul_cfg_t* ul_cfg, srsran_enb_ul_t* enb_ul, bool* have_sib1, bool* have_sib2) 
{
    // std::cout << "In SF Decoder!" << std::endl;
    int tartti;
    int ret = 0;
    if (srsran_sfidx_tdd_type(dl_sf_cfg->tdd_config, sf_idx) == SRSRAN_TDD_SF_U) {
        return 0;
    }
    // ngscope_config->ngscope_config_mutex.lock();
    // pdsch_cfg->rnti = ngscope_config->RNTI;
    // ngscope_config->ngscope_config_mutex.unlock();
    // std::cout << "In SFDecoder!" << std::endl;

    uint8_t* data[SRSRAN_MAX_CODEWORDS];
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        data[i] = srsran_vec_u8_malloc(2000 * 8);
    }
    bool acks[SRSRAN_MAX_CODEWORDS] = {false};
    dl_sf_cfg->tti = tti;

    // for (int i = 0x0001; i <= 0x0960; i++){
    //     pdsch_cfg->rnti = i;
    //     ret = srsran_ue_dl_find_and_decode(ue_dl, dl_sf_cfg, ue_dl_cfg, pdsch_cfg, data, acks);
    //     if (ret > 0) {
    //         std:: cout << "RNTI = " << i << std::endl;
    //     }
    // }

    // ret = srsran_ue_dl_find_and_decode(ue_dl, dl_sf_cfg, ue_dl_cfg, pdsch_cfg, data, acks);
    // if (srsran_sfidx_tdd_type(dl_sf_cfg->tdd_config, sf_idx) == SRSRAN_TDD_SF_D) {
        
    // }
    // if (srsran_sfidx_tdd_type(dl_sf_cfg->tdd_config, sf_idx) != SRSRAN_TDD_SF_U) {
    //   ret = srsran_ue_dl_find_and_decode_pdsch_ul_grant(ue_dl, ue_ul, dl_sf_cfg, sf_ul, ue_dl_cfg, ue_ul_cfg, pdsch_cfg, pusch_cfg, data, acks);
    //   if (ret > 0) {
    //   }
    // }
    //     if (ret > 0) {
    //       // std::cout << "ret = " << ret << std::endl;
    //       // std::cout << "MCS = " << pdsch_cfg->grant.tb[0].mcs_idx << std::endl;
    //     }
    // int tddconfig = dl_sf_cfg->tdd_config.sf_config;
    // std::cout << "tdd config: " << tddconfig << std::endl;
    // srsran_ul_cfg_t ul_cfg;
    // ZERO_OBJECT(ul_cfg);
    // if (srsran_sfidx_tdd_type(dl_sf_cfg->tdd_config, sf_idx) == SRSRAN_TDD_SF_U) {
    //     return 0;
    // }
    // std::cout << "In UL Mode Sfdecoder!" << std::endl;
    
    // if (!*enb_ul_init) {
    //     if (sfWorkerUlInit(*pdsch_cfg, *dl_sf_cfg, *ue_dl, *ue_dl_cfg, enb_ul, dmrs_pusch_cfg)) {
    //        if (srsran_enb_ul_set_cell(enb_ul, cell, dmrs_pusch_cfg, srs_cfg)) {
    //         ERROR("Error initiating enb uplink cell");
    //       }
    //     } else {
    //       return 0;
    //     }
    // }
        // try
        // {
        //     asn1::rrc::sib_type2_s sib2 = ngscope_config->getSib2();
        // }
        // catch(const std::exception& e)
        // {
        //     std::cerr << "wrong" << '\n';
        // }
        
        // if (ngscope_config) {
        //     asn1::rrc::sib_type2_s sib2 = ngscope_config->getSib2();
        // ngscope_config->ngscope_config_mutex.lock();
        // ngscope_config->ngscope_config_mutex.unlock();

        // ul_cfg.dmrs.cyclic_shift = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.cyclic_shift;
        // ul_cfg.dmrs.group_hopping_en = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.group_hop_enabled;
        // ul_cfg.dmrs.sequence_hopping_en = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.seq_hop_enabled;
        // ul_cfg.dmrs.delta_ss = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.group_assign_pusch;

        // ul_cfg.hopping.hop_mode = srsran_pusch_hopping_cfg_t::SRSRAN_PUSCH_HOP_MODE_INTER_SF;
        // ul_cfg.hopping.hopping_offset = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.pusch_hop_offset;
        // ul_cfg.hopping.n_sb = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.n_sb;
        // ul_cfg.hopping.n_rb_ho = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.pusch_hop_offset;

        // *dmrs_pusch_cfg = ul_cfg.dmrs;

        // }
       
    //     std::cout << "Finish enb ul init!" << std::endl;
    //     *enb_ul_init = true;
    // }


    // pdsch_cfg->rnti = 0x84;
    // dl_sf_cfg->tti = tti;
    // dl_sf_cfg->tdd_config.configured = true;
    // sf_ul->tti = tti;

    // puschGrant_t puschGrant;
    // std::cout << sf_idx << std::endl;
    
    // if ((srsran_sfidx_tdd_type(dl_sf_cfg->tdd_config, sf_idx) == SRSRAN_TDD_SF_D) || (srsran_sfidx_tdd_type(dl_sf_cfg->tdd_config, sf_idx) == SRSRAN_TDD_SF_S)) {
        // std::cout << "In DL sf" << std::endl;
      // ret = srsran_ue_dl_find_and_decode_pdsch_ul_grant(ue_dl, ue_ul, dl_sf_cfg, sf_ul, ue_dl_cfg, ue_ul_cfg, pdsch_cfg, pusch_cfg, data, acks);
      // if (ret) {
      //   std::cout << "Got!" << std::endl;
      // }

        // if (ret > 0) {
        //     puschGrant.pusch_grant = pusch_cfg->grant;
        //     puschGrant.target_tti = grantTargetTti(tti, tddconfig);
        //     ngscope_config->ngscope_config_mutex.lock();
        //     ngscope_config->puschGrantPush(puschGrant);
        //     ngscope_config->ngscope_config_mutex.unlock();
        // }
    // } else if (srsran_sfidx_tdd_type(dl_sf_cfg->tdd_config, sf_idx) == SRSRAN_TDD_SF_U) {
        // std::cout << "In ul decoding..." << std::endl;
        // ngscope_config->ngscope_config_mutex.lock();
        // puschGrant = ngscope_config->puschGrantPop(tti);
        // ngscope_config->ngscope_config_mutex.unlock();

        // ret = decode_pusch(pusch_cfg, puschGrant.pusch_grant, enb_ul, sf_ul, pdsch_cfg->rnti);
    // }
    // for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
    //     if (data[i]) {
    //       free(data[i]);
    //     }
    // }
    return ret;

}


int decode_pdcch_and_pdsch(srsran_ue_dl_t* ue_dl, srsran_ue_ul_t* ue_ul, srsran_dl_sf_cfg_t* dl_sf_cfg, srsran_ul_sf_cfg_t* ul_sf_cfg, srsran_ue_dl_cfg_t* ue_dl_cfg, srsran_ue_ul_cfg_t* ue_ul_cfg,
    srsran_dl_cfg_t* dl_cfg, srsran_ul_cfg_t* ul_cfg, int* ret_dl, int* ret_ul) 
{
    int ret_dl_tmp = SRSRAN_ERROR;
    int ret_ul_tmp = SRSRAN_ERROR;
    uint16_t rnti = ue_dl_cfg->cfg.pdsch.rnti;
    // std::cout << "rnti = " << rnti << std::endl;

    srsran_dci_dl_t dci_dl[SRSRAN_MAX_DCI_MSG];
    ZERO_OBJECT(dci_dl);
    srsran_dci_ul_t dci_ul[SRSRAN_MAX_DCI_MSG];
    ZERO_OBJECT(dci_ul);
    srsran_pdsch_res_t pdsch_res[SRSRAN_MAX_CODEWORDS];
    ZERO_OBJECT(pdsch_res);
    
    srsran_ue_dl_set_mi_auto(ue_dl);
    if (srsran_ue_dl_decode_fft_estimate(ue_dl, dl_sf_cfg, ue_dl_cfg) < 0) {
        return SRSRAN_ERROR;
    }

    ret_dl_tmp = srsran_ue_dl_find_dl_dci(ue_dl, dl_sf_cfg, ue_dl_cfg, rnti, dci_dl);
    // std::cout << "Number of DCI messages for pdsch: " << ret_dl_tmp << std::endl;

    if (ret_dl_tmp > 0) {
        for (int dci_idx = 0; dci_idx < ret_dl_tmp; dci_idx++) {
            char str[512];
            srsran_dci_dl_info(&dci_dl[dci_idx], str, 512);
            std::cout << "Downlink PDCCH:" << str << ", SNR = " << ue_dl->chest_res.snr_db << std::endl;

            if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[dci_idx], &ue_dl_cfg->cfg.pdsch.grant)) {
                ERROR("Error unpacking dl DCI!");
            }
        }

        uint8_t* data[SRSRAN_MAX_CODEWORDS];
        for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
            data[i] = srsran_vec_u8_malloc(2000 * 8);
        }
        bool acks[SRSRAN_MAX_CODEWORDS] = {false};
        for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
            if (dl_cfg->pdsch.grant.tb[i].enabled) {
                if (dl_cfg->pdsch.grant.tb[i].rv < 0) {
                    uint32_t sfn = dl_sf_cfg->tti / 10;
                    uint32_t k = (sfn / 2) % 4;
                    dl_cfg->pdsch.grant.tb[i].rv = ((uint32_t)ceilf((float)1.5 * k)) % 4;
                }
                srsran_softbuffer_rx_reset_tbs(dl_cfg->pdsch.softbuffers.rx[i], (uint32_t)dl_cfg->pdsch.grant.tb[i].tbs);
            }
        }

        bool decode_enable = false;
        for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
            if (dl_cfg->pdsch.grant.tb[tb].enabled) {
                decode_enable         = true;
                pdsch_res[tb].payload = data[tb];
                pdsch_res[tb].crc     = false;
            }
        }

        if (decode_enable) {
            if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, &ue_dl_cfg->cfg.pdsch, pdsch_res)) {
              ERROR("ERROR: Decoding PDSCH");
              // ret = -1;
            }
            // pmch_cfg.pdsch_cfg = *pdsch_cfg;
            // if (srsran_ue_dl_decode_pmch(q, sf, &pmch_cfg, pdsch_res)) {
            //   ERROR("Decoding PMCH");
            //   // ret = -1;
            // }
            // auto& mac_logger = srslog::fetch_basic_logger("MAC");
            // srsran::sch_pdu mac_msg_dl(10, mac_logger);
            // mac_msg_dl.init_rx(pdsch_cfg->grant.tb[0].nof_bits / 8, false);
            // mac_msg_dl.parse_packet(pdsch_res->payload);
            // while(mac_msg_dl.next()) {
            //     uint32_t buff_size_idx[4]   = {};
            //     uint32_t buff_size_bytes[4] = {};
            //     if (mac_msg_dl.get()) {
            //         std::cout << "Decoding Downlink PDU header, lcid = " << int(mac_msg_dl.get()->dl_sch_ce_type()) << ", nof_subheaders = " << mac_msg_dl.nof_subh() << std::endl;
            //         switch (mac_msg_dl.get()->dl_sch_ce_type())
            //         {
            //         case srsran::dl_sch_lcid::CCCH:
            //             break;
            //         case srsran::dl_sch_lcid::RESERVED:
            //             break;
            //         case srsran::dl_sch_lcid::SCELL_ACTIVATION:
            //             break;
            //         case srsran::dl_sch_lcid::CON_RES_ID:
            //             break;
            //         case srsran::dl_sch_lcid::TA_CMD:
            //             std::cout << "It's timing advance command, " << mac_msg_dl.get()->get_ta_cmd() << std::endl;
            //             break;
            //         case srsran::dl_sch_lcid::DRX_CMD:
            //             break;
            //         case srsran::dl_sch_lcid::PADDING:
            //             break;
                    
            //         default:
            //             break;
            //         }
            //     }
            // }
        }

        for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
            if (dl_cfg->pdsch.grant.tb[tb].enabled) {
                acks[tb] = pdsch_res[tb].crc;
            }
        }
    }
    return 0;
}


int srsran_ue_dl_find_and_decode_pdsch_ul_grant(srsran_ue_dl_t *q, srsran_ue_ul_t *q_ul, srsran_dl_sf_cfg_t *sf, srsran_ul_sf_cfg_t *sf_ul, srsran_ue_dl_cfg_t *ue_dl_cfg, srsran_ue_ul_cfg_t *ue_ul_cfg, 
srsran_pdsch_cfg_t *pdsch_cfg, srsran_pusch_cfg_t *pusch_cfg, uint8_t *data[SRSRAN_MAX_CODEWORDS], bool acks[SRSRAN_MAX_CODEWORDS])
{
  // int ret = SRSRAN_ERROR;
  int ret_dl = SRSRAN_ERROR;
  int ret_ul = SRSRAN_ERROR;

  srsran_dci_dl_t    dci_dl[SRSRAN_MAX_DCI_MSG];
  ZERO_OBJECT(dci_dl);
  srsran_dci_ul_t    dci_ul[SRSRAN_MAX_DCI_MSG];
  ZERO_OBJECT(dci_ul);
  srsran_pmch_cfg_t  pmch_cfg;
  srsran_pdsch_res_t pdsch_res[SRSRAN_MAX_CODEWORDS];
  ZERO_OBJECT(pdsch_res);

  // Use default values for PDSCH decoder
  ZERO_OBJECT(pmch_cfg);

  uint32_t mi_set_len;
  if (q->cell.frame_type == SRSRAN_TDD && !sf->tdd_config.configured) {
    mi_set_len = 3;
  } else {
    mi_set_len = 1;
  }

  // Blind search PHICH mi value
//   std::cout << "tdd config: " << sf->tdd_config.configured << ", mi_set_len = " << mi_set_len <<std::endl;
  ret_dl = 0;
  srsran_ue_dl_set_mi_auto(q);
  if (srsran_ue_dl_decode_fft_estimate(q, sf, ue_dl_cfg) < 0) {
      return SRSRAN_ERROR;
      // return ret_dl;
  }

  ret_dl = srsran_ue_dl_find_dl_dci(q, sf, ue_dl_cfg, pdsch_cfg->rnti, dci_dl);
  std::cout << "first ret_dl = " << ret_dl << std::endl;

  for (uint32_t i = 0; i < mi_set_len && !ret_dl; i++) {
//   for (uint32_t i = 0; i < mi_set_len && !(ret_dl && ret_ul); i++) {
    if (mi_set_len == 1) {
      srsran_ue_dl_set_mi_auto(q);
    } else {
      srsran_ue_dl_set_mi_manual(q, i);
    }

    if (srsran_ue_dl_decode_fft_estimate(q, sf, ue_dl_cfg) < 0) {
      continue;
      // return ret_dl;
    } 

    ret_dl = srsran_ue_dl_find_dl_dci(q, sf, ue_dl_cfg, pdsch_cfg->rnti, dci_dl);
    std::cout << "ret_dl = " << ret_dl << std::endl;
    if (ret_dl > 0) {
      // std::cout << "ret_dl = " << ret_dl << std::endl;
      for (int dci_idx = 0; dci_idx < ret_dl; dci_idx++) {
        char str[512];
        srsran_dci_dl_info(&dci_dl[dci_idx], str, 512);
        INFO("PDCCH: %s, snr=%.1f dB", str, q->chest_res.snr_db);
        printf("Downlink PDCCH: %s, snr=%.1f dB\n", str, q->chest_res.snr_db);

        if (srsran_ue_dl_dci_to_pdsch_grant(q, sf, ue_dl_cfg, &dci_dl[dci_idx], &pdsch_cfg->grant)) {
          ERROR("Error unpacking DCI");
          // return SRSRAN_ERROR;
        }

        // Calculate RV if not provided in the grant and reset softbuffer
        for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
          if (pdsch_cfg->grant.tb[i].enabled) {
            if (pdsch_cfg->grant.tb[i].rv < 0) {
              uint32_t sfn              = sf->tti / 10;
              uint32_t k                = (sfn / 2) % 4;
              pdsch_cfg->grant.tb[i].rv = ((uint32_t)ceilf((float)1.5 * k)) % 4;
            }
            srsran_softbuffer_rx_reset_tbs(pdsch_cfg->softbuffers.rx[i], (uint32_t)pdsch_cfg->grant.tb[i].tbs);
          }
        }

        bool decode_enable = false;
        for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
          if (pdsch_cfg->grant.tb[tb].enabled) {
            decode_enable         = true;
            pdsch_res[tb].payload = data[tb];
            pdsch_res[tb].crc     = false;
          }
        }

        if (decode_enable) {
            if (srsran_ue_dl_decode_pdsch(q, sf, pdsch_cfg, pdsch_res)) {
              ERROR("ERROR: Decoding PDSCH");
              // ret = -1;
            }
            // pmch_cfg.pdsch_cfg = *pdsch_cfg;
            // if (srsran_ue_dl_decode_pmch(q, sf, &pmch_cfg, pdsch_res)) {
            //   ERROR("Decoding PMCH");
            //   // ret = -1;
            // }
            auto& mac_logger = srslog::fetch_basic_logger("MAC");
            srsran::sch_pdu mac_msg_dl(10, mac_logger);
            mac_msg_dl.init_rx(pdsch_cfg->grant.tb[0].nof_bits / 8, false);
            mac_msg_dl.parse_packet(pdsch_res->payload);
            while(mac_msg_dl.next()) {
                uint32_t buff_size_idx[4]   = {};
                uint32_t buff_size_bytes[4] = {};
                if (mac_msg_dl.get()) {
                    std::cout << "Decoding Downlink PDU header, lcid = " << int(mac_msg_dl.get()->dl_sch_ce_type()) << ", nof_subheaders = " << mac_msg_dl.nof_subh() << std::endl;
                    switch (mac_msg_dl.get()->dl_sch_ce_type())
                    {
                    case srsran::dl_sch_lcid::CCCH:
                        break;
                    case srsran::dl_sch_lcid::RESERVED:
                        break;
                    case srsran::dl_sch_lcid::SCELL_ACTIVATION:
                        break;
                    case srsran::dl_sch_lcid::CON_RES_ID:
                        break;
                    case srsran::dl_sch_lcid::TA_CMD:
                        std::cout << "It's timing advance command, " << mac_msg_dl.get()->get_ta_cmd() << std::endl;
                        break;
                    case srsran::dl_sch_lcid::DRX_CMD:
                        break;
                    case srsran::dl_sch_lcid::PADDING:
                        break;
                    
                    default:
                        break;
                    }
                }
            }
        }

        for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
          if (pdsch_cfg->grant.tb[tb].enabled) {
            acks[tb] = pdsch_res[tb].crc;
          }
        }
      }
    }

    ret_ul = srsran_ue_dl_find_ul_dci(q, sf, ue_dl_cfg, pdsch_cfg->rnti, dci_ul);
    if (ret_ul > 0) {
    //   std::cout << "ret_ul = " << ret_ul << std::endl;
      for (int dci_idx = 0; dci_idx < ret_ul; dci_idx++) {
        char str[512];
        srsran_dci_ul_info(&dci_ul[dci_idx], str, 512);
        INFO("PDCCH UL: %s", str);
        // printf("PDCCH UL: %s\n", str);

        if (srsran_ue_ul_dci_to_pusch_grant(q_ul, sf_ul, ue_ul_cfg, &dci_ul[dci_idx], &pusch_cfg->grant)) {
          ERROR("Error unpacking DCI");
          // return SRSRAN_ERROR;
        } 
      }
    // std::cout << "here ret_ul = " << ret_ul << std::endl;
    return ret_ul;
    }
    // return ret_ul;
  }

  // if ((ret_dl > 0) && (ret_ul > 0)) {
  // if (ret_dl) {
  //   // ret = 1;

  //   for (int dci_idx = 0; dci_idx < ret_dl; dci_idx++) {
  //     char str[512];
  //     srsran_dci_dl_info(&dci_dl[dci_idx], str, 512);
  //     INFO("PDCCH: %s, snr=%.1f dB", str, q->chest_res.snr_db);
  //     printf("PDCCH: %s, snr=%.1f dB\n", str, q->chest_res.snr_db);

  //     if (srsran_ue_dl_dci_to_pdsch_grant(q, sf, cfg, &dci_dl[dci_idx], &pdsch_cfg->grant)) {
  //       ERROR("Error unpacking DCI");
  //       // return SRSRAN_ERROR;
  //     }

  //     // Calculate RV if not provided in the grant and reset softbuffer
  //     for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
  //       if (pdsch_cfg->grant.tb[i].enabled) {
  //         if (pdsch_cfg->grant.tb[i].rv < 0) {
  //           uint32_t sfn              = sf->tti / 10;
  //           uint32_t k                = (sfn / 2) % 4;
  //           pdsch_cfg->grant.tb[i].rv = ((uint32_t)ceilf((float)1.5 * k)) % 4;
  //         }
  //         srsran_softbuffer_rx_reset_tbs(pdsch_cfg->softbuffers.rx[i], (uint32_t)pdsch_cfg->grant.tb[i].tbs);
  //       }
  //     }

  //     bool decode_enable = false;
  //     for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
  //       if (pdsch_cfg->grant.tb[tb].enabled) {
  //         decode_enable         = true;
  //         pdsch_res[tb].payload = data[tb];
  //         pdsch_res[tb].crc     = false;
  //       }
  //     }

  //     if (decode_enable) {
  //       if (sf->sf_type == SRSRAN_SF_NORM) {
  //         if (srsran_ue_dl_decode_pdsch(q, sf, pdsch_cfg, pdsch_res)) {
  //           ERROR("ERROR: Decoding PDSCH");
  //           // ret = -1;
  //         }
  //       } else {
  //         pmch_cfg.pdsch_cfg = *pdsch_cfg;
  //         if (srsran_ue_dl_decode_pmch(q, sf, &pmch_cfg, pdsch_res)) {
  //           ERROR("Decoding PMCH");
  //           // ret = -1;
  //         }
  //       }
  //     }

  //     for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
  //       if (pdsch_cfg->grant.tb[tb].enabled) {
  //         acks[tb] = pdsch_res[tb].crc;
  //       }
  //     }
  //   }
    
  // }
  
  // std::cout << "here ret_ul = " << ret_ul << std::endl;
  // if (ret_ul) {
  //   // std::cout << "here ret_ul = " << ret_ul << std::endl;
  //   for (int dci_idx = 0; dci_idx < ret_ul; dci_idx++) {
  //     char str[512];
  //     srsran_dci_ul_info(&dci_ul[dci_idx], str, 512);
  //     INFO("PDCCH UL: %s", str);
  //     printf("PDCCH UL: %s\n", str);

  //     if (srsran_ue_ul_dci_to_pusch_grant(q_ul, sf_ul, cfg_ul, &dci_ul[dci_idx], &pusch_cfg->grant)) {
  //       ERROR("Error unpacking DCI");
  //       // return SRSRAN_ERROR;
  //     } 
  //   }
    
  // }

    
    // printf("PDCCH UL: %s\n", str);
    // Logging
    // if (SRSRAN_DEBUG_ENABLED && get_srsran_verbose_level() >= SRSRAN_VERBOSE_INFO) {
    //   char str[512];
    //   srsran_dci_dl_info(&dci_dl[0], str, 512);
    //   INFO("PDCCH: %s, snr=%.1f dB", str, q->chest_res.snr_db);
    //   printf("PDCCH: %s, snr=%.1f dB\n", str, q->chest_res.snr_db);

    //   srsran_dci_ul_info(&dci_ul[0], str, 512);
    //   INFO("PDCCH UL: %s", str);
    //   printf("PDCCH UL: %s\n", str);
    // }

    // Convert DCI message to DL grant
    
  // return ret_ul;
  return ret_ul;
}


uint32_t grantTargetTti(uint32_t curtti, int tddconfig) {
    int sf_idx = curtti % 10;
    // return (curtti + 4) % 10240;
    if (tddconfig == -1){ //FDD mode
        return (curtti + 4) % 10240;
    } else {//TDD mode
        if (tddconfig == 0) {
            if (sf_idx == 0) {
                  return (curtti + 4) % 10240;
            } else if (sf_idx == 1) {
                  return (curtti + 6) % 10240;
            } else if (sf_idx == 5) {
                  return (curtti + 4) % 10240;
            } else if (sf_idx == 6) {
                  return (curtti + 6) % 10240;
            }
        } else if (tddconfig == 1) {
            if (sf_idx == 1) {
                  return (curtti + 6) % 10240;
            } else if (sf_idx == 4) {
                  return (curtti + 4) % 10240;
            } else if (sf_idx == 6) {
                  return (curtti + 6) % 10240;
            } else if (sf_idx == 9) {
                  return (curtti + 4) % 10240;
            } 
        } else if (tddconfig == 2) {
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


int decode_pdsch(srsran_ue_dl_t*     q,
                                 srsran_dl_sf_cfg_t* sf,
                                 srsran_ue_dl_cfg_t* cfg,
                                 srsran_pdsch_cfg_t* pdsch_cfg,
                                 uint8_t*            data[SRSRAN_MAX_CODEWORDS],
                                 bool                acks[SRSRAN_MAX_CODEWORDS])
{
  int ret = SRSRAN_ERROR;

  srsran_dci_dl_t    dci_dl[SRSRAN_MAX_DCI_MSG] = {};
  srsran_pmch_cfg_t  pmch_cfg;
  srsran_pdsch_res_t pdsch_res[SRSRAN_MAX_CODEWORDS];

  // Use default values for PDSCH decoder
  ZERO_OBJECT(pmch_cfg);

  uint32_t mi_set_len;
  if (q->cell.frame_type == SRSRAN_TDD && !sf->tdd_config.configured) {
    mi_set_len = 3;
  } else {
    mi_set_len = 1;
  }

  // Blind search PHICH mi value
  ret = 0;
  for (uint32_t i = 0; i < mi_set_len && !ret; i++) {
    if (mi_set_len == 1) {
      srsran_ue_dl_set_mi_auto(q);
    } else {
      srsran_ue_dl_set_mi_manual(q, i);
    }

    if ((ret = srsran_ue_dl_decode_fft_estimate(q, sf, cfg)) < 0) {
      return ret;
    }
  
    ret = srsran_ue_dl_find_dl_dci(q, sf, cfg, pdsch_cfg->rnti, dci_dl);
    // printf("dci ret = %d\n", ret);
  }

  if (ret) {
    // Logging
    printf("dci ret = %d\n", ret);
    // if (SRSRAN_DEBUG_ENABLED && get_srsran_verbose_level() >= SRSRAN_VERBOSE_INFO) {
      char str[512];
      srsran_dci_dl_info(&dci_dl[0], str, 512);
      INFO("PDCCH: %s, snr=%.1f dB", str, q->chest_res.snr_db);
      printf("PDCCH: %s, snr=%.1f dB\n", str, q->chest_res.snr_db);
    // }

    // Force known MBSFN grant
    if (sf->sf_type == SRSRAN_SF_MBSFN) {
      dci_dl[0].rnti                    = SRSRAN_MRNTI;
      dci_dl[0].alloc_type              = SRSRAN_RA_ALLOC_TYPE0;
      dci_dl[0].type0_alloc.rbg_bitmask = 0xffffffff;
      dci_dl[0].tb[0].rv                = 0;
      dci_dl[0].tb[0].mcs_idx           = 2;
      dci_dl[0].format                  = SRSRAN_DCI_FORMAT1;
    }

    // Convert DCI message to DL grant
    if (srsran_ue_dl_dci_to_pdsch_grant(q, sf, cfg, &dci_dl[0], &pdsch_cfg->grant)) {
      ERROR("Error unpacking DCI");
      return SRSRAN_ERROR;
    }

    // Calculate RV if not provided in the grant and reset softbuffer
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
      if (pdsch_cfg->grant.tb[i].enabled) {
        if (pdsch_cfg->grant.tb[i].rv < 0) {
          uint32_t sfn              = sf->tti / 10;
          uint32_t k                = (sfn / 2) % 4;
          pdsch_cfg->grant.tb[i].rv = ((uint32_t)ceilf((float)1.5 * k)) % 4;
        }
        srsran_softbuffer_rx_reset_tbs(pdsch_cfg->softbuffers.rx[i], (uint32_t)pdsch_cfg->grant.tb[i].tbs);
      }
    }

    bool decode_enable = false;
    for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
      if (pdsch_cfg->grant.tb[tb].enabled) {
        decode_enable         = true;
        pdsch_res[tb].payload = data[tb];
        pdsch_res[tb].crc     = false;
      }
    }

    if (decode_enable) {
      if (sf->sf_type == SRSRAN_SF_NORM) {
        if (srsran_ue_dl_decode_pdsch(q, sf, pdsch_cfg, pdsch_res)) {
          ERROR("ERROR: Decoding PDSCH");
          ret = -1;
        }
      } else {
        pmch_cfg.pdsch_cfg = *pdsch_cfg;
        if (srsran_ue_dl_decode_pmch(q, sf, &pmch_cfg, pdsch_res)) {
          ERROR("Decoding PMCH");
          ret = -1;
        }
      }
    }

    for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
      if (pdsch_cfg->grant.tb[tb].enabled) {
        acks[tb] = pdsch_res[tb].crc;
      }
    }

     auto& mac_logger = srslog::fetch_basic_logger("MAC");
            srsran::sch_pdu mac_msg_dl(10, mac_logger);
            mac_msg_dl.init_rx(pdsch_cfg->grant.tb[0].nof_bits / 8, false);
            mac_msg_dl.parse_packet(pdsch_res->payload);
            while(mac_msg_dl.next()) {
                uint32_t buff_size_idx[4]   = {};
                uint32_t buff_size_bytes[4] = {};
                if (mac_msg_dl.get()) {
                    std::cout << "Decoding Downlink PDU header, lcid = " << int(mac_msg_dl.get()->dl_sch_ce_type()) << ", nof_subheaders = " << mac_msg_dl.nof_subh() << std::endl;
                    switch (mac_msg_dl.get()->dl_sch_ce_type())
                    {
                    case srsran::dl_sch_lcid::CCCH:
                        break;
                    case srsran::dl_sch_lcid::RESERVED:
                        break;
                    case srsran::dl_sch_lcid::SCELL_ACTIVATION:
                        break;
                    case srsran::dl_sch_lcid::CON_RES_ID:
                        break;
                    case srsran::dl_sch_lcid::TA_CMD:
                        std::cout << "It's timing advance command, " << mac_msg_dl.get()->get_ta_cmd() << std::endl;
                        break;
                    case srsran::dl_sch_lcid::DRX_CMD:
                        break;
                    case srsran::dl_sch_lcid::PADDING:
                        break;
                    
                    default:
                        break;
                    }
                }
            }
  }
  return ret;
}


int decode_pucch(srsran_enb_ul_t* enb_ul, srsran_ul_sf_cfg_t* ul_sf, srsran_pucch_cfg_t* pucch_cfg, srsran_pucch_res_t* pucch_res) {
    srsran_pucch_format_t format;
    uint32_t nof_resources; 
    uint32_t n_pucch_i[SRSRAN_PUCCH_MAX_ALLOC] = {};
    int ret = -1;
    for (format = SRSRAN_PUCCH_FORMAT_1; format < SRSRAN_PUCCH_FORMAT_ERROR; format = static_cast<srsran_pucch_format_t>(static_cast<int>(format) + 1)) {
        pucch_cfg->format = format;
        for (nof_resources = 1; nof_resources <= SRSRAN_PUCCH_CS_MAX_ACK; nof_resources++) {
            for (int i = 0; i < nof_resources; i++) {
                *pucch_res = { };
                // Prepare configuration
                if (srsran_chest_ul_estimate_pucch(&enb_ul->chest, ul_sf, pucch_cfg, enb_ul->sf_symbols, &enb_ul->chest_res)) {
                    ERROR("Error estimating PUCCH DMRS");
                    return SRSRAN_ERROR;
                }

                pucch_res->snr_db    = enb_ul->chest_res.snr_db;
                pucch_res->rssi_dbFs = enb_ul->chest_res.epre_dBfs;
                pucch_res->ni_dbFs   = enb_ul->chest_res.noise_estimate_dbFs;

                ret = srsran_pucch_decode(&enb_ul->pucch, ul_sf, pucch_cfg, &enb_ul->chest_res, enb_ul->sf_symbols, pucch_res);
                if (ret < 0) {
                    break;
                } else {
                    char str[512];
                    srsran_pucch_rx_info(pucch_cfg, pucch_res, str, sizeof(str));
                    printf("Hahahaha Decoded PUCCH: %s\n", str);
                    std::cout << "Decoded PUCCH: format = " << pucch_cfg->format << ", SR = " << pucch_res->uci_data.scheduling_request << std::endl;
                }
            }
            if (ret < 0) {
                break;
            } else {
                continue;
            }
        }
        if (ret < 0) {
            continue;
        } else {
            break;
        }
    }
}


int decode_pusch(srsran_ul_cfg_t* ul_cfg, srsran_pusch_grant_t pusch_grant, srsran_enb_ul_t* enb_ul, srsran_ul_sf_cfg_t* ul_sf, uint32_t rnti) {
    srsran_pusch_res_t pusch_res = {};
    srsran_pucch_res_t pucch_res = {};
    srsran_pusch_cfg_t pusch_cfg = ul_cfg->pusch;
    srsran_pucch_cfg_t pucch_cfg = ul_cfg->pucch;
    int ret = -1;

    // pusch_cfg.rnti = rnti;
    // pusch_cfg.meas_ta_en = false;
    // pusch_cfg.grant = pusch_grant;
    // pusch_cfg.max_nof_iterations = 1000;
    // pusch_cfg.csi_enable = false;

    pusch_res.data = srsran_vec_u8_malloc(2000 * 8);

    srsran_softbuffer_rx_t* rx_softbuffers;
    rx_softbuffers = new srsran_softbuffer_rx_t;
    pusch_cfg.softbuffers.rx = rx_softbuffers;
    // srsran_softbuffer_rx_init(pusch_cfg.softbuffers.rx, SRSRAN_MAX_PRB);
    srsran_softbuffer_rx_init(pusch_cfg.softbuffers.rx, pusch_grant.tb.tbs);
    // srsran_vec_zero(pusch_cfg.softbuffers.rx, SRSRAN_MAX_PRB);

    srsran_enb_ul_fft(enb_ul);

    if (rnti == SRSRAN_INVALID_RNTI) {
        return SRSRAN_ERROR;
    }
    
    // pusch_cfg.uci_cfg.cqi.data_enable = true;
    ret = srsran_chest_ul_estimate_pusch(&enb_ul->chest, ul_sf, &pusch_cfg, enb_ul->sf_symbols, &enb_ul->chest_res);
    pusch_res.crc = false;
    if (ret == SRSRAN_SUCCESS) {
        std::cout << "Finish pusch chest!" << std::endl;
        ret = srsran_pusch_decode(&enb_ul->pusch, ul_sf, &pusch_cfg, &enb_ul->chest_res, enb_ul->sf_symbols, &pusch_res);
    }
    if (pusch_res.crc == false) {
        pusch_cfg.enable_64qam = true;
        srsran_chest_ul_estimate_pusch(&enb_ul->chest, ul_sf, &pusch_cfg, enb_ul->sf_symbols, &enb_ul->chest_res);
        ret = srsran_pusch_decode(&enb_ul->pusch, ul_sf, &pusch_cfg, &enb_ul->chest_res, enb_ul->sf_symbols, &pusch_res);
    }
    // ret = srsran_enb_ul_get_pusch(enb_ul, ul_sf, &pusch_cfg, &pusch_res);
    printf("pusch ret = %d\n", ret);
    // std::cout << "ACK = " << pusch_res.uci.ack.ack_value << std::endl;
    // std::cout << "UCI: SR = " << pusch_res.uci.scheduling_request << std::endl;
    // std::cout <<"RI = " << pusch_res.uci.ri << std::endl;
    // std::cout << "CQI wideband: " << pusch_res.uci.cqi.wideband.wideband_cqi << " | " << pusch_res.uci.cqi.wideband.pmi << " | " << pusch_res.uci.cqi.wideband.spatial_diff_cqi << std::endl;
    // std::cout << "CQI UE: " << pusch_res.uci.cqi.subband_ue.subband_label << " | " << pusch_res.uci.cqi.subband_ue.subband_cqi << std::endl;
    // std::cout << "CQI hl sub cw0: " << pusch_res.uci.cqi.subband_hl.wideband_cqi_cw0 << " | " << "CQI hl sub cw1: " << pusch_res.uci.cqi.subband_hl.subband_diff_cqi_cw1 << std::endl;
    char str[512];
    srsran_pusch_rx_info(&pusch_cfg, &pusch_res, &enb_ul->chest_res, str, sizeof(str));
    printf("Haha Decoded PUSCH: %s\n", str);
    
    // auto& mac_logger = srslog::fetch_basic_logger("MAC");

    // srsran::sch_pdu mac_msg_ul(10, mac_logger);
    // mac_msg_ul.init_rx(pusch_cfg.grant.tb.tbs / 8, true);
    // mac_msg_ul.parse_packet(pusch_res.data);
    // while (mac_msg_ul.next()) {
    //     uint32_t buff_size_idx[4]   = {};
    //     uint32_t buff_size_bytes[4] = {};
    //     // srsran::sch_subh* mac_msg_subh = mac_msg_ul.get();
    //     // assert(mac_msg_ul.get());
    //     if (mac_msg_ul.get()) {  
    //     // if (mac_msg_ul.get()->is_sdu()) {  
    //         std::cout << "Decoding PDU header, lcid = " << int(mac_msg_ul.get()->ul_sch_ce_type()) << ", nof_subheaders = " << mac_msg_ul.nof_subh() << std::endl;
    //         switch (mac_msg_ul.get()->ul_sch_ce_type())
    //         {
    //         case srsran::ul_sch_lcid::PHR_REPORT:
    //             std::cout << "sdu is phr report" << std::endl;
    //             break;
            
    //         case srsran::ul_sch_lcid::CRNTI:     
    //             std::cout << "C-rnti, bsr = " << mac_msg_ul.get()->get_bsr(buff_size_idx, buff_size_bytes) << std::endl;    

    //             break;

    //         case srsran::ul_sch_lcid::TRUNC_BSR:
    //             std::cout << "Trunc bsr: " << mac_msg_ul.get()->get_bsr(buff_size_idx, buff_size_bytes) << std::endl;  
    //             break;

    //         case srsran::ul_sch_lcid::SHORT_BSR:
    //             std::cout << "Short bsr: " << mac_msg_ul.get()->get_bsr(buff_size_idx, buff_size_bytes) << std::endl;  
    //             break;

    //         case srsran::ul_sch_lcid::LONG_BSR:
    //             std::cout << "Long bsr: " << mac_msg_ul.get()->get_bsr(buff_size_idx, buff_size_bytes) << std::endl;  
                
    //             break;

    //         case srsran::ul_sch_lcid::PADDING:
    //             std::cout << "Padding, bsr = " << mac_msg_ul.get()->get_bsr(buff_size_idx, buff_size_bytes) << std::endl;  
    //             break;

    //         default:
    //             break;
    //         }
    //     }
    // }
    
    // pucch_cfg.rnti = rnti;
    // pucch_cfg.group_hopping_en = false;
    // pucch_cfg.meas_ta_en = false;
    // pucch_cfg.sr_configured = true;

    // srsran_pucch_format_t format;
    // for (format = SRSRAN_PUCCH_FORMAT_1; format < SRSRAN_PUCCH_FORMAT_ERROR; format = static_cast<srsran_pucch_format_t>(static_cast<int>(format) + 1)) {
    //     for (uint32_t d = 0; d <= 3; d++) {
    //         for (uint32_t ncs = 0; ncs < 8; ncs ++) {
    //             for (uint32_t n_pucch = 1; n_pucch < 130; n_pucch++) {
    //                 ul_cfg.pucch.delta_pucch_shift = d;
    //                 ul_cfg.pucch.group_hopping_en  = false;
    //                 ul_cfg.pucch.N_cs              = ncs;
    //                 ul_cfg.pucch.n_rb_2            = 0;
    //                 ul_cfg.pucch.format            = format;
    //                 ul_cfg.pucch.n_pucch           = n_pucch;

    //                 ret = srsran_enb_ul_get_pucch(enb_ul, ul_sf, &ul_cfg.pucch, &pucch_res);
    //             }
    //         }
    //     }
    // }

    // ret = srsran_enb_ul_get_pucch(enb_ul, ul_sf, &pucch_cfg, &pucch_res);
    // ret = decode_pucch(enb_ul, ul_sf, &pucch_cfg, &pucch_res);
    
    
    return ret;
}


int SFDecoder::sfWorkerUlInit(srsran_pdsch_cfg_t pdsch_cfg, srsran_dl_sf_cfg_t dl_sf_cfg, srsran_ue_dl_t ue_dl, srsran_ue_dl_cfg_t ue_dl_cfg, srsran_enb_ul_t* enb_ul, srsran_refsignal_dmrs_pusch_cfg_t* dmrs_pusch_cfg) {
    uint8_t* data[SRSRAN_MAX_CODEWORDS];
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        data[i] = srsran_vec_u8_malloc(2000 * 8);
    }
    srsran_softbuffer_rx_t rx_softbuffers[SRSRAN_MAX_CODEWORDS];
    for (uint32_t i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        pdsch_cfg.softbuffers.rx[i] = &rx_softbuffers[i];
        srsran_softbuffer_rx_init(pdsch_cfg.softbuffers.rx[i], ngscope_config->cell.nof_prb);
    }

    bool acks[SRSRAN_MAX_CODEWORDS] = {false};

    int ret = SRSRAN_ERROR;

    dl_sf_cfg.tti = tti;
    srsran_dci_dl_t    dci_dl[SRSRAN_MAX_DCI_MSG] = {};
    srsran_pmch_cfg_t  pmch_cfg;
    srsran_pdsch_res_t pdsch_res[SRSRAN_MAX_CODEWORDS];
    pdsch_cfg.rnti = SRSRAN_SIRNTI;

    // Use default values for PDSCH decoder
    ZERO_OBJECT(pmch_cfg);

    uint32_t mi_set_len;
    if (ue_dl.cell.frame_type == SRSRAN_TDD && !dl_sf_cfg.tdd_config.configured) {
        mi_set_len = 3;
    } else {
        mi_set_len = 1;
    }

    // Blind search PHICH mi value
    ret = 0;
    for (uint32_t i = 0; i < mi_set_len && !ret; i++) {
        if (mi_set_len == 1) {
            srsran_ue_dl_set_mi_auto(&ue_dl);
        } else {
            srsran_ue_dl_set_mi_manual(&ue_dl, i);
        }

        if ((ret = srsran_ue_dl_decode_fft_estimate(&ue_dl, &dl_sf_cfg, &ue_dl_cfg)) < 0) {
            return ret;
        }
        // printf("Finding DCI...\n");
        ret = srsran_ue_dl_find_dl_dci_sirnti(&ue_dl, &dl_sf_cfg, &ue_dl_cfg, SRSRAN_SIRNTI, dci_dl);
        // std::cout << "ret = " << ret << std::endl;
        }

    if (ret == 1) {
        char str[512];
        srsran_dci_dl_info(&dci_dl[0], str, 512);
        std::cout << "SIB1 Decoder found DCI: PDCCH: " << str << ", snr = " << ue_dl.chest_res.snr_db << " dB." << std::endl;

        // Convert DCI message to DL grant
        if (srsran_ue_dl_dci_to_pdsch_grant(&ue_dl, &dl_sf_cfg, &ue_dl_cfg, &dci_dl[0], &pdsch_cfg.grant)) {
            ERROR("Error unpacking DCI");
            //   return SRSRAN_ERROR;
        }

        // std::cout << "Successfully got pdsch grant!" << std::endl;

        // Calculate RV if not provided in the grant and reset softbuffer
        for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
            if (pdsch_cfg.grant.tb[i].enabled) {
                if (pdsch_cfg.grant.tb[i].rv < 0) {
                  uint32_t sfn = dl_sf_cfg.tti / 10;
                  uint32_t k = (sfn / 2) % 4;
                  pdsch_cfg.grant.tb[i].rv = ((uint32_t)ceilf((float)1.5 * k)) % 4;
                }
            srsran_softbuffer_rx_reset_tbs(pdsch_cfg.softbuffers.rx[i], (uint32_t)pdsch_cfg.grant.tb[i].tbs);
            }
        }

        bool decode_enable = false;
        for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
            if (pdsch_cfg.grant.tb[tb].enabled) {
            decode_enable         = true;
            pdsch_res[tb].payload = data[tb];
            pdsch_res[tb].crc     = false;
        }
        }

        if (decode_enable) {
            if (dl_sf_cfg.sf_type == SRSRAN_SF_NORM) {
                if (srsran_ue_dl_decode_pdsch(&ue_dl, &dl_sf_cfg, &pdsch_cfg, pdsch_res)) {
                ERROR("ERROR: Decoding PDSCH");
                ret = -1;
                }
            } else {
                pmch_cfg.pdsch_cfg = pdsch_cfg;
                if (srsran_ue_dl_decode_pmch(&ue_dl, &dl_sf_cfg, &pmch_cfg, pdsch_res)) {
                    ERROR("Decoding PMCH");
                    ret = -1;
                }
            }
        }

        for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
            if (pdsch_cfg.grant.tb[tb].enabled) {
                acks[tb] = pdsch_res[tb].crc;
            }
        }
        asn1::rrc::bcch_dl_sch_msg_s dlsch;
        asn1::rrc::sys_info_s sibs; // sib2 and beyond
        asn1::cbit_ref dlsch_bref(pdsch_res->payload, pdsch_cfg.grant.tb[0].tbs / 8);
        // asn1::json_writer js_sib2;
        asn1::SRSASN_CODE err = dlsch.unpack(dlsch_bref);
        asn1::rrc::sib_type2_s sib2;

        if (err == asn1::SRSASN_SUCCESS) {
            if (dlsch.msg.c1().type() == asn1::rrc::bcch_dl_sch_msg_type_c::c1_c_::types::sib_type1) {
                
            } else {
                std::cout << "Decoded SIB2 or beyond. " << std::endl;
                sibs = dlsch.msg.c1().sys_info();
                // sibs.to_json(js_sib2);
                // std::cout << js_sib2.to_string().c_str() << std::endl;
                asn1::rrc::sys_info_r8_ies_s::sib_type_and_info_l_ &sib_list = dlsch.msg.c1().sys_info().crit_exts.sys_info_r8().sib_type_and_info;
                for (uint32_t i = 0; i < sib_list.size(); i++) {
                    if (sib_list[i].type().value == asn1::rrc::sib_info_item_c::types::sib2) {
                        sib2 = sib_list[i].sib2();
                        dmrs_pusch_cfg->cyclic_shift = sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.cyclic_shift;
                        dmrs_pusch_cfg->group_hopping_en = sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.group_hop_enabled;
                        dmrs_pusch_cfg->sequence_hopping_en = sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.seq_hop_enabled;
                        dmrs_pusch_cfg->delta_ss = sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.group_assign_pusch;
                        return true;
                    }
                }
                return false;
            }
        } else {
          return false;
        }
    }
    return false;
}


int ra_ul_grant_to_grant_prb_allocation_qam256_ngscope(srsran_dci_ul_t* dci, srsran_pusch_grant_t* grant, uint32_t n_rb_ho, uint32_t nof_prb) {
  uint32_t n_prb_1    = 0;
  uint32_t n_rb_pusch = 0;

  srsran_ra_type2_from_riv(dci->type2_alloc.riv, &grant->L_prb, &n_prb_1, nof_prb, nof_prb);
  if (n_rb_ho % 2) {
    n_rb_ho++;
  }

  if (dci->freq_hop_fl == srsran_dci_ul_t::SRSRAN_RA_PUSCH_HOP_DISABLED || dci->freq_hop_fl == srsran_dci_ul_t::SRSRAN_RA_PUSCH_HOP_TYPE2) {
    /* For no freq hopping or type2 freq hopping, n_prb is the same
     * n_prb_tilde is calculated during resource mapping
     */
    for (uint32_t i = 0; i < 2; i++) {
      grant->n_prb[i] = n_prb_1;
    }
    if (dci->freq_hop_fl == srsran_dci_ul_t::SRSRAN_RA_PUSCH_HOP_DISABLED) {
      grant->freq_hopping = 0;
    } else {
      grant->freq_hopping = 2;
    }
    // INFO("prb1: %d, prb2: %d, L: %d", grant->n_prb[0], grant->n_prb[1], grant->L_prb);
  } else {
    /* Type1 frequency hopping as defined in 8.4.1 of 36.213
     * frequency offset between 1st and 2nd slot is fixed.
     */
    n_rb_pusch = nof_prb - n_rb_ho - (nof_prb % 2);

    // starting prb idx for slot 0 is as given by resource dci
    grant->n_prb[0] = n_prb_1;
    if (n_prb_1 < n_rb_ho / 2) {
    //   INFO("Invalid Frequency Hopping parameters. Offset: %d, n_prb_1: %d", n_rb_ho, n_prb_1);
      return SRSRAN_ERROR;
    }
    uint32_t n_prb_1_tilde = n_prb_1;

    // prb idx for slot 1
    switch (dci->freq_hop_fl) {
      case srsran_dci_ul_t::SRSRAN_RA_PUSCH_HOP_QUART:
        grant->n_prb[1] = (n_rb_pusch / 4 + n_prb_1_tilde) % n_rb_pusch;
        break;
      case srsran_dci_ul_t::SRSRAN_RA_PUSCH_HOP_QUART_NEG:
        if (n_prb_1 < n_rb_pusch / 4) {
          grant->n_prb[1] = (n_rb_pusch + n_prb_1_tilde - n_rb_pusch / 4);
        } else {
          grant->n_prb[1] = (n_prb_1_tilde - n_rb_pusch / 4);
        }
        break;
      case srsran_dci_ul_t::SRSRAN_RA_PUSCH_HOP_HALF:
        grant->n_prb[1] = (n_rb_pusch / 2 + n_prb_1_tilde) % n_rb_pusch;
        break;
      default:
        break;
    }
    // INFO("n_rb_pusch: %d, prb1: %d, prb2: %d, L: %d", n_rb_pusch, grant->n_prb[0], grant->n_prb[1], grant->L_prb);
    grant->freq_hopping = 1;
  }

  if (grant->n_prb[0] + grant->L_prb <= nof_prb && grant->n_prb[1] + grant->L_prb <= nof_prb) {
    return SRSRAN_SUCCESS;
  } else {
    return SRSRAN_ERROR;
  }
}


/*implemetation of modulation and coding scheme convertion in table 8.6.1-3 on 36.213*/
void ul_fill_ra_mcs_256(srsran_ra_tb_t* tb, srsran_ra_tb_t* last_tb, uint32_t L_prb, bool cqi_request)
{
  if (tb->mcs_idx <= 28) {
    /* Table 8.6.1-3 on 36.213 */
    if (tb->mcs_idx < 6) {
      tb->mod = SRSRAN_MOD_QPSK;
      tb->tbs = srsran_ra_tbs_from_idx((tb->mcs_idx)*2, L_prb);
    } else if (tb->mcs_idx < 14) {
      tb->mod = SRSRAN_MOD_16QAM;
      if (tb->mcs_idx < 10){
        tb->tbs = srsran_ra_tbs_from_idx(tb->mcs_idx + 5, L_prb);
      }else if (tb->mcs_idx < 14){
        tb->tbs = srsran_ra_tbs_from_idx(tb->mcs_idx + 6, L_prb);
      }
    } else if (tb->mcs_idx < 23) {
      tb->mod = SRSRAN_MOD_64QAM;
      if (tb->mcs_idx < 19){
        tb->tbs = srsran_ra_tbs_from_idx(tb->mcs_idx + 6, L_prb);
      }else if (tb->mcs_idx < 23){
        tb->tbs = srsran_ra_tbs_from_idx(tb->mcs_idx + 7, L_prb);
      }
    } else if (tb->mcs_idx < 29) {
      tb->mod = SRSRAN_MOD_256QAM;
      if (tb->mcs_idx < 26){
        tb->tbs = srsran_ra_tbs_from_idx(tb->mcs_idx + 7, L_prb);
      } else if ( tb->mcs_idx == 26 && L_prb > 0 && L_prb < 111){
        tb->tbs = tbs_table_32A[L_prb-1]; //implementation of mcs idex 32A on uplink
      }else if (tb->mcs_idx < 29){
        tb->tbs = srsran_ra_tbs_from_idx(tb->mcs_idx + 6, L_prb); 
      }
    } else {
      ERROR("Invalid MCS index %d", tb->mcs_idx);
    }
  } else if (tb->mcs_idx == 29 && cqi_request && L_prb <= 4) {
    // 8.6.1 and 8.6.2 36.213 second paragraph
    tb->mod = SRSRAN_MOD_QPSK;
    tb->tbs = 0;
    tb->rv  = 1;
  } else if (tb->mcs_idx >= 29) {
    // Else use last TBS/Modulation and use mcs to obtain rv_idx
    tb->tbs = last_tb->tbs;
    tb->mod = last_tb->mod;
    tb->rv  = tb->mcs_idx - 28;
  }
}


int srsran_ra_ul_dci_to_grant_qam256_ngscope(srsran_cell_t* cell, srsran_ul_sf_cfg_t* sf, srsran_pusch_hopping_cfg_t* hopping_cfg, srsran_dci_ul_t* dci, srsran_pusch_grant_t* grant) {
  // Compute PRB allocation
  if (!ra_ul_grant_to_grant_prb_allocation_qam256_ngscope(dci, grant, hopping_cfg->n_rb_ho, cell->nof_prb)) {
    // copy default values from DCI. RV can be updated by ul_fill_ra_mcs() in case of Adaptive retx (mcs>28)
    grant->tb.mcs_idx = dci->tb.mcs_idx;
    grant->tb.rv      = dci->tb.rv;

    // Compute MCS
    ul_fill_ra_mcs_256(&grant->tb, &grant->last_tb, grant->L_prb, dci->cqi_request);

    /* Compute RE assuming shortened is false*/
    srsran_ra_ul_compute_nof_re(grant, cell->cp, 0);

    // TODO: Need to compute hopping here before determining if there is collision with SRS, but only MAC knows if it's
    // a
    //  new tx or a retx. Need to split MAC interface in 2 calls. For now, assume hopping is the same
    for (uint32_t i = 0; i < 2; i++) {
      grant->n_prb_tilde[i] = grant->n_prb[i];
    }

    if (grant->nof_symb == 0 || grant->nof_re == 0) {
      INFO("Error converting ul_dci to grant, nof_symb=%d, nof_re=%d", grant->nof_symb, grant->nof_re);
      return SRSRAN_ERROR;
    }

    return SRSRAN_SUCCESS;
  } else {
    return SRSRAN_ERROR;
  }
}


int srsran_ue_ul_dci_to_pusch_grant_qam256_ngscope(srsran_ue_ul_t* q, srsran_ul_sf_cfg_t* sf, srsran_ue_ul_cfg_t* cfg, srsran_dci_ul_t* dci, srsran_pusch_grant_t* grant) {
  // Convert DCI to Grant
  if (srsran_ra_ul_dci_to_grant_qam256_ngscope(&q->cell, sf, &cfg->ul_cfg.hopping, dci, grant)) {
    return SRSRAN_ERROR;
  }

  // Update shortened before computing grant
  srsran_refsignal_srs_pusch_shortened(&q->signals, sf, &cfg->ul_cfg.srs, &cfg->ul_cfg.pusch);

  // Update RE assuming if shortened is true
  if (sf->shortened) {
    srsran_ra_ul_compute_nof_re(grant, q->cell.cp, true);
  }

  // Assert Grant is valid
  return srsran_pusch_assert_grant(grant);
}
#include "../hdr/SIBDecoder.h"

int SIBDecoder::run_ul_mode(SFWorker* curworker) {
    srsran_ue_dl_t* ue_dl = &curworker->ue_dl;
    srsran_ue_dl_cfg_t* ue_dl_cfg = &curworker->ue_dl_cfg;
    srsran_dl_sf_cfg_t* dl_sf_cfg = &curworker->dl_sf_cfg;
    cf_t** sf_buffer = curworker->sf_buffer;
    srsran_ul_cfg_t* ul_cfg = &curworker->ue_ul_cfg.ul_cfg;
    srsran_enb_ul_t* enb_ul = &curworker->enb_ul;
    srsran_cell_t cell = ngscope_config->cell;
    int ret = SRSRAN_ERROR;

    if (!sibs->have_sys()) {
        ret = this->decodeSib(&sib1, &sib2, ue_dl, sf_buffer, ue_dl_cfg, dl_sf_cfg, ue_dl_cfg->cfg.pdsch);
        // asn1::rrc::sib_type1_s sib1tmp;
        // asn1::rrc::sib_type2_s sib2tmp;
        // sib1tmp = sib1;
        // sib2tmp = sib2;
        if (ret == 1) {
            sibs->set_sib1(sib1);
        } else if (ret == 2) {
            sibs->set_sib2(sib2);
        }
    } else if (!curworker->isConfigured()) {
        sib1 = sibs->getSIB1();
        sib2 = sibs->getSIB2();
        // const asn1::rrc::sib_type1_s& sib1ref = sibs->getSIB1();
        // const asn1::rrc::sib_type2_s& sib2ref = sibs->getSIB2();
        // const std::shared_ptr<const asn1::rrc::sib_type1_s> sib1ptr = sibs->getSIB1ptr();
        // const std::shared_ptr<const asn1::rrc::sib_type2_s> sib2ptr = sibs->getSIB2ptr();
        // asn1::rrc::rr_cfg_common_sib_s rrc_config = sib2.rr_cfg_common;

        if (curworker->cell.frame_type == SRSRAN_TDD) {
            dl_sf_cfg->tdd_config.sf_config = sib1.tdd_cfg.sf_assign.to_number();
            dl_sf_cfg->tdd_config.ss_config = sib1.tdd_cfg.special_sf_patterns.to_number();
            dl_sf_cfg->tdd_config.configured = true;
        } else {
            dl_sf_cfg->tdd_config.configured = false;
        }

        ul_cfg->dmrs.cyclic_shift = sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.cyclic_shift;
        ul_cfg->dmrs.group_hopping_en = sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.group_hop_enabled;
        ul_cfg->dmrs.sequence_hopping_en = sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.seq_hop_enabled;
        ul_cfg->dmrs.delta_ss = sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.group_assign_pusch;

        ul_cfg->hopping.hop_mode = srsran_pusch_hopping_cfg_t::SRSRAN_PUSCH_HOP_MODE_INTER_SF;
        ul_cfg->hopping.hopping_offset = sib2.rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.pusch_hop_offset;
        ul_cfg->hopping.n_sb = sib2.rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.n_sb;
        ul_cfg->hopping.n_rb_ho = sib2.rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.pusch_hop_offset;

        ul_cfg->pucch.delta_pucch_shift = sib2.rr_cfg_common.pucch_cfg_common.delta_pucch_shift;
        ul_cfg->pucch.N_cs = sib2.rr_cfg_common.pucch_cfg_common.ncs_an;
        ul_cfg->pucch.n_rb_2 = sib2.rr_cfg_common.pucch_cfg_common.nrb_cqi;
        ul_cfg->pucch.N_pucch_1 = sib2.rr_cfg_common.pucch_cfg_common.n1_pucch_an;
        ul_cfg->power_ctrl.delta_f_pucch[0] = sib2.rr_cfg_common.ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format1.to_number();
        ul_cfg->power_ctrl.delta_f_pucch[1] = sib2.rr_cfg_common.ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format1b.to_number();
        ul_cfg->power_ctrl.delta_f_pucch[2] = sib2.rr_cfg_common.ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format2.to_number();
        ul_cfg->power_ctrl.delta_f_pucch[3] = sib2.rr_cfg_common.ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format2a.to_number();
        ul_cfg->power_ctrl.delta_f_pucch[4] = sib2.rr_cfg_common.ul_pwr_ctrl_common.delta_flist_pucch.delta_f_pucch_format2b.to_number();

        ul_cfg->pusch.enable_64qam = sib2.rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.enable64_qam;

        if (srsran_enb_ul_set_cell(enb_ul, cell, &ul_cfg->dmrs, &ul_cfg->srs)) {
            ERROR("Error initiating enb uplink cell");
        }

        curworker->setConfigured();
        std::cout << "Finish worker config!" << std::endl;
    }
    return ret;
}

int SIBDecoder::execute(srsran_ue_dl_t* ue_dl, cf_t** sf_buffer, srsran_ue_dl_cfg_t* ue_dl_cfg, srsran_dl_sf_cfg_t* dl_sf_cfg, srsran_pdsch_cfg_t* pdsch_cfg) {
    int ret  = 0;
    // asn1::rrc::sib_type1_s sib1;
    // asn1::rrc::sib_type2_s sib2;
    if (ue_dl && sf_buffer && ue_dl_cfg && dl_sf_cfg && pdsch_cfg) {
        this->decodeSib(&sib1, &sib2, ue_dl, sf_buffer, ue_dl_cfg, dl_sf_cfg, *pdsch_cfg);
    }

    uint8_t* data[SRSRAN_MAX_CODEWORDS];
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        data[i] = srsran_vec_u8_malloc(2000 * 8);
    }
    bool acks[SRSRAN_MAX_CODEWORDS] = {false};

    ret = srsran_ue_dl_find_and_decode(ue_dl, dl_sf_cfg, ue_dl_cfg, pdsch_cfg, data, acks);

    return ret;
}
    

int SIBDecoder::execute_ul(srsran_cell_t cell, srsran_ue_dl_t* ue_dl, srsran_ue_ul_t* ue_ul, cf_t** sf_buffer, srsran_ue_dl_cfg_t* ue_dl_cfg, srsran_dl_sf_cfg_t* dl_sf_cfg, 
srsran_ul_sf_cfg_t *sf_ul, srsran_ue_ul_cfg_t *ue_ul_cfg, srsran_pdsch_cfg_t* pdsch_cfg, srsran_dl_cfg_t* dl_cfg, srsran_ul_cfg_t* ul_cfg, srsran_enb_ul_t* enb_ul, bool* have_sib1, bool* have_sib2) 
{   
    // int ret = SRSRAN_ERROR;
    // if (!sibs->have_sys()) {
    //     ret = this->decodeSib(&sib1, &sib2, ue_dl, sf_buffer, ue_dl_cfg, dl_sf_cfg, ue_dl_cfg->cfg.pdsch);
    //     if (ret == 1) {
    //         sibs->set_sib1(sib1);
    //     } else if (ret == 2) {
    //         sibs->set_sib2(sib2);
    //     }
    // } else {

    // }
    // if (!*have_sib1) {
    //     ret = this->decodeSib(&sib1, &sib2, ue_dl, sf_buffer, ue_dl_cfg, dl_sf_cfg, ue_dl_cfg->cfg.pdsch);
    //     if (ret == 1) {
    //         *have_sib1 = true;
    //         std::cout << "Successfully init enb ul!" << std::endl;
    //         std::cout << "sf_config " << dl_sf_cfg->tdd_config.sf_config << std::endl;
    //     }
    //     return 0;
    // }
    // if (!*have_sib2) {
    //     ret = this->decodeSib(&sib1, &sib2, ue_dl, sf_buffer, ue_dl_cfg, dl_sf_cfg, ue_dl_cfg->cfg.pdsch);
    //     if (ret == 2) {
    //         // std::lock_guard<std::mutex> lock(ngscope_config->ngscope_config_mutex);
    //         // ul_cfg.dmrs.cyclic_shift = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.cyclic_shift;
    //         // ul_cfg.dmrs.group_hopping_en = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.group_hop_enabled;
    //         // ul_cfg.dmrs.sequence_hopping_en = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.seq_hop_enabled;
    //         // ul_cfg.dmrs.delta_ss = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.group_assign_pusch;

    //         // ul_cfg.hopping.hop_mode = srsran_pusch_hopping_cfg_t::SRSRAN_PUSCH_HOP_MODE_INTER_SF;
    //         // ul_cfg.hopping.hopping_offset = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.pusch_hop_offset;
    //         // ul_cfg.hopping.n_sb = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.n_sb;
    //         // ul_cfg.hopping.n_rb_ho = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.pusch_hop_offset;

    //         asn1::rrc::rr_cfg_common_sib_s rrc_config = sib2.rr_cfg_common;
    //         // asn1::rrc::rr_cfg_common_sib_s rrc_config = ngscope_config->sib2.rr_cfg_common;

    //         // ngscope_config->ngscope_config_mutex.unlock();

    //         ul_cfg->dmrs.cyclic_shift = rrc_config.pusch_cfg_common.ul_ref_sigs_pusch.cyclic_shift;
    //         ul_cfg->dmrs.group_hopping_en = rrc_config.pusch_cfg_common.ul_ref_sigs_pusch.group_hop_enabled;
    //         ul_cfg->dmrs.sequence_hopping_en = rrc_config.pusch_cfg_common.ul_ref_sigs_pusch.seq_hop_enabled;
    //         ul_cfg->dmrs.delta_ss = rrc_config.pusch_cfg_common.ul_ref_sigs_pusch.group_assign_pusch;

    //         ul_cfg->hopping.hop_mode = srsran_pusch_hopping_cfg_t::SRSRAN_PUSCH_HOP_MODE_INTER_SF;
    //         ul_cfg->hopping.hopping_offset = rrc_config.pusch_cfg_common.pusch_cfg_basic.pusch_hop_offset;
    //         ul_cfg->hopping.n_sb = rrc_config.pusch_cfg_common.pusch_cfg_basic.n_sb;
    //         ul_cfg->hopping.n_rb_ho = rrc_config.pusch_cfg_common.pusch_cfg_basic.pusch_hop_offset;

    //         ul_cfg->pucch.delta_pucch_shift = rrc_config.pucch_cfg_common.delta_pucch_shift;
    //         ul_cfg->pucch.N_cs = rrc_config.pucch_cfg_common.ncs_an;
    //         ul_cfg->pucch.n_rb_2 = rrc_config.pucch_cfg_common.nrb_cqi;
    //         ul_cfg->pucch.N_pucch_1 = rrc_config.pucch_cfg_common.n1_pucch_an;

    //         std::cout << "SIB2 delta_pucch_shift = " << rrc_config.pucch_cfg_common.delta_pucch_shift << ", N_cs = " << rrc_config.pucch_cfg_common.ncs_an << ", n_rb_2 = " << rrc_config.pucch_cfg_common.nrb_cqi << ", N_pucch_1 = " << rrc_config.pucch_cfg_common.n1_pucch_an <<std::endl;
    //         std::cout << "delta_pucch_shift = " << ul_cfg->pucch.delta_pucch_shift << ", N_cs = " << ul_cfg->pucch.N_cs << ", n_rb_2 = " << ul_cfg->pucch.n_rb_2 << ", N_pucch_1 = " << ul_cfg->pucch.N_pucch_1 <<std::endl;

    //         if (srsran_enb_ul_set_cell(enb_ul, cell, &ul_cfg->dmrs, &ul_cfg->srs)) {
    //             ERROR("Error initiating enb uplink cell");
    //         }

    //         *have_sib2 = true;
    //         std::cout << "Successfully init enb ul!" << std::endl;
    //     }
    //     return 0;
    // }

    // uint8_t* data[SRSRAN_MAX_CODEWORDS];
    // for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
    //     data[i] = srsran_vec_u8_malloc(2000 * 8);
    // }
    // bool acks[SRSRAN_MAX_CODEWORDS] = {false};
    // // puschGrant_t puschGrant;
    // // std::cout << "tddconfig: " << tddconfig << std::endl;
    // dl_sf_cfg->tti = tti;
    // std::vector<srsran_pusch_grant_t> *puschGrantObtained = nullptr;

    // // ret = srsran_ue_dl_find_and_decode_pdsch_ul_grant(ue_dl, ue_ul, dl_sf_cfg, sf_ul, ue_dl_cfg, ue_ul_cfg, pdsch_cfg, pusch_cfg, data, acks);
    // // ret = srsran_ue_dl_find_and_decode(ue_dl, dl_sf_cfg, ue_dl_cfg, pdsch_cfg, data, acks);
    // int ret_dl = -1;
    // int ret_ul = -1;


    // if ((!*have_sib1) || (!*have_sib2)) {
    //     return 0;
    // }

    // if (srsran_sfidx_tdd_type(dl_sf_cfg->tdd_config, tti % 10) != SRSRAN_TDD_SF_U) {
    //     ret = decodeRach(ue_dl, sf_buffer, ue_dl_cfg, ue_ul_cfg, dl_sf_cfg);
        // ret = srsran_ue_dl_find_and_decode_pdsch_ul_grant(ue_dl, ue_ul, dl_sf_cfg, sf_ul, ue_dl_cfg, ue_ul_cfg, pdsch_cfg, &ul_cfg->pusch, data, acks);
        // for (int i = 0x0001; i <= 0x0960; i++){
            // ue_dl_cfg->cfg.pdsch.rnti = 141;
        //     ret = srsran_ue_dl_find_and_decode(ue_dl, dl_sf_cfg, ue_dl_cfg, &ue_dl_cfg->cfg.pdsch, data, acks);
        //     if (ret > 0) {
        //         std:: cout << "RNTI = " << i << std::endl;
        //     }
        // // }
        // for (int i = 0; i <= 7; i++) {
        //     ue_dl_cfg->cfg.tm = (srsran_tm_t)i;
        //     ret = srsran_ue_dl_find_and_decode(ue_dl, dl_sf_cfg, ue_dl_cfg, &ue_dl_cfg->cfg.pdsch, data, acks);
        //     if (ret > 0) {
        //         std::cout << "tm = " << i << std::endl;
        //     }
        // }
        // ue_dl_cfg->cfg.tm = (srsran_tm_t)2;
        // ret = decodeRach(ue_dl, sf_buffer, ue_dl_cfg, ue_ul_cfg, dl_sf_cfg);
        // ret = decode_pdsch(ue_dl, dl_sf_cfg, ue_dl_cfg, &ue_dl_cfg->cfg.pdsch, data, acks);
        // ret = srsran_ue_dl_find_and_decode_pdsch_ul_grant(ue_dl, ue_ul, dl_sf_cfg, )

    //     ret = decode_pdcch_and_pdsch(ue_dl, ue_ul, dl_sf_cfg, sf_ul, ue_dl_cfg, ue_ul_cfg, dl_cfg, ul_cfg, &ret_dl, &ret_ul);
    //     // std::cout << "sfidx = " << sf_idx << std::endl;
    //     if (ret > 0) {
    //         puschSche->pushULSche(tti, dl_sf_cfg->tdd_config.sf_config, ul_cfg->pusch.grant);
    //         // puschGrant.pusch_grant = pusch_cfg->grant;
    //         // puschGrant.target_tti = grantTargetTti(tti, tddconfig);
    //         // ngscope_config->ngscope_config_mutex.lock();
    //         // ngscope_config->puschGrantPush(puschGrant);
    //         // ngscope_config->ngscope_config_mutex.unlock();
    //     }
    // } else {
    //     puschGrantObtained = puschSche->getULSche(tti);
    //     // std::cout << "In ul decoding..." << std::endl;
    //     // ngscope_config->ngscope_config_mutex.lock();
    //     // puschGrant = ngscope_config->puschGrantPop(tti);
    //     // ngscope_config->ngscope_config_mutex.unlock();
    //     if (puschGrantObtained) {
    //         int nof_pusch_grant = 0;
    //         for (std::vector<srsran_pusch_grant_t>::iterator iter = puschGrantObtained->begin(); iter != puschGrantObtained->end(); iter++) {
    //             nof_pusch_grant++;
    //             ret = decode_pusch(ul_cfg, *iter, enb_ul, sf_ul, pdsch_cfg->rnti);
    //         }
    //         std::cout << "nof pusch grant = " << nof_pusch_grant << std::endl;
    //         puschSche->deleteULSche(tti);
    //         // ret = decode_pusch(pusch_cfg, puschGrant.pusch_grant, enb_ul, sf_ul, pdsch_cfg->rnti);
    //     }
    // }

    // uint8_t* data[SRSRAN_MAX_CODEWORDS];
    // for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
    //     data[i] = srsran_vec_u8_malloc(2000 * 8);
    // }
    // srsran_softbuffer_rx_t rx_softbuffers[SRSRAN_MAX_CODEWORDS];
    // for (uint32_t i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
    //     pdsch_cfg->softbuffers.rx[i] = &rx_softbuffers[i];
    //     srsran_softbuffer_rx_init(pdsch_cfg->softbuffers.rx[i], ngscope_config->cell.nof_prb);
    // }

    // std::cout << "In sib decoding!" << std::endl;

    // bool acks[SRSRAN_MAX_CODEWORDS] = {false};

    // int ret = SRSRAN_ERROR;

    // dl_sf_cfg->tti = tti;
    // srsran_dci_dl_t    dci_dl[SRSRAN_MAX_DCI_MSG] = {};
    // srsran_pmch_cfg_t  pmch_cfg;
    // srsran_pdsch_res_t pdsch_res[SRSRAN_MAX_CODEWORDS];
    // int RNTI = pdsch_cfg->rnti;
    // pdsch_cfg->rnti = SRSRAN_SIRNTI;

    // // Use default values for PDSCH decoder
    // ZERO_OBJECT(pmch_cfg);

    // uint32_t mi_set_len;
    // if (ue_dl->cell.frame_type == SRSRAN_TDD && !dl_sf_cfg->tdd_config.configured) {
    //     mi_set_len = 3;
    // } else {
    //     mi_set_len = 1;
    // }

    // // Blind search PHICH mi value
    // ret = 0;
    // for (uint32_t i = 0; i < mi_set_len && !ret; i++) {
    //     if (mi_set_len == 1) {
    //         srsran_ue_dl_set_mi_auto(ue_dl);
    //     } else {
    //         srsran_ue_dl_set_mi_manual(ue_dl, i);
    //     }

    //     if ((ret = srsran_ue_dl_decode_fft_estimate(ue_dl, dl_sf_cfg, ue_dl_cfg)) < 0) {
    //         return ret;
    //     }
    //     // printf("Finding DCI...\n");
    //     ret = srsran_ue_dl_find_dl_dci_sirnti(ue_dl, dl_sf_cfg, ue_dl_cfg, SRSRAN_SIRNTI, dci_dl);
    //     // std::cout << "ret = " << ret << std::endl;
    //     }

    // if (ret == 1) {
    //     char str[512];
    //     srsran_dci_dl_info(&dci_dl[0], str, 512);
    //     std::cout << "SIB1 Decoder found DCI: PDCCH: " << str << ", snr = " << ue_dl->chest_res.snr_db << " dB." << std::endl;

    //     // Convert DCI message to DL grant
    //     if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[0], &pdsch_cfg->grant)) {
    //         ERROR("Error unpacking DCI");
    //         //   return SRSRAN_ERROR;
    //     }

    //     std::cout << "Successfully got pdsch grant!" << std::endl;

    //     // Calculate RV if not provided in the grant and reset softbuffer
    //     for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
    //         if (pdsch_cfg->grant.tb[i].enabled) {
    //             if (pdsch_cfg->grant.tb[i].rv < 0) {
    //             uint32_t sfn = dl_sf_cfg->tti / 10;
    //             uint32_t k = (sfn / 2) % 4;
    //             pdsch_cfg->grant.tb[i].rv = ((uint32_t)ceilf((float)1.5 * k)) % 4;
    //             }
    //         srsran_softbuffer_rx_reset_tbs(pdsch_cfg->softbuffers.rx[i], (uint32_t)pdsch_cfg->grant.tb[i].tbs);
    //         }
    //     }

    //     bool decode_enable = false;
    //     for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
    //         if (pdsch_cfg->grant.tb[tb].enabled) {
    //         decode_enable         = true;
    //         pdsch_res[tb].payload = data[tb];
    //         pdsch_res[tb].crc     = false;
    //     }
    //     }

    //     if (decode_enable) {
    //         if (dl_sf_cfg->sf_type == SRSRAN_SF_NORM) {
    //             if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, pdsch_cfg, pdsch_res)) {
    //             ERROR("ERROR: Decoding PDSCH");
    //             ret = -1;
    //             }
    //         } else {
    //             pmch_cfg.pdsch_cfg = *pdsch_cfg;
    //             if (srsran_ue_dl_decode_pmch(ue_dl, dl_sf_cfg, &pmch_cfg, pdsch_res)) {
    //                 ERROR("Decoding PMCH");
    //                 ret = -1;
    //             }
    //         }
    //     }

    //     for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
    //         if (pdsch_cfg->grant.tb[tb].enabled) {
    //             acks[tb] = pdsch_res[tb].crc;
    //         }
    //     }
    //     asn1::rrc::bcch_dl_sch_msg_s dlsch;
    //     asn1::rrc::sib_type1_s sib1;
    //     asn1::rrc::sys_info_s sibs; // sib2 and beyond
    //     asn1::cbit_ref dlsch_bref(pdsch_res->payload, pdsch_cfg->grant.tb[0].tbs / 8);
    //     asn1::json_writer js_sib1;
    //     asn1::json_writer js_sib2;
    //     asn1::SRSASN_CODE err = dlsch.unpack(dlsch_bref);

    //     if (err == asn1::SRSASN_SUCCESS) {
    //         if (dlsch.msg.c1().type() == asn1::rrc::bcch_dl_sch_msg_type_c::c1_c_::types::sib_type1) {
    //             std::cout << "Decoded SIB1. " << std::endl;
    //             sib1 = dlsch.msg.c1().sib_type1();
    //             sib1.to_json(js_sib1);
    //             std::cout << js_sib1.to_string().c_str() << std::endl;
    //             std::lock_guard<std::mutex> lock(ngscope_config->ngscope_config_mutex);
    //             ngscope_config->sib1 = sib1;
    //             ngscope_config->have_sib1 = true;
    //         } else {
    //             std::cout << "Decoded SIB2 or beyond. " << std::endl;
    //             sibs = dlsch.msg.c1().sys_info();
    //             sibs.to_json(js_sib2);
    //             std::cout << js_sib2.to_string().c_str() << std::endl;
    //             asn1::rrc::sys_info_r8_ies_s::sib_type_and_info_l_ &sib_list = dlsch.msg.c1().sys_info().crit_exts.sys_info_r8().sib_type_and_info;
    //             for (uint32_t i = 0; i < sib_list.size(); i++) {
    //                 if (sib_list[i].type().value == asn1::rrc::sib_info_item_c::types::sib2) {
    //                     std::lock_guard<std::mutex> lock(ngscope_config->ngscope_config_mutex);
    //                     ngscope_config->sib2 = sib_list[i].sib2();
    //                     ngscope_config->have_sib2 = true;
    //                     ngscope_config->ngscope_config_mutex.unlock();
    //                     // std::lock_guard<std::mutex> siblock(SibMutex);
    //                     // *sib2 = sib_list[i].sib2();
    //                 }
    //             }
    //         }
    //     }
    // }
    // pdsch_cfg->rnti = RNTI;
    return 0;
}


int SIBDecoder::decodeSib(asn1::rrc::sib_type1_s* sib1ret, asn1::rrc::sib_type2_s* sib2ret, srsran_ue_dl_t* ue_dl, cf_t** sf_buffer, srsran_ue_dl_cfg_t* ue_dl_cfg, srsran_dl_sf_cfg_t* dl_sf_cfg, srsran_pdsch_cfg_t pdsch_cfg) {
    uint8_t* data[SRSRAN_MAX_CODEWORDS];
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        data[i] = srsran_vec_u8_malloc(2000 * 8);
    }
    // srsran_softbuffer_rx_t rx_softbuffers[SRSRAN_MAX_CODEWORDS];
    // for (uint32_t i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
    //     pdsch_cfg.softbuffers.rx[i] = &rx_softbuffers[i];
    //     srsran_softbuffer_rx_init(pdsch_cfg.softbuffers.rx[i], ngscope_config->cell.nof_prb);
    // }

    // std::cout << "In sib decoding!" << std::endl;

    bool acks[SRSRAN_MAX_CODEWORDS] = {false};

    int ret = SRSRAN_ERROR;
    int ret1 = SRSRAN_ERROR;
    int ret2 = SRSRAN_ERROR;

    dl_sf_cfg->tti = tti;
    srsran_dci_dl_t    dci_dl[SRSRAN_MAX_DCI_MSG] = {};
    srsran_pmch_cfg_t  pmch_cfg;
    srsran_pdsch_res_t pdsch_res[SRSRAN_MAX_CODEWORDS];
    int RNTI = pdsch_cfg.rnti;
    pdsch_cfg.rnti = SRSRAN_SIRNTI;

    // Use default values for PDSCH decoder
    ZERO_OBJECT(pmch_cfg);

    uint32_t mi_set_len;
    if (ue_dl->cell.frame_type == SRSRAN_TDD && !dl_sf_cfg->tdd_config.configured) {
        mi_set_len = 3;
    } else {
        mi_set_len = 1;
    }

    // Blind search PHICH mi value
    ret = 0;
    for (uint32_t i = 0; i < mi_set_len && !ret; i++) {
        if (mi_set_len == 1) {
            srsran_ue_dl_set_mi_auto(ue_dl);
        } else {
            srsran_ue_dl_set_mi_manual(ue_dl, i);
        }

        if ((ret = srsran_ue_dl_decode_fft_estimate(ue_dl, dl_sf_cfg, ue_dl_cfg)) < 0) {
            return ret;
        }
        // printf("Finding DCI...\n");
        ret = srsran_ue_dl_find_dl_dci_sirnti(ue_dl, dl_sf_cfg, ue_dl_cfg, SRSRAN_SIRNTI, dci_dl);
        // std::cout << "ret = " << ret << std::endl;
    }

    if (ret == 1) {
        computerPower(ue_dl->sf_symbols[0]);
        
        char str[512];
        srsran_dci_dl_info(&dci_dl[0], str, 512);
        std::cout << "SIB1 Decoder found DCI: PDCCH: " << str << ", snr = " << ue_dl->chest_res.snr_db << " dB." << std::endl;

        // Convert DCI message to DL grant
        if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[0], &pdsch_cfg.grant)) {
            ERROR("Error unpacking DCI");
            //   return SRSRAN_ERROR;
        }

        for (uint32_t prb_idx = 0; prb_idx < nof_prb; prb_idx++) {
            std::cout << "prb_idx: " << prb_idx << ", power: " << rb_power.at(prb_idx) << std::endl;
        }

        std::cout << "Successfully got pdsch grant, rb: " << pdsch_cfg.grant.last_tbs[0] << "-" << pdsch_cfg.grant.last_tbs[0]+pdsch_cfg.grant.nof_prb-1 << std::endl;

        // srsran_softbuffer_rx_t rx_softbuffers[SRSRAN_MAX_CODEWORDS];
        // for (uint32_t i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        //     pdsch_cfg.softbuffers.rx[i] = &rx_softbuffers[i];
        //     srsran_softbuffer_rx_init(pdsch_cfg.softbuffers.rx[i], cell.nof_prb);
        // }

        // Calculate RV if not provided in the grant and reset softbuffer
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
                } else {
                    char str[512];
                    srsran_pdsch_rx_info(&pdsch_cfg, pdsch_res, str, sizeof(str));
                    printf("Decoded PDSCH: %s\n", str);
                }
            } else {
                pmch_cfg.pdsch_cfg = pdsch_cfg;
                if (srsran_ue_dl_decode_pmch(ue_dl, dl_sf_cfg, &pmch_cfg, pdsch_res)) {
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
        auto& mac_logger = srslog::fetch_basic_logger("MAC");
        srsran::sch_pdu sibpdu(10, mac_logger);
        sibpdu.init_rx(pdsch_cfg.grant.tb[0].tbs / 8, false);
        sibpdu.parse_packet(pdsch_res[0].payload);
        while (sibpdu.next()) {
            std::cout << "PDSCH size: " << pdsch_cfg.grant.tb[0].tbs / 8 << ", nof subheaders: " << sibpdu.nof_subh() << ", pdu size: " << sibpdu.get_pdu_len() << std::endl;
            if (sibpdu.get()) {
                std::cout << "subheader size: " << sibpdu.get()->get_payload_size() << std::endl;
            }
        }

        asn1::rrc::bcch_dl_sch_msg_s dlsch;
        asn1::rrc::sib_type1_s sib1;
        asn1::rrc::sys_info_s sibs; // sib2 and beyond
        asn1::cbit_ref dlsch_bref(pdsch_res->payload, pdsch_cfg.grant.tb[0].tbs / 8);
        asn1::json_writer js_sib1;
        asn1::json_writer js_sib2;
        asn1::SRSASN_CODE err = dlsch.unpack(dlsch_bref);

        if (err == asn1::SRSASN_SUCCESS) {
            if (dlsch.msg.c1().type() == asn1::rrc::bcch_dl_sch_msg_type_c::c1_c_::types::sib_type1) {
                // std::cout << "Decoded SIB1. " << std::endl;
                sib1 = dlsch.msg.c1().sib_type1();
                sib1.to_json(js_sib1);
                std::cout << js_sib1.to_string().c_str() << std::endl;
                dl_sf_cfg->tdd_config.sf_config = sib1.tdd_cfg.sf_assign.to_number();
                dl_sf_cfg->tdd_config.ss_config = sib1.tdd_cfg.special_sf_patterns.to_number();
                dl_sf_cfg->tdd_config.configured = true;
                ret = 1;
                *sib1ret = sib1;
                
                std::cout << "cell id: " << sib1.cell_access_related_info.cell_id.to_number() << std::endl;
                // std::lock_guard<std::mutex> lock(ngscope_config->ngscope_config_mutex);
                // ngscope_config->sib1 = sib1;
                // ngscope_config->have_sib1 = true;
            } else {
                // std::cout << "Decoded SIB2 or beyond. " << std::endl;
                sibs = dlsch.msg.c1().sys_info();
                sibs.to_json(js_sib2);
                std::cout << js_sib2.to_string().c_str() << std::endl;
                asn1::rrc::sys_info_r8_ies_s::sib_type_and_info_l_ &sib_list = dlsch.msg.c1().sys_info().crit_exts.sys_info_r8().sib_type_and_info;
                for (uint32_t i = 0; i < sib_list.size(); i++) {
                    if (sib_list[i].type().value == asn1::rrc::sib_info_item_c::types::sib2) {
                        // std::lock_guard<std::mutex> lock(ngscope_config->ngscope_config_mutex);
                        // ngscope_config->sib2 = sib_list[i].sib2();
                        // ngscope_config->have_sib2 = true;
                        ret = 2;
                        *sib2ret = sib_list[i].sib2();
                        
                    }
                }
            }
        }
    }
    pdsch_cfg.rnti = RNTI;
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        free(data[i]);
        data[i] = NULL;
    }
    // auto tdd_ack_feedback_mode = dlccch.msg.set_c1().set_rrc_conn_reest().crit_exts.set_c1().set_rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded.pucch_cfg_ded.tdd_ack_nack_feedback_mode;
    // auto tdd_ack_feedback_mode1 = dlccch.msg.set_c1().set_rrc_conn_setup().crit_exts.set_c1().set_rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.pucch_cfg_ded.tdd_ack_nack_feedback_mode;
    // auto transmode = dlccch.msg.set_c1().set_rrc_conn_setup().crit_exts.set_c1().set_rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.ant_info_r10.get()->explicit_value_r10().tx_mode_r10.to_number();
    return ret;
}


void SIBDecoder::computerPower(const cf_t *sf_symbols) {
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


int SIBDecoder::rachInit() {
    uint8_t* data[SRSRAN_MAX_CODEWORDS];
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        data[i] = srsran_vec_u8_malloc(2000 * 8);
    }
    bool acks[SRSRAN_MAX_CODEWORDS] = {false};

    srsran_dci_dl_t    dci_dl[SRSRAN_MAX_DCI_MSG] = {};
    srsran_pdsch_res_t pdsch_res[SRSRAN_MAX_CODEWORDS];

    return 0;
}


int SIBDecoder::decodeRach(srsran_ue_dl_t* ue_dl, cf_t** sf_buffer, srsran_ue_dl_cfg_t* ue_dl_cfg, srsran_ue_ul_cfg_t* ue_ul_cfg, srsran_dl_sf_cfg_t* dl_sf_cfg) {
    int ret = SRSRAN_ERROR;
    uint16_t rnti;
    uint16_t c_rnti;
    uint8_t* data[SRSRAN_MAX_CODEWORDS];
    srsran_ue_dl_cfg_t temp_ue_dl_cfg = *ue_dl_cfg;
    srsran_ue_ul_cfg_t temp_ue_ul_cfg = *ue_ul_cfg;
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        data[i] = srsran_vec_u8_malloc(2000 * 8);
    }
    bool acks[SRSRAN_MAX_CODEWORDS] = {false};

    
    srsran_pdsch_cfg_t pdsch_cfg = ue_dl_cfg->cfg.pdsch;

    dl_sf_cfg->tti = tti;
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
            std::cout << "RACH Decoder found DCI: PDCCH: " << str << ", snr = " << ue_dl->chest_res.snr_db << " dB, ret = " << ret << std::endl;
            pdsch_cfg.use_tbs_index_alt = false;
            // Convert DCI message to DL grant
            if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[0], &pdsch_cfg.grant)) {
                // std::cout << "ra type: " << dci_dl[0].alloc_type << std::endl;
                ERROR("Error unpacking DCI");
                //   return SRSRAN_ERROR;
            } else {
                // std::cout << pdsch_cfg.grant.tx_scheme << std::endl;

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

            // if (pdsch_cfg.grant.nof_tb == 2) {
            //     pdsch_cfg.grant.nof_tb = 1;
            //     pdsch_cfg.grant.tb[1].enabled = false;
            //     if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, &pdsch_cfg, pdsch_res)) {
            //         ERROR("ERROR: Decoding PDSCH");
            //         ret = -1;
            //     }

            //     pdsch_cfg.grant.tb[0].enabled = false;
            //     pdsch_cfg.grant.tb[1].enabled = true;
            //     if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, &pdsch_cfg, pdsch_res)) {
            //         ERROR("ERROR: Decoding PDSCH");
            //         ret = -1;
            //     }
            // } else if (decode_enable) {
            //     if (dl_sf_cfg->sf_type == SRSRAN_SF_NORM) {
            //         if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, &pdsch_cfg, pdsch_res)) {
            //             ERROR("ERROR: Decoding PDSCH");
            //             ret = -1;
            //         }
            //     } 
            // }

                if (decode_enable) {
                    if (dl_sf_cfg->sf_type == SRSRAN_SF_NORM) {
                        if (srsran_ue_dl_decode_pdsch(ue_dl, dl_sf_cfg, &pdsch_cfg, pdsch_res)) {
                            ERROR("ERROR: Decoding PDSCH");
                            ret = -1;
                        }
                    } 
                }
                if (pdsch_res[0].crc == false && pdsch_res[1].crc == false) {
                    pdsch_cfg.use_tbs_index_alt = true;
                    if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[0], &pdsch_cfg.grant)) {
                        ERROR("Error unpacking DCI");
                    } else {
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
                }
                // for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
                //     if (pdsch_cfg.grant.tb[tb].enabled) {
                //         acks[tb] = pdsch_res[tb].crc;
                //     }
                // }
                char str[512];
                srsran_pdsch_rx_info(&pdsch_cfg, pdsch_res, str, sizeof(str));
                printf("Rach Decoded PDSCH: %s\n", str);
                if (pdsch_res[0].crc == true || pdsch_res[1].crc == true) {
                    srsran::rar_pdu rar_pdu_msg;
                    if (pdsch_cfg.grant.tb[0].enabled) {
                        rar_pdu_msg.init_rx(pdsch_cfg.grant.tb[0].nof_bits / 8, false);
                        ret = rar_pdu_msg.parse_packet(pdsch_res[0].payload);
                    } else if (pdsch_cfg.grant.tb[1].enabled) {
                        rar_pdu_msg.init_rx(pdsch_cfg.grant.tb[1].nof_bits / 8, false);
                        ret = rar_pdu_msg.parse_packet(pdsch_res[1].payload);
                    }
                uint16_t t_crnti = 0;
                if (ret == SRSRAN_SUCCESS) {
                    while (rar_pdu_msg.next()) {
                        if (rar_pdu_msg.get()->has_rapid()) {
                            t_crnti = rar_pdu_msg.get()->get_temp_crnti();
                            if (t_crnti) {
                                std::cout << "Found RACH response! TC-RNTI = " << t_crnti << ", ta cmd = " << rar_pdu_msg.get()->get_ta_cmd() << ", index = " << rar_pdu_msg.get()->has_rapid() << std::endl;
                                tcrntiDatabase->push_tcRNTI(t_crnti);
                            }
                        }
                    }
                }
                }
                
            }
        }
    }

    std::vector<uint16_t> tcrntiVec = tcrntiDatabase->get_tcRNTI();
    asn1::json_writer phys_ded_json;
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        srsran_vec_u8_zero(data[i], 2000 * 8);
    }

    for (std::vector<uint16_t>::iterator iter = tcrntiVec.begin(); iter != tcrntiVec.end(); iter++) {
        srsran_dci_dl_t dci_dl[SRSRAN_MAX_DCI_MSG] = {};
        srsran_pdsch_res_t pdsch_res[SRSRAN_MAX_CODEWORDS] = {};
        rnti = *iter;
        pdsch_cfg.rnti = rnti;
        ret = srsran_ue_dl_find_dl_dci(ue_dl, dl_sf_cfg, ue_dl_cfg, rnti, dci_dl);

        if (ret == 1) {
            char str[512];
            srsran_dci_dl_info(&dci_dl[0], str, 512);
            std::cout << "RACH Decoder found DCI for msg4: PDCCH: " << str << ", snr = " << ue_dl->chest_res.snr_db << " dB." << std::endl;

            // Convert DCI message to DL grant
            if (srsran_ue_dl_dci_to_pdsch_grant(ue_dl, dl_sf_cfg, ue_dl_cfg, &dci_dl[0], &pdsch_cfg.grant)) {
                ERROR("Error unpacking DCI");
                tcrntiDatabase->delete_tcRNTI(rnti);
                //   return SRSRAN_ERROR;
            } else {
                // std::cout << pdsch_cfg.grant.tx_scheme << std::endl;
                // ue_dl->pdsch.nof_rx_antennas = 2;
                std::cout << "nof_rx_ant: " << ue_dl->pdsch.nof_rx_antennas << std::endl;
            
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
                for (uint32_t tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
                    if (pdsch_cfg.grant.tb[tb].enabled) {
                        acks[tb] = pdsch_res[tb].crc;
                    }
                }
                asn1::rrc::dl_ccch_msg_s dlcch_msg;
                int nof_bytes = 0;
                for (int tb = 0; tb < SRSRAN_MAX_CODEWORDS; tb++) {
                    if (pdsch_cfg.grant.tb[tb].enabled) {
                        nof_bytes = nof_bytes + pdsch_cfg.grant.tb[tb].nof_bits / 8;
                    }
                }
                asn1::cbit_ref dlcch_bref(pdsch_res->payload, pdsch_cfg.grant.tb[0].tbs / 8);
                asn1::SRSASN_CODE err = dlcch_msg.unpack(dlcch_bref);
                if (err != SRSRAN_SUCCESS) {
                    ERROR("Failed to unpack DL-CCCH message!");
                    tcrntiDatabase->delete_tcRNTI(rnti);
                    return 0;
                } 

                auto& mac_logger = srslog::fetch_basic_logger("MAC");

                asn1::rrc::rrc_conn_setup_s rrc_setup = dlcch_msg.msg.c1().rrc_conn_setup();
                std::cout << "Msg 4 Decoded." << std::endl;
                switch (dlcch_msg.msg.c1().type().value){
                    case asn1::rrc::dl_ccch_msg_type_c::c1_c_::types::rrc_conn_reject:
                    {
                        std::cout << "Unfortunately, this is a rrc reject for this tc-rnti: " << rnti << std::endl;
                        tcrntiDatabase->delete_tcRNTI(rnti);
                        break;
                    }

                    case asn1::rrc::dl_ccch_msg_type_c::c1_c_::types::rrc_conn_setup:
                    {
                        std::cout << "Cool! This is a rrc setup! c-rnti: " << rnti << std::endl;
                        tcrntiDatabase->delete_tcRNTI(rnti);
                    
                        temp_ue_dl_cfg.cfg.pdsch.rnti = rnti;
                        asn1::SRSASN_CODE err = dlcch_msg.unpack(dlcch_bref);
                        if (err == asn1::SRSASN_SUCCESS) {
                            asn1::rrc::phys_cfg_ded_s phys_cfg_ded = dlcch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded;
                            phys_cfg_ded.to_json(phys_ded_json);
                            std::cout << phys_ded_json.to_string() << std::endl;
                        }
                    // temp_ue_dl_cfg.cfg.tm = (srsran_tm_t)dlcch_msg.msg.c1().rrc_conn_setup().crit_exts.c1().rrc_conn_setup_r8().rr_cfg_ded.phys_cfg_ded.ant_info_r10.get()->explicit_value_r10().tx_mode_r10.to_number();
                    
                        break;
                    }

                    case asn1::rrc::dl_ccch_msg_type_c::c1_c_::types::rrc_conn_reest_reject:
                    {
                        std::cout << "Unfortunately, this is a rrc re-establishment reject for this tc-rnti: " << rnti << std::endl;
                        tcrntiDatabase->delete_tcRNTI(rnti);
                        break;
                    }

                    case asn1::rrc::dl_ccch_msg_type_c::c1_c_::types::rrc_conn_reest:
                    {
                        std::cout << "Cool! This is a rrc re-establishment! c-rnti: " << rnti << std::endl;
                        tcrntiDatabase->delete_tcRNTI(rnti);
                        temp_ue_dl_cfg.cfg.pdsch.rnti = rnti;
                    // asn1::rrc::phys_cfg_ded_s phys_cfg_ded = dlcch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded;
                    // phys_cfg_ded.unpack(phys_cbref);
                        asn1::SRSASN_CODE err = dlcch_msg.unpack(dlcch_bref);
                        if (err == asn1::SRSASN_SUCCESS) {
                            asn1::rrc::phys_cfg_ded_s phys_cfg_ded = dlcch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded;
                            phys_cfg_ded.to_json(phys_ded_json);
                            std::cout << phys_ded_json.to_string() << std::endl;
                        }
                    // temp_ue_dl_cfg.cfg.tm = (srsran_tm_t)dlcch_msg.msg.c1().rrc_conn_reest().crit_exts.c1().rrc_conn_reest_r8().rr_cfg_ded.phys_cfg_ded.
                        break;
                    }
            
                    default:
                    {
                        break;
                    }
                }
            }
        }

    }
    return 0;
}
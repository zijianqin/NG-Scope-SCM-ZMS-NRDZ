#include "../hdr/singleThread.h"

enum single_worker_state {mibdecode, sfdecode}single_worker_state;


void sf_decoder_init(srsran_cell_t cell, srsran_ul_cfg_t* ul_cfg, srsran_pdsch_cfg_t* pdsch_cfg, srsran_pusch_cfg_t* pusch_cfg, srsran_ue_dl_t* ue_dl, srsran_ue_ul_t* ue_ul, srsran_ue_dl_cfg_t* ue_dl_cfg,
srsran_ue_ul_cfg_t* ue_ul_cfg, srsran_dl_sf_cfg_t* dl_sf_cfg, srsran_ul_sf_cfg_t* ul_sf_cfg, srsran_refsignal_dmrs_pusch_cfg_t* dmrs_pusch_cfg, srsran_refsignal_srs_cfg_t* srs_cfg, srsran_enb_ul_t* enb_ul,
cf_t** sf_buffer, cf_t** ul_buffer, std::shared_ptr<ngscope_config_t> ngscope_config) 
{
    // ZERO_OBJECT(ul_cfg);
    // ZERO_OBJECT(pdsch_cfg);
    // ZERO_OBJECT(pusch_cfg);
    // ZERO_OBJECT(ue_dl);
    // ZERO_OBJECT(ue_ul);
    // ZERO_OBJECT(ue_dl_cfg);
    // ZERO_OBJECT(ue_ul_cfg);
    // ZERO_OBJECT(dl_sf_cfg);
    // ZERO_OBJECT(ul_sf_cfg);
    // ZERO_OBJECT(dmrs_pusch_cfg);
    // ZERO_OBJECT(srs_cfg);
    // ZERO_OBJECT(enb_ul);
    uint32_t max_num_samples = 3 * SRSRAN_SF_LEN_PRB(cell.nof_prb);
    srsran_chest_dl_cfg_t chest_pdsch_cfg;
    // sf_buffer = new cf_t*[SRSRAN_MAX_PORTS];
    // for (int i = 0; i < SRSRAN_MAX_PORTS; i++) {
    //     sf_buffer[i] = srsran_vec_cf_malloc(max_num_samples);
    // }

    ul_buffer = new cf_t*[SRSRAN_MAX_PORTS];
    for (int i = 0; i < SRSRAN_MAX_PORTS; i++) {
        ul_buffer[i] = srsran_vec_cf_malloc(max_num_samples);
    }

    if (srsran_ue_dl_init(ue_dl, sf_buffer, cell.nof_prb, 1)) {
        ERROR("Error initiating UE downlink processing module");
        exit(-1);
    }
    if (srsran_ue_dl_set_cell(ue_dl, cell)) {
        ERROR("Error initiating UE downlink processing module");
        exit(-1);
    }

    if (srsran_ue_ul_init(ue_ul, ul_buffer[0], cell.nof_prb))
    {
        ERROR("Error initiating UE uplink");
        exit(-1);
    }

    if (srsran_ue_ul_set_cell(ue_ul, cell)) {
        ERROR("Error initiating UE uplink cell");
        exit(-1);
    }

    if (srsran_enb_ul_init(enb_ul, sf_buffer[0], cell.nof_prb)) {
        ERROR("Error initiating enb uplink");
        exit(-1);
    }

    bool enb_ul_init = false;

    chest_pdsch_cfg.cfo_estimate_enable   = ngscope_config->prog_args.enable_cfo_ref;
    chest_pdsch_cfg.cfo_estimate_sf_mask  = 1023;
    chest_pdsch_cfg.estimator_alg         = srsran_chest_dl_str2estimator_alg(ngscope_config->prog_args.estimator_alg);
    chest_pdsch_cfg.sync_error_enable     = true;

    dl_sf_cfg->sf_type = SRSRAN_SF_NORM;
    if (ngscope_config->cell.frame_type == SRSRAN_TDD && ngscope_config->prog_args.tdd_special_sf >= 0 && ngscope_config->prog_args.sf_config >= 0) {
        dl_sf_cfg->tdd_config.ss_config  = ngscope_config->prog_args.tdd_special_sf;
        dl_sf_cfg->tdd_config.sf_config  = ngscope_config->prog_args.sf_config;
        dl_sf_cfg->tdd_config.configured = true;
    }

    ue_dl_cfg->cfg.tm = (srsran_tm_t)1;
    ue_dl_cfg->cfg.pdsch.use_tbs_index_alt = true;
    ue_dl_cfg->cfg.dci.multiple_csi_request_enabled = false;
    ue_dl_cfg->chest_cfg = chest_pdsch_cfg;

    srsran_softbuffer_rx_t rx_softbuffers[SRSRAN_MAX_CODEWORDS];
    for (uint32_t i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        pdsch_cfg->softbuffers.rx[i] = &rx_softbuffers[i];
        srsran_softbuffer_rx_init(pdsch_cfg->softbuffers.rx[i], ngscope_config->cell.nof_prb);
    }
    pdsch_cfg->meas_evm_en = true;

    srsran_softbuffer_rx_init(pusch_cfg->softbuffers.rx, ngscope_config->cell.nof_prb);

    srsran_ue_dl_set_non_mbsfn_region(ue_dl, ngscope_config->prog_args.non_mbsfn_region);

    pdsch_cfg->rnti = ngscope_config->RNTI;
}


void decode_sf_single(int tti, int sf_idx, srsran_cell_t cell, srsran_ul_cfg_t* ul_cfg, srsran_pdsch_cfg_t* pdsch_cfg, srsran_pusch_cfg_t* pusch_cfg, srsran_ue_dl_t* ue_dl, 
srsran_ue_ul_t* ue_ul, srsran_ue_dl_cfg_t* ue_dl_cfg, srsran_ue_ul_cfg_t* ue_ul_cfg, srsran_dl_sf_cfg_t* dl_sf_cfg, srsran_ul_sf_cfg_t* ul_sf_cfg, 
srsran_refsignal_dmrs_pusch_cfg_t* dmrs_pusch_cfg, srsran_refsignal_srs_cfg_t* srs_cfg, srsran_enb_ul_t* enb_ul, std::shared_ptr<ngscope_config_t> ngscope_config) 
{
    int ret = 0;


    uint8_t* data[SRSRAN_MAX_CODEWORDS];
    for (int i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        data[i] = srsran_vec_u8_malloc(2000 * 8);
    }
    bool acks[SRSRAN_MAX_CODEWORDS] = {false};
    dl_sf_cfg->tti = tti;

    int tddconfig = dl_sf_cfg->tdd_config.sf_config;

    // pdsch_cfg->rnti = 0x84;
    dl_sf_cfg->tti = tti;
    dl_sf_cfg->tdd_config.configured = true;
    ul_sf_cfg->tti = tti;

    // puschGrant_t puschGrant;
    // std::cout << sf_idx << std::endl;
    
    if (srsran_sfidx_tdd_type(dl_sf_cfg->tdd_config, sf_idx) != SRSRAN_TDD_SF_U) {
        // std::cout << "In DL sf" << std::endl;
        // ret = srsran_ue_dl_find_and_decode_pdsch_ul_grant(ue_dl, ue_ul, dl_sf_cfg, ul_sf_cfg, ue_dl_cfg, ue_ul_cfg, pdsch_cfg, pusch_cfg, data, acks);
    }

    
}


void singleThreadProc(std::shared_ptr<ngscope_config_t> ngscope_config, srsran_ue_sync_t* ue_sync, srsran_cell_t cell, cf_t** cur_buffer) {
    bool go_exit = false;
    int ret = 0;
    int sf_idx;
    int tti;
    uint32_t sfn;
    uint32_t max_num_samples = 3 * SRSRAN_SF_LEN_PRB(cell.nof_prb); 
    srsran_ue_mib_t ue_mib = { };
    srsran_ul_cfg_t ul_cfg = { };
    srsran_pdsch_cfg_t pdsch_cfg = { };
    srsran_pusch_cfg_t pusch_cfg = { }; 
    srsran_ue_dl_t ue_dl = { };
    srsran_ue_ul_t ue_ul = { };
    srsran_ue_dl_cfg_t ue_dl_cfg = { };
    srsran_ue_ul_cfg_t ue_ul_cfg = { }; 
    srsran_dl_sf_cfg_t dl_sf_cfg = { }; 
    srsran_ul_sf_cfg_t ul_sf_cfg = { }; 
    srsran_refsignal_dmrs_pusch_cfg_t dmrs_pusch_cfg = { }; 
    srsran_refsignal_srs_cfg_t srs_cfg = { }; 
    srsran_enb_ul_t enb_ul { };
    cf_t* ul_buffer[SRSRAN_MAX_PORTS] = {NULL};


    srsran_pbch_decode_reset(&ue_mib.pbch);

    if (srsran_ue_mib_init(&ue_mib, cur_buffer[0], cell.nof_prb)) {
        ERROR("Error initaiting UE MIB decoder");
        exit(-1);
    }
    if (srsran_ue_mib_set_cell(&ue_mib, cell)) {
        ERROR("Error initaiting UE MIB decoder");
        exit(-1);
    }

    // sf_decoder_init(cell, &ul_cfg, &pdsch_cfg, &pusch_cfg, &ue_dl, &ue_ul, &ue_dl_cfg, &ue_ul_cfg, &dl_sf_cfg, &ul_sf_cfg, &dmrs_pusch_cfg, &srs_cfg, &enb_ul, cur_buffer, ul_buffer, ngscope_config);

    single_worker_state = mibdecode;

    while (!go_exit) {
        cf_t* buffers[SRSRAN_MAX_CHANNELS] = {};
        for (int p = 0; p < SRSRAN_MAX_PORTS; p++) {
            buffers[p] =cur_buffer[p];
        }
        ret = srsran_ue_sync_zerocopy(ue_sync, buffers, max_num_samples);

        if (ret < 0) {
            ERROR("Error calling srsran_ue_sync_work()");
        } else if (ret == 1) {
            sf_idx = srsran_ue_sync_get_sfidx(ue_sync);

            switch (single_worker_state)
            {
            case mibdecode:
                if (sf_idx == 0) {
                    uint8_t bch_payload[SRSRAN_BCH_PAYLOAD_LEN];
                    int sfn_offset;
                    int n = srsran_ue_mib_decode(&ue_mib, bch_payload, NULL, &sfn_offset);

                    if (n < 0) {
                        ERROR("Error decoding UE MIB!");
                        exit(-1);
                    } else if (n == SRSRAN_UE_MIB_FOUND) {
                        srsran_pbch_mib_unpack(bch_payload, &cell, &sfn);
                        srsran_cell_fprint(stdout, &cell, sfn);
                        sfn = (sfn + sfn_offset) % 1024;

                        srsran_ue_mib_free(&ue_mib);

                        single_worker_state = sfdecode;
                        sf_decoder_init(cell, &ul_cfg, &pdsch_cfg, &pusch_cfg, &ue_dl, &ue_ul, &ue_dl_cfg, &ue_ul_cfg, &dl_sf_cfg, &ul_sf_cfg, &dmrs_pusch_cfg, &srs_cfg, &enb_ul, cur_buffer, ul_buffer, ngscope_config);
                    }
                }
                break;
            
            case sfdecode:
                tti = sfn * 10 + sf_idx;
                // std::cout << tti << std::endl;
                decode_sf_single(tti, sf_idx, cell, &ul_cfg, &pdsch_cfg, &pusch_cfg, &ue_dl, &ue_ul, &ue_dl_cfg, &ue_ul_cfg, &dl_sf_cfg, &ul_sf_cfg, &dmrs_pusch_cfg, &srs_cfg, &enb_ul, ngscope_config);
                if (sf_idx == 5 && ngscope_config->prog_args.enable_cfo_ref) {
                    srsran_ue_sync_set_cfo_ref(ue_sync, ue_dl.chest_res.cfo);
                }

                break;

            default:
                break;
            }
            if (sf_idx == 9) {
                sfn++;
                if (sfn == 1024) {
                    sfn = 0;
                }
            } 
        } 
    }
}
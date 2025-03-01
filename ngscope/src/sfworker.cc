#include "../hdr/sfworker.h"

SFWorker::SFWorker(uint32_t workerIdx, std::shared_ptr<ngscope_config_t> &ngscope_config) : 
    workerIdx(workerIdx), ngscope_config(ngscope_config), stop(false), isAvailable(true)
{   
    ngscopeMode = ngscope_config->ngscopeMode;
    ZERO_OBJECT(dl_cfg);
    ZERO_OBJECT(ul_cfg);
    ZERO_OBJECT(pdsch_cfg);
    ZERO_OBJECT(pusch_cfg);
    ZERO_OBJECT(pucch_cfg);
    ZERO_OBJECT(ue_dl);
    ZERO_OBJECT(ue_ul);
    ZERO_OBJECT(ue_dl_cfg);
    ZERO_OBJECT(ue_ul_cfg);
    ZERO_OBJECT(dl_sf_cfg);
    ZERO_OBJECT(ul_sf_cfg);
    ZERO_OBJECT(dmrs_pusch_cfg);
    ZERO_OBJECT(srs_cfg);
    ZERO_OBJECT(enb_ul);
    cell = ngscope_config->cell;
    isConfig = false;
    have_sib1 = false;
    have_sib2 = false;
    enb_ul_init = false;
    uint32_t max_num_samples = 3 * SRSRAN_SF_LEN_PRB(ngscope_config->cell.nof_prb);
    sf_buffer = new cf_t*[SRSRAN_MAX_PORTS];
    for (int i = 0; i < SRSRAN_MAX_PORTS; i++) {
        sf_buffer[i] = static_cast<cf_t*>(srsran_vec_malloc(3*static_cast<uint32_t>(sizeof(cf_t))*static_cast<uint32_t>(SRSRAN_SF_LEN_PRB(100))));
    }

    ul_buffer = new cf_t*[SRSRAN_MAX_PORTS];
    for (int i = 0; i < SRSRAN_MAX_PORTS; i++) {
        ul_buffer[i] = srsran_vec_cf_malloc(max_num_samples);
    }

    ue_dl.nof_rx_antennas = ngscope_config->prog_args.rf_nof_rx_ant;
    ue_dl.pdsch.nof_rx_antennas = ngscope_config->prog_args.rf_nof_rx_ant;

    if (srsran_ue_dl_init(&ue_dl, sf_buffer, ngscope_config->cell.nof_prb, ngscope_config->prog_args.rf_nof_rx_ant)) {
        ERROR("Error initiating UE downlink processing module");
        exit(-1);
    }
    if (srsran_ue_dl_set_cell(&ue_dl, ngscope_config->cell)) {
        ERROR("Error initiating UE downlink processing module");
        exit(-1);
    }

    if (srsran_ue_ul_init(&ue_ul, ul_buffer[0], ngscope_config->cell.nof_prb))
    {
        ERROR("Error initiating UE uplink");
        exit(-1);
    }

    if (srsran_ue_ul_set_cell(&ue_ul, ngscope_config->cell)) {
        ERROR("Error initiating UE uplink cell");
        exit(-1);
    }

    if (srsran_enb_ul_init(&enb_ul, sf_buffer[0], 110)) {
        ERROR("Error initiating enb uplink");
        exit(-1);
    }

    // if (srsran_pdcch_init_ue(&ue_dl.pdcch, ngscope_config->cell.nof_prb, ngscope_config->prog_args.rf_nof_rx_ant)) {
    //     ERROR("Error initiating ue pdcch");
    //     exit(-1);
    // }

    // if (srsran_pdsch_init_ue(&ue_dl.pdsch, ngscope_config->cell.nof_prb, ngscope_config->prog_args.rf_nof_rx_ant)) {
    //     ERROR("Error initiating ue pdsch");
    //     exit(-1);
    // }

    bool enb_ul_init = false;

    chest_pdsch_cfg.cfo_estimate_enable   = ngscope_config->prog_args.enable_cfo_ref;
    chest_pdsch_cfg.cfo_estimate_sf_mask  = 1023;
    chest_pdsch_cfg.estimator_alg         = srsran_chest_dl_str2estimator_alg(ngscope_config->prog_args.estimator_alg);
    chest_pdsch_cfg.sync_error_enable     = true;

    dl_sf_cfg.sf_type = SRSRAN_SF_NORM;
    
    ue_dl_cfg.cfg.tm = (srsran_tm_t)1;
    ue_dl_cfg.cfg.pdsch.use_tbs_index_alt = false;
    ue_dl_cfg.cfg.pdsch.rnti = ngscope_config->RNTI;
    ue_dl_cfg.cfg.pdsch.max_nof_iterations = 12;
    ue_dl_cfg.cfg.pdsch.csi_enable = false;
    ue_dl_cfg.cfg.pdsch.meas_evm_en = true;
    ue_dl_cfg.cfg.pdsch.meas_time_en = false;
    ue_dl_cfg.cfg.pdsch.power_scale = true;
    ue_dl_cfg.cfg.dci.multiple_csi_request_enabled = false;
    ue_dl_cfg.cfg.dci.srs_request_enabled = false;
    ue_dl_cfg.cfg.dci.cif_enabled = false;
    ue_dl_cfg.cfg.dci.ra_format_enabled = false;
    ue_dl_cfg.cfg.cqi_report.periodic_mode = SRSRAN_CQI_MODE_12;
    ue_dl_cfg.chest_cfg = chest_pdsch_cfg;

    have_cfg_ded = false;

    // srsran_softbuffer_rx_t rx_softbuffers[SRSRAN_MAX_CODEWORDS];
    srsran_softbuffer_rx_t* rx_softbuffers;
    rx_softbuffers = new srsran_softbuffer_rx_t[SRSRAN_MAX_CODEWORDS];
    for (uint32_t i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
        ue_dl_cfg.cfg.pdsch.softbuffers.rx[i] = &rx_softbuffers[i];
        srsran_softbuffer_rx_init(ue_dl_cfg.cfg.pdsch.softbuffers.rx[i], cell.nof_prb);
    }

    // for (uint32_t i = 0; i < SRSRAN_MAX_CODEWORDS; i++) {
    //     pdsch_cfg.softbuffers.rx[i] = &rx_softbuffers[i];
    //     srsran_softbuffer_rx_init(pdsch_cfg.softbuffers.rx[i], cell.nof_prb);
    // }
    ue_dl_cfg.cfg.pdsch.meas_evm_en = true;

    srsran_softbuffer_rx_t* rx_ul_softbuffers;
    rx_ul_softbuffers = new srsran_softbuffer_rx_t;
    ue_ul_cfg.ul_cfg.pusch.softbuffers.rx = rx_ul_softbuffers;
    srsran_softbuffer_rx_init(ue_ul_cfg.ul_cfg.pusch.softbuffers.rx, SRSRAN_MAX_PRB);

    ue_dl_cfg.cfg.pdsch.csi_enable = true;
    ue_dl_cfg.cfg.pdsch.rnti = ngscope_config->RNTI;
    // ue_dl_cfg.cfg.pdsch = pdsch_cfg;
    // dl_cfg.pdsch = pdsch_cfg;
    // ue_dl_cfg.cfg.pdsch = pdsch_cfg;
    ue_ul_cfg.ul_cfg.pusch.rnti = pdsch_cfg.rnti;
    // ul_cfg.pusch = pusch_cfg;
    // asn1::rrc::sib_type2_s sib2 = ngscope_config->sib2;
    // srsran_cell_t cell = ngscope_config->cell;

    sfThread = std::thread(&SFWorker::sfWorkerThread, this);
    setThreadAffinity(sfThread, workerIdx);
    sfThread.detach();
}


SFWorker::~SFWorker() {
    
    stop = true;
    cv.notify_all();
    if (sfThread.joinable()) {
        sfThread.join();
    }
    
    for (int i = 0; i < SRSRAN_MAX_PORTS; i++) {
        delete[] sf_buffer[i];
    }
    delete[] sf_buffer;
}


void SFWorker::sfWorkerThread() {
    while (!stop) {
        std::unique_lock<std::mutex> lock(mutex);
        cv.wait(lock, [this] {return task != nullptr || stop;});

        if (stop) {
            break;
        }

        if (task) {
            if (ngscopeMode == DL_MODE) {
                // std::cout << "dl task executing..." << std::endl;
                task->execute(&ue_dl, sf_buffer, &ue_dl_cfg, &dl_sf_cfg, &pdsch_cfg);
            } else if (ngscopeMode == UL_MODE) {
                task->run_ul_mode(this);
                // task->execute_ul(cell, &ue_dl, &ue_ul, sf_buffer, &ue_dl_cfg, &dl_sf_cfg, &ul_sf_cfg, &ue_ul_cfg, &pdsch_cfg, &dl_cfg, &ul_cfg, &enb_ul, &have_sib1, &have_sib2);
                // std::cout << "ul task executing..." << std::endl;
            }
            task = nullptr;
            isAvailable = true;
        }
        lock.unlock();
    }
}


void SFWorker::assignTask(std::unique_ptr<sfTask> task) {
    {
        // std::lock_guard<std::mutex> lock(mutex);
        this->task = std::move(task);
        isAvailable = false;
    }
    cv.notify_one();
}


void SFWorker::config(asn1::rrc::sib_type2_s sib2) {
    if (!isConfig) {
        dmrs_pusch_cfg.cyclic_shift = sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.cyclic_shift;
        dmrs_pusch_cfg.group_hopping_en = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.group_hop_enabled;
        dmrs_pusch_cfg.sequence_hopping_en = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.seq_hop_enabled;
        dmrs_pusch_cfg.delta_ss = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.ul_ref_sigs_pusch.group_assign_pusch;

        ul_cfg.hopping.hop_mode = srsran_pusch_hopping_cfg_t::SRSRAN_PUSCH_HOP_MODE_INTER_SF;
        ul_cfg.hopping.hopping_offset = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.pusch_hop_offset;
        ul_cfg.hopping.n_sb = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.n_sb;
        ul_cfg.hopping.n_rb_ho = ngscope_config->sib2.rr_cfg_common.pusch_cfg_common.pusch_cfg_basic.pusch_hop_offset;

        isConfig = true;

        std::cout << "Finish worker init!" << std::endl;
    }
}


void SFWorker::setThreadAffinity(std::thread& thread, int cpu_id) {
    pthread_t native_handle = thread.native_handle();

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(cpu_id, &cpu_set);

    int result = pthread_setaffinity_np(native_handle, sizeof(cpu_set_t), &cpu_set);
    if (result != 0) {
        std::cerr << "Error setting CPU affinity for thread: " << result << std::endl;
    } else {
        std::cout << "Thread affinity set to CPU " << cpu_id << std::endl;
    }
}
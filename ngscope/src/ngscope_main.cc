/*************************************************
 * A new version of NG-Scope 4G
 * Feature: 1. decode both downlink and uplink data
 *          2. support SCM generation API
 *          3. support COSMOS API
 *          4. object-oriented programming
 *          5. support X310 and X410 USRP
 *          6. support mac, rlc, and pdcp layer decoding
 * Author: Princeton Advanced Wireless Systems
*************************************************/
#include "../hdr/main.h"

enum state {DECODE_MIB, DECODE_SIB, DECODE_SF} state;

enum{main_func, config_file, arg_num} comman_arg;


static SRSRAN_AGC_CALLBACK(srsran_rf_set_rx_gain_th_wrapper_)
{
  srsran_rf_set_rx_gain_th((srsran_rf_t*)h, gain_db);
}

int main(int argc, char** argv){
    if (argc != arg_num) {
        std::cout << "The total number of command line args should be " << arg_num << "." <<std::endl;
    }

    srsran_rf_t rf;
    cell_search_cfg_t cell_detect_config = {.max_frames_pbch      = SRSRAN_DEFAULT_MAX_FRAMES_PBCH,
                                            .max_frames_pss       = SRSRAN_DEFAULT_MAX_FRAMES_PSS,
                                            .nof_valid_pss_frames = SRSRAN_DEFAULT_NOF_VALID_PSS_FRAMES,
                                            .init_agc             = 0,
                                            .force_tdd            = false};
    srsran_ue_sync_t ue_sync = { };
    srsran_ue_mib_t ue_mib = { };
    float search_cell_cfo = 0;

    args ngscopeArgs(argc, argv);
    std::shared_ptr<ngscope_config_t> ngscope_config = std::make_shared<ngscope_config_t>();
    ngscopeArgs.init_ngscope_config(ngscope_config);
    std::cout << "Successfully init ngscope config!" << std::endl;
    
    // std::unique_ptr<srsran::radio> radio = std::unique_ptr<srsran::radio>(new srsran::radio);
    // srsran::rf_args_t rf_args = {};
    // rf_args.device_args = ngscopeArgs.get_device_name();
    // rf_args.nof_carriers = 2;
    // rf_args.nof_antennas = 2;

    // radio->init(rf_args, nullptr);
    // radio->set_rx_srate(SRSRAN_CS_SAMP_FREQ);
    // radio->set_rx_freq(0, ngscopeArgs.get_dl_freq());
    // radio->set_rx_gain(ngscopeArgs.get_rf_gain());

    bool have_sib2 = false;
    bool go_exit = false;
    int ret = 0;
    uint32_t sfn = 0;
    uint32_t sf_cnt = 0;
    uint32_t sf_idx = 0;
    uint32_t tti = 0;
    long long dl_freq = ngscopeArgs.get_ul_freq();
    long long ul_freq = ngscopeArgs.get_dl_freq();
    asn1::rrc::sib_type2_s sib2;

    // if (srsran_rf_open_multi(&rf, ngscopeArgs.get_device_name(), ngscopeArgs.get_nof_rx_ant()) != SRSRAN_SUCCESS) {
    if (srsran_rf_open_devname(&rf, "", ngscopeArgs.get_device_name(), ngscopeArgs.get_nof_rx_ant()) != SRSRAN_SUCCESS) {
        ERROR("Error opening rf devices!\n");
        exit(-1);
    }
    std::cout << "nof rx ant: " << ngscopeArgs.get_nof_rx_ant() << std::endl;

    // uhd::device_addr_t dev_addr;
    // dev_addr["addr0"] = "192.168.10.4";
    // uhd::usrp::multi_usrp::sptr dev = uhd::usrp::multi_usrp::make("192.1");
    // const std::string ch0 = "A:A";
    // const std::string ch1 = "A:B";
    // dev->set_rx_subdev_spec(ch0, 0);
    // dev->set_rx_subdev_spec(ch1, 1);
    // (uhd::usrp::multi_usrp::sptr)(rf.dev)->set_rx_subdev_spec("A:A", 0);
    // (uhd::usrp::multi_usrp::sptr)(rf.dev)->set_rx_subdev_spec("A:B", 1);


    if (ngscopeArgs.get_rf_gain() > 0) {
        srsran_rf_set_rx_gain(&rf, ngscopeArgs.get_rf_gain());
    } else {
        printf("Starting AGC thread...\n");
        if (srsran_rf_start_gain_thread(&rf, false)) {
            ERROR("Error opening rf");
            exit(-1);
        }
        srsran_rf_set_rx_gain(&rf, srsran_rf_get_rx_gain(&rf));
        cell_detect_config.init_agc = srsran_rf_get_rx_gain(&rf);
    }

    srsran_rf_set_rx_freq(&rf, 0, ngscopeArgs.get_dl_freq());
    srsran_rf_set_rx_freq(&rf, 1, ngscopeArgs.get_ul_freq());
    // if (dl_freq == ul_freq) {
    //     srsran_rf_set_rx_freq(&rf, 0, dl_freq);
    // } else {
    //     srsran_rf_set_rx_freq(&rf, 1, ngscopeArgs.get_ul_freq());
    //     srsran_rf_set_rx_freq(&rf, 0, ngscopeArgs.get_dl_freq());
    // }

    // if (ngscopeArgs.get_ngscope_mode() == UL_MODE) {
    //     if (srsran_rf_set_rx_freq(&rf, 0, ngscopeArgs.get_dl_freq() + ngscope_config->prog_args.file_offset_freq)) {
    //         ERROR("Error setting dl freq!\n");
    //         exit(-1);
    //     } else {
    //         std::cout << "Turning downlink frequency to " << (ngscopeArgs.get_dl_freq() + ngscope_config->prog_args.file_offset_freq) / 1000000 << "MHz." << std::endl;
    //     }

    //     if (srsran_rf_set_rx_freq(&rf, 1, ngscopeArgs.get_ul_freq())) {
    //         ERROR("Error setting ul freq!\n");
    //         exit(-1);
    //     } else {
    //         std::cout << "Turning uplink frequency to " << ngscopeArgs.get_ul_freq() / 1000000 << "MHz." << std::endl;
    //     }
    // } else {
    //     std::cout << "Turning downlink frequency to " << (ngscopeArgs.get_dl_freq() + ngscope_config->prog_args.file_offset_freq) / 1000000 << "MHz." << std::endl;
    //     srsran_rf_set_rx_freq(&rf, 0, ngscopeArgs.get_dl_freq());
    // }

    for (uint32_t ntrial = 0; ntrial < 50 && ret == 0; ntrial++){
        ret = mib_search(&rf, 1, &cell_detect_config, ngscope_config->prog_args.force_N_id_2, &ngscope_config->cell, &search_cell_cfo);
        // ret = rf_search_and_decode_mib(&rf, ngscope_config->prog_args.rf_nof_rx_ant, &cell_detect_config, ngscope_config->prog_args.force_N_id_2, &ngscope_config->cell, &search_cell_cfo);
        // std::cout << "ret = " << ret << std::endl;
        if (ret > 0) {
            break;
        } else if (ret < 0) {
            ERROR("Error searching for cell");
            exit(-1);
        } else if (ret == 0 && ntrial < 50) {
            std::cout << "Cell not found after " << ntrial << " trials. Trying again (Press Ctrl+C to exit)" << std::endl;
        }
    }
    // std::cout << "nof prb = " << ngscope_config->cell.nof_prb << std::endl;

    int srate = srsran_sampling_freq_hz(ngscope_config->cell.nof_prb);
    if (srate != -1) {
      std::cout << "Setting sampling rate" << srate / 1000000 << " MHz." << std::endl;
      float srate_rf = srsran_rf_set_rx_srate(&rf, (double)srate);
      if (srate_rf != srate) {
        ERROR("Could not set sampling rate");
        exit(-1);
      }
    } else {
        ERROR("Invalid number of PRB %d", ngscope_config->cell.nof_prb);
        exit(-1);
    }


    int decimate = 0;
    if (ngscope_config->prog_args.decimate) {
        if (ngscope_config->prog_args.decimate > 4 || ngscope_config->prog_args.decimate < 0) {
            printf("Invalid decimation factor, setting to 1 \n");
        } else {
            decimate = ngscope_config->prog_args.decimate;
        }
    }
    if (srsran_ue_sync_init_multi_decim(&ue_sync,
                                        ngscope_config->cell.nof_prb,
                                        ngscope_config->cell.id == 1000,
                                        srsran_rf_recv_wrapper,
                                        ngscope_config->prog_args.rf_nof_rx_ant,
                                        (void*)&rf,
                                        decimate)) {
      ERROR("Error initiating ue_sync");
      exit(-1);
    }
    if (srsran_ue_sync_set_cell(&ue_sync, ngscope_config->cell)) {
      ERROR("Error initiating ue_sync");
      exit(-1);
    }

    srsran_rf_start_rx_stream(&rf, false);

    srsran_rf_info_t* rf_info = srsran_rf_get_info(&rf);
    srsran_ue_sync_start_agc(&ue_sync,
                             srsran_rf_set_rx_gain_th_wrapper_,
                             rf_info->min_rx_gain,
                             rf_info->max_rx_gain,
                             cell_detect_config.init_agc);

    ue_sync.cfo_current_value = search_cell_cfo / 15000;
    ue_sync.cfo_is_copied = true;
    ue_sync.cfo_correct_enable_find = true;
    ue_sync.cfo_correct_enable_track = !ngscope_config->prog_args.disable_cfo;
    srsran_pbch_decode_reset(&ue_mib.pbch);

    uint32_t max_num_samples = 3 * SRSRAN_SF_LEN_PRB(ngscope_config->cell.nof_prb);

    BufferPool sfBufferPool(2, ngscope_config->cell.nof_prb);

    cf_t** cur_buffer = sfBufferPool.getBuffer();
    if (srsran_ue_mib_init(&ue_mib, cur_buffer[0], ngscope_config->cell.nof_prb)) {
        ERROR("Error initaiting UE MIB decoder");
        exit(-1);
    }
    if (srsran_ue_mib_set_cell(&ue_mib, ngscope_config->cell)) {
        ERROR("Error initaiting UE MIB decoder");
        exit(-1);
    }

    state = DECODE_MIB;

    int multi_worker = ngscopeArgs.get_multi_worker();

    std::cout << "nof rx ant: " << ngscope_config->prog_args.rf_nof_rx_ant << std::endl;

    if (multi_worker > 0) {
        uint32_t workerIdx = 0;
        std::unique_ptr<sfTask> task = nullptr;

        ThreadPool sfWorkerPool(multi_worker, ngscope_config);
        SFWorker* curWorker = nullptr;

        std::cout << "Successfully init worker pool!" << std::endl;

        while(!go_exit) {
            // cf_t** buffer = sfBufferPool.getBuffer();
            cf_t* buffers[2] = {};
            for (int p = 0; p < 2; p++) {
                buffers[p] =cur_buffer[p];
            }
            ret = srsran_ue_sync_zerocopy(&ue_sync, cur_buffer, max_num_samples);
            // std::cout << "ret = " << ret << std::endl;
            if (ret < 0) {
                ERROR("Error calling srsran_ue_sync_work()");
            } else if (ret == 1) {
                sf_idx = srsran_ue_sync_get_sfidx(&ue_sync);
                // std::cout << "sf_idx = " << sf_idx << std::endl;
                switch (state)
                {
                case DECODE_MIB:
                    if (sf_idx == 0) {
                        uint8_t bch_payload[SRSRAN_BCH_PAYLOAD_LEN];
                        int sfn_offset;
                        int n = srsran_ue_mib_decode(&ue_mib, bch_payload, NULL, &sfn_offset);
                        std::cout << "n = " << n << std::endl;
                        if (n < 0) {
                            ERROR("Error decoding UE MIB!");
                            exit(-1);
                        } else if (n == SRSRAN_UE_MIB_FOUND) {
                            srsran_pbch_mib_unpack(bch_payload, &ngscope_config->cell, &sfn);
                            srsran_cell_fprint(stdout, &ngscope_config->cell, sfn);
                            sfn = (sfn + sfn_offset) % 1024;

                            sfBufferPool.returnBuffer(cur_buffer);
                            srsran_ue_mib_free(&ue_mib);

                            std::cout << "nof_ports = " << ngscope_config->cell.nof_ports << std::endl;

                            state = DECODE_SIB;

                            // curWorker = sfWorkerPool.findWorkerByIdx(workerIdx);
                            curWorker = sfWorkerPool.findAvailableWorker();
                            if (curWorker) {
                                cur_buffer = curWorker->getBuffer();
                            }
                        }
                    }

                    break;

                case DECODE_SIB:
                    tti = sfn * 10 + sf_idx;
                    task = std::make_unique<SIBDecoder>(ngscope_config, sfn * 10 + sf_idx, sf_idx);
                    // task = std::make_unique<SIBDecoder>(tti, sf_idx);
                    sfWorkerPool.enqueue(std::move(task), curWorker);

                    curWorker = sfWorkerPool.findAvailableWorker();
                    // curWorker = sfWorkerPool.findWorkerByIdx(workerIdx);
                    if (curWorker) {
                        cur_buffer = curWorker->getBuffer();
                    }

                    if (ngscope_config->sibs.have_sys()) {
                        state = DECODE_SF;
                    }

                    break;

                case DECODE_SF:
                    tti = sfn * 10 + sf_idx;

                    task = std::make_unique<SFDecoder>(ngscope_config, tti, sf_idx);

                    sfWorkerPool.enqueue(std::move(task), curWorker);

                    curWorker = sfWorkerPool.findAvailableWorker();
                    if (curWorker) {
                        cur_buffer = curWorker->getBuffer();
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


            } else if (ret == 0) {

            }
            sf_cnt++;
        }
    } else {
        singleThreadProc(ngscope_config, &ue_sync, ngscope_config->cell, cur_buffer);
        sfBufferPool.returnBuffer(cur_buffer);
    }

    return SRSRAN_SUCCESS;
}
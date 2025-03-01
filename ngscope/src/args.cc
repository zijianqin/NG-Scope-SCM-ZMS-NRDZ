/*************************
 * file name: args.cc
 * author: PAWS
*************************/
#include "../hdr/args.h"

args::args(int argc, char** argv){
    std::cout << "Start reading USRP config... " << std::endl;

    char name[50];
    std::string configFileStr = argv[1];
    const char* configFile_t = configFileStr.c_str();

    std::cout << "Start default args... " << std::endl;
    config_t *cfg = new config_t;
    config_init(cfg);

    std::cout << configFile_t << std::endl;

    config_read_file(cfg, configFile_t);

    // if (!config_read_file(cfg, configFile_t)) {
    //     ERROR("Error opening the config file!\n");
    //     exit(-1);
    // }

    default_args();


    if (!config_lookup_int(cfg, "nof_usrp", &nof_usrp)) {
        ERROR("Error reading number of USRPs!\n");
        exit(-1);
    } else {
        std::cout << "Number of USRP: " << nof_usrp << "." << std::endl;
    }

    if (config_lookup_int(cfg, "multi_worker", &multi_worker)) {
        std::cout << "workers num " << multi_worker << std::endl;
    }

    if (config_lookup_int(cfg, "ngscope_mode", &ngscope_mode)) {
        std::cout << "NG-Scope mode " << ngscope_mode << std::endl;
    }

    if(!config_lookup_int64(cfg, "dl_freq", &dl_freq)) {
        ERROR("Error reading downlink frequency!\n");
        exit(-1);
    } else {
        std::cout << "Downlink frequency: " << dl_freq << "." <<std::endl;
    }

    if(ngscope_mode == UL_MODE && !config_lookup_int64(cfg, "ul_freq", &ul_freq)) {
        ERROR("Error reading uplink frequency");
        exit(-1);
    } else {
        std::cout << "Uplink frequency: " << ul_freq << "." <<std::endl;
    }

    if (!config_lookup_int(cfg, "rnti", &rnti)) {
        std::cout << "No RNTI input." << std::endl;
    } else {
        std::cout << "Target RNTI: " << rnti << std::endl;
    }

    for (int i = 0; i < nof_usrp; i++) {
        sprintf(name, "rf_config%d.rf_args",i);
        const char *rf_args_tmp;
        if(!config_lookup_string(cfg, name, &rf_args_tmp)) {
            ERROR("Error reading rf_args for %dth USRP!\n", i);
            exit(-1);
        } else {
            strcpy(rf_args[i], rf_args_tmp);
            std::cout << "rf_args of " << i << "th USRP: " << rf_args[i] << "." << std::endl;
        }

        sprintf(name, "rf_config%d.N_id_2",i);
        if(!config_lookup_int(cfg, name, &N_id_2[i])) {
            ERROR("Error reading N_id_2 for %dth USRP!\n", i);
            exit(-1);
        } else {
            std::cout << "N_id_2 of " << i << "th USRP: " << N_id_2[i] << "." << std::endl;
        }
    }
    std::cout << "Successfully read config for USRPs!" << std::endl;

}


void args::default_args(){
    ngscope_mode = DL_MODE;
    rf_gain = -1.0;
    nof_rf_rx_ant = 1;

    for (int i = 0; i < MAX_USRP_NUM; i++) {
        dl_freq = -1;
        ul_freq = -1;
        // rf_args[i] = "";
    }
}


void args::init_ngscope_config(std::shared_ptr<ngscope_config_t> ngscope_config) {
    ngscope_config->ngscopeMode = ngscope_mode;
    ngscope_config->RNTI = rnti;
    if (rnti != 0) {
        ngscope_config->ueDataBase.pushUE(rnti);
    }

    ngscope_config->prog_args.disable_plots = 1;
    ngscope_config->prog_args.force_N_id_2 = -1;
    ngscope_config->prog_args.input_file_name = NULL;
    ngscope_config->prog_args.disable_plots_except_constellation = false;
    ngscope_config->prog_args.nof_subframes = -1;
    ngscope_config->prog_args.cpu_affinity = -1;
    ngscope_config->prog_args.disable_cfo = false;
    ngscope_config->prog_args.time_offset = 0;
    ngscope_config->prog_args.file_offset_freq = 0;
    ngscope_config->prog_args.file_offset_time = 0;
    ngscope_config->prog_args.enable_256qam = false;
    ngscope_config->prog_args.enable_cfo_ref = false;
    ngscope_config->prog_args.estimator_alg = "interpolate";
    ngscope_config->prog_args.rf_gain = -1;
    ngscope_config->prog_args.net_port = -1;
    ngscope_config->prog_args.net_port_signal = -1;
    ngscope_config->prog_args.net_address = "127.0.0.1";
    ngscope_config->prog_args.net_address_signal = "127.0.0.1";
    ngscope_config->prog_args.mbsfn_sf_mask = 32;
    ngscope_config->prog_args.mbsfn_area_id = -1;
    ngscope_config->prog_args.non_mbsfn_region = 2;
    ngscope_config->prog_args.rf_nof_rx_ant = RF_ANT_NUM;
    ngscope_config->prog_args.tdd_special_sf = -1;
    ngscope_config->prog_args.sf_config = 2;

    ZERO_OBJECT(ngscope_config->cell);
}
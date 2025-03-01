#ifndef _MAIN_CELL_SCM_
#define _MAIN_CELL_SCM_

#include <iostream>
#include <memory>
#include <string>
#include <stdio.h>
#include <libconfig.h>
#include <unistd.h>
#include <time.h>

#include "srsran/srsran.h"
#include "srsran/phy/common/phy_common.h"
#include "srsran/phy/rf/rf_utils.h"
#include "srsran/asn1/rrc/si.h"
#include "srsran/asn1/rrc.h"
#include "srsran/asn1/rrc_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

int mib_search(srsran_rf_t* rf, uint32_t nof_rx_channels, cell_search_cfg_t* config, int force_N_id_2, srsran_cell_t* cell, float* cfo);

typedef struct {
    srsran_ue_mib_t mib;
    asn1::rrc::sib_type1_s sib1;
    asn1::rrc::sib_type2_s sib2;

    int bw;
}cell_cfg_t;

typedef struct {
    /* data */
}ue_db_t;

typedef struct {
    srsran_cell_t cell;
    cell_cfg_t cell_cfg;
    ue_db_t ue_db;
}cells_t;


#ifdef __cplusplus
}
#endif

#endif
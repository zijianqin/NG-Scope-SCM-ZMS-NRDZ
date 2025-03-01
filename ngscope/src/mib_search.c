#include "srsran/srsran.h"
#include "srsran/phy/rf/rf_utils.h"

int mib_search(srsran_rf_t* rf,
                             uint32_t           nof_rx_channels,
                             cell_search_cfg_t* config,
                             int                force_N_id_2,
                             srsran_cell_t*     cell,
                             float*             cfo) {
    return rf_search_and_decode_mib(rf, nof_rx_channels, config, force_N_id_2, cell, cfo);
}
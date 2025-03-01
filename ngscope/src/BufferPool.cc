#include "../hdr/BufferPool.h"

BufferPool::BufferPool(uint32_t nof_buffer, uint32_t nof_prb) : nof_buffer(nof_buffer), nof_prb(nof_prb) {
    for (int i = 0; i < nof_buffer; i++) {
        sfBuffers.push(createBuffer());
    }
}


BufferPool::~BufferPool(){
    while(!sfBuffers.empty()) {
        cf_t** buffer = sfBuffers.front();
        sfBuffers.pop();

        for (int i = 0; i < SRSRAN_MAX_PORTS; i++) {
            delete[] buffer[i];
        }
        delete[] buffer;
    }
}


cf_t** BufferPool::getBuffer(){
    if (sfBuffers.empty()) {
        std::cerr << "No available buffers in the pool!" << std::endl;
        return nullptr;
    }

    cf_t** buffer = sfBuffers.front();
    sfBuffers.pop();
    return buffer;
}


void BufferPool::returnBuffer(cf_t** buffer){
    sfBuffers.push(buffer);
}


cf_t** BufferPool::createBuffer(){
    cf_t** buffer = new cf_t*[SRSRAN_MAX_PORTS];
    uint32_t max_num_samples = 3 * SRSRAN_SF_LEN_PRB(nof_prb);

    for (int i = 0; i < SRSRAN_MAX_PORTS; i++) {
        buffer[i] = srsran_vec_cf_malloc(max_num_samples);
    }

    return buffer;
}
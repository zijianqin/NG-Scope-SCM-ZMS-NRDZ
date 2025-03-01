#include "../hdr/ThreadPool.h"

ThreadPool::ThreadPool(const size_t nof_sfworkers, std::shared_ptr<ngscope_config_t> ngscope_config) : ngscope_config(ngscope_config) {
    std::cout << "Start init Worker pool..." << std::endl;
    for (size_t i = 0; i < nof_sfworkers; i++) {
        sfWorkers.emplace_back(std::make_unique<SFWorker>(i, ngscope_config));
        // sfWorkers.emplace_back(sfTaskQueue, queueMutex, cv, stop, ngscope_config);
        // sfThreads.emplace_back(&SFWorker::sfWorkerThread, &sfWorkers.back());
    }
}


void ThreadPool::enqueue(std::unique_ptr<sfTask> task, SFWorker* curWorker) {
    std::lock_guard<std::mutex> lock(workerMutex);
    if (curWorker) {
        curWorker->assignTask(std::move(task));
        // std::cout << "Enqueue a new task!" << std::endl;
    } else {
        std::cout << "No available worker at the moment." << std::endl;
    }
}


SFWorker* ThreadPool::findAvailableWorker() {
    std::lock_guard<std::mutex> lock(workerMutex);
    for (auto& worker : sfWorkers) {
        if (worker->available()) {
            return worker.get();
        }
    }
    return nullptr;
}


SFWorker* ThreadPool::findWorkerByIdx(uint32_t idx) {
    std::lock_guard<std::mutex> lock(workerMutex);
    if (sfWorkers[idx]->available()) {
        return sfWorkers[idx].get();
    }
    // for (auto& worker : sfWorkers) {
    //     if (worker->getWorkerIdx() == idx) {
    //         if (worker->available()) {
    //             return worker.get();
    //         }
    //     }
    // }
    return nullptr;
}


cf_t** ThreadPool::getWorkerBuffer(int workerIdx) {
    if (workerIdx < sfWorkers.size()) {
        return sfWorkers[workerIdx]->getBuffer();
    }
    return nullptr;
}

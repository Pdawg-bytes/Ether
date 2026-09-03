#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class ThreadPool
{
public:
    explicit ThreadPool(u32 threadCount = 0);
    ~ThreadPool();

    ThreadPool(const ThreadPool&)            = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    u32 ThreadCount() const { return (u32)_workers.size(); }

    void Dispatch(const std::function<void()>& job);

private:
    void WorkerLoop();

    std::vector<std::thread> _workers;
    std::mutex			     _mutex;
    std::condition_variable  _wakeCv;
    std::condition_variable  _doneCv;
    std::function<void()>    _job;

    u32 _generation = 0;
    u32 _pending    = 0;
    bool _shutdown  = false;
};

ThreadPool& GetSharedThreadPool();
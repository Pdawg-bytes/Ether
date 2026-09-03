#include "ThreadPool.h"

ThreadPool::ThreadPool(u32 threadCount)
{
    if (threadCount == 0)
    {
        u32 hw      = std::thread::hardware_concurrency();
        threadCount = hw == 0 ? 1 : hw;
    }

    _workers.reserve(threadCount);

    for (u32 i = 0; i < threadCount; i++)
        _workers.emplace_back([this] { WorkerLoop(); });
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard<std::mutex> lock(_mutex);
        _shutdown = true;
    }

    _wakeCv.notify_all();

    for (auto& worker : _workers)
        worker.join();
}

void ThreadPool::Dispatch(const std::function<void()>& job)
{
    {
        std::lock_guard<std::mutex> lock(_mutex);

        _job     = job;
        _pending = (u32)_workers.size();

        _generation++;
    }

    _wakeCv.notify_all();

    std::unique_lock<std::mutex> lock(_mutex);
    _doneCv.wait(lock, [this] { return _pending == 0; });
}

void ThreadPool::WorkerLoop()
{
    u32 lastGeneration = 0;

    for (;;)
    {
        std::function<void()> job;

        {
            std::unique_lock<std::mutex> lock(_mutex);
            _wakeCv.wait(lock, [this, lastGeneration] { return _shutdown || _generation != lastGeneration; });

            if (_shutdown)
                return;

            lastGeneration = _generation;
            job            = _job;
        }

        job();

        {
            std::lock_guard<std::mutex> lock(_mutex);
            if (--_pending == 0)
                _doneCv.notify_one();
        }
    }
}

ThreadPool& GetSharedThreadPool()
{
    static ThreadPool pool;
    return pool;
}
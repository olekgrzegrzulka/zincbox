#pragma once

/**
 * asynchronous job runner with progress polling
 *
 * 1. define structs:
 *    struct MyProgress { float val; };
 *    struct MyResult   { std::string data; };
 *
 * 2. define worker function (last arg must be JobContext<MyProgress, MyResult>):
 *    void task(int x, JobContext<MyProgress, MyResult> ctx) {
 *        ctx.set_progress({0.5f});
 *        ctx.set_result({"done " + std::to_string(x)});
 *    }
 *
 * 3. instantiate and run:
 *    Worker<task, Prog, Res> worker;
 *    auto id = worker.run(42);
 *
 * 4. poll:
 *    auto status = worker.get(id);
 *    if (status.is_empty())    { }
 *    if (status.is_progress()) { const Prog* p = status.get_progress(); }
 *    if (status.is_result())   { std::unique_ptr<Res> r = status.result(); }
 */

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <variant>
#include "common/types.hpp"

template <typename Progress, typename Result> class JobContext {
  public:
    struct State {
        std::mutex mtx;
        Progress progress{};
        std::unique_ptr<Result> result = nullptr;
        std::atomic<bool> finished = false;
    };

    JobContext(std::shared_ptr<State>);

    void set_progress(Progress);
    void set_result(Result);

  private:
    std::shared_ptr<State> m_state;
};

template <typename Progress, typename Result>
class JobStatus : public std::variant<std::monostate, Progress, std::unique_ptr<Result>> {
  public:
    using Base = std::variant<std::monostate, Progress, std::unique_ptr<Result>>;
    using Base::Base;

    bool is_empty() const noexcept { return std::holds_alternative<std::monostate>(*this); }
    bool is_progress() const noexcept { return std::holds_alternative<Progress>(*this); }
    bool is_result() const noexcept { return std::holds_alternative<std::unique_ptr<Result>>(*this); }

    Progress* get_progress() noexcept { return std::get_if<Progress>(this); }
    const Progress* get_progress() const noexcept { return std::get_if<Progress>(this); }
    std::unique_ptr<Result> result() noexcept {
      if (auto* res = std::get_if<std::unique_ptr<Result>>(this)) { return std::move(*res); }
      return nullptr;
    }
};

template <auto Fn, typename Progress, typename Result> class Worker {
  private:
    using State = typename JobContext<Progress, Result>::State;

    struct ITask {
        virtual ~ITask() = default;
        virtual void execute() = 0;
    };

    template <typename F> struct TaskImpl : ITask {
        F func;
        TaskImpl(F&& f) : func(std::move(f)) {}
        void execute() override { func(); }
    };

    std::mutex m_jobs_mutex;
    u64 m_next_job_id = 1;
    std::unordered_map<u64, std::shared_ptr<State>> m_jobs;

    std::mutex m_queue_mutex;
    std::condition_variable m_cv;
    std::queue<std::unique_ptr<ITask>> m_queue;
    bool m_stop = false;
    std::thread m_thread;

  public:
    using job_id_t = u64;

    Worker();
    ~Worker();

    template <typename... Args> job_id_t run(Args&&... args);

    JobStatus<Progress, Result> get(job_id_t id);
};

#include "worker.tpp"

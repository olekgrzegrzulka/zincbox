#pragma once
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <variant>

template <typename Progress, typename Result>
JobContext<Progress, Result>::JobContext(std::shared_ptr<State> state) : m_state(std::move(state)) {}

template <typename Progress, typename Result> void JobContext<Progress, Result>::set_progress(Progress p) {
  std::unique_lock lock(m_state->mtx);
  m_state->progress = std::move(p);
}

template <typename Progress, typename Result> void JobContext<Progress, Result>::set_result(Result r) {
  std::unique_lock lock(m_state->mtx);
  m_state->result = std::make_unique<Result>(std::move(r));
  m_state->finished = true;
}

template <auto Fn, typename Progress, typename Result>
Worker<Fn, Progress, Result>::Worker() {
  m_thread = std::thread([this]() {
    while (true) {
      std::unique_ptr<ITask> task;
      {
        std::unique_lock lock(m_queue_mutex);
        m_cv.wait(lock, [this]() { return m_stop || !m_queue.empty(); });
        if (m_stop && m_queue.empty()) {
          return;
        }
        task = std::move(m_queue.front());
        m_queue.pop();
      }
      task->execute();
    }
  });
}

template <auto Fn, typename Progress, typename Result>
Worker<Fn, Progress, Result>::~Worker() {
  {
    std::unique_lock lock(m_queue_mutex);
    m_stop = true;
  }
  m_cv.notify_one();
  if (m_thread.joinable()) {
    m_thread.join();
  }
}

template <auto Fn, typename Progress, typename Result>
template <typename... Args>
typename Worker<Fn, Progress, Result>::job_id_t Worker<Fn, Progress, Result>::run(Args&&... args) {
  std::shared_ptr<State> state;
  job_id_t id;

  {
    std::unique_lock lock(m_jobs_mutex);
    id = m_next_job_id++;
    state = std::make_shared<State>();
    m_jobs[id] = state;
  }

  auto func = [state, ... args = std::forward<Args>(args)]() mutable {
    JobContext<Progress, Result> ctx(state);
    Fn(std::move(args)..., ctx);

    std::unique_lock slock(state->mtx);
    state->finished = true;
  };

  {
    std::unique_lock lock(m_queue_mutex);
    m_queue.push(std::make_unique<TaskImpl<decltype(func)>>(std::move(func)));
  }
  m_cv.notify_one();

  return id;
}

template <auto Fn, typename Progress, typename Result>
JobStatus<Progress, Result> Worker<Fn, Progress, Result>::get(job_id_t id) {
  std::shared_ptr<State> state;
  {
    std::unique_lock lock(m_jobs_mutex);
    auto it = m_jobs.find(id);
    if (it == m_jobs.end()) { 
      return std::monostate{}; 
    }
    state = it->second;
  }

  if (state->finished) {
    std::unique_ptr<Result> res = std::move(state->result);
    {
      std::unique_lock map_lock(m_jobs_mutex);
      m_jobs.erase(id);
    }
    return res;
  } else {
    return state->progress;
  }
}

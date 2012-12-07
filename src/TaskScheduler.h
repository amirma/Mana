/**
* @file TaskScheduler.h
* TaskScheduler implementation
*
* @brief This class implements a Scheduler for executing one-time
* or recurrent (periodical) tasks.
*
* The scheduler uses boost::asio::io_servie to run tasks. This way we use the same
* thread pool that the io_service provides to us.
*
* @author Amir Malekpour
* @version 0.1
*
* Copyright © 2012 Amir Malekpour
*
*  Mana is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  Mana is distributed in the hope that it will be useful, but WITHOUT ANY
*  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
*  FOR A PARTICULAR PURPOSE. For more details see the GNU General Public License
*  at <http: *www.gnu.org/licenses/>
**/

#ifndef TASKSCHEDULER_H_
#define TASKSCHEDULER_H_


#include <functional>
#include <queue>
#include <mutex>
#include <utility>
#include <functional>
#include <memory>
#include <assert.h>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>
#include "TaskScheduler.h"
#include "ManaException.h"
#include "common.h"

using namespace std;

namespace mana {

enum TimeUnit {
    second,
    millisecond
};

template <class T>
class TaskScheduler {

public:

/**
*  @brief Constructor
*  @param srv is the io_service whose thread pool we want to use to run tasks
*  */
TaskScheduler(boost::asio::io_service& srv) : io_service_(srv), timer_(srv),
    flag_task_schedueled_(false) {}

virtual ~TaskScheduler() {
    try{
        cancell_all();
    } catch(exception& e){
        // ignore
    }
}

/**
* @brief Schedule a task to run after a certain time, starting from now.
*
*  The function is thread safe; multiple threads can concurrently call this method
*  on the same object.
*  @param f is a function object. (callable as 'f()'). If this parameter is a rvalue
*  reference then it will be moved (which is more efficient) into the internal storage of the class
*  TaskScheduler. But if it's a lvalue reference then it will be copied so in this case
*  'f' needs to have a copy constructor.
*  @param dur is the time after which we want the task to be executed
*  @param tu is a value of TimeUnit
*  */
void schedule_after_duration(T&& f, unsigned int dur, TimeUnit tu) {
    // if time unit if seconds convert 'dur' to milliseconds
    unsigned t_milisec = (tu == TimeUnit::second ? dur * 1000 : dur);
    auto task = make_shared<TaskWrapper>(std::forward<T>(f), t_milisec);
    insert_task(std::move(task));
}

/**
*  @brief Schedule a task to run after a certain time, starting from now.
*
*  The function is thread safe; multiple threads can concurrently call this method
*  on the same object.
*  @param f is a function object. (callable as 'f()'). If this parameter is a rvalue
*  reference then it will be moved (which is more efficient) into the internal storage of the class
*  TaskScheduler. But if it's a lvalue reference then it will be copied so in this case
*  'f' needs to have a copy constructor.
*  @param period is the interval between task executions. The first execution will happen at
*  now() + dur
*  @param tu is a value of TimeUnit
*  */
void schedule_at_periods(T&& f, unsigned int period, TimeUnit tu) {
    // if time unit if seconds convert 'dur' to milliseconds
    unsigned t_milisec = (tu == TimeUnit::second ? period * 1000 : period);
    auto task = make_shared<TaskWrapper>(std::forward<T>(f), t_milisec);
    task->flag_is_recurrent_ = true;
    insert_task(std::move(task));
}

void cancell_all() {
    timer_.cancel();
    // std::priority_queue does not have a clear() method.
    // Go figure why.
    while(!task_queue_.empty())
        task_queue_.pop();
}

// delete copy const. and assignment
TaskScheduler(const TaskScheduler&) = delete;
TaskScheduler& operator=(const TaskScheduler&) = delete;

private:

/**  Wrap a function object along with its relative execution time and a
 *  flag that tells if the task is recurrent or not. For internal purposes only */
struct TaskWrapper {
    TaskWrapper(T&& f, unsigned int t) : p_functor_(std::forward<T>(f)),
    interval_millisec_(t) , flag_is_recurrent_(false) {
        exec_time_ = std::chrono::system_clock::now() + interval_millisec_;
    }
    // delete copy const. and assignment.
    TaskWrapper(const TaskWrapper&) = delete;
    TaskWrapper& operator=(const TaskWrapper&) = delete;
    // properties
    T p_functor_;
    std::chrono::milliseconds interval_millisec_;
    std::chrono::time_point<std::chrono::system_clock> exec_time_;
    bool flag_is_recurrent_;
};

typedef shared_ptr<TaskWrapper> TaskWrapperPointer;

void insert_task(TaskWrapperPointer&& t) {
    lock_guard<mutex> lock(task_q_mutex_);
    // if there's no task already schedueled then we simply insert the new task
    // into the queue and schedule a dispatch
    if(flag_task_schedueled_ == false) {
        task_queue_.push(t);
        schedule_next_task();
        return;
    }
    /* otherwise we need to consider two cases:
     * 1. Either this new task has to be dispatched before anyother pending
     * task in our task queue. In this case we need to reset/set the timer to dispatch
     * the next task as the next one.
     * 2. Otherwise we don't need a rescheduling of the next task that it
     * going to be dispatched.
     * In both cased though, we need to adjust the time of the task in order
     * to achieve the proper ordering of the tasks in the priority queue.
     */
    bool flag_resched = false;
    if(task_queue_.top()->exec_time_ > t->exec_time_)
        flag_resched = true;
    task_queue_.push(t);
    if(flag_resched == true) {
        timer_.cancel();
        schedule_next_task();
    }
}

/* NOTE: this method must be called with 'mutex_' locked.
 * Both methods that call TaskScheduler::scheduele_next have already locked the
 * mutex_ */
void schedule_next_task() {
    // The mutex must be locked before calling this method
    assert(task_q_mutex_.try_lock() == false);
    auto& time_of_next_task =  task_queue_.top()->exec_time_;
    timer_.expires_from_now(time_of_next_task - std::chrono::system_clock::now());
    //Start an asynchronous wait.
    timer_.async_wait(std::bind(&TaskScheduler::timer_handler, this, std::placeholders::_1));
    flag_task_schedueled_ = true;
}

void timer_handler(const boost::system::error_code& e) {
    if (e != boost::asio::error::operation_aborted) {
        // Timer was not cancelled
        bool flag_reinsert = false;
        TaskWrapperPointer trp = nullptr;
        TaskWrapperPointer temp;
        {
            lock_guard<mutex> lock(task_q_mutex_);
            trp = task_queue_.top();
            temp = task_queue_.top();
            // if this is not a recurrent (periodical) task just
            // remove it from the queue
            task_queue_.pop();
            if(trp->flag_is_recurrent_) {
                trp->exec_time_ += trp->interval_millisec_;
                flag_reinsert = true;
            }
            flag_task_schedueled_ = false;
        }
        if(flag_reinsert) {
            insert_task(std::move(temp));
        } else { // if we did not re-insert the task we need to manually call 'scheddule_next_task()'
            lock_guard<mutex> lock(task_q_mutex_);
            schedule_next_task();
        }
        trp->p_functor_();
    }
}

/**  Comparator between to task wrappers. The one with the smaller execution time
 *  is less that the other. We need this because we want to arrange task wrappers
 *  in a priority queue */
struct task_wrapper_comparator {
    bool operator()(const TaskWrapperPointer& t1, const TaskWrapperPointer& t2) {
        return t1->exec_time_ > t2->exec_time_;
    }
};

// class properties of TaskScheduler
boost::asio::io_service& io_service_;
priority_queue<TaskWrapperPointer, vector<TaskWrapperPointer>, task_wrapper_comparator> task_queue_;
mutex task_q_mutex_;
boost::asio::high_resolution_timer timer_;
bool flag_task_schedueled_;
};

} /* namespace mana */

#endif /* TASKSCHEDULER_H_ */

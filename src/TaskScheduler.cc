/*
* @file TaskScheduler.cc
* TaskScheduler implementation
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
*/

#include <memory>
#include <assert.h>
#include <chrono>
#include "TaskScheduler.h"
#include "ManaException.h"
#include "common.h"

using namespace std;

namespace mana {

TaskScheduler::TaskScheduler(boost::asio::io_service& srv) : io_service_(srv), timer_(srv) {

}

void TaskScheduler::insert_task(TaskWrapperPointer& t) {
    lock_guard<mutex> lock(task_q_mutex_);
    bool flag_first = task_queue_.empty();
    task_queue_.push(t);
    //if this is the first item that was inserted into the queue then schedule
    // a timer
    if(flag_first)
        schedule_next_task();
}

void TaskScheduler::schedule_after_duration(function<void()>&& f, unsigned int dur, TimeUnit tu) {
    // if time unit if seconds convert 'dur' to milliseconds
    unsigned t_milisec = (tu == second ? dur * 1000 : dur);
    auto task = make_shared<TaskWrapper>(std::forward<function<void()>>(f), t_milisec);
    insert_task(task);
}

void TaskScheduler::schedule_at_periods(function<void()>&& f, unsigned int period, TimeUnit tu) {
    // if time unit if seconds convert 'dur' to milliseconds
    unsigned t_milisec = (tu == second ? period * 1000 : period);
    auto task = make_shared<TaskWrapper>(std::forward<function<void()>>(f), t_milisec);
    task->flag_is_recurrent_ = true;
    insert_task(task);
}

/* Notice that both methods that call TaskScheduler::scheduele_next have already locked the
 * task_queu */
void TaskScheduler::schedule_next_task() {
    // The mutex must be locked before calling this method
    assert(task_q_mutex_.try_lock() == false);
    unsigned int next_time = task_queue_.top()->exec_time_milisec_;
    timer_.expires_from_now(std::chrono::milliseconds(next_time));
    //Start an asynchronous wait.
    timer_.async_wait(std::bind(&TaskScheduler::timer_handler, this, std::placeholders::_1));
}

void TaskScheduler::timer_handler(const boost::system::error_code& e) {
    if (e != boost::asio::error::operation_aborted) {
        // Timer was not cancelled
        TaskWrapperPointer trp = nullptr;
        {
            lock_guard<mutex> lock(task_q_mutex_);
            trp = task_queue_.top();
            // if this is not a recurrent (periodical) task just
            // remove it from the queue
            if(!trp->flag_is_recurrent_)
                task_queue_.pop();
        }
        trp->func_();
        {
            lock_guard<mutex> lock(task_q_mutex_);
            if(!task_queue_.empty())
                schedule_next_task();
        }
    }
}

} /* namespace mana */

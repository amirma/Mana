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
#include <boost/asio.hpp>
#include <boost/asio/high_resolution_timer.hpp>

using namespace std;

namespace mana {

class TaskScheduler {
public:
    enum TimeUnit {
            second,
            millisecond
    };
    /**
     *  @brief Constructor
     *  @param srv is the io_service whose thread pool we want to use to run tasks
     *  */
    TaskScheduler(boost::asio::io_service& srv);
    // delete copy const. and assignment
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler& operator=(const TaskScheduler&) = delete;
     /**
      * @brief Schedule a task to run after a certain time, starting from now
     *  @param f is a function object. (callable as 'f()')
     *  @param dur is the time after which we want the task to be executed
     *  @param tu is a value of TimeUnit
     *  */
    void schedule_after_duration(function<void()>&& f, unsigned int dur, TimeUnit tu);
    /**
     *  @brief Schedule a task to run after a certain time, starting from now
     *  @param f is a function object. (callable as 'f()')
     *  @param period is the interval between task executions. The first execution will happen at
     *  now() + dur
     *  @parameter tu is a value of TimeUnit
     *  */
    void schedule_at_periods(function<void()>&& f, unsigned int period, TimeUnit tu);

private:

    /**  Wrap a function object along with its relative execution time and a
     *  flag that tells if the task is recurrent or not. For internal purposes only */
    struct TaskWrapper {
        TaskWrapper(function<void()>&& f, unsigned int t) :
        	func_(std::forward<std::function<void()>>(f)),
        	exec_time_milisec_(t) , flag_is_recurrent_(false) {}
        // delete copy const. and assignment.
        TaskWrapper(const TaskWrapper&) = delete;
        TaskWrapper& operator=(const TaskWrapper&) = delete;
        // properties
        const function<void()> func_;
        unsigned int exec_time_milisec_;
        bool flag_is_recurrent_;
    };

    typedef shared_ptr<TaskWrapper> TaskWrapperPointer;
    /**  Comparator between to task wrappers. The one with the smaller execution time
     *  is less that the other. We need this because we want to arrange task wrappers
     *  in a priority queue */
    struct compare_tasks_priority {
        bool operator()(TaskWrapperPointer& t1, TaskWrapperPointer& t2) {
            return t1->exec_time_milisec_ < t2->exec_time_milisec_;
        }
    };

    // private methods of TaskScheduler for internal use only.
    void schedule_next_task();
    void timer_handler(const boost::system::error_code& e);
    void insert_task(TaskWrapperPointer& t);
    // class properties of TaskScheduler
    boost::asio::io_service& io_service_;
    priority_queue<TaskWrapperPointer, vector<TaskWrapperPointer>, compare_tasks_priority> task_queue_;
    boost::asio::high_resolution_timer timer_;
    mutex task_q_mutex_;
};

} /* namespace mana */

#endif /* TASKSCHEDULER_H_ */

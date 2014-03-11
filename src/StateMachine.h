/*
 * StateMachine.h
 *
 *  Created on: Feb 28, 2013
 *      Author: amir
 */

#ifndef STATEMACHINE_H_
#define STATEMACHINE_H_

#include <map>
#include <mutex>
#include <condition_variable>
#include <atomic>

using namespace std;

namespace mana {

class StateMachine {

private:

struct State {
	State(int n = 0) :
		name_(n), flg_block_(false), flg_notified_(false) {
	}
	int name_;
	map<int, int> transitions_;
	function<void()> handler_;
	mutex cond_mutex_;
	condition_variable cond_var_; //the condition variable
	// on which the entering thread will wait
	bool flg_block_; // determines if the entering thread into this
	// state blocks or not
	bool flg_notified_;
};

public:
	StateMachine(int num);
	virtual ~StateMachine();
	int current_state() const;
	void process(int input);
	void add_transition(int source, int sink, int input);
	template<class F> void add_handler(const int s, F&& f);
	void set_blocking(const int s);

private:
	atomic_int current_state_;
	const int num_of_states_;
	State* states_;
	mutex mutex_;
};

} /* namespace mana */
#endif /* STATEMACHINE_H_ */

/*
 * StateMachine.cc
 *
 *  Created on: Feb 28, 2013
 *      Author: amir
 */

#include <assert.h>
#include "StateMachine.h"
#include "Log.h"

namespace mana {

StateMachine::StateMachine(int num) : num_of_states_(num) {
	states_ = new State[num_of_states_];
	current_state_ = 0;
	for (int i = 0; i < num_of_states_; i++) {
		states_[i].name_ = i;
	}
}

StateMachine::~StateMachine() {
	delete[] states_;
}

int StateMachine::current_state() const {
	return current_state_;
}

void StateMachine::process(int input) {
	int previous = current_state_;
	int new_current;
	{
		lock_guard < mutex > lock(mutex_);
		auto& tmp_tr = states_[current_state_].transitions_;
		if (tmp_tr.find(input) == tmp_tr.end()) {
			FILE_LOG(logWARNING)<< "Warning: no transition from state " << current_state_ << " with input " << input;
			return;
		}
		current_state_ = states_[current_state_].transitions_[input];
		new_current = current_state_;
	}

	// notify pending thread in the previous state
	auto& temp = states_[previous];
	if (temp.flg_block_) {
		unique_lock < std::mutex > lock(temp.cond_mutex_);
		temp.flg_notified_ = true;
		temp.cond_var_.notify_all();
	}

	// execute the handler
	// Note: This should be done, having notified threads
	// that were blocking in the previous state. If not, there
	// is the danger that the handler causes a state transition in
	// the state machine and blocks. This can potentially lead to a deadlock
	states_[new_current].handler_();

	// now block if we have to
	auto& temp_s1 = states_[new_current];
	if (temp_s1.flg_block_) {
		unique_lock < std::mutex > lock(temp_s1.cond_mutex_);
		while (temp_s1.flg_notified_ == false)
			temp_s1.cond_var_.wait(lock);
	}
}

void StateMachine::add_transition(int source, int sink, int input) {
	states_[source].transitions_[input] = sink;
	FILE_LOG(logDEBUG3) << "transision added " << source << "  " << sink;
}

template<class F>
void StateMachine::add_handler(const int s, F&& f) {
	states_[s].handler_ = std::forward<F>(f);
}

void StateMachine::set_blocking(const int s) {
	states_[s].flg_block_ = true;
}

} /* namespace mana */

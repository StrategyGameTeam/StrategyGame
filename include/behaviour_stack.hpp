#pragma once

#include <map>
#include <queue>
#include <stack>
#include <memory>
#include <variant>

class BehaviourStack;

template <typename T>
concept ViableBehaviour = requires(T behaviour, BehaviourStack &bs) {
	{ behaviour.loop(bs) };
};

class BehaviourStack
{
private:
	struct VoidErasedBehaviourElement {
		// taken from https://herbsutter.com/2016/09/25/to-store-a-destructor/
		void *behaviour;
		void (*destroy)(const void*);
		void (*loop_fn)(void*, BehaviourStack&);

		void run_loop(BehaviourStack& bs) {
			loop_fn(behaviour, bs);
		}

		void run_destroy() {
			destroy(behaviour);
		}
	};

	struct PopAction {};
	struct ClearAction {};
	struct PushAction {
		VoidErasedBehaviourElement element;
	};

	using Action = std::variant<PopAction, ClearAction, PushAction>;

	std::vector<VoidErasedBehaviourElement> states_stack;
	std::vector<Action> actions_queue;

	void perform_queued_actions();
	void update();

public:
	BehaviourStack();

	template <ViableBehaviour T>
	void defer_push(T *b) {
		actions_queue.emplace_back(PushAction{
			.element = {
				.behaviour = b,
				.destroy = [](const void* bp) { static_cast<const T*>(bp)->~T(); },
				.loop_fn = [](void* bp, BehaviourStack& bs) { static_cast<T*>(bp)->loop(bs); } // i believe we cannot remove that layer of indirection, because we need to convert between calling conventions
			}
		});
	};

	template <ViableBehaviour T>
	void push(T *b) {
		states_stack.emplace_back(VoidErasedBehaviourElement{
			.behaviour = b,
			.destroy = [](const void* bp) { static_cast<const T*>(bp)->~T(); },
			.loop_fn = [](void* bp, BehaviourStack& bs) { static_cast<T*>(bp)->loop(bs); }
		});
	};

	void pop();
	void defer_pop();
	
	void clear();
	void defer_clear();

	void run();
};
#pragma once

#include<vector>
#include<map>
#include<functional>
#include<iostream>

#include "Event.hpp"

#include <spdlog/spdlog.h>
#include <iostream>

/*
	事件分发器
	用于分发事件
	参考：https://zhuanlan.zhihu.com/p/535574136
*/

namespace EventSystem {

	class Dispatcher {

	public:

		Dispatcher(const Dispatcher&) = delete; // 禁止拷贝构造
		Dispatcher& operator=(const Dispatcher&) = delete; // 禁止拷贝赋值



		// 单例
		static Dispatcher& GetInstance() {
			static Dispatcher instance;
			return instance;
		}

		void Subscribe(EventType type, std::function<void(const Event& e)> callback) {
			observers[type].push_back(callback);
		}

		void Dispatch(const Event& e) {
			EventType type = e.type();



			for (auto& callback : observers[type]) {
				callback(e);
			}


		}

		~Dispatcher() {
			cout << " ~Dispatcher" << endl;
		}

	private:
		Dispatcher() {}

		std::map<EventType, std::vector<std::function<void(const Event& e)>>> observers;
	};

}
#pragma once

namespace EventSystem {

	enum class EventType {
		None = 0,
		SetCameraPos
	};


	class Event
	{
	public:
		virtual EventType type() const = 0;
	};


}
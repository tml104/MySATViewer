#pragma once

#include "Event.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace EventSystem {

	class SetCameraPosEvent : public Event {
	public:
		static constexpr EventType etype = EventType::SetCameraPos;

		glm::vec3 pos;

		EventType type() const override
		{
			return etype;
		}

		SetCameraPosEvent(const glm::vec3& pos) : pos(pos) {}
	};
}
#pragma once


#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

#include <iostream>
#include <fstream>

#include <stdexcept>
#include <cassert>

#include <spdlog/spdlog.h>

#include "json.hpp"


#include "TopologyInfo.hpp"



namespace Info {

	struct DebugShowInfo {

		using json = nlohmann::json;

		struct DebugShowThings {
			std::vector<Info::DebugShowPointInfo> pointInfos;
		} things;

		void LoadFromDebugShowJson(const std::string& json_path) {

			std::ifstream f(json_path);
			json data = json::parse(f);

			_LoadDebugShowPoints(data);

		}

		void _LoadDebugShowPoints(const json& data) {


			auto debug_points = data["debug_points"];

			things.pointInfos.reserve(debug_points.size());

			for (auto&& element: debug_points) {

				try {
					std::string name = element.at("name");
					float x = element.at("x");
					float y = element.at("y");
					float z = element.at("z");

					// TEMP: color 
					glm::vec3 color = (name.length() < 4) ? (glm::vec3{ 1.0f, 0.0f, 0.0f }) : (glm::vec3{ 0.0f, 1.0f, 0.0f });

					things.pointInfos.push_back({ name, glm::vec3{x, y, z},  color});
				}
				catch (std::exception& e) {
					SPDLOG_INFO("DebugShowPoints load failed");
				}

			}
		}

	};

}
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

	struct RayInfo {

		using json = nlohmann::json;

		struct RayInfoThings {
			std::vector<Info::OneRayInfo> rayInfos;
		} things;


		void LoadFromRayJson(const std::string& json_path){
			std::ifstream f(json_path);
			json data = json::parse(f);

			_LoadRayJson(data);
		}

		void _LoadRayJson(const json& data){

			auto ray_infos = data["ray_infos"];

			things.rayInfos.reserve(ray_infos.size());


			for (auto&& element : ray_infos) {
				try {
					auto start_point_element = element.at("st");
					glm::vec3 start_point{ start_point_element[0],  start_point_element[1], start_point_element[2]};

					auto direction_element = element.at("d");
					glm::vec3 direction{ direction_element[0], direction_element[1], direction_element[2] };

					float r = element.at("r");

					int id = element.at("id");
					int from_cell_id = element.at("from");
					int to_meshbox_id = element.at("to");
					int result = element.at("res");

					things.rayInfos.push_back({ start_point , direction , r, id, from_cell_id, to_meshbox_id, result });
				}
				catch (json::out_of_range& e) {
					SPDLOG_INFO("Ray load failed");
				}

			}

		}

	};

}
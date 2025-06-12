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

	struct CellInfo {

		using json = nlohmann::json;

		struct CellInfoThings {
			std::vector<Info::OneBoxInfo> cellBoxInfos; // Cell Box Infos
		} things;


		void LoadFromCellBoxJson(const std::string& json_path){
			std::ifstream f(json_path);
			json data = json::parse(f);

			_LoadCellBoxes(data);
		}

		void _LoadCellBoxes(const json& data) {
			auto cell_boxes = data["cell_boxes"];
			things.cellBoxInfos.reserve(cell_boxes.size());
			for (auto&& element : cell_boxes) {
				try {

					// TODO

				}
				catch (json::out_of_range& e) {
					SPDLOG_INFO("Cell Box load failed");
				}
			}

		}
	};

}
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

					int id = element.at("id");

					float min_x = element.at("min_x");
					float min_y = element.at("min_y");
					float min_z = element.at("min_z");

					float max_x = element.at("max_x");
					float max_y = element.at("max_y");
					float max_z = element.at("max_z");

					things.cellBoxInfos.push_back({ id, glm::vec3{min_x, min_y, min_z}, glm::vec3{max_x, max_y, max_z } });
				}
				catch (json::out_of_range& e) {
					SPDLOG_INFO("Cell Box load failed");
				}
			}

		}
	};


	//struct MeshBoxInfo {

	//	using json = nlohmann::json;

	//	struct MeshBoxInfoThings {
	//		std::vector<Info::OneBoxInfo> meshBoxInfos; // Mesh Box Infos
	//	} things ;

	//	void LoadFromMeshBoxJson(const std::string& json_path){
	//		std::ifstream f(json_path);
	//		json data = json::parse(f);

	//		_LoadMeshBoxJson(data);
	//	}

	//	void _LoadMeshBoxJson(const json& data){
	//		auto mesh_boxes = data["mesh_boxes"];

	//		things.meshBoxInfos.reserve(mesh_boxes.size());
	//		for (auto&& element : mesh_boxes) {
	//			try {

	//				int id = element.at("id");

	//				float min_x = element.at("min_x");
	//				float min_y = element.at("min_y");
	//				float min_z = element.at("min_z");

	//				float max_x = element.at("max_x");
	//				float max_y = element.at("max_y");
	//				float max_z = element.at("max_z");

	//				things.meshBoxInfos.push_back({ id, glm::vec3{min_x, min_y, min_z}, glm::vec3{max_x, max_y, max_z } });
	//			}
	//			catch (json::out_of_range& e) {
	//				SPDLOG_INFO("Mesh Box load failed");
	//			}

	//		}

	//	}
	//};
}
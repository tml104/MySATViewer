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

#include "stl_reader.h"
#include "json.hpp"

#include "TopologyInfo.hpp"

#include <spdlog/spdlog.h>


namespace Info {

	struct SatInfo {

		using json = nlohmann::json;


		// 统计信息
		struct BasicStat {
			int marknum_body;
			int marknum_coedge;
			int marknum_edge;
			int marknum_face;
			int marknum_loop;
			int marknum_lump;
			int marknum_shell;
			int marknum_vertex;
			int marknum_wire;
		} stats;

		// stl信息：用于渲染sat模型的表面
		struct StlSOA {
			std::vector<float> stlVertices; // 注意：对每个点来说都含有6个float的信息（坐标3个，normal 3个），因此实际采样点数量需要除以6。每3个连续的点（也即这个数组中每3*6=18个值）是一个三角形
			int stlVerticesCount; // 记录的是不考虑重合情况下stl中所有顶点数（数值应当等于stlVertices.size() / 6）
			std::vector<int> stlTriangleToFaceMarkNums; // 每个三角形对应的面的markNum。输入索引应当是三角形的索引（索引范围：0 to stlVerticesCount/3 -1 ）。用途：可借由此用颜色区分各个面
		}stl;

		// 边界表示信息，用于在GUI中显示sat模型中各个对象列表
		struct BrepInfo {
			std::vector<VertexInfo> vertexInfos; // 顶点信息
			std::vector<EdgeInfo> edgeInfos; // 边信息
			std::vector<HalfEdgeInfo> halfEdgeInfos; // 半边信息
			std::vector<LoopInfo> loopInfos; // 环信息
			std::vector<FaceInfo> faceInfos;
		}brepInfo;

		glm::vec3 newCameraPos; // 加载完成后，用来更新默认相机位置


		void LoadStl(const std::string& stl_path, const std::string& stl_triangle_marknum_file_path = "") {

			newCameraPos = glm::vec3{ 100.0f };
			stl_reader::StlMesh <float, unsigned int> mesh(stl_path);

			stl.stlVertices.clear();
			stl.stlVerticesCount = 0;

			for (size_t i_solid = 0; i_solid < mesh.num_solids(); i_solid++) {
				for (size_t j_tri = mesh.solid_tris_begin(i_solid); j_tri < mesh.solid_tris_end(i_solid); j_tri++) {

					glm::vec3 triangle_points[3];

					for (int j = 0; j < 3; j++) {
						triangle_points[j].x = mesh.tri_corner_coords(j_tri, j)[0];
						triangle_points[j].y = mesh.tri_corner_coords(j_tri, j)[1];
						triangle_points[j].z = mesh.tri_corner_coords(j_tri, j)[2];
					}

					glm::vec3 u = triangle_points[2] - triangle_points[0];
					glm::vec3 v = triangle_points[1] - triangle_points[0];

					glm::vec3 normalized_normal = glm::normalize(glm::cross(v, u));

					for (int j = 0; j < 3; j++) {
						stl.stlVertices.emplace_back(triangle_points[j].x);
						stl.stlVertices.emplace_back(triangle_points[j].y);
						stl.stlVertices.emplace_back(triangle_points[j].z);

						stl.stlVertices.emplace_back(normalized_normal.x);
						stl.stlVertices.emplace_back(normalized_normal.y);
						stl.stlVertices.emplace_back(normalized_normal.z);

						newCameraPos.x = std::min(newCameraPos.x, triangle_points[j].x);
						newCameraPos.y = std::min(newCameraPos.y, triangle_points[j].y);
						newCameraPos.z = std::min(newCameraPos.z, triangle_points[j].z);

						stl.stlVerticesCount++;
					}
				}
			}

			newCameraPos += glm::vec3{ 20.0f }; // 偏移

			// TODO: 加载每个三角形面是对应哪个面的
			if (stl_triangle_marknum_file_path != "") {
				try {
					// TODO
				}
				catch (std::exception& e) {
					stl.stlTriangleToFaceMarkNums.clear();
				}
			}
		}

		// TODO: 把selected_body接入？但是可能后面这个东西要接入的话只能是渲染层面上做接入
		void LoadGeometryJson(const std::string& json_path, int selected_body = -1) {

			std::ifstream f(json_path);
			json data = json::parse(f);

			_LoadBasicStat(data);

			_LoadVertexInfos(data);
			_LoadEdgeInfos(data);
			_LoadHalfEdgeInfos(data);
			_LoadLoopInfos(data);
			_LoadFaceInfos(data);

			_SortInfos(brepInfo.vertexInfos);
			_SortInfos(brepInfo.edgeInfos);
			_SortInfos(brepInfo.halfEdgeInfos);
			_SortInfos(brepInfo.loopInfos);
			_SortInfos(brepInfo.faceInfos);
		}


		void _LoadBasicStat(const json& data) {

			auto root_basic_statistics = data["basic_statistics"];

			stats.marknum_body = root_basic_statistics["marknum_body"];
			stats.marknum_coedge = root_basic_statistics["marknum_coedge"];
			stats.marknum_edge = root_basic_statistics["marknum_edge"];
			stats.marknum_face = root_basic_statistics["marknum_face"];
			stats.marknum_loop = root_basic_statistics["marknum_loop"];
			stats.marknum_lump = root_basic_statistics["marknum_lump"];
			stats.marknum_shell = root_basic_statistics["marknum_shell"];
			stats.marknum_vertex = root_basic_statistics["marknum_vertex"];
			stats.marknum_wire = root_basic_statistics["marknum_wire"];

		}

		void _LoadVertexInfos(const json& data) {

			auto root_vertices = data["root_vertices"];

			brepInfo.vertexInfos.reserve(stats.marknum_vertex);

			int ii = 0;
			for (auto&& element : root_vertices) {
				try {
					int body_id = element.at("body");
					int marknum = element.at("marknum");

					auto point = element.at("point");
					float x = point.at("x");
					float y = point.at("y");
					float z = point.at("z");

					brepInfo.vertexInfos[ii].pos = glm::vec3{ x,y,z };
					brepInfo.vertexInfos[ii].bodyId = body_id;
					brepInfo.vertexInfos[ii].markNum = marknum;

					ii++;
				}
				catch (json::out_of_range& e) {
					SPDLOG_INFO("Vertex {} load failed", ii++);
				}

			}

		}

		void _LoadEdgeInfos(const json& data) {
			auto root_edges = data["root_edges"];
			brepInfo.edgeInfos.reserve(stats.marknum_edge);
			int ii = 0;
			for (auto&& element : root_edges) {
				try {
					int body_id = element.at("body");
					int marknum = element.at("marknum");
					int st_marknum = element.at("st_marknum");
					int ed_marknum = element.at("ed_marknum");
					int nonmanifold_count = element.at("nonmanifold_count");

					brepInfo.edgeInfos[ii].bodyId = body_id;
					brepInfo.edgeInfos[ii].markNum = marknum;

					brepInfo.edgeInfos[ii].stMarkNum = st_marknum;
					brepInfo.edgeInfos[ii].edMarkNum = ed_marknum;
					brepInfo.edgeInfos[ii].nonmanifoldCount = nonmanifold_count;

					// TODO: halfEdgesMarkNums

					// at: 如果元素不存在会抛出异常
					// value(key, default_value): 如果元素不存在则用默认值

					std::string curve_type = element.value("curve_type", "");
					

					if (curve_type == "straight") {
						auto property = element.at("property");

						float root_point_x = property.at("root_point_x");
						float root_point_y = property.at("root_point_y");
						float root_point_z = property.at("root_point_z");

						float direction_x = property.at("direction_x");
						float direction_y = property.at("direction_y");
						float direction_z = property.at("direction_z");


						auto geometry_ptr = std::make_unique<StraightEdgeGeometry>();
						geometry_ptr->rootPoint = glm::vec3{ root_point_x ,root_point_y, root_point_z };
						geometry_ptr->direction = glm::vec3{ direction_x ,direction_y,direction_z };
						brepInfo.edgeInfos[ii].geometryPtr = std::move(geometry_ptr);
					}
					else if (curve_type == "intcurve") {
						auto property = element.at("property");
						auto ctrlpts = property.at("ctrlpts");

						auto geometry_ptr = std::make_unique<IntcurveEdgeGeometry>();
						for (auto&& ctrlpt : ctrlpts)
						{
							float x = ctrlpt.at("x");
							float y = ctrlpt.at("y");
							float z = ctrlpt.at("z");

							geometry_ptr->ctrlpts.emplace_back(x, y, z);
						}

						brepInfo.edgeInfos[ii].geometryPtr = std::move(geometry_ptr);

					}
					else if (curve_type == "ellipse") {
						auto property = element.at("property");

						float centre_x = property.at("centre_x");
						float centre_y = property.at("centre_y");
						float centre_z = property.at("centre_z");

						float normal_x = property.at("normal_x");
						float normal_y = property.at("normal_y");
						float normal_z = property.at("normal_z");

						float major_axis_x = property.at("major_axis_x");
						float major_axis_y = property.at("major_axis_y");
						float major_axis_z = property.at("major_axis_z");

						float major_length = property.at("major_length");
						float minor_length = property.at("minor_length");

						auto geometry_ptr = std::make_unique<EllipseEdgeGeometry>();

						geometry_ptr->centre = glm::vec3{ centre_x, centre_y, centre_z };
						geometry_ptr->normal = glm::vec3{ normal_x, normal_y, normal_z };
						geometry_ptr->majorAxis = glm::vec3{ major_axis_x, major_axis_y, major_axis_z };
						geometry_ptr->majorAxisLength = major_length;
						geometry_ptr->minorAxisLength = minor_length;

						brepInfo.edgeInfos[ii].geometryPtr = std::move(geometry_ptr);
					}
					else {
						brepInfo.edgeInfos[ii].geometryPtr = std::make_unique<EdgeGeometry>();
					}

					brepInfo.edgeInfos[ii].geometryPtr->curveType = curve_type;
					
					auto sampled_points = element.at("sampled_points");
					for (auto&& point : sampled_points) {
						float x = point.at("x");
						float y = point.at("y");
						float z = point.at("z");

						brepInfo.edgeInfos[ii].geometryPtr->sampledPoints.emplace_back(x, y, z);
					}

					ii++;
				}
				catch (json::out_of_range& e) {
					SPDLOG_INFO("Edge {} load failed", ii++);
				}
			}
		}

		void _LoadHalfEdgeInfos(const json& data) {
			// TODO
		}

		void _LoadLoopInfos(const json& data) {
			// TODO
		}

		void _LoadFaceInfos(const json& data) {
			// TODO
		}

		template<typename T>
		void _SortInfos(std::vector<T>& infos) {
			std::sort(infos.begin(), infos.end(), [](const T& a, const T& b) {
				return a.markNum < b.markNum;
			});
		}
	};
}
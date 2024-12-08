#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>
#include <cmath>

#include "shader_s.h"
#include "camera.h"

#include "stl_reader.h"
#include "json.hpp"
#include "tiny_obj_loader.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

using json = nlohmann::json;

namespace MyRenderEngine {

	namespace Utils {
		glm::mat4 CalculateModelMatrix(const glm::vec3& position, const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f)) { // 原来引用能设置默认变量吗……
			glm::mat4 trans = glm::mat4(1.0f);

			trans = glm::translate(trans, position);
			trans = glm::rotate(trans, glm::radians(rotation.x), glm::vec3(1.0, 0.0, 0.0));
			trans = glm::rotate(trans, glm::radians(rotation.y), glm::vec3(0.0, 1.0, 0.0));
			trans = glm::rotate(trans, glm::radians(rotation.z), glm::vec3(0.0, 0.0, 1.0));
			trans = glm::scale(trans, scale);

			return trans;
		}

		auto SplitStr(const std::string& s, char split_char = ',')
		{
			std::vector<std::string> res;
			//lambda
			auto string_find_first_not = [](const std::string& s, size_t pos = 0, char split_char = ',') {
				for (size_t i = pos; i < s.size(); i++)
				{
					if (s[i] != split_char && s[i] != ' ' && s[i] != '\t')
						return i;
				}
				return std::string::npos;
				};

			size_t begin_pos = string_find_first_not(s, 0, split_char);
			size_t end_pos = s.find(split_char, begin_pos);

			while (begin_pos != std::string::npos)
			{
				size_t end_pos2 = end_pos - 1;
				while (begin_pos < end_pos2 && (s[end_pos2] == '\t' || s[end_pos2] == ' '))
				{
					end_pos2--;
				}
				res.emplace_back(s.substr(begin_pos, end_pos2 + 1 - begin_pos));
				begin_pos = string_find_first_not(s, end_pos, split_char);
				end_pos = s.find(split_char, begin_pos);
			}
			return res;
		}

		template <typename T>
		T clamp(T value, T l, T r) {
			if (value < l) return l;
			if (value > r) return r;
			return value;
		}
	}

	struct RenderInfo {
		glm::mat4 projection_matrix;
		glm::mat4 view_matrix;
		glm::vec3 camera_pos;

		bool showModel;

		RenderInfo() :
			showModel(false)
		{}
	};

	class IRenderable {
	public:
		virtual void Render(
			const RenderInfo& renderInfo
		) = 0;
		virtual ~IRenderable() {};
	};

	struct EdgeInfo {
		int edgeIndex;
		int markNum;
		int bodyId;
		int nonmanifoldCount;

		glm::vec3 edgeMidPos;
	};

	class SatInfo {
	public:
		std::vector<float> stlVertices; // 注意：对每个点来说都含有6个float的信息（坐标3个，normal 3个），因此实际采样点数量需要除以6
		int stlVerticesCount; // 记录的是不考虑重合情况下stl中所有顶点数

		std::vector<std::vector<float>> edgeSampledPoints; // 注意：这里一个edge是由多组线段组成的，每个vector里面对每个点来说都含有3个float的信息，因此实际采样点数量需要除以3
		std::vector<int> edgeSampledPointsCounts;
		std::vector<glm::vec3> edgeColors;
		std::unordered_map<int, EdgeInfo> edgeMap; // 每个边的 Marknum -> edgeSampledPoints中vector的对应索引

		glm::vec3 newCameraPos;

		void LoadStl(const std::string& stl_path) {
			stlVertices.clear();
			stlVerticesCount = 0;
			newCameraPos = glm::vec3{ 100.0f };

			stl_reader::StlMesh <float, unsigned int> mesh(stl_path);

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
						stlVertices.emplace_back(triangle_points[j].x);
						stlVertices.emplace_back(triangle_points[j].y);
						stlVertices.emplace_back(triangle_points[j].z);

						stlVertices.emplace_back(normalized_normal.x);
						stlVertices.emplace_back(normalized_normal.y);
						stlVertices.emplace_back(normalized_normal.z);

						newCameraPos.x = std::min(newCameraPos.x, triangle_points[j].x);
						newCameraPos.y = std::min(newCameraPos.y, triangle_points[j].y);
						newCameraPos.z = std::min(newCameraPos.z, triangle_points[j].z);

						stlVerticesCount++;
					}
				}
			}

			newCameraPos += glm::vec3{ 20.0f }; // 偏移
		}

		void LoadGeometryJson(const std::string& json_path, int selected_body = -1) {
			edgeSampledPoints.clear();
			edgeSampledPointsCounts.clear();
			edgeColors.clear();
			edgeMap.clear();

			std::ifstream f(json_path);
			json data = json::parse(f);

			int ii = 0;
			auto root_edges = data["root_edges"];
			for (auto&& element : root_edges) { // for each edge
				int nonmanifold_count = element["nonmanifold_count"];
				int body_id = element["body"];
				int marknum = element["marknum"];

				if (selected_body != -1 && selected_body != body_id)
				{
					continue;
				}

				if (nonmanifold_count != 2)
				{
					printf("[LoadGeometryJson] nonmanifold: edge: %d, body: %d, count: %d \n", int(element["marknum"]), body_id, nonmanifold_count);
					//spdlog::info("nonmanifold: edge: {}, body: {], count: {}", int(element["marknum"]), body_id, nonmanifold_count);
				}

				glm::vec3 color;
				if (nonmanifold_count == 1) {
					color = glm::vec3(1.0, 0.0, 0.0);
				}
				else if (nonmanifold_count == 2) {
					color = glm::vec3(0.0, 1.0, 0.0);
				}
				else if (nonmanifold_count == 4) {
					color = glm::vec3(0.0, 0.0, 1.0);
				}
				else if (nonmanifold_count == 3) {
					color = glm::vec3(1.0, 1.0, 0.0);
				}
				else {
					color = glm::vec3(0.0, 1.0, 1.0);
				}

				edgeColors.emplace_back(color);

				auto sampled_points = element["sampled_points"];
				std::vector<float> vertices;
				int point_count = 0;

				for (auto&& point : sampled_points) {
					float x = point["x"];
					float y = point["y"];
					float z = point["z"];

					//std::cout << x << " " << y << " " << z << " " << std::endl;
					vertices.emplace_back(x);
					vertices.emplace_back(y);
					vertices.emplace_back(z);

					point_count++;
				}

				edgeSampledPoints.emplace_back(vertices);
				edgeSampledPointsCounts.emplace_back(point_count);

				// mid pos
				glm::vec3 mid_pos;
				mid_pos.x = sampled_points[point_count / 2]["x"];
				mid_pos.y = sampled_points[point_count / 2]["y"];
				mid_pos.z = sampled_points[point_count / 2]["z"];

				edgeMap[marknum] = { ii++, marknum, body_id, nonmanifold_count, mid_pos };
			}
		}
	};


	using T_NUM = float;
	const T_NUM GLOBAL_TOLERANCE = 1e-6;

	struct Coordinate {
		T_NUM coords[3];

		Coordinate() {
			for (int i = 0; i < 3; i++) {
				coords[i] = 0.0;
			}
		}

		Coordinate(const T_NUM& x, const T_NUM& y, const T_NUM& z) {
			coords[0] = x;
			coords[1] = y;
			coords[2] = z;
		}

		Coordinate(const T_NUM* num_list) {
			for (int i = 0; i < 3; i++) {
				coords[i] = num_list[i];
			}
		}

		T_NUM& x() {
			return coords[0];
		}

		T_NUM& y() {
			return coords[1];
		}

		T_NUM& z() {
			return coords[2];
		}

		T_NUM& operator[](unsigned int i) {
			return coords[i];
		}

		const T_NUM& operator[](unsigned int i) const {
			return coords[i];
		}

		bool operator==(const Coordinate& other_coord) const {
			for (int i = 0; i < 3; i++) {
				if (abs(other_coord[i] - coords[i]) > GLOBAL_TOLERANCE) {
					return false;
				}
			}
			return true;
		}

		Coordinate operator+(const Coordinate& other_coord) const {
			return Coordinate(
				coords[0] + other_coord[0],
				coords[1] + other_coord[1],
				coords[2] + other_coord[2]
			);
		}

		Coordinate operator-(const Coordinate& other_coord) const {
			return Coordinate(
				coords[0] - other_coord[0],
				coords[1] - other_coord[1],
				coords[2] - other_coord[2]
			);
		}

		Coordinate operator*(const T_NUM& k) const {
			return Coordinate(
				coords[0] * k,
				coords[1] * k,
				coords[2] * k
			);
		}

		Coordinate operator/(const T_NUM& k) const {
			return Coordinate(
				coords[0] / k,
				coords[1] / k,
				coords[2] / k
			);
		}

		T_NUM Dot(const Coordinate& other_coord) const {
			T_NUM ans = 0.0;
			for (int i = 0; i < 3; i++) {
				ans += coords[i] * other_coord[i];
			}

			return ans;
		}

		Coordinate Cross(const Coordinate& other_coord) const {
			return Coordinate(
				coords[1] * other_coord[2] - coords[2] * other_coord[1],
				coords[2] * other_coord[0] - coords[0] * other_coord[2],
				coords[0] * other_coord[1] - coords[1] * other_coord[0]
			);
		}

		Coordinate Min(const Coordinate& other_coord) const {
			return Coordinate(
				std::min(coords[0], other_coord[0]),
				std::min(coords[1], other_coord[1]),
				std::min(coords[2], other_coord[2])
			);
		}

		Coordinate Max(const Coordinate& other_coord) const {
			return Coordinate(
				std::max(coords[0], other_coord[0]),
				std::max(coords[1], other_coord[1]),
				std::max(coords[2], other_coord[2])
			);
		}

		T_NUM Distance(const Coordinate& other_coord) const {
			T_NUM dis = 0.0;
			for (int i = 0; i < 3; i++) {
				dis += (coords[i] - other_coord[i]) * (coords[i] - other_coord[i]);
			}

			return sqrt(dis);
		}

		T_NUM Length() const {
			T_NUM length = 0.0;

			for (int i = 0; i < 3; i++) {
				length += (coords[i] * coords[i]);
			}

			return sqrt(length);
		}

		void Normalize() {
			T_NUM length = Length();

			for (int i = 0; i < 3; i++) {
				coords[i] /= length;
			}
		}
	};

	class ObjInfo {
	public:
		std::vector<float> vertices;
		std::vector<unsigned int> indices;

		std::vector<std::pair<int, int>> solidIndicesRange;

		void LoadObj(const std::string& obj_path) {
			tinyobj::ObjReader reader;

			if (!reader.ParseFromFile(obj_path)) {
				if (!reader.Error().empty()) {
					std::cerr << "TinyObjReader: " << reader.Error();
				}
				throw::runtime_error("TinyObjReader read failed.");
			}

			if (!reader.Warning().empty()) {
				std::cout << "TinyObjReader: " << reader.Warning();
			}

			auto& attrib = reader.GetAttrib();
			auto& shapes = reader.GetShapes();
			//auto& materials = reader.GetMaterials();

			vertices = attrib.vertices; // 复制

			// 遍历每个solid
			int last_end = 0;
			for (size_t s = 0; s < shapes.size(); s++) {
				// Loop over faces(polygon)
				size_t index_offset = 0;
				for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
					size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]); // 3
					assert(fv == 3);

					// Loop over vertices in the face.
					for (size_t v = 0; v < fv; v++) {
						// access to vertex
						tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
						//tinyobj::real_t vx = attrib.vertices[3 * size_t(idx.vertex_index) + 0];
						//tinyobj::real_t vy = attrib.vertices[3 * size_t(idx.vertex_index) + 1];
						//tinyobj::real_t vz = attrib.vertices[3 * size_t(idx.vertex_index) + 2];

						indices.emplace_back(idx.vertex_index);
					}
					index_offset += fv;
				}

				solidIndicesRange.emplace_back(last_end, indices.size());
				last_end = indices.size();
			}
			std::cout << "OBJ vertices size: " << vertices.size() << std::endl;
			std::cout << "OBJ indices size: " << indices.size() << std::endl;
			std::cout << "Solid size: " << solidIndicesRange.size() << std::endl;
		}

		Coordinate GetPoint(int index) {
			return Coordinate(vertices[3 * index + 0], vertices[3 * index + 1], vertices[3 * index + 2]);
		}
	};

	struct Entity {};
	struct Vertex;
	struct HalfEdge;
	struct Edge;
	struct Loop;
	struct Face;
	struct Solid;

	struct Vertex : public Entity {
		Coordinate pointCoord;
	};

	struct Edge : public Entity {
		std::shared_ptr<Vertex> st, ed;
		std::vector<std::shared_ptr<HalfEdge>> halfEdges; // 不保证顺序

		T_NUM Length() const {
			return st->pointCoord.Distance(ed->pointCoord);
		}

		void AddHalfEdge(const std::shared_ptr<HalfEdge>& he) {
			halfEdges.emplace_back(he);
		}

		void UpdateHalfEdgesPartner();
	};

	struct HalfEdge : public Entity {
		// false: 与edge一致, true: 不一致
		bool sense;
		std::shared_ptr<HalfEdge> partner;
		std::shared_ptr<Loop> loop;
		std::shared_ptr<HalfEdge> pre, next;
		std::shared_ptr<Edge> edge;

		std::shared_ptr<Vertex> GetStart() const {
			return sense ? (edge->ed) : (edge->st);
		}

		std::shared_ptr<Vertex> GetEnd() const {
			return sense ? (edge->st) : (edge->ed);
		}

		T_NUM Length() const {
			return edge->Length();
		}
	};

	struct Loop : public Entity {
		std::shared_ptr<HalfEdge> st;
		std::shared_ptr<Face> face;
	};

	struct Face : public Entity {
		std::shared_ptr<Loop> st;
		std::shared_ptr<Solid> solid;
	};

	struct Solid : public Entity {
		std::set<std::shared_ptr<Face>> faces;

		void AddFace(const std::shared_ptr<Face>& f) {
			faces.insert(f);
		}

		void RemoveFace(const std::shared_ptr<Face>& f) {
			if (auto it = faces.find(f); it != faces.end()) {
				faces.erase(it);
			}
		}
	};

	void Edge::UpdateHalfEdgesPartner() {
		// 按照halfedge添加顺序串成一个环
		for (int h = 0; h < halfEdges.size(); h++) {
			halfEdges[h]->partner = halfEdges[(h + 1) % halfEdges.size()];
		}
	}

	enum class TopoType {
		NoExist = 0,
		Entity = 1,
		Solid = 2,
		Face = 3,
		Loop = 4,
		HalfEdge = 5,
		Edge = 6,
		Vertex = 7
	};

	class ObjMarkNum {
	public:
		std::map<TopoType, int> capacities;// 对应类型最大容量计数
		std::map<Entity*, std::pair<TopoType, int>> markNumMap; // 用于由指针访问类型与对应计数
		std::vector<std::shared_ptr<Solid>> solids; // 用于访问所有实体

		std::map<TopoType, std::list<int>> deletedIdListsMap;

		// 有特殊用处需要维护的数据结构
		std::map<std::pair<int, int>, std::shared_ptr<Edge>> edgesMap; // (vertex id, vertex id（有序对）) -> edge，注意：索引是有序对！

		// 单例
		static ObjMarkNum& GetInstance() {
			static ObjMarkNum marknum_instance;
			return marknum_instance;
		}

		// 由ObjInfo加载半边数据结构
		void LoadFromObjInfo(ObjInfo& obj_info) {

			Clear();

			// 顶点
			std::vector<std::shared_ptr<Vertex>> vertex_ptrs; // vertices: 顶点列表（顶点导出不能依赖于这个列表）

			for (int i = 0, j = 0; i < obj_info.vertices.size(); i += 3, j += 1) {
				auto vertex_ptr = std::make_shared<Vertex>();
				UpdateMarkNumMap(vertex_ptr);
				vertex_ptr->pointCoord = obj_info.GetPoint(j);

				vertex_ptrs.emplace_back(vertex_ptr);
			}

			auto make_edge = [&](int i, int j) -> std::shared_ptr<Edge> {

				// 注意：这里只有map的key是需要保证有序的，而下面构造edge的时候会用到原始顺序的索引，因此这里要单独使用变量保存key
				int search_i = i;
				int search_j = j;

				if (search_i > search_j) {
					std::swap(search_i, search_j);
				}

				// 不存在：创建新边
				if (auto it = edgesMap.find({ search_i,search_j }); it == edgesMap.end()) {
					std::shared_ptr<Edge> edge_ptr = std::make_shared<Edge>();
					UpdateMarkNumMap(edge_ptr);

					// 初始化边的信息（到时候可能要挪到函数里面）
					// 此处必须使用原始顶点索引
					edge_ptr->st = vertex_ptrs[i];
					edge_ptr->ed = vertex_ptrs[j];

					edgesMap[{search_i, search_j}] = edge_ptr;

					return edge_ptr;
				}
				// 存在：返回
				else {
					return it->second;
				}
				};

			auto make_halfedge = [&](int i, int j) -> std::shared_ptr<HalfEdge> {
				auto edge_ptr = make_edge(i, j);
				std::shared_ptr<HalfEdge> halfedge_ptr = std::make_shared<HalfEdge>();
				UpdateMarkNumMap(halfedge_ptr);

				// 更新halfedge的edge
				halfedge_ptr->edge = edge_ptr;

				// 更新sense
				if (markNumMap[edge_ptr->st.get()].second == i && markNumMap[edge_ptr->ed.get()].second == j) {
					halfedge_ptr->sense = false;
				}
				else {
					halfedge_ptr->sense = true;
				}

				// 添加此halfedge到edge的halfedgeList中，同时更新halfedgeList中已经有的halfedge的相邻关系
				edge_ptr->halfEdges.emplace_back(halfedge_ptr);

				// 按照halfedge添加顺序串成一个环
				edge_ptr->UpdateHalfEdgesPartner();

				return halfedge_ptr;
				};

			auto make_loop = [&](const std::shared_ptr<HalfEdge>& a, const std::shared_ptr<HalfEdge>& b, const std::shared_ptr<HalfEdge>& c) -> std::shared_ptr<Loop> {

				// set next & pre
				a->next = b;
				a->pre = c;

				b->next = c;
				b->pre = a;

				c->next = a;
				c->pre = b;

				// make loop
				std::shared_ptr<Loop> lp = std::make_shared<Loop>();
				UpdateMarkNumMap(lp);
				lp->st = a;

				// set 
				a->loop = lp;
				b->loop = lp;
				c->loop = lp;

				return lp;
				};

			auto make_face = [&](const std::shared_ptr<Loop>& lp) -> std::shared_ptr<Face> {
				// make face
				std::shared_ptr<Face> f = std::make_shared<Face>();
				UpdateMarkNumMap(f);

				f->st = lp;

				// set lp face
				lp->face = f;

				return f;
				};

			auto make_solid = [&](const std::set<std::shared_ptr<Face>>& faces) -> std::shared_ptr<Solid> {

				// make solid
				std::shared_ptr<Solid> solid = std::make_shared<Solid>();
				UpdateMarkNumMap(solid);

				// update face solid
				for (auto f : faces) {
					f->solid = solid;
				}

				// copy faces vector
				solid->faces = faces;

				return solid;
				};

			// 对于每个solid
			int s = 0;
			for (auto solid_range : obj_info.solidIndicesRange) {
				int range_begin = solid_range.first;
				int range_end = solid_range.second;

				std::set<std::shared_ptr<Face>> faces;
				// 对应solid的顶点范围: 每3个点的索引构成一个三角形
				for (int i = range_begin; i < range_end; i += 3) {
					int j = i + 1;
					int k = i + 2;

					int index_i = obj_info.indices[i];
					int index_j = obj_info.indices[j];
					int index_k = obj_info.indices[k];

					// half edge
					auto halfedge_ij = make_halfedge(index_i, index_j);
					auto halfedge_jk = make_halfedge(index_j, index_k);
					auto halfedge_ki = make_halfedge(index_k, index_i);

					// loop
					auto loop = make_loop(halfedge_ij, halfedge_jk, halfedge_ki);

					// face
					auto face = make_face(loop);
					faces.insert(face);
				}

				// solid
				auto solid = make_solid(faces);
				solids.emplace_back(solid);

				s++;
			}

		}


		std::shared_ptr<Edge> FindEdgeBetweenVertices(const std::shared_ptr<Vertex>& v1, const std::shared_ptr<Vertex>& v2) {

			int v1_id = -1, v2_id = -1;
			if (auto it = markNumMap.find(v1.get()); it != markNumMap.end()) {
				v1_id = it->second.second;
			}
			else {
				//SPDLOG_ERROR("v1 is not exist in markNumMap.");
				return nullptr;
			}

			if (auto it = markNumMap.find(v2.get()); it != markNumMap.end()) {
				v2_id = it->second.second;
			}
			else {
				//SPDLOG_ERROR("v2 is not exist in markNumMap.");
				return nullptr;
			}

			if (auto v1_v2_edge_it = edgesMap.find({ v1_id, v2_id }); v1_v2_edge_it != edgesMap.end()) {
				return v1_v2_edge_it->second;
			}

			if (auto v2_v1_edge_it = edgesMap.find({ v2_id, v1_id }); v2_v1_edge_it != edgesMap.end()) {
				return v2_v1_edge_it->second;
			}

			return nullptr;
		}

		void Clear() {
			capacities.clear();
			markNumMap.clear();
			solids.clear();
			deletedIdListsMap.clear();

			edgesMap.clear();

		}

		int GetId(const std::shared_ptr<Entity>& p) {
			if (auto it = markNumMap.find(p.get()); it != markNumMap.end()) {
				return it->second.second;
			}

			return -1;
		}

		TopoType GetType(const std::shared_ptr<Entity>& p) {

			if (auto it = markNumMap.find(p.get()); it != markNumMap.end()) {
				return it->second.first;
			}

			return TopoType::NoExist;
		}

		template<typename T>
		TopoType GetTypeFromTemplate(const std::shared_ptr<T>& p) {
			TopoType topotype_name = TopoType::NoExist;

			if constexpr (std::is_same_v<T, Vertex>) {
				topotype_name = TopoType::Vertex;
			}
			else if constexpr (std::is_same_v<T, HalfEdge>) {
				topotype_name = TopoType::HalfEdge;
			}
			else if constexpr (std::is_same_v<T, Edge>) {
				topotype_name = TopoType::Edge;
			}
			else if constexpr (std::is_same_v<T, Loop>) {
				topotype_name = TopoType::Loop;
			}
			else if constexpr (std::is_same_v<T, Face>) {
				topotype_name = TopoType::Face;
			}
			else if constexpr (std::is_same_v<T, Solid>) {
				topotype_name = TopoType::Solid;
			}
			else {
				topotype_name = TopoType::NoExist;
			}

			return topotype_name;
		}


	private:

		ObjMarkNum() {}

		// 由指针类型更新UpdateMarkNumMap
		// 更新逻辑
		template<typename T>
		void UpdateMarkNumMap(const std::shared_ptr<T>& ptr) {
			TopoType topotype_name = GetTypeFromTemplate(ptr);
			markNumMap[ptr.get()] = { topotype_name, (capacities[topotype_name])++ };
		}

	};



	class SatStlRenderer: public IRenderable {
	public:
		unsigned int VAO;
		unsigned int VBO;
		int stlVerticesCount;
		Shader shader;

		glm::mat4 modelMatrix{ 1.0f };

		void Render(
			const RenderInfo& renderInfo
		) override {
			if (renderInfo.showModel) {
				shader.use();

				shader.setMatrix4("projection", renderInfo.projection_matrix);
				shader.setMatrix4("view", renderInfo.view_matrix);
				shader.setMatrix4("model", modelMatrix);
				shader.setVec3("viewPos", renderInfo.camera_pos);

				glBindVertexArray(VAO);
				glDrawArrays(GL_TRIANGLES, 0, stlVerticesCount);
			}
		}

		void LoadFromSatInfo(SatInfo& satInfo) {
			stlVerticesCount = satInfo.stlVerticesCount;

			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * satInfo.stlVertices.size(), satInfo.stlVertices.data(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		// 通过移动转移shader所有权
		SatStlRenderer(Shader&& shader):
			shader(std::move(shader)),
			VAO(0),
			VBO(0)
		{}

		~SatStlRenderer() {}
	};

	class SatLineRenderer : public IRenderable {
	public:
		std::vector<unsigned int> VAOs;
		std::vector<unsigned int> VBOs;
		std::vector<int> edgeSampledPointsCounts;
		std::vector<glm::vec3> edgeColors;
		Shader shader;

		glm::mat4 modelMatrix{ 1.0f };

		void Render(
			const RenderInfo& renderInfo
		) override {
			shader.use();

			shader.setMatrix4("projection", renderInfo.projection_matrix);
			shader.setMatrix4("view", renderInfo.view_matrix);
			shader.setMatrix4("model", modelMatrix);
			//shader.setVec3("viewPos", camera_pos);

			for (int h = 0; h < VAOs.size(); h++) {
				int VAO = VAOs[h];
				int VAO_size = edgeSampledPointsCounts[h];
				glm::vec3 color = edgeColors[h];

				shader.setVec3("subcolor", color);
				glBindVertexArray(VAO);
				glDrawArrays(GL_LINE_STRIP, 0, VAO_size);
			}
		}

		void LoadFromSatInfo(SatInfo& satInfo) {
			VAOs.clear();
			VBOs.clear();
			edgeSampledPointsCounts = satInfo.edgeSampledPointsCounts;
			edgeColors = satInfo.edgeColors;
			
			for (auto vertices : satInfo.edgeSampledPoints) {
				unsigned VAO, VBO;
				glGenVertexArrays(1, &VAO);
				glGenBuffers(1, &VBO);

				glBindVertexArray(VAO);
				glBindBuffer(GL_ARRAY_BUFFER, VBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertices.size(), vertices.data(), GL_STATIC_DRAW);

				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

				glBindBuffer(GL_ARRAY_BUFFER, 0);
				glBindVertexArray(0);

				VAOs.emplace_back(VAO);
				VBOs.emplace_back(VBO);
			}
		}

		// 通过移动转移shader所有权
		SatLineRenderer(Shader&& shader) :
			shader(std::move(shader))
		{}

		~SatLineRenderer() {}
	};


	void ProcessToggleKey(GLFWwindow* window, int key, bool& var, bool& varPressed) {
		if (glfwGetKey(window, key) == GLFW_PRESS && !varPressed) {
			var = !var;
			varPressed = true;

			std::cout << "showModel: " << var << std::endl;
		}
		if (glfwGetKey(window, key) == GLFW_RELEASE) {
			varPressed = false;
		}
	}

	class MouseController {

	public:
		void MouseCallback(GLFWwindow* window, double xposIn, double yposIn) {
			float xpos = static_cast<float>(xposIn);
			float ypos = static_cast<float>(yposIn);

			if (firstMouse) {
				lastX = xpos;
				lastY = ypos;
				firstMouse = false;
			}

			float xoffset = xpos - lastX;
			float yoffset = -ypos + lastY; // reversed since y-coordinates go from bottom to top
			lastX = xpos;
			lastY = ypos;

			camera.ProcessMouseMovement(xoffset, yoffset);
		}

		void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
			camera.ProcessMouseScroll(static_cast<float>(yoffset));
		}

		MouseController(Camera& icamera):
			lastX(0.0f),
			lastY(0.0f),
			firstMouse(true),
			camera(icamera)
		{}

	private:
		Camera& camera;
		bool firstMouse;
		float lastX;
		float lastY;
	};

	class KeyboardController {
	public:
		bool& showModel;
		bool showModelPressed;
		float keyboardMovementSpeed;

		void KeyboardProcessInput(GLFWwindow* window, float deltaTime) {
			if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
				glfwSetWindowShouldClose(window, true);

			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
				camera.ChangeMovementSpeed(0.1);
				std::cout << "MovementSpeed: " << camera.MovementSpeed << std::endl;
			}

			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
				camera.ChangeMovementSpeed(-0.1);
				std::cout << "MovementSpeed: " << camera.MovementSpeed << std::endl;
			}

			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
				camera.ProcessKeyboard(FORWARD, deltaTime);
			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
				camera.ProcessKeyboard(BACKWARD, deltaTime);
			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				camera.ProcessKeyboard(LEFT, deltaTime);
			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				camera.ProcessKeyboard(RIGHT, deltaTime);

			// 为了方便远程的尝试
			if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
				camera.ProcessMouseMovement(0.0f, keyboardMovementSpeed);
			if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS)
				camera.ProcessMouseMovement(0.0f, -keyboardMovementSpeed);
			if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS)
				camera.ProcessMouseMovement(-keyboardMovementSpeed, 0.0f);
			if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
				camera.ProcessMouseMovement(keyboardMovementSpeed, 0.0f);

			ProcessToggleKey(window, GLFW_KEY_B, showModel, showModelPressed);
		}

		KeyboardController(Camera& icamera, bool& showModel) :
			camera(icamera),
			showModel(showModel),
			showModelPressed(false),
			keyboardMovementSpeed(5.0f)
		{}

	private:
		Camera& camera;
	};

	class MyRenderEngine {

	public:
		// settings
		unsigned int screenWidth;
		unsigned int screenHeight;
		glm::vec4 backgroundColor;
		bool showModel;

		GLFWwindow* window;
		Camera camera;
		MouseController mouseController;
		KeyboardController keyboardController;

		std::vector<std::shared_ptr<IRenderable>> IRenderables;

		// glfw: whenever the window size changed (by OS or user resize) this callback function executes
		void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
			// make sure the viewport matches the new window dimensions; note that width and 
			// height will be significantly larger than specified on retina displays.
			glViewport(0, 0, width, height);
		}

		void AddRenderable(const std::shared_ptr<IRenderable>& r) {
			IRenderables.emplace_back(r);
		}

		void SetCameraPos(const glm::vec3& pos) {
			camera.Position = pos;
		}

		void StartRenderLoop() {

			RenderInfo renderInfo;

			while (!glfwWindowShouldClose(window)) {
				// poll IO events (keys pressed/released, mouse moved etc.)
				glfwPollEvents();

				// Imgui settings first
				if (glfwGetWindowAttrib(window, GLFW_ICONIFIED) != 0)
				{
					ImGui_ImplGlfw_Sleep(10);
					continue;
				}

				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplGlfw_NewFrame();
				ImGui::NewFrame();

				ImGui::ShowDemoWindow();

				// per-frame time logic
				float currentFrame = static_cast<float>(glfwGetTime());
				deltaTime = currentFrame - lastFrame;
				lastFrame = currentFrame;

				// input
				// -----
				keyboardController.KeyboardProcessInput(window, deltaTime);

				// render
				// -----
				int display_w, display_h;
				glfwGetFramebufferSize(window, &display_w, &display_h);
				glViewport(0, 0, display_w, display_h);

				glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				glm::mat4 projectionMatrix = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(screenWidth) / static_cast<float>(screenHeight), camera.Near, camera.Far);
				glm::mat4 viewMatrix = camera.GetViewMatrix();
				glm::mat4 modelMatrix{ 1.0f };

				renderInfo.projection_matrix = projectionMatrix;
				renderInfo.view_matrix = viewMatrix;
				renderInfo.camera_pos = camera.Position;

				renderInfo.showModel = showModel;

				// render IRenderable
				for (auto&& r : IRenderables) {
					r->Render(renderInfo);
				}

				// Imgui rendering
				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

				// glfw: swap buffers
				// -------------------------------------------------------------------------------
				glfwSwapBuffers(window);
			}

			// glfw: terminate, clearing all previously allocated GLFW resources.
			// ------------------------------------------------------------------
			glfwTerminate();
		}

		void SetupImGui() {
			// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

			// Setup Dear ImGui style
			ImGui::StyleColorsDark();
			//ImGui::StyleColorsLight();

			// Setup Platform/Renderer backends
			ImGui_ImplGlfw_InitForOpenGL(window, true);
			ImGui_ImplOpenGL3_Init("#version 130");
		}

		void SetupGlobalOpenglState() {
			glEnable(GL_DEPTH_TEST);
			//glEnable(GL_BLEND);
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		}

		MyRenderEngine(): 
			camera(glm::vec3{1.0f}),
			mouseController(camera),
			keyboardController(camera, showModel),
			deltaTime(0.0f),
			lastFrame(0.0f),

			screenWidth(1024),
			screenHeight(768),
			backgroundColor(0.2f, 0.3f, 0.3f, 1.0f)
		{
			int init_res = InitWindow(window);
			if (init_res != 0) {
				throw std::runtime_error("Init window failed");
			}
			glfwMakeContextCurrent(window);
			glfwSwapInterval(1); // Enable vsync

			SetupImGui();
			SetupGlobalOpenglState();
		}

	private:
		float deltaTime;
		float lastFrame;

		int InitWindow(GLFWwindow* &window) {
			glfwInit();
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			glfwWindowHint(GLFW_SAMPLES, 4);

#ifdef __APPLE__
			glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
			window = glfwCreateWindow(screenWidth, screenHeight, "MySatViewer", NULL, NULL);
			if (window == nullptr) {
				std::cout << "Failed to create GLFW window" << std::endl;
				glfwTerminate();
				return -1;
			}
			glfwMakeContextCurrent(window);
			glfwSetWindowPos(window, 200, 200);

			auto framebufferSizeCallback = [](GLFWwindow* window, int width, int height) {
				MyRenderEngine* app = static_cast<MyRenderEngine*>(glfwGetWindowUserPointer(window));
				if (app) {
					app->FramebufferSizeCallback(window, width, height);
				}
			};

			auto mouseCallback = [](GLFWwindow* window, double xposIn, double yposIn) {
				MyRenderEngine* app = static_cast<MyRenderEngine*>(glfwGetWindowUserPointer(window));
				if (app) {
					app->mouseController.MouseCallback(window, xposIn, yposIn);
				}
			};

			auto scrollCallback = [](GLFWwindow* window, double xoffset, double yoffset) {
				MyRenderEngine* app = static_cast<MyRenderEngine*>(glfwGetWindowUserPointer(window));
				if (app) {
					app->mouseController.ScrollCallback(window, xoffset, yoffset);
				}
			};

			glfwSetWindowUserPointer(window, this);
			glfwSetFramebufferSizeCallback(window, framebufferSizeCallback);
			//glfwSetCursorPosCallback(window, mouseCallback); // 暂时禁用鼠标
			glfwSetScrollCallback(window, scrollCallback);

			// glad: load all OpenGL function pointers
			// ---------------------------------------
			if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
			{
				std::cout << "Failed to initialize GLAD" << std::endl;
				return -1;
			}

			return 0;
		}
	};

	class SatGuiRenderer : public IRenderable {
	public:
		SatInfo& satInfo;

		void Render(
			const RenderInfo& renderInfo
		) override {

			ImGui::Begin("SAT Edges Info");
			if (ImGui::TreeNode("Edges")) {
				int id = 0;

				// 按照edge编号排序
				// TODO: 可以优化
				std::vector<std::pair<int, EdgeInfo>> edges_info;
				for (auto&& [edge_marknum, edge_info] : satInfo.edgeMap) {
					edges_info.emplace_back(edge_marknum, edge_info);
				}

				std::sort(edges_info.begin(), edges_info.end(), [](const std::pair<int, EdgeInfo>& a, const std::pair<int, EdgeInfo>& b) {return a.first < b.first; });

				for (auto [edge_marknum, edge_info] : edges_info) {

					bool color_flag = false;
					ImU32 treenode_color;
					if (edge_info.nonmanifoldCount == 1) {
						color_flag = true;
						treenode_color = IM_COL32(255, 0, 0, 255); // Red 
					}
					else if (edge_info.nonmanifoldCount > 2) {
						color_flag = true;
						treenode_color = IM_COL32(0, 0, 255, 255); // Blue
					}

					if (color_flag) {
						ImGui::PushStyleColor(ImGuiCol_Text, treenode_color);
					}

					ImGui::PushID(id++);
					if (ImGui::TreeNode("", "Edge: %d", edge_marknum)) {

						ImGui::Text("Index: %d", edge_info.edgeIndex);
						ImGui::Text("Nonmanifold Count: %d", edge_info.nonmanifoldCount);
						ImGui::Text("Body ID: %d", edge_info.bodyId);
						ImGui::Text("Edge Mid Pos: (%d, %d, %d)", edge_info.edgeMidPos.x, edge_info.edgeMidPos.y, edge_info.edgeMidPos.z);
						if (ImGui::Button("Go")) {
							myRenderEngine.SetCameraPos(edge_info.edgeMidPos);
						}

						ImGui::TreePop();
					}

					if (color_flag) {
						ImGui::PopStyleColor();
					}

					ImGui::PopID();
				}

				ImGui::TreePop();
			}


			ImGui::End();
		}

		SatGuiRenderer(SatInfo& satInfo, MyRenderEngine& myRenderEngine) : satInfo(satInfo), myRenderEngine(myRenderEngine) {}

		~SatGuiRenderer() {}

	private:
		MyRenderEngine& myRenderEngine;
	};

	class BasicGuiRenderer : public IRenderable {
	public:

		void Render(
			const RenderInfo& renderInfo
		) override {
			ImGui::Begin("Basic Info");

			// 相机位置
			float& camX = myRenderEngine.camera.Position.x;
			float& camY = myRenderEngine.camera.Position.y;
			float& camZ = myRenderEngine.camera.Position.z;

			ImGui::InputFloat("Camera X", &camX);
			ImGui::InputFloat("Camera Y", &camY);
			ImGui::InputFloat("Camera Z", &camZ);

			// 移动速度
			ImGui::InputFloat("Camera Speed", &myRenderEngine.camera.MovementSpeed);


			ImGui::End();


		}

		BasicGuiRenderer(MyRenderEngine& myRenderEngine): myRenderEngine(myRenderEngine) {

		}

	private:
		MyRenderEngine& myRenderEngine;
	};

	// Temp: no use
	class ObjRenderer : public IRenderable {
	public:
		ObjInfo& objInfo;

		unsigned int VAO;
		unsigned int VBO;
		unsigned int EBO;
		Shader shader;

		glm::mat4 modelMatrix{ 1.0f };

		void Setup() {
			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);
			glGenBuffers(1, &EBO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * objInfo.vertices.size(), objInfo.vertices.data(), GL_STATIC_DRAW);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * objInfo.indices.size(), objInfo.indices.data(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
			glBindVertexArray(0);
		}

		void Render(
			const RenderInfo& renderInfo
		) override {
			shader.use();

			shader.setMatrix4("projection", renderInfo.projection_matrix);
			shader.setMatrix4("view", renderInfo.view_matrix);
			shader.setMatrix4("model", modelMatrix);

			glBindVertexArray(VAO);
			glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(objInfo.indices.size()), GL_UNSIGNED_INT, 0);
			glBindVertexArray(0);
		}


		ObjRenderer(ObjInfo& objInfo, Shader&& shader): objInfo(objInfo), shader(std::move(shader)) {}
		~ObjRenderer() {}
	};


	struct ObjLineWithGuiRendererInputs {
		float scale_factor;
		double distance_threshold;
		double angle_threshold;
	};

	class ObjLineWithGuiRenderer : public IRenderable {
	public:
		ObjInfo& objInfo;
		ObjLineWithGuiRendererInputs inputs; // 这个必须要用复制传递，否则会有生命周期问题

		std::vector<float> yellow_lines;
		std::vector<float> green_lines;
		std::vector<float> red_lines;

		unsigned int yellowVAO;
		unsigned int yellowVBO;

		unsigned int greenVAO;
		unsigned int greenVBO;

		unsigned int redVAO;
		unsigned int redVBO;

		Shader* shader;

		glm::mat4 modelMatrix{ 1.0f };

		void ModifyModelMatrixWithScale() {
			auto new_matrix = Utils::CalculateModelMatrix(glm::vec3{ 0.0f }, glm::vec3{ 0.0f }, glm::vec3{ inputs.scale_factor });
			modelMatrix = new_matrix;
		}

		void OnApplyNewInputs() {
			Setup();
		}

		void OnGoClick(glm::vec3 p1, glm::vec3 p2) {
			p1 = glm::vec4(p1, 1.0f) * modelMatrix;
			p2 = glm::vec4(p2, 1.0f) * modelMatrix;

			glm::vec3 mid = (p1 + p2) / 2.0f;
			myRenderEngine.SetCameraPos(mid);
		}

		void DeleteBuffers() {
			glDeleteVertexArrays(1, &yellowVAO);
			glDeleteVertexArrays(1, &greenVAO);
			glDeleteVertexArrays(1, &redVAO);
			glDeleteBuffers(1, &yellowVBO);
			glDeleteBuffers(1, &greenVBO);
			glDeleteBuffers(1, &redVBO);
		}

		void Setup() {
			ModifyModelMatrixWithScale();

			yellow_lines.clear();
			green_lines.clear();
			red_lines.clear();

			std::set<std::pair<int, int>> ps;
			std::map<std::pair<int, int>, glm::vec3> color_map;
			int red_count = 0;
			int yellow_count = 0;

			auto swap_pair = [](std::pair<int, int> p) {
				if (p.first > p.second) {
					std::swap(p.first, p.second);
				}
			};

			auto add_point_and_color = [&](std::pair<int, int> p, glm::vec3 point1, glm::vec3 point2, glm::vec3 color) {
				if (ps.count(p) == 0) {

					if (color == glm::vec3{ 1.0f, 1.0f, 0.0f }) { // Y
						yellow_count++;
						yellow_lines.emplace_back(point1.x);
						yellow_lines.emplace_back(point1.y);
						yellow_lines.emplace_back(point1.z);

						yellow_lines.emplace_back(point2.x);
						yellow_lines.emplace_back(point2.y);
						yellow_lines.emplace_back(point2.z);
					}
					else if (color == glm::vec3{ 1.0f, 0.0f, 0.0f }){ // R
						red_count++;

						red_lines.emplace_back(point1.x);
						red_lines.emplace_back(point1.y);
						red_lines.emplace_back(point1.z);

						red_lines.emplace_back(point2.x);
						red_lines.emplace_back(point2.y);
						red_lines.emplace_back(point2.z);
					}
					else {
						green_lines.emplace_back(point1.x);
						green_lines.emplace_back(point1.y);
						green_lines.emplace_back(point1.z);

						green_lines.emplace_back(point2.x);
						green_lines.emplace_back(point2.y);
						green_lines.emplace_back(point2.z);
					}

					ps.insert(p);
				}
			};

			auto get_color = [&](std::pair<int, int> p, glm::vec3 point1, glm::vec3 point2, glm::vec3 point3) -> glm::vec3 {
				glm::vec3 color{ 0.0f, 1.0f, 0.0f }; // G

				// 距离
				float dis = glm::distance(point1, point2);
				if (dis < inputs.distance_threshold) {
					color = glm::vec3{ 1.0f, 0.0f, 0.0f }; // R
					std::cout << "dis: " << dis << std::endl;
				}

				// 角度
				glm::vec3 v1 = point1 - point3;
				glm::vec3 v2 = point2 - point3;
				float dot_value = glm::dot(v1, v2);

				float angle = acos(dot_value / (glm::length(v1) * glm::length(v2))) * 180.0f / M_PI;

				if (angle > inputs.angle_threshold) {
					color = glm::vec3{ 1.0f, 1.0f, 0.0f }; // Y
					std::cout << "angle: "<< angle << std::endl;
				}

				return color;
			};

			auto assign_color = [&](std::pair<int, int> p, glm::vec3 point1, glm::vec3 point2, glm::vec3 point3) {
				if (color_map.find(p) == color_map.end() || color_map[p] == glm::vec3{ 0.0f, 1.0f, 0.0f }) {
					color_map[p] = get_color(p, point1, point2, point3);
				}
			};

			// 标记颜色
			for (int i = 0; i < objInfo.indices.size(); i += 3) {
				int j = i + 1;
				int k = i + 2;

				int ii = objInfo.indices[i];
				int jj = objInfo.indices[j];
				int kk = objInfo.indices[k];

				glm::vec3 point1{
					objInfo.vertices[3 * (ii)+0],
					objInfo.vertices[3 * (ii)+1],
					objInfo.vertices[3 * (ii)+2]
				};

				glm::vec3 point2{
					objInfo.vertices[3 * (jj)+0],
					objInfo.vertices[3 * (jj)+1],
					objInfo.vertices[3 * (jj)+2]
				};

				glm::vec3 point3{
					objInfo.vertices[3 * (kk)+0],
					objInfo.vertices[3 * (kk)+1],
					objInfo.vertices[3 * (kk)+2]
				};

				auto p1 = std::make_pair(ii, jj);
				auto p2 = std::make_pair(jj, kk);
				auto p3 = std::make_pair(ii, kk);

				swap_pair(p1); // 1, 2
				swap_pair(p2); // 2, 3
				swap_pair(p3); // 1, 3

				assign_color(p1, point1, point2, point3);
				assign_color(p2, point2, point3, point1);
				assign_color(p3, point1, point3, point2);
				
			}

			// 标记完成后，将颜色等信息添加到vector中
			for (int i = 0; i < objInfo.indices.size(); i+=3) {
				int j = i + 1;
				int k = i + 2;

				int ii = objInfo.indices[i];
				int jj = objInfo.indices[j];
				int kk = objInfo.indices[k];

				glm::vec3 point1{
					objInfo.vertices[3 * (ii)+0],
					objInfo.vertices[3 * (ii)+1],
					objInfo.vertices[3 * (ii)+2]
				};

				glm::vec3 point2{
					objInfo.vertices[3 * (jj)+0],
					objInfo.vertices[3 * (jj)+1],
					objInfo.vertices[3 * (jj)+2]
				};

				glm::vec3 point3{
					objInfo.vertices[3 * (kk)+0],
					objInfo.vertices[3 * (kk)+1],
					objInfo.vertices[3 * (kk)+2]
				};

				auto p1 = std::make_pair(ii, jj);
				auto p2 = std::make_pair(jj, kk);
				auto p3 = std::make_pair(ii, kk);

				swap_pair(p1);
				swap_pair(p2);
				swap_pair(p3);

				add_point_and_color(p1, point1, point2, color_map[p1]);
				add_point_and_color(p2, point2, point3, color_map[p2]);
				add_point_and_color(p3, point1, point3, color_map[p3]);

			}

			std::cout << "yellow count: " << yellow_count << std::endl;
			std::cout << "red count: " << red_count << std::endl;

			auto set_vao = [&](unsigned int& VAO, unsigned int& VBO, std::vector<float>& lines) {
				glGenVertexArrays(1, &VAO);
				glGenBuffers(1, &VBO);
				//glGenBuffers(1, &EBO);

				glBindVertexArray(VAO);
				glBindBuffer(GL_ARRAY_BUFFER, VBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(float) * lines.size(), lines.data(), GL_STATIC_DRAW);

				//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
				//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * objInfo.indices.size(), objInfo.indices.data(), GL_STATIC_DRAW);

				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
				//glEnableVertexAttribArray(1);
				//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

				glBindVertexArray(0);
			};

			DeleteBuffers();

			set_vao(yellowVAO, yellowVBO, yellow_lines);
			set_vao(redVAO, redVBO, red_lines);
			set_vao(greenVAO, greenVBO, green_lines);
		}

		void RenderGui() {
			ImGui::Begin("OBJ Edges Info");

			if (ImGui::TreeNode("Inputs Control")) {

				ImGui::InputFloat("scale_factor", &inputs.scale_factor, 0.1f, 1.0f, "%.3f");
				ImGui::InputDouble("distance_threshold", &inputs.distance_threshold, 0.0001f, 0.01f, "%.8f");
				ImGui::InputDouble("angle_threshold", &inputs.angle_threshold, 0.1f, 10.0f, "%.8f");

				if (ImGui::Button("Change Inputs")) {
					OnApplyNewInputs();
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Red Edges")) {
				int id = 0;
				for (int i = 0; i < red_lines.size(); i += 6) {
					ImGui::PushID(id++);
					if (ImGui::TreeNode("", "Red Edge: %d", i / 6)) {
						if (ImGui::Button("Go")) {

							glm::vec3 p1{
								red_lines[i + 0],
								red_lines[i + 1],
								red_lines[i + 2]
							};

							glm::vec3 p2{
								red_lines[i + 3],
								red_lines[i + 4],
								red_lines[i + 5]
							};

							OnGoClick(p1, p2);
						}

						ImGui::TreePop();
					}
					ImGui::PopID();
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Yellow Edges")) {
				int id = 0;
				for (int i = 0; i < yellow_lines.size(); i += 6) {
					ImGui::PushID(id++);
					if (ImGui::TreeNode("", "Yellow Edge: %d", i / 6)) {
						if (ImGui::Button("Go")) {

							glm::vec3 p1{
								yellow_lines[i + 0],
								yellow_lines[i + 1],
								yellow_lines[i + 2]
							};

							glm::vec3 p2{
								yellow_lines[i + 3],
								yellow_lines[i + 4],
								yellow_lines[i + 5]
							};

							OnGoClick(p1, p2);
						}

						ImGui::TreePop();
					}
					ImGui::PopID();
				}
				ImGui::TreePop();
			}

			

			ImGui::End();
		}

		void Render(
			const RenderInfo& renderInfo
		) override {
			shader->use();

			shader->setMatrix4("projection", renderInfo.projection_matrix);
			shader->setMatrix4("view", renderInfo.view_matrix);
			shader->setMatrix4("model", modelMatrix);

			/*glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(objInfo.indices.size()), GL_UNSIGNED_INT, 0);*/

			shader->setVec3("subcolor", glm::vec3(1.0f, 1.0f, 0.0f));
			glBindVertexArray(yellowVAO);
			glDrawArrays(GL_LINES, 0, yellow_lines.size()/3);
			glBindVertexArray(0);

			shader->setVec3("subcolor", glm::vec3(1.0f, 0.0f, 0.0f));
			glBindVertexArray(redVAO);
			glDrawArrays(GL_LINES, 0, red_lines.size() / 3);
			glBindVertexArray(0);

			shader->setVec3("subcolor", glm::vec3(0.0f, 1.0f, 0.0f));
			glBindVertexArray(greenVAO);
			glDrawArrays(GL_LINES, 0, green_lines.size() / 3);
			glBindVertexArray(0);

			RenderGui();
		}


		ObjLineWithGuiRenderer(ObjInfo& objInfo, ObjLineWithGuiRendererInputs inputs, Shader* shader, MyRenderEngine& myRenderEngine) : objInfo(objInfo), inputs(inputs), shader(shader), myRenderEngine(myRenderEngine) {}

		~ObjLineWithGuiRenderer() {
			DeleteBuffers();
		}

	private:
		MyRenderEngine& myRenderEngine;
	};

	struct ObjNonManifoldLineWithGuiRendererInputs {
		float scale_factor;
	};

	class ObjNonManifoldLineWithGuiRenderer : public IRenderable {
	public:
		ObjNonManifoldLineWithGuiRendererInputs inputs;

		std::vector<float> yellow_lines;
		std::vector<float> green_lines;
		std::vector<float> red_lines;

		unsigned int yellowVAO;
		unsigned int yellowVBO;

		unsigned int greenVAO;
		unsigned int greenVBO;

		unsigned int redVAO;
		unsigned int redVBO;

		Shader* shader;

		glm::mat4 modelMatrix{ 1.0f };

		void ModifyModelMatrixWithScale() {
			auto new_matrix = Utils::CalculateModelMatrix(glm::vec3{ 0.0f }, glm::vec3{ 0.0f }, glm::vec3{ inputs.scale_factor });
			modelMatrix = new_matrix;
		}

		void OnApplyNewInputs() {
			
			// 这里如果不改变模型本身着色性质的话可以直接调用修改模型矩阵的函数就好了
			ModifyModelMatrixWithScale();

		}

		void OnGoClick(glm::vec3 p1, glm::vec3 p2) {
			p1 = glm::vec4(p1, 1.0f) * modelMatrix;
			p2 = glm::vec4(p2, 1.0f) * modelMatrix;

			glm::vec3 mid = (p1 + p2) / 2.0f;
			myRenderEngine.SetCameraPos(mid);
		}

		void DeleteBuffers() {
			glDeleteVertexArrays(1, &yellowVAO);
			glDeleteVertexArrays(1, &greenVAO);
			glDeleteVertexArrays(1, &redVAO);
			glDeleteBuffers(1, &yellowVBO);
			glDeleteBuffers(1, &greenVBO);
			glDeleteBuffers(1, &redVBO);
		}

		void SetUp() {
			ModifyModelMatrixWithScale();

			yellow_lines.clear();
			green_lines.clear();
			red_lines.clear();

			// 添加顶点坐标到3个vector中，每个vector中按照3个开始点，3个结束点这样交替存放顶点
			for (auto e_pair : objMarkNum.edgesMap) {
				auto e = e_pair.second;
				auto st_coord = e->st->pointCoord;
				auto ed_coord = e->ed->pointCoord;
				if (e->halfEdges.size() == 2) { // green
					green_lines.emplace_back(st_coord.x());
					green_lines.emplace_back(st_coord.y());
					green_lines.emplace_back(st_coord.z());
					
					green_lines.emplace_back(ed_coord.x());
					green_lines.emplace_back(ed_coord.y());
					green_lines.emplace_back(ed_coord.z());
				}
				else if (e->halfEdges.size() == 1) { // red
					red_lines.emplace_back(st_coord.x());
					red_lines.emplace_back(st_coord.y());
					red_lines.emplace_back(st_coord.z());

					red_lines.emplace_back(ed_coord.x());
					red_lines.emplace_back(ed_coord.y());
					red_lines.emplace_back(ed_coord.z());
				}
				else { //yellow
					yellow_lines.emplace_back(st_coord.x());
					yellow_lines.emplace_back(st_coord.y());
					yellow_lines.emplace_back(st_coord.z());

					yellow_lines.emplace_back(ed_coord.x());
					yellow_lines.emplace_back(ed_coord.y());
					yellow_lines.emplace_back(ed_coord.z());
				}
			}

			auto set_vao = [&](unsigned int& VAO, unsigned int& VBO, std::vector<float>& lines) {
				glGenVertexArrays(1, &VAO);
				glGenBuffers(1, &VBO);
				//glGenBuffers(1, &EBO);

				glBindVertexArray(VAO);
				glBindBuffer(GL_ARRAY_BUFFER, VBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(float) * lines.size(), lines.data(), GL_STATIC_DRAW);

				//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
				//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * objInfo.indices.size(), objInfo.indices.data(), GL_STATIC_DRAW);

				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
				//glEnableVertexAttribArray(1);
				//glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

				glBindVertexArray(0);
			};


			DeleteBuffers();

			set_vao(yellowVAO, yellowVBO, yellow_lines);
			set_vao(redVAO, redVBO, red_lines);
			set_vao(greenVAO, greenVBO, green_lines);
		}

		void RenderGui() {
			ImGui::Begin("OBJ Edges Info v2");

			if (ImGui::TreeNode("Inputs Control")) {

				ImGui::InputFloat("scale_factor", &inputs.scale_factor, 0.1f, 1.0f, "%.3f");

				if (ImGui::Button("Change Inputs")) {
					OnApplyNewInputs();
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Red Edges")) {
				int id = 0;
				for (int i = 0; i < red_lines.size(); i += 6) {
					ImGui::PushID(id++);
					if (ImGui::TreeNode("", "Red Edge: %d", i / 6)) {
						if (ImGui::Button("Go")) {

							glm::vec3 p1{
								red_lines[i + 0],
								red_lines[i + 1],
								red_lines[i + 2]
							};

							glm::vec3 p2{
								red_lines[i + 3],
								red_lines[i + 4],
								red_lines[i + 5]
							};

							OnGoClick(p1, p2);
						}

						ImGui::TreePop();
					}
					ImGui::PopID();
				}

				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Yellow Edges")) {
				int id = 0;
				for (int i = 0; i < yellow_lines.size(); i += 6) {
					ImGui::PushID(id++);
					if (ImGui::TreeNode("", "Yellow Edge: %d", i / 6)) {
						if (ImGui::Button("Go")) {

							glm::vec3 p1{
								yellow_lines[i + 0],
								yellow_lines[i + 1],
								yellow_lines[i + 2]
							};

							glm::vec3 p2{
								yellow_lines[i + 3],
								yellow_lines[i + 4],
								yellow_lines[i + 5]
							};

							OnGoClick(p1, p2);
						}

						ImGui::TreePop();
					}
					ImGui::PopID();
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Green Edges")) {
				int id = 0;
				for (int i = 0; i < green_lines.size(); i += 6) {
					ImGui::PushID(id++);
					if (ImGui::TreeNode("", "Green Edge: %d", i / 6)) {
						if (ImGui::Button("Go")) {

							glm::vec3 p1{
								green_lines[i + 0],
								green_lines[i + 1],
								green_lines[i + 2]
							};

							glm::vec3 p2{
								green_lines[i + 3],
								green_lines[i + 4],
								green_lines[i + 5]
							};

							OnGoClick(p1, p2);
						}

						ImGui::TreePop();
					}
					ImGui::PopID();
				}
				ImGui::TreePop();
			}

			ImGui::End();
		}

		void Render(
			const RenderInfo& renderInfo
		) override {
			shader->use();

			shader->setMatrix4("projection", renderInfo.projection_matrix);
			shader->setMatrix4("view", renderInfo.view_matrix);
			shader->setMatrix4("model", modelMatrix);

			shader->setVec3("subcolor", glm::vec3(1.0f, 1.0f, 0.0f));
			glBindVertexArray(yellowVAO);
			glDrawArrays(GL_LINES, 0, yellow_lines.size() / 3);
			glBindVertexArray(0);

			shader->setVec3("subcolor", glm::vec3(1.0f, 0.0f, 0.0f));
			glBindVertexArray(redVAO);
			glDrawArrays(GL_LINES, 0, red_lines.size() / 3);
			glBindVertexArray(0);

			shader->setVec3("subcolor", glm::vec3(0.0f, 1.0f, 0.0f));
			glBindVertexArray(greenVAO);
			glDrawArrays(GL_LINES, 0, green_lines.size() / 3);
			glBindVertexArray(0);

			RenderGui();
		}

		ObjNonManifoldLineWithGuiRenderer(ObjMarkNum& objMarkNum, ObjNonManifoldLineWithGuiRendererInputs inputs, Shader* shader, MyRenderEngine& myRenderEngine) : objMarkNum(objMarkNum), inputs(inputs), shader(shader), myRenderEngine(myRenderEngine) {}

		~ObjNonManifoldLineWithGuiRenderer() {
			DeleteBuffers();
		};

	private:
		MyRenderEngine& myRenderEngine;
		ObjMarkNum& objMarkNum;
	};
} // namespace MyRenderEngine
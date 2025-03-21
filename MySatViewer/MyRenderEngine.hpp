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

	// Configs
	const glm::vec4 GREEN_BACKGROUND{ 0.2f, 0.3f, 0.3f, 1.0f };
	const glm::vec4 BLACK_BACKGROUND{ 0.0f, 0.0f, 0.0f, 0.0f };

	const glm::vec3 CAMERA_INIT_POS{ 0.0f, 0.0f, 5.0f };

	const unsigned int SCR_WIDTH = 1024;
	const unsigned int SCR_HEIGHT = 768;
	const unsigned int SCR_X_POS = 200;
	const unsigned int SCR_Y_POS = 200;

	const glm::vec4 ZERO_VEC(0.0f);
	const glm::vec4 ONE_VEC(1.0f);

	// utility function for loading a 2D texture from file
	// 绑定纹理对象与实际数据，并返回纹理对象ID以供随后激活纹理单元并将纹理对象与纹理单元绑定
	// ---------------------------------------------------
	unsigned int loadTexture(char const* path) {
		unsigned int textureID;
		glGenTextures(1, &textureID);

		int width, height, nrComponents;
		unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);

		if (data) {
			GLenum format;
			if (nrComponents == 1) {
				format = GL_RED;
			}
			else if (nrComponents == 3) {
				format = GL_RGB;
			}
			else if (nrComponents == 4) {
				format = GL_RGBA;
			}

			// 绑定：绑定纹理对象与实际数据
			glBindTexture(GL_TEXTURE_2D, textureID);
			glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			glGenerateMipmap(GL_TEXTURE_2D);

			// 纹理环绕方式（与绑定之间的顺序随意）
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			stbi_image_free(data);

		}
		else {
			std::cout << "Texture failed to load at path:" << path << std::endl;
			stbi_image_free(data);
		}

		return textureID;
	}

	// loads a cubemap texture from 6 individual texture faces
	// order:
	// +X (right)
	// -X (left)
	// +Y (top)
	// -Y (bottom)
	// +Z (front) 
	// -Z (back)
	// -------------------------------------------------------
	unsigned int loadCubemap(vector<std::string> faces)
	{
		unsigned int textureID;
		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

		int width, height, nrChannels;
		for (unsigned int i = 0; i < faces.size(); i++)
		{
			unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
			if (data)
			{
				glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
				stbi_image_free(data);
			}
			else
			{
				std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
				stbi_image_free(data);
			}
		}
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		return textureID;
	}

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
		float scale_factor;

		bool showModel;
		bool transparentModel; // 可透明物体需要通过这个变量判断调用哪个着色器，当然渲染到的Target还是需要提前手动指定的

		RenderInfo() :
			showModel(false),
			transparentModel(false)
		{}
	};

	// 这里就不用RenderableInfo这个设计了，因为之前里面只是用来传递isOpaque的，而这个量只在调用myRenderEngine.AddRenderable的时候有用。现在需要改成能够动态调整的，所以就不用了

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
				try {
					int nonmanifold_count = element.at("nonmanifold_count");
					int body_id = element.at("body");
					int marknum = element.at("marknum");
					int st_marknum = element.at("st_marknum");
					int ed_marknum = element.at("ed_marknum");

					if (selected_body != -1 && selected_body != body_id)
					{
						continue;
					}

					if (nonmanifold_count != 2)
					{
						printf("[LoadGeometryJson] nonmanifold: edge: %d (in json: %d), body: %d, count: %d, st: %d, ed: %d \n", marknum, ii, body_id, nonmanifold_count, st_marknum, ed_marknum);
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

					auto sampled_points = element.at("sampled_points");
					std::vector<float> vertices;
					int point_count = 0;

					for (auto&& point : sampled_points) {
						float x = point.at("x");
						float y = point.at("y");
						float z = point.at("z");

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
				catch (json::out_of_range& e) {

					printf("[LoadGeometryJson] Edge (in json: %d) load failed: maybe no geometry exist: %s", ii++, e.what());
				}
				
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

		Shader* shader;
		Shader* transparentShader;

		glm::mat4 modelMatrix{ 1.0f };

		void Render(
			const RenderInfo& renderInfo
		) override {
			if (renderInfo.showModel) {
				Shader* s;

				if (renderInfo.transparentModel == false) {
					s = shader;
				}
				else {
					s = transparentShader;
				}

				s->use();

				s->setMatrix4("projection", renderInfo.projection_matrix);
				s->setMatrix4("view", renderInfo.view_matrix);
				s->setMatrix4("model", modelMatrix);
				s->setVec3("viewPos", renderInfo.camera_pos);

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

		SatStlRenderer(Shader* shader, Shader* transparentShader):
			shader(shader),
			transparentShader(transparentShader),
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
		Shader* shader;

		glm::mat4 modelMatrix{ 1.0f };

		void Render(
			const RenderInfo& renderInfo
		) override {
			shader->use();

			shader->setMatrix4("projection", renderInfo.projection_matrix);
			shader->setMatrix4("view", renderInfo.view_matrix);
			shader->setMatrix4("model", modelMatrix);
			//shader.setVec3("viewPos", camera_pos);

			for (int h = 0; h < VAOs.size(); h++) {
				int VAO = VAOs[h];
				int VAO_size = edgeSampledPointsCounts[h];
				glm::vec3 color = edgeColors[h];

				shader->setVec3("subcolor", color);
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

		SatLineRenderer(Shader* shader) :
			shader(shader)
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
		unsigned int screenXPos;
		unsigned int screenYPos;

		glm::vec4 backgroundColor;
		bool showModel;
		float scaleFactor;
		bool transparentModel;

		GLFWwindow* window;
		Camera camera;
		MouseController mouseController;
		KeyboardController keyboardController;

		// 渲染对象列表
		// opaqueRenderables：固定渲染在不透明framebuffer上的， opaqueOrTransparentRenderables： 可能渲染在不透明或透明framebuffer上的（通过transparentModel切换）
		std::vector<std::shared_ptr<IRenderable>> opaqueRenderables, opaqueOrTransparentRenderables, guiRenderables;

		std::shared_ptr<IRenderable> screenQuad; // [后设置]

		// Framebuffer
		// 生命周期已经闭环
		unsigned int opaqueFBO;
		unsigned int transparentFBO;

		unsigned int opaqueTexture;
		unsigned int depthTexture;

		unsigned int accumTexture;
		unsigned int revealTexture;

		// OIT Use Shaders
		Shader* compositeShader;
		Shader* screenShader;

		// glfw: whenever the window size changed (by OS or user resize) this callback function executes
		void FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
			// make sure the viewport matches the new window dimensions; note that width and 
			// height will be significantly larger than specified on retina displays.
			glViewport(0, 0, width, height);
		}
		
		void AddOpaqueRenderable(const std::shared_ptr<IRenderable>& r) {
			opaqueRenderables.emplace_back(r);
		}

		void AddOpaqueOrTransparentRenderable(const std::shared_ptr<IRenderable>& r) {
			opaqueOrTransparentRenderables.emplace_back(r);
		}

		void AddScreenQuadRenderable(const std::shared_ptr<IRenderable>& r) {
			screenQuad = r;
		}

		void AddGuiRenderable(const std::shared_ptr<IRenderable>& r) {
			guiRenderables.emplace_back(r);
		}

		void SetCameraPos(const glm::vec3& pos) {
			camera.Position = pos;
		}

		void SetCompositeShader(Shader* shader) {
			compositeShader = shader;
		}

		void SetScreenShader(Shader* shader) {
			screenShader = shader;
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

				for (auto& r : guiRenderables) {
					r->Render(renderInfo);
				}


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

				glm::mat4 projectionMatrix = glm::perspective(glm::radians(camera.Zoom), static_cast<float>(screenWidth) / static_cast<float>(screenHeight), camera.Near, camera.Far);
				glm::mat4 viewMatrix = camera.GetViewMatrix();

				renderInfo.projection_matrix = projectionMatrix;
				renderInfo.view_matrix = viewMatrix;
				renderInfo.camera_pos = camera.Position;

				renderInfo.showModel = showModel;
				renderInfo.transparentModel = transparentModel;
				renderInfo.scale_factor = scaleFactor;

				// render IRenderable to opaqueFBO & transparentFBO
				// -> Opaque (solid pass)
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				glDepthMask(GL_TRUE);
				glDisable(GL_BLEND);
				glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);

				glBindFramebuffer(GL_FRAMEBUFFER, opaqueFBO);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

				for (auto& r : opaqueRenderables) {
					r->Render(renderInfo);
				}

				if (transparentModel == false) {
					for (auto& r : opaqueOrTransparentRenderables) {
						r->Render(renderInfo);
					}
				}

				// -> Transparent (transparent pass)
				glDepthMask(GL_FALSE);
				glEnable(GL_BLEND);
				glBlendFunci(0, GL_ONE, GL_ONE);
				glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
				glBlendEquation(GL_FUNC_ADD);

				glBindFramebuffer(GL_FRAMEBUFFER, transparentFBO);
				glClearBufferfv(GL_COLOR, 0, &ZERO_VEC[0]); // 新函数
				glClearBufferfv(GL_COLOR, 1, &ONE_VEC[0]);

				if (transparentModel == true) {
					for (auto& r : opaqueOrTransparentRenderables) {
						r->Render(renderInfo);
					}
				}

				// render composite image (composite pass)
				glDepthFunc(GL_ALWAYS);
				glEnable(GL_BLEND);
				glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

				glBindFramebuffer(GL_FRAMEBUFFER, opaqueFBO);

				compositeShader->use();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, accumTexture);
				glActiveTexture(GL_TEXTURE1);
				glBindTexture(GL_TEXTURE_2D, revealTexture);

				screenQuad->Render(renderInfo);

				// render screenQuad (draw to backbuffer) (final pass)
				glDisable(GL_DEPTH_TEST);
				glDepthMask(GL_TRUE); // enable depth writes so glClear won't ignore clearing the depth buffer
				glDisable(GL_BLEND);

				glBindFramebuffer(GL_FRAMEBUFFER, 0);
				glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

				screenShader->use();
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, opaqueTexture);

				screenQuad->Render(renderInfo);

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

		// 这个没用上
		void SetupGlobalOpenglState() {
			glEnable(GL_DEPTH_TEST);
			//glEnable(GL_BLEND);
			//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glDepthFunc(GL_LESS); // 深度缓冲比较通过条件：小于 （默认就是这个吧）
			glDepthMask(GL_TRUE); // 允许更新深度缓冲
			glDisable(GL_BLEND);
			glClearColor(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
		}

		void SetupFrameBuffers() {
			glGenFramebuffers(1, &opaqueFBO);
			glGenFramebuffers(1, &transparentFBO);

			// opaqueTexture
			glGenTextures(1, &opaqueTexture);
			glBindTexture(GL_TEXTURE_2D, opaqueTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_HALF_FLOAT, NULL); // 注意这里用的是 gl_half_float
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);

			// depthTexture
			glGenTextures(1, &depthTexture);
			glBindTexture(GL_TEXTURE_2D, depthTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT,
				0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
			glBindTexture(GL_TEXTURE_2D, 0);

			// Bind opaqueTexture & depthTexture to opaqueFBO
			glBindFramebuffer(GL_FRAMEBUFFER, opaqueFBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, opaqueTexture, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "ERROR::FRAMEBUFFER:: Opaque framebuffer is not complete!" << std::endl;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			// set up attachments for transparent framebuffer
			// accumTexture
			glGenTextures(1, &accumTexture);
			glBindTexture(GL_TEXTURE_2D, accumTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_HALF_FLOAT, NULL);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);

			// revealTexture
			glGenTextures(1, &revealTexture);
			glBindTexture(GL_TEXTURE_2D, revealTexture);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, SCR_WIDTH, SCR_HEIGHT, 0, GL_RED, GL_FLOAT, NULL); // 注意这里因为只有一个通道所以会有所不同！
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);

			// Bind accumTexture & revealTexture & depthTexture to transparent
			glBindFramebuffer(GL_FRAMEBUFFER, transparentFBO);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumTexture, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, revealTexture, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTexture, 0); // opaque framebuffer's depth texture

			// don't forget to explicitly tell OpenGL that your transparent framebuffer has two draw buffers
			const GLenum transparentDrawBuffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
			glDrawBuffers(2, transparentDrawBuffers);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				std::cout << "ERROR::FRAMEBUFFER:: Transparent framebuffer is not complete!" << std::endl;

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}

		MyRenderEngine(): 
			camera(CAMERA_INIT_POS),
			mouseController(camera),
			keyboardController(camera, showModel),
			deltaTime(0.0f),
			lastFrame(0.0f),

			screenWidth(SCR_WIDTH),
			screenHeight(SCR_HEIGHT),
			screenXPos(SCR_X_POS),
			screenYPos(SCR_Y_POS),
			backgroundColor(BLACK_BACKGROUND),
			scaleFactor(1.0f),
			transparentModel(false),
			showModel(false)
		{
			int init_res = InitWindow(window);
			if (init_res != 0) {
				throw std::runtime_error("Init window failed");
			}
			glfwMakeContextCurrent(window);
			glfwSwapInterval(1); // Enable vsync

			SetupImGui();
			//SetupGlobalOpenglState();
			SetupFrameBuffers();

		}

		~MyRenderEngine() {
			glDeleteTextures(1, &opaqueTexture);
			glDeleteTextures(1, &depthTexture);
			glDeleteTextures(1, &accumTexture);
			glDeleteTextures(1, &revealTexture);
			glDeleteFramebuffers(1, &opaqueFBO);
			glDeleteFramebuffers(1, &transparentFBO);

			std::cout << "MyrenderEngine Destructor executed." << std::endl;
		}

	private:
		float deltaTime;
		float lastFrame;

		int InitWindow(GLFWwindow* &window) {
			glfwInit();
			glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
			glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
			//glfwWindowHint(GLFW_SAMPLES, 4); // 注意因为这里用了别的framebuffer所以这里没有意义
			glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

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

			// 模型放大倍率
			ImGui::InputFloat("Scale Factor", &myRenderEngine.scaleFactor, 0.1f, 1.0f, "%.3f");

			// 显示模型
			ImGui::Checkbox("Show Model", &myRenderEngine.showModel);

			// 模型透明
			ImGui::Checkbox("Transparent Model", &myRenderEngine.transparentModel);

			ImGui::End();
		}

		BasicGuiRenderer(MyRenderEngine& myRenderEngine): myRenderEngine(myRenderEngine) {

		}

	private:
		MyRenderEngine& myRenderEngine;
	};

	class ObjRenderer : public IRenderable {
	public:
		ObjInfo& objInfo;

		unsigned int VAO;
		unsigned int VBO;

		std::vector<float> newVerticesWithNormal;
		int verticesCount;

		Shader* shader;
		Shader* transparentShader;

		glm::mat4 modelMatrix{ 1.0f };

		void Setup() {
			newVerticesWithNormal.clear();
			verticesCount = 0;
			VAO = VBO = 0;

			for (int i = 0; i < objInfo.indices.size(); i+=3) {
				int j = i + 1;
				int k = i + 2;

				int index_i = objInfo.indices[i];
				int index_j = objInfo.indices[j];
				int index_k = objInfo.indices[k];

				auto p1 = objInfo.GetPoint(index_i);
				auto p2 = objInfo.GetPoint(index_j);
				auto p3 = objInfo.GetPoint(index_k);

				auto v12 = p2 - p1;
				auto v13 = p3 - p1;
				auto normal = v12.Cross(v13);

				newVerticesWithNormal.emplace_back(p1.x());
				newVerticesWithNormal.emplace_back(p1.y());
				newVerticesWithNormal.emplace_back(p1.z());
				newVerticesWithNormal.emplace_back(normal.x());
				newVerticesWithNormal.emplace_back(normal.y());
				newVerticesWithNormal.emplace_back(normal.z());

				newVerticesWithNormal.emplace_back(p2.x());
				newVerticesWithNormal.emplace_back(p2.y());
				newVerticesWithNormal.emplace_back(p2.z());
				newVerticesWithNormal.emplace_back(normal.x());
				newVerticesWithNormal.emplace_back(normal.y());
				newVerticesWithNormal.emplace_back(normal.z());

				newVerticesWithNormal.emplace_back(p3.x());
				newVerticesWithNormal.emplace_back(p3.y());
				newVerticesWithNormal.emplace_back(p3.z());
				newVerticesWithNormal.emplace_back(normal.x());
				newVerticesWithNormal.emplace_back(normal.y());
				newVerticesWithNormal.emplace_back(normal.z());

				verticesCount += 3;
			}

			glGenVertexArrays(1, &VAO);
			glGenBuffers(1, &VBO);

			glBindVertexArray(VAO);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(float) * newVerticesWithNormal.size(), newVerticesWithNormal.data(), GL_STATIC_DRAW);

			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3*sizeof(float)));

			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glBindVertexArray(0);
		}

		void Render(
			const RenderInfo& renderInfo
		) override {
			if (renderInfo.showModel) {
				Shader* s;

				if (renderInfo.transparentModel == false) {
					s = shader;
				}
				else {
					s = transparentShader;
				}

				s->use();

				s->setMatrix4("projection", renderInfo.projection_matrix);
				s->setMatrix4("view", renderInfo.view_matrix);
				s->setMatrix4("model", glm::scale(modelMatrix, glm::vec3( renderInfo.scale_factor)));
				s->setVec3("viewPos", renderInfo.camera_pos);

				glBindVertexArray(VAO);
				//glDrawElements(GL_TRIANGLES, static_cast<unsigned int>(objInfo.indices.size()), GL_UNSIGNED_INT, 0);
				glDrawArrays(GL_TRIANGLES, 0, verticesCount);
				glBindVertexArray(0);

			}
		}

		ObjRenderer(ObjInfo& objInfo, Shader* shader, Shader* transparentShader): objInfo(objInfo), shader(shader), transparentShader(transparentShader), VAO(0), VBO(0), verticesCount(0) {}
		~ObjRenderer() {}
	};

	class ObjNonManifoldLineWithGuiRenderer : public IRenderable {
	public:

		std::vector<float> yellow_lines;
		std::vector<float> green_lines;
		std::vector<float> red_lines;

		struct YellowInfo{
			int halfEdgesCount;
			int edgeMarkNum;

			int stMarkNum;
			int edMarkNum;
		};

		std::vector<YellowInfo> yellowInfos;
		std::vector<YellowInfo> greenInfos;
		std::vector<YellowInfo> redInfos;

		unsigned int yellowVAO;
		unsigned int yellowVBO;

		unsigned int greenVAO;
		unsigned int greenVBO;

		unsigned int redVAO;
		unsigned int redVBO;

		Shader* shader;

		glm::mat4 modelMatrix{ 1.0f };

		void OnGoClick(glm::vec3 p1, glm::vec3 p2) {
			p1 = glm::vec4(p1, 1.0f) * modelMatrix * myRenderEngine.scaleFactor;
			p2 = glm::vec4(p2, 1.0f) * modelMatrix * myRenderEngine.scaleFactor;

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

			yellow_lines.clear();
			green_lines.clear();
			red_lines.clear();

			yellowInfos.clear();
			greenInfos.clear();
			redInfos.clear();

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

					greenInfos.push_back({ static_cast<int>(e->halfEdges.size()), objMarkNum.GetId(e), objMarkNum.GetId(e->st), objMarkNum.GetId(e->ed)});
				}
				else if (e->halfEdges.size() == 1) { // red
					red_lines.emplace_back(st_coord.x());
					red_lines.emplace_back(st_coord.y());
					red_lines.emplace_back(st_coord.z());

					red_lines.emplace_back(ed_coord.x());
					red_lines.emplace_back(ed_coord.y());
					red_lines.emplace_back(ed_coord.z());

					redInfos.push_back({ static_cast<int>(e->halfEdges.size()), objMarkNum.GetId(e), objMarkNum.GetId(e->st), objMarkNum.GetId(e->ed) });
				}
				else { //yellow
					yellow_lines.emplace_back(st_coord.x());
					yellow_lines.emplace_back(st_coord.y());
					yellow_lines.emplace_back(st_coord.z());

					yellow_lines.emplace_back(ed_coord.x());
					yellow_lines.emplace_back(ed_coord.y());
					yellow_lines.emplace_back(ed_coord.z());

					yellowInfos.push_back({ static_cast<int>(e->halfEdges.size()), objMarkNum.GetId(e), objMarkNum.GetId(e->st), objMarkNum.GetId(e->ed) });
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
			ImGui::Begin("OBJ Edges Info v3");

			if (ImGui::TreeNode("Red Edges")) {
				int id = 0;
				for (int i = 0; i < red_lines.size(); i += 6) {
					ImGui::PushID(id);
					if (ImGui::TreeNode("", "Red Edge: %d", i / 6)) {
						ImGui::Text("half edges count: %d", redInfos[id].halfEdgesCount);
						ImGui::Text("edge MarkNum: %d", redInfos[id].edgeMarkNum);
						ImGui::Text("st MarkNum: %d", redInfos[id].stMarkNum);
						ImGui::Text("ed MarkNum: %d", redInfos[id].edMarkNum);

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
					id++;
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Yellow Edges")) {
				int id = 0;
				for (int i = 0; i < yellow_lines.size(); i += 6) {
					ImGui::PushID(id);
					if (ImGui::TreeNode("", "Yellow Edge: %d", i / 6)) {

						ImGui::Text("half edges count: %d", yellowInfos[id].halfEdgesCount);
						ImGui::Text("edge MarkNum: %d", yellowInfos[id].edgeMarkNum);
						ImGui::Text("st MarkNum: %d", yellowInfos[id].stMarkNum);
						ImGui::Text("ed MarkNum: %d", yellowInfos[id].edMarkNum);

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
					id++;
				}
				ImGui::TreePop();
			}

			if (ImGui::TreeNode("Green Edges")) {
				int id = 0;
				for (int i = 0; i < green_lines.size(); i += 6) {
					ImGui::PushID(id);
					if (ImGui::TreeNode("", "Green Edge: %d", i / 6)) {

						ImGui::Text("half edges count: %d", greenInfos[id].halfEdgesCount);
						ImGui::Text("edge MarkNum: %d", greenInfos[id].edgeMarkNum);
						ImGui::Text("st MarkNum: %d", greenInfos[id].stMarkNum);
						ImGui::Text("ed MarkNum: %d", greenInfos[id].edMarkNum);

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
					id++;
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

			shader->setMatrix4("model", glm::scale(modelMatrix, glm::vec3(renderInfo.scale_factor)));


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

		ObjNonManifoldLineWithGuiRenderer(ObjMarkNum& objMarkNum, Shader* shader, MyRenderEngine& myRenderEngine) : objMarkNum(objMarkNum), shader(shader), myRenderEngine(myRenderEngine) {}

		~ObjNonManifoldLineWithGuiRenderer() {
			DeleteBuffers();
		};

	private:
		MyRenderEngine& myRenderEngine;
		ObjMarkNum& objMarkNum;
	};

	class ScreenQuad : public IRenderable {
	public:
		unsigned int quadVAO;
		unsigned int quadVBO;

		int verticesCount;

		//glm::mat4 modelMatrix; // 这版实现没有GetModelMatrix函数，所以先不用指定这个

		void Render(
			const RenderInfo& renderInfo
		) override {
			// without shader here

			glBindVertexArray(quadVAO);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, verticesCount);
			glBindVertexArray(0);
		}

		ScreenQuad() {
			static float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
				 1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
				 1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
			};

			verticesCount = 4;

			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);

			glBindVertexArray(quadVAO);
				glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
				glEnableVertexAttribArray(1);
				glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
			glBindVertexArray(0);
		}

		~ScreenQuad() {}
	};

} // namespace MyRenderEngine
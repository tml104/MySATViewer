#pragma once

#include "Topology.hpp"
#include "ObjInfo.hpp"

using namespace Topology;

/*
	全局计数类
*/


class ObjMarkNum {
public:
	std::map<TopoType, int> capacities;// 对应类型最大容量计数
	std::map<Entity*, std::pair<TopoType, int>> markNumMap; // 用于由指针访问类型与对应计数
	std::map<std::pair<TopoType, int>, Entity*> markNumInverseMap; // 上面这个map的反map

	std::vector<std::shared_ptr<Solid>> solids; // 用于访问所有实体

	// 有特殊用处需要维护的数据结构
	std::map<TopoType, std::list<int>> deletedIdListsMap; // 删除元素时需要使用的map
	std::map<std::pair<int, int>, std::shared_ptr<Edge>> edgesMap; // (vertex id, vertex id（有序对）) -> edge，注意：索引是有序对！

	ObjMarkNum(const ObjMarkNum&) = delete;
	ObjMarkNum& operator=(const ObjMarkNum&) = delete;

	// 单例
	static ObjMarkNum& GetInstance() {
		static ObjMarkNum marknum_instance;
		return marknum_instance;
	}

	// 由ObjInfo加载半边数据结构
	void LoadFromObjInfo(Info::ObjInfo& obj_info) {

		Clear();

		// 顶点
		std::vector<std::shared_ptr<Topology::Vertex>> vertex_ptrs; // vertices: 顶点列表（顶点导出不能依赖于这个列表）

		for (int i = 0, j = 0; i < obj_info.vertices.size(); i += 3, j += 1) {
			auto vertex_ptr = std::make_shared<Topology::Vertex>();
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


	std::shared_ptr<Edge> FindEdgeBetweenVertices(const std::shared_ptr<Topology::Vertex>& v1, const std::shared_ptr<Topology::Vertex>& v2) {

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
		markNumInverseMap.clear();
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
	static TopoType GetTypeFromTemplate(const std::shared_ptr<T>& p) {
		TopoType topotype_name = TopoType::NoExist;

		if constexpr (std::is_same_v<T, Topology::Vertex>) {
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


	Entity* GetEntityPtr(const std::pair<TopoType, int>& p) {
		
		if (auto it = markNumInverseMap.find(p); it != markNumInverseMap.end()) {
			return it->second;
		}

		return nullptr;
	}

private:

	ObjMarkNum() {}

	// 由指针类型更新UpdateMarkNumMap
	// 更新逻辑
	template<typename T>
	void UpdateMarkNumMap(const std::shared_ptr<T>& ptr) {
		TopoType topotype_name = GetTypeFromTemplate(ptr);

		auto marknum_pair = std::make_pair(topotype_name, (capacities[topotype_name])++);

		markNumMap[ptr.get()] = marknum_pair;
		markNumInverseMap[marknum_pair] = ptr.get();
	}

};
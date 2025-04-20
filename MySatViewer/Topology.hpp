#pragma once

#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>


/*
	Obj模型加载后会用到的拓扑结构
	（Sat模型暂时没必要用这个）
*/

namespace Topology {
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

	//struct Parpos {
	//	T_NUM _u, _v;

	//	Parpos() {
	//		_u = _v = 0.0;
	//	}

	//	Parpos(const T_NUM& uu, const T_NUM& vv) {
	//		_u = uu;
	//		_v = vv;
	//	}

	//	T_NUM& u() {
	//		return _u;
	//	}

	//	T_NUM& v() {
	//		return _v;
	//	}
	//};

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
}
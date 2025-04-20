#pragma once

#include "Topology.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/*
	主要用于保存GUI中处理拓扑显示的逻辑
	注意：由于渲染会不可避免地涉及到要使用数组（SOA）来表示有关渲染信息，因此所有品类开头第一项一定都是Index。对于特定类型的Info，可以用这个索引来反向定位数组（SOA）中的信息
*/

namespace Info {

	// Edge Geometry
	struct EdgeGeometry {
		std::string curveType;
		std::vector<glm::vec3> sampledPoints;
	};

	struct IntcurveEdgeGeometry : public EdgeGeometry {
		std::vector<glm::vec3> ctrlpts;
	};

	struct EllipseEdgeGeometry : public EdgeGeometry {
		glm::vec3 centre;
		glm::vec3 normal;
		glm::vec3 majorAxis;

		double majorAxisLength;
		double minorAxisLength;
	};

	struct StraightEdgeGeometry : public EdgeGeometry {
		glm::vec3 rootPoint;
		glm::vec3 direction;
	};

	struct HelixEdgeGeometry : public EdgeGeometry {
		// TODO
	};

	struct UndefcEdgeGeometry : public EdgeGeometry {
		// TODO
	};

	// HalfEdge Geometry
	struct HalfEdgeGeometry {
		std::string curveType;
	};

	struct PcurveHalfEdgeGeometry: public HalfEdgeGeometry {
		int curveDegree;
		std::vector<glm::vec2> ctrlpts;
		std::vector<double> knots;
	};


	// Face Geometry
	struct FaceGeometry {
		std::string faceType;
	};

	struct ConeFaceGeometry: public FaceGeometry {
		glm::vec3 rootPoint;
		glm::vec3 majorAxis;
		double ratio;
		glm::vec3 direction;

		double cos;
		double sin;
		double angle;
	};

	struct MeshsurfFaceGeometry : public FaceGeometry {
		// TODO
	};

	struct PlaneFaceGeometry : public FaceGeometry {
		glm::vec3 rootPoint;
		glm::vec3 normal;
	};

	struct SphereFaceGeometry : public FaceGeometry {
		glm::vec3 centre;
		double radius;
	};

	struct SplineFaceGeometry : public FaceGeometry {
		int degreeU;
		int degreeV;
		int numU;
		int numV;
		int numKnotsU;
		int numKnotsV;
		int numWeightU;
		int numWeightV;

		std::vector<glm::vec3> ctrlpts;
		std::vector<double> uknots;
		std::vector<double> vknots;
		std::vector<double> weights;
	};

	struct TorusFaceGeometry : public FaceGeometry {
		glm::vec3 centre;
		glm::vec3 normal;
		double minorRadius;
		double majorRadius;
	};


	struct TopologyInfo {
		int index; // 加载后的对应下标，这个下标会用于在gui中显示索引（这个下标是和读取有关的，和模型本身没有关系）

		int markNum;
		int bodyId;

		glm::vec3 pos; // 用于go按钮
	};


	struct EdgeInfo: public TopologyInfo {
		int nonmanifoldCount;

		int stMarkNum; // 起点的markNum
		int edMarkNum; // 终点的markNum
		std::vector<int> halfEdgesMarkNums; // 所引用的所有半边的markNum

		// Geometry info need to show in here
		std::unique_ptr<EdgeGeometry> geometryPtr;


		glm::vec3 GetColor() {
			if (nonmanifoldCount == 1) {
				return glm::vec3(1.0f, 0.0f, 0.0f); // Red
			}
			else if (nonmanifoldCount > 2) {
				return glm::vec3(0.0f, 0.0f, 1.0f); // Blue
			}
			else {
				return glm::vec3(0.0f, 1.0f, 0.0f); // Green
			}
		}
	};

	struct VertexInfo: public TopologyInfo {
		// 由于坐标等同于pos，因此此处不再增加数据
	};

	struct HalfEdgeInfo: public TopologyInfo {
		bool sense;
		int edgeMarkNum; // 对应的边的markNum
		int partnerMarkNum; // 对应的partner的markNum
		int preMarkNum; // 同一个环中的前一个半边
		int nextMarkNum; // 同一个环中的后一个半边
		int loopMarkNum; // 对应的环的markNum

		// Geometry info need to show in here
		std::unique_ptr<HalfEdgeGeometry> halfEdgeGeometry;
	};

	struct LoopInfo: public TopologyInfo {
		
		std::vector<int> halfEdgesMarkNums; // 所引用的所有半边的markNum

		int faceMarkNum; // 对应的面的markNum
	};

	struct FaceInfo : public TopologyInfo {
		int stLoopMarkNum; // 对应的环的markNum

		// Geometry info need to show in here

		std::unique_ptr<FaceGeometry> faceGeometry;
	};

}
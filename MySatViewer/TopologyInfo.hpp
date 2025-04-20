#pragma once

#include "Topology.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

/*
	��Ҫ���ڱ���GUI�д���������ʾ���߼�
	ע�⣺������Ⱦ�᲻�ɱ�����漰��Ҫʹ�����飨SOA������ʾ�й���Ⱦ��Ϣ���������Ʒ�࿪ͷ��һ��һ������Index�������ض����͵�Info���������������������λ���飨SOA���е���Ϣ
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
		int index; // ���غ�Ķ�Ӧ�±꣬����±��������gui����ʾ����������±��ǺͶ�ȡ�йصģ���ģ�ͱ���û�й�ϵ��

		int markNum;
		int bodyId;

		glm::vec3 pos; // ����go��ť
	};


	struct EdgeInfo: public TopologyInfo {
		int nonmanifoldCount;

		int stMarkNum; // ����markNum
		int edMarkNum; // �յ��markNum
		std::vector<int> halfEdgesMarkNums; // �����õ����а�ߵ�markNum

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
		// ���������ͬ��pos����˴˴�������������
	};

	struct HalfEdgeInfo: public TopologyInfo {
		bool sense;
		int edgeMarkNum; // ��Ӧ�ıߵ�markNum
		int partnerMarkNum; // ��Ӧ��partner��markNum
		int preMarkNum; // ͬһ�����е�ǰһ�����
		int nextMarkNum; // ͬһ�����еĺ�һ�����
		int loopMarkNum; // ��Ӧ�Ļ���markNum

		// Geometry info need to show in here
		std::unique_ptr<HalfEdgeGeometry> halfEdgeGeometry;
	};

	struct LoopInfo: public TopologyInfo {
		
		std::vector<int> halfEdgesMarkNums; // �����õ����а�ߵ�markNum

		int faceMarkNum; // ��Ӧ�����markNum
	};

	struct FaceInfo : public TopologyInfo {
		int stLoopMarkNum; // ��Ӧ�Ļ���markNum

		// Geometry info need to show in here

		std::unique_ptr<FaceGeometry> faceGeometry;
	};

}
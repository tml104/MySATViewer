#pragma once


#include <algorithm>
#include <cmath>
#include <memory>
#include <vector>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

#include <stdexcept>
#include <cassert>

#include "Topology.hpp"
#include "tiny_obj_loader.h"

#include <spdlog/spdlog.h>

/*
	Obj信息类
    用途：包含理论上所有的信息，支持从文件中加载到组织好的数据结构中，方便后续渲染等使用
    渲染用信息需要从这个类中再度提取
*/

namespace Info {

    struct ObjInfo {
        std::vector<tinyobj::real_t> vertices;
        std::vector<int> indices;

        std::vector<std::pair<int, int>> solidIndicesRange;

        void LoadFromObj(const std::string& obj_path) {
            SPDLOG_INFO("Loading OBJ from: {}", obj_path);

            tinyobj::ObjReader reader;

            if (!reader.ParseFromFile(obj_path)) {
                if (!reader.Error().empty()) {
                    SPDLOG_ERROR("TinyObjReader: {}", reader.Error());
                }
                throw std::runtime_error("TinyObjReader read failed.");
            }

            if (!reader.Warning().empty()) {
                SPDLOG_WARN("TinyObjReader: {}", reader.Warning());
            }

            auto& attrib = reader.GetAttrib();
            auto& shapes = reader.GetShapes();

            vertices = attrib.vertices; // 复制

            // 遍历每个solid
            int last_end = 0;
            for (size_t s = 0; s < shapes.size(); s++) {
                size_t index_offset = 0;

                // 遍历此solid中的每个face
                for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
                    size_t fv = size_t(shapes[s].mesh.num_face_vertices[f]);
                    assert(fv == 3);

                    // 遍历此face中的每个顶点
                    for (size_t v = 0; v < fv; v++) {
                        // 取得顶点索引类
                        tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];

                        // 取得顶点索引对应的顶点坐标分量
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

            SPDLOG_INFO("Loading OBJ done.");
        }

        Topology::Coordinate GetPoint(int index) {
            return Topology::Coordinate(vertices[3 * index + 0], vertices[3 * index + 1], vertices[3 * index + 2]);
        }
    };


}

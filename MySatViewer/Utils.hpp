#pragma once

#include <string>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


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
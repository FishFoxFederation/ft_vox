#pragma once

#include "define.hpp"

#include <vulkan/vulkan.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/hash.hpp>

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstring>
#include <regex>
#include <unordered_map>
#include <stdexcept>

struct ObjVertex
{
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 tex_coord;

	static VkVertexInputBindingDescription getBindingDescription()
	{
		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(ObjVertex);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

		return bindingDescription;
	}

	static std::vector<VkVertexInputAttributeDescription> getAttributeDescriptions()
	{
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions(3);

		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(ObjVertex, pos);

		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1;
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(ObjVertex, normal);

		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(ObjVertex, tex_coord);

		return attributeDescriptions;
	}

	bool operator==(const ObjVertex& other) const
	{
		return pos == other.pos && normal == other.normal && tex_coord == other.tex_coord;
	}
};

namespace std
{
	template<> struct hash<ObjVertex>
	{
		size_t operator()(const ObjVertex & vertex) const
		{
			return ((hash<glm::vec3>()(vertex.pos) ^
				(hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^
				(hash<glm::vec2>()(vertex.tex_coord) << 1);
		}
	};
}

struct Face
{
	uint32_t vertexIndex[3] = {0};
	uint32_t texCoordIndex[3] = {0};
	uint32_t normalIndex[3] = {0};

	void log() const
	{
		std::cout << "f";
		for (size_t i = 0; i < 3; i++)
		{
			std::cout << " " << vertexIndex[i] << "/" << texCoordIndex[i] << "/" << normalIndex[i];
		}
		std::cout << std::endl;
	}
};

class ObjLoader
{

public:

	ObjLoader(const std::string & m_path)
	{
		this->m_path = m_path;

		this->readFile();
		this->removeComments();
		this->parse();
		this->populateVerticesAndIndices();
	}

	const std::vector<ObjVertex> & vertices() const
	{
		return this->m_vertices;
	}

	const std::vector<uint32_t> & indices() const
	{
		return this->m_indices;
	}

private:

	std::string m_path;
	std::vector<std::string> m_lines;

	std::vector<glm::vec3> m_vertexPos;
	std::vector<glm::vec2> m_texCoords;
	std::vector<glm::vec3> m_normals;

	std::vector<Face> m_faces;

	bool m_hasTexCoords = false;
	bool m_hasNormals = false;

	std::vector<ObjVertex> m_vertices;
	std::vector<uint32_t> m_indices;


	void readFile()
	{
		std::ifstream file(this->m_path);
		if (!file.is_open())
		{
			throw std::runtime_error("Failed to open file '" + this->m_path + "'");
		}

		std::string line;
		while (std::getline(file, line))
		{
			this->m_lines.push_back(line);
		}
	}

	void removeComments()
	{
		for (size_t i = 0; i < this->m_lines.size(); i++)
		{
			std::string line = this->m_lines[i];
			size_t commentPos = line.find('#');
			if (commentPos != std::string::npos)
			{
				line = line.substr(0, commentPos);
			}
			this->m_lines[i] = line;
		}
	}

	void parse()
	{

		std::regex faceRegex_v(R"(^f(\s\d+){3,}$)");
		std::regex faceRegex_v_vt(R"(^f(\s\d+\/\d+){3,}$)");
		std::regex faceRegex_v_vn(R"(^f(\s\d+\/\/\d+){3,}$)");
		std::regex faceRegex_v_vt_vn(R"(^f(\s\d+\/\d+\/\d+){3,}$)");

		for (uint32_t i = 0; i < this->m_lines.size(); i++)
		{

			/* Skip empty m_lines or m_lines with only spaces */
			if (std::all_of(this->m_lines[i].begin(), this->m_lines[i].end(), isspace))
			{
				continue;
			}

			std::string line = this->m_lines[i];
			std::stringstream ss(line);
			ss.exceptions(std::ios_base::failbit | std::ios_base::badbit);

			try
			{

			if (memcmp(line.c_str(), "v ", 2) == 0)
			{
				glm::vec3 vertex;
				ss.ignore(2);
				ss >> vertex[0] >> vertex[1] >> vertex[2];
				this->m_vertexPos.push_back(vertex);
			}
			else if (memcmp(line.c_str(), "vt ", 3) == 0)
			{
				glm::vec2 texCoord;
				ss.ignore(3);
				ss >> texCoord[0] >> texCoord[1];
				this->m_texCoords.push_back(texCoord);
				this->m_hasTexCoords = true;
			}
			else if (memcmp(line.c_str(), "vn ", 3) == 0)
			{
				glm::vec3 normal;
				ss.ignore(3);
				ss >> normal[0] >> normal[1] >> normal[2];
				this->m_normals.push_back(normal);
				this->m_hasNormals = true;
			}
			else if (memcmp(line.c_str(), "f ", 2) == 0)
			{
				std::vector<Face> tmp_faces;

				/* Check which face regex matches */
				bool v = false, v_vt = false, v_vn = false, v_vt_vn = false;
				if (std::regex_match(line, faceRegex_v))
					v = true;
				else if (std::regex_match(line, faceRegex_v_vt) && m_hasTexCoords)
					v_vt = true;
				else if (std::regex_match(line, faceRegex_v_vn) && m_hasNormals)
					v_vn = true;
				else if (std::regex_match(line, faceRegex_v_vt_vn) && m_hasTexCoords && m_hasNormals)
					v_vt_vn = true;
				else
					throw std::string("Parsing syntax error: Invalid face format");

				ss.ignore(2);

				/* Read each vertex of the face */
				size_t j = 0;
				while (ss.rdstate() == std::ios_base::goodbit)
				{
					Face face;
					char c;
					if (v)
						ss >> face.vertexIndex[0];
					else if (v_vt)
						ss >> face.vertexIndex[0] >> c >> face.texCoordIndex[0];
					else if (v_vn)
						ss >> face.vertexIndex[0] >> c >> c >> face.normalIndex[0];
					else if (v_vt_vn)
						ss >> face.vertexIndex[0] >> c >> face.texCoordIndex[0] >> c >> face.normalIndex[0];

					if (this->checkIndices(face) == false)
					{
						throw std::string("Parsing value error");
					}

					tmp_faces.push_back(face);
					j++;
				}

				/* Construct a face for each triangle (e.i. 3 vertices) */
				for (j = 1; j + 1 < tmp_faces.size(); j++)
				{
					Face face;
					face.vertexIndex[0] = tmp_faces[0].vertexIndex[0];
					face.vertexIndex[1] = tmp_faces[j].vertexIndex[0];
					face.vertexIndex[2] = tmp_faces[j + 1].vertexIndex[0];
					if (v_vt || v_vt_vn)
					{
						face.texCoordIndex[0] = tmp_faces[0].texCoordIndex[0];
						face.texCoordIndex[1] = tmp_faces[j].texCoordIndex[0];
						face.texCoordIndex[2] = tmp_faces[j + 1].texCoordIndex[0];
					}
					if (v_vn || v_vt_vn)
					{
						face.normalIndex[0] = tmp_faces[0].normalIndex[0];
						face.normalIndex[1] = tmp_faces[j].normalIndex[0];
						face.normalIndex[2] = tmp_faces[j + 1].normalIndex[0];
					}
					this->m_faces.push_back(face);
				}
			}
			else if (
				memcmp(line.c_str(), "mtllib ", 7) != 0
				&& memcmp(line.c_str(), "usemtl ", 7) != 0
				&& memcmp(line.c_str(), "s ", 2) != 0
				&& memcmp(line.c_str(), "g ", 2) != 0
				&& memcmp(line.c_str(), "o ", 2) != 0
			)
			{
				throw std::string("Parsing syntax error: Unknown line type");
			}

			} catch (std::string& e)
			{
				throw std::runtime_error(this->m_path + ": line " + std::to_string(i + 1) + ": " + e);
			}
		}
	}

	bool checkIndices(const Face & face)
	{
		if (face.vertexIndex[0] > this->m_vertexPos.size() || face.vertexIndex[0] <= 0)
		{
			return false;
		}
		if (this->m_hasTexCoords && (face.texCoordIndex[0] > this->m_texCoords.size() || face.texCoordIndex[0] <= 0))
		{
			return false;
		}
		if (this->m_hasNormals && (face.normalIndex[0] > this->m_normals.size() || face.normalIndex[0] <= 0))
		{
			return false;
		}
		return true;
	}

	void populateVerticesAndIndices()
	{
		std::unordered_map<ObjVertex, uint32_t> uniqueVertices{};

		for (const auto& face : this->m_faces)
		{
			glm::vec2 arbitraryTexCoords[3] =
			{
				glm::vec2(0.0f, 0.0f),
				glm::vec2(1.0f, 0.0f),
				glm::vec2(0.0f, 1.0f)
			};

			for (size_t i = 0; i < 3; i++)
			{
				ObjVertex vertex{};

				vertex.pos = this->m_vertexPos[face.vertexIndex[i] - 1];

				if (this->m_hasTexCoords)
				{
					vertex.tex_coord = this->m_texCoords[face.texCoordIndex[i] - 1];
				}
				else
				{
					vertex.tex_coord = arbitraryTexCoords[i];
				}

				if (this->m_hasNormals)
				{
					vertex.normal = this->m_normals[face.normalIndex[i] - 1];
				}

				if (uniqueVertices.count(vertex) == 0)
				{
					uniqueVertices[vertex] = static_cast<uint32_t>(m_vertices.size());
					m_vertices.push_back(vertex);
				}

				m_indices.push_back(uniqueVertices[vertex]);
			}
		}
	}

};

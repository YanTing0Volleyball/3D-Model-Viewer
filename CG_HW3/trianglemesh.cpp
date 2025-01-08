#include "trianglemesh.h"

// Constructor of a triangle mesh.
TriangleMesh::TriangleMesh()
{
	numVertices = 0;
	numTriangles = 0;
	objCenter = glm::vec3(0.0f, 0.0f, 0.0f);
	objExtent = glm::vec3(0.0f, 0.0f, 0.0f);
	vboId = 0;
}

// Destructor of a triangle mesh.
TriangleMesh::~TriangleMesh()
{
	vertices.clear();
	materialMap.clear();
	vertexIdMap.clear();
	for (auto element : subMeshes) {
		glDeleteBuffers(1, &(element.iboId));
		element.vertexIndices.clear();
	}
	glDeleteBuffers(1, &vboId);
}

// Load the geometry and material data from an OBJ file.
bool TriangleMesh::LoadFromFile(const std::string& filePath, const bool normalized)
{
	//Find Object Name
	std::stringstream ss(filePath);
	std::string item;
	std::vector<std::string> elements;
	while (std::getline(ss, item, '/')) {
		elements.push_back(item);
	}
	std::string objectName = elements.back();

	//Some space to Store Loaded Data
	std::vector<glm::vec3> positions = {};
	std::vector<glm::vec2> textures = {};
	std::vector<glm::vec3> normals = {};

	//For Finding Bounded Box
	float maxX = FLT_MIN, maxY = FLT_MIN, maxZ = FLT_MIN;
	float minX = FLT_MAX, minY = FLT_MAX, minZ = FLT_MAX;

	//For Current Face Material Record
	PhongMaterial* phongMaterial = nullptr;

	//For Current SubMesh Record
	SubMesh* subMesh = nullptr;

	//Open File refer to filePath
	std::ifstream inputFile(filePath + '/' + objectName + ".obj");
	if (inputFile.is_open()) {
		std::cout << "Obj File Open Successful" << std::endl;
		std::string line;
		std::string head;
		//Read File Line by Line
		std::cout << "Obj File Loading..." << std::endl;
		while (getline(inputFile, line)) {
			if (line.empty())continue;

			std::stringstream ss(line);
			ss >> head;

			//Dealing read Information
			if (head == "v") {
				//Collect Vertex Position Info
				glm::vec3 position;
				ss >> position.x >> position.y >> position.z;

				positions.push_back(position);

				//Check Bouned Box Coordinates
				//Check X
				if (position.x > maxX) maxX = position.x;
				if (position.x < minX) minX = position.x;

				//check Y
				if (position.y > maxY) maxY = position.y;
				if (position.y < minY) minY = position.y;

				//check Z
				if (position.z > maxZ) maxZ = position.z;
				if (position.z < minZ) minZ = position.z;

			}
			else if (head == "vt") {
				//Collect Vertex Texture Coordinates Info
				glm::vec2 texture;
				ss >> texture.x >> texture.y;

				textures.push_back(texture);
			}
			else if (head == "vn") {
				//Collect Vertex Normal Info
				glm::vec3 normal;
				ss >> normal.x >> normal.y >> normal.z;

				normals.push_back(normal);
			}
			else if (head == "mtllib") {
				ss >> head;
				LoadMtlFile(filePath + "/" + head, filePath);
			}
			else if (head == "usemtl") {
				// Push SubMesh to SubMeshes
				if (subMesh != nullptr) {
					subMeshes.push_back(*subMesh);
				}

				// Find Material Refer to Material Map
				std::string mtlFlag;
				ss >> mtlFlag;
				phongMaterial = &materialMap[mtlFlag];
				subMesh = new SubMesh();
				subMesh->material = phongMaterial;
			}
			else if (head == "f") {
				//Dealing Face Data One by One (in one file line)
				int forPolygonCheck = 0;
				unsigned firstIndex, lastIndex;
				std::string fData;
				while (ss >> fData) {
					std::stringstream sfData(fData);
					int pIndex, tIndex, nIndex;
					char slash;
					int index;

					sfData >> pIndex >> slash >> tIndex >> slash >> nIndex;

					//Build Vertex Information
					VertexPTN vertex = VertexPTN(positions[pIndex - 1],
						normals[nIndex - 1],
						textures[tIndex - 1]);

					if (vertexIdMap[fData] != NULL) {
						index = vertexIdMap[fData];
					}
					else {
						index = numVertices;
						vertexIdMap[fData] = numVertices;
						//Add New Vertex to vertices
						numVertices++;
						vertices.push_back(vertex);
					}

					forPolygonCheck++;
					//Add index to vertexIndices
					switch (forPolygonCheck) {
					case 1:
						firstIndex = index;
						break;
					case 2:
						lastIndex = index;
						break;
					default:
						subMesh->vertexIndices.push_back(firstIndex);
						subMesh->vertexIndices.push_back(lastIndex);
						subMesh->vertexIndices.push_back(index);
						lastIndex = index;
						numTriangles++;
					}
				}
			}
			else continue;
		}

		// Push The Last SubMesh to SubMeshes
		if (subMesh != nullptr) {
			subMeshes.push_back(*subMesh);
		}

		inputFile.close();
		std::cout << "Obj File Loaging Finished" << std::endl;
	}
	else std::cout << "Obj File Open Failed" << std::endl;


	if (normalized) {
		// Normalize the geometry data.	

		std::cout << "Vertex Normalizing..." << std::endl;
		//Translate to Center
		glm::vec3 centerCoordinate = { (maxX + minX) / 2,
									   (maxY + minY) / 2,
									   (maxZ + minZ) / 2 };

		for (unsigned int i = 0; i < GetNumVertices(); i++) {
			vertices[i].position -= centerCoordinate;
		}

		//Normalize Longest Axis to One
		float xAxis = std::abs(maxX - minX);
		float yAxis = std::abs(maxY - minY);
		float zAxis = std::abs(maxZ - minZ);
		float longestAxis;
		if (xAxis >= yAxis && xAxis >= zAxis) longestAxis = xAxis;
		if (yAxis >= xAxis && yAxis >= zAxis) longestAxis = yAxis;
		if (zAxis >= yAxis && zAxis >= xAxis) longestAxis = zAxis;

		for (unsigned int i = 0; i < numVertices; i++)
			vertices[i].position /= longestAxis;


		objCenter = centerCoordinate;
		objExtent = glm::vec3(xAxis / longestAxis, yAxis / longestAxis, zAxis / longestAxis);
	}
	std::cout << "Vertex Normalizing Finished" << std::endl;

	return true;

	return true;
}

bool TriangleMesh::LoadMtlFile(const std::string& filePath, const std::string& folderPath) {
	//Open File refer to filePath
	std::ifstream inputFile(filePath);
	if (inputFile.is_open()) {
		std::cout << "Mtl File Open Successful" << std::endl;
		std::string line;
		std::string head;
		//Read File Line by Line
		std::cout << "Mtl File Loading..." << std::endl;
		while (getline(inputFile, line)) {
			std::stringstream ss(line);
			ss >> head;

			if (head == "newmtl") {
				std::string flag;
				std::string element;
				ss >> flag;

				// Initila PhongMaterial
				PhongMaterial newMaterial;
				newMaterial.SetName(flag);

				// Read Ns, Kd, Kd, Ks, map_Kd Line by Line
				for (int i = 0; i < 5; i++) {
					getline(inputFile, line);
					std::stringstream ss(line);
					ss >> head;

					if (head == "Ns") {
						float Ns;
						ss >> Ns;
						newMaterial.SetNs(Ns);
					}
					else if (head == "Ka") {
						float Ka1, Ka2, Ka3;
						ss >> Ka1 >> Ka2 >> Ka3;
						glm::vec3 Ka(Ka1, Ka2, Ka3);
						newMaterial.SetKa(Ka);
					}
					else if (head == "Kd") {
						float Kd1, Kd2, Kd3;
						ss >> Kd1 >> Kd2 >> Kd3;
						glm::vec3 Kd(Kd1, Kd2, Kd3);
						newMaterial.SetKd(Kd);
					}
					else if (head == "Ks") {
						float Ks1, Ks2, Ks3;
						ss >> Ks1 >> Ks2 >> Ks3;
						glm::vec3 Ks(Ks1, Ks2, Ks3);
						newMaterial.SetKs(Ks);
					}
					else if (head == "map_Kd") {
						std::string imageFile;
						ss >> imageFile;
						newMaterial.SetMapKd(new ImageTexture(folderPath + '/' + imageFile));
					}
				}

				// Add New Material To Material Map
				materialMap[flag] = newMaterial;
			}
			else continue;
		}

		std::cout << "Mtl File Loaging Finished" << std::endl;
		return true;
	}
	else {
		std::cout << "Mtl File Open Failed" << std::endl;
		return false;
	}
}

// Show model information.
void TriangleMesh::ShowInfo()
{
	std::cout << "# Vertices: " << numVertices << std::endl;
	std::cout << "# Triangles: " << numTriangles << std::endl;
	std::cout << "Total " << subMeshes.size() << " subMeshes loaded" << std::endl;
	for (unsigned int i = 0; i < subMeshes.size(); ++i) {
		const SubMesh& g = subMeshes[i];
		std::cout << "SubMesh " << i << " with material: " << g.material->GetName() << std::endl;
		std::cout << "Num. triangles in the subMesh: " << g.vertexIndices.size() / 3 << std::endl;
	}
	std::cout << "Model Center: " << objCenter.x << ", " << objCenter.y << ", " << objCenter.z << std::endl;
	std::cout << "Model Extent: " << objExtent.x << " x " << objExtent.y << " x " << objExtent.z << std::endl;
}

// Create Vertex and Index Buffer
void TriangleMesh::CreateBuffer() {
	// Create Vertex Buffer
	glGenBuffers(1, &vboId);
	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glBufferData(GL_ARRAY_BUFFER, sizeof(VertexPTN) * numVertices, vertices.data(), GL_STATIC_DRAW);

	// Create index Buffer
	for (auto& subMesh : subMeshes) {
		glGenBuffers(1, &(subMesh.iboId));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subMesh.iboId);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * subMesh.vertexIndices.size(), subMesh.vertexIndices.data(), GL_STATIC_DRAW);
	}
}


// Render SubMesh
void TriangleMesh::Render(SubMesh subMesh) {
	// Draw SubMesh
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);

	glBindBuffer(GL_ARRAY_BUFFER, vboId);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPTN), 0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPTN), (const GLvoid*)12);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPTN), (const GLvoid*)24);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, subMesh.iboId);
	glDrawElements(GL_TRIANGLES, (GLsizei)(subMesh.vertexIndices.size()), GL_UNSIGNED_INT, 0);

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
	glDisableVertexAttribArray(2);

	return;
}
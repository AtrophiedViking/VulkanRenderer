#define _CRT_SECURE_NO_WARNINGS
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>
#include "headers/models.h"
#include <ktxvulkan.h>
//utility
static void processNode(tinygltf::Model& model, tinygltf::Node& node, Node* parent, const std::string& baseDir) {
	Node* newNode = new Node();
	newNode->name = node.name;
	std::cout << "Processing node: " << node.name << std::endl;
	// If the node has a mesh, process it
	if(parent){
		newNode->parent = parent;
		parent->children.push_back(newNode);
	}
	if (node.mesh >= 0) {
		const tinygltf::Mesh& mesh = model.meshes[node.mesh];
		std::cout << "Processing mesh: " << mesh.name << std::endl;

		// Process the mesh and its primitives
		for (const auto& primitive : mesh.primitives) {
			// Process the primitive (e.g., load vertex data, indices, materials)
			std::cout << "Processing primitive with material index: " << primitive.material << std::endl;
			// (Implementation of primitive processing is omitted for brevity)
			Mesh newMesh;
			// Get indices
			const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
			const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
			const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
			std::printf("Index accessor count: %i\n", (int)indexAccessor.count);


			// Get vertex positions
			const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
			const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
			const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];
			std::printf("Position accessor count: %i\n", (int)posAccessor.count);

			// Get vertex normals if available
			bool                        hasNormals = primitive.attributes.find("NORMAL") != primitive.attributes.end();
			const tinygltf::Accessor* normalAccessor = nullptr;
			const tinygltf::BufferView* normalBufferView = nullptr;
			const tinygltf::Buffer* normalBuffer = nullptr;

			if (hasNormals) {
				const tinygltf::Accessor* normalAccessor = &model.accessors[primitive.attributes.at("NORMAL")];
				const tinygltf::BufferView* normalBufferView = &model.bufferViews[normalAccessor->bufferView];
				const tinygltf::Buffer* normalBuffer = &model.buffers[normalBufferView->buffer];
				std::printf("Normal accessor count: %i\n", (int)normalAccessor->count);
			};
			// Get texture coordinates if available
			bool                        hasTexCoords = primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end();
			const tinygltf::Accessor* texCoordAccessor = nullptr;
			const tinygltf::BufferView* texCoordBufferView = nullptr;
			const tinygltf::Buffer* texCoordBuffer = nullptr;

			if (hasTexCoords)
			{
				texCoordAccessor = &model.accessors[primitive.attributes.at("TEXCOORD_0")];
				texCoordBufferView = &model.bufferViews[texCoordAccessor->bufferView];
				texCoordBuffer = &model.buffers[texCoordBufferView->buffer];
				std::printf("TexCoord accessor count: %i\n", (int)texCoordAccessor->count);
			}

			uint32_t baseVertex = static_cast<uint32_t>(newMesh.vertices.size());

			for (size_t i = 0; i < posAccessor.count; i++)
			{
				Vertex vertex{};

				const float* pos = reinterpret_cast<const float*>(&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset + i * 12]);
				// glTF uses a right-handed coordinate system with Y-up
				// Vulkan uses a right-handed coordinate system with Y-down
				// We need to flip the Y coordinate
				vertex.pos = { pos[0], -pos[1], pos[2] };

				if (hasTexCoords)
				{
					const float* texCoord = reinterpret_cast<const float*>(&texCoordBuffer->data[texCoordBufferView->byteOffset + texCoordAccessor->byteOffset + i * 8]);
					vertex.texCoord = { texCoord[0], texCoord[1] };
				}
				else
				{
					vertex.texCoord = { 0.0f, 0.0f };
				}

				vertex.color = { 1.0f, 1.0f, 1.0f };

				newMesh.vertices.push_back(vertex);
			}

			const unsigned char* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];
			size_t               indexCount = indexAccessor.count;
			size_t               indexStride = 0;

			// Determine index stride based on component type
			if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
			{
				indexStride = sizeof(uint16_t);
			}
			else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
			{
				indexStride = sizeof(uint32_t);
			}
			else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
			{
				indexStride = sizeof(uint8_t);
			}
			else
			{
				throw std::runtime_error("Unsupported index component type");
			}

			newMesh.indices.reserve(newMesh.indices.size() + indexCount);

			for (size_t i = 0; i < indexCount; i++)
			{
				uint32_t index = 0;

				if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
				{
					index = *reinterpret_cast<const uint16_t*>(indexData + i * indexStride);
				}
				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
				{
					index = *reinterpret_cast<const uint32_t*>(indexData + i * indexStride);
				}
				else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
				{
					index = *reinterpret_cast<const uint8_t*>(indexData + i * indexStride);
				}

				newMesh.indices.push_back(baseVertex + index);
			}
			// Set material
			if (primitive.material >= 0) {
				newMesh.materialIndex = primitive.material;
			}

			// Add the processed mesh to the game object
			newNode->meshes.push_back(newMesh);
		}
	}
	// Process child nodes recursively
	for (int childIndex : node.children) {
		processNode(model, model.nodes[childIndex], newNode, baseDir);
	}
};


//Loading
void modelLoad(State *state, std::string modelPath)
{
	// Use tinygltf to load the model instead of tinyobjloader
	tinygltf::Model    gltfModel;
	tinygltf::TinyGLTF loader;
	std::string        err;
	std::string        warn;

	// Detect file extension to determine which loader to use
	bool ret = false;
	std::string extension = modelPath.substr(modelPath.find_last_of(".") + 1);
	std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

	if (extension == "glb") {
		ret = loader.LoadBinaryFromFile(&gltfModel, &err, &warn, modelPath);
	}
	else if (extension == "gltf") {
		ret = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, modelPath);
	}
	else {
		err = "Unsupported file extension: " + extension + ". Expected .gltf or .glb";
	}

	if (!warn.empty())
	{
		std::cout << "glTF warning: " << warn << std::endl;
	}

	if (!err.empty())
	{
		std::cout << "glTF error: " << err << std::endl;
	}

	if (!ret)
	{
		throw std::runtime_error("Failed to load glTF model");
	}

	state->scene.rootNode = new Node();
	state->scene.rootNode->name = "Root";

	std::string baseDir = "";
	size_t lastSlashPos = modelPath.find_last_of("/\\");
	if (lastSlashPos != std::string::npos) {
		baseDir = modelPath.substr(0, lastSlashPos + 1);
	};

	const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
	for (int nodeIndex : scene.nodes) {
		// Process the node and its children recursively
		// (Implementation of node processing is omitted for brevity)
		tinygltf::Node node = gltfModel.nodes[nodeIndex];
		std::cout << "Loaded node: " << node.name << std::endl;
		processNode(gltfModel, node, state->scene.rootNode, baseDir);
		gltfModel.nodes[nodeIndex] = node; // Update the node in the model with any changes made during processing
	}

	textureImageCreate(state, state->config.KOBOLD_TEXTURE_PATH);
	textureImageViewCreate(state);
	textureSamplerCreate(state);
}

void drawMesh(State* state, VkCommandBuffer cmd, const Mesh& mesh) {
	// 1. Bind material/texture if available
	if (mesh.materialIndex >= 0) {
		const Material& mat = state->scene.materials[mesh.materialIndex];

		if (mat.baseColorTextureIndex >= 0) {
			const Texture& tex = state->scene.textures[mat.baseColorTextureIndex];

			vkCmdBindDescriptorSets(
				cmd,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				state->renderer.pipelineLayout,
				1, // descriptor set 1 = material/texture
				1,
				&tex.descriptorSet, // you will create this per texture
				0,
				nullptr
			);
		}
	}

	// 2. Bind mesh vertex/index buffers
	VkBuffer vertexBuffers[] = { mesh.vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, vertexBuffers, offsets);

	vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	// 3. Draw
	vkCmdDrawIndexed(cmd, static_cast<uint32_t>(mesh.indices.size()), 1, 0, 0, 0);
}

void drawNode(State* state, VkCommandBuffer cmd, const Node* node) {
    // Push node transform (if you want)
    glm::mat4 modelMatrix = node->getGlobalMatrix();
    vkCmdPushConstants(cmd,
        state->renderer.pipelineLayout,
        VK_SHADER_STAGE_VERTEX_BIT,
        0,
        sizeof(glm::mat4),
        &modelMatrix
    );

    // Draw all meshes in this node
    for (const Mesh& mesh : node->meshes) {
        drawMesh(state, cmd, mesh);
    }

    // Recurse
    for (const Node* child : node->children) {
        drawNode(state, cmd, child);
    }
}


#define _CRT_SECURE_NO_WARNINGS
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>
#include "headers/models.h"
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
				normalAccessor = &model.accessors[primitive.attributes.at("NORMAL")];
				normalBufferView = &model.bufferViews[normalAccessor->bufferView];
				normalBuffer = &model.buffers[normalBufferView->buffer];
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
				vertex.pos = { pos[0], pos[2], -pos[1] };

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

void createMeshBuffers(State* state, Node* node) {
	for (Mesh& mesh : node->meshes) {
		std::cout << "createMeshBuffers: node=" << node->name
			<< " verts=" << mesh.vertices.size()
			<< " idx=" << mesh.indices.size() << "\n";

		if (!mesh.vertices.empty()) {
			vertexBufferCreateForMesh(state, mesh.vertices, mesh.vertexBuffer, mesh.vertexMemory);
			std::cout << "  -> VBO created: " << (mesh.vertexBuffer != VK_NULL_HANDLE) << "\n";
		}
		if (!mesh.indices.empty()) {
			indexBufferCreateForMesh(state, mesh.indices, mesh.indexBuffer, mesh.indexMemory);
			std::cout << "  -> IBO created: " << (mesh.indexBuffer != VK_NULL_HANDLE) << "\n";
		}
	}

	for (Node* child : node->children) {
		createMeshBuffers(state, child);
	}
}


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

	state->scene.materials.clear();
	state->scene.materials.reserve(gltfModel.materials.size());


	std::vector<int> textureToImage;
	for (const auto& m : gltfModel.materials) {
		Material mat{};
		// baseColorFactor
		if (m.pbrMetallicRoughness.baseColorFactor.size() == 4) {
			mat.baseColorFactor = glm::vec4(
				m.pbrMetallicRoughness.baseColorFactor[0],
				m.pbrMetallicRoughness.baseColorFactor[1],
				m.pbrMetallicRoughness.baseColorFactor[2],
				m.pbrMetallicRoughness.baseColorFactor[3]
			);
		}

		int texIndex = m.pbrMetallicRoughness.baseColorTexture.index;
		if (texIndex >= 0 && texIndex < textureToImage.size()) {
			mat.baseColorTextureIndex = textureToImage[texIndex];
		}
		else {
			mat.baseColorTextureIndex = -1;
		}

		mat.metallicFactor = m.pbrMetallicRoughness.metallicFactor;
		mat.roughnessFactor = m.pbrMetallicRoughness.roughnessFactor;

		state->scene.materials.push_back(mat);
	}


	const tinygltf::Scene& scene = gltfModel.scenes[gltfModel.defaultScene > -1 ? gltfModel.defaultScene : 0];
	for (int nodeIndex : scene.nodes) {
		// Process the node and its children recursively
		// (Implementation of node processing is omitted for brevity)
		tinygltf::Node node = gltfModel.nodes[nodeIndex];
		std::cout << "Loaded node: " << node.name << std::endl;
		processNode(gltfModel, node, state->scene.rootNode, baseDir);
		gltfModel.nodes[nodeIndex] = node; // Update the node in the model with any changes made during processing
	}

	createMeshBuffers(state, state->scene.rootNode);


	if (!gltfModel.images.empty()) {

    for (const auto& image : gltfModel.images) {
        Texture tex{};
        tex.name = image.name;

        createTextureFromMemory(
			state,
            image.image.data(),
            image.image.size(),
            image.width,
            image.height,
            image.component,
            tex
        );

        state->scene.textures.push_back(tex);


		textureToImage.reserve(gltfModel.textures.size());

		for (const auto& gltfTex : gltfModel.textures) {
			textureToImage.push_back(gltfTex.source); // maps texture index → image index
		}

    }

} else {
    // No textures in GLB → load fallback
    Texture tex{};
    textureImageCreate(state, state->config.KOBOLD_TEXTURE_PATH);
    textureImageViewCreate(state);
    textureSamplerCreate(state);

    tex.textureImageView = state->texture.textureImageView;
    tex.textureSampler   = state->texture.textureSampler;

    state->scene.textures.push_back(tex);
}

}

void modelUnload(State* state) {
	// Clean up mesh buffers
	std::function<void(Node*)> cleanupNode = [&](Node* node) {
		for (Mesh& mesh : node->meshes) {
			if (mesh.vertexBuffer != VK_NULL_HANDLE) {
				vkDestroyBuffer(state->context.device, mesh.vertexBuffer, nullptr);
				mesh.vertexBuffer = VK_NULL_HANDLE;
			}
			if (mesh.vertexMemory != VK_NULL_HANDLE) {
				vkFreeMemory(state->context.device, mesh.vertexMemory, nullptr);
				mesh.vertexMemory = VK_NULL_HANDLE;
			}
			if (mesh.indexBuffer != VK_NULL_HANDLE) {
				vkDestroyBuffer(state->context.device, mesh.indexBuffer, nullptr);
				mesh.indexBuffer = VK_NULL_HANDLE;
			}
			if (mesh.indexMemory != VK_NULL_HANDLE) {
				vkFreeMemory(state->context.device, mesh.indexMemory, nullptr);
				mesh.indexMemory = VK_NULL_HANDLE;
			}
		}
		for (Node* child : node->children) {
			cleanupNode(child);
		}
	};
	if (state->scene.rootNode) {
		cleanupNode(state->scene.rootNode);
		delete state->scene.rootNode;
		state->scene.rootNode = nullptr;
	}
	textureImageDestroy(state);
}

void createTextureDescriptorSets(State* state) {
	for (Texture& tex : state->scene.textures) {
		VkDescriptorSetAllocateInfo allocInfo{
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = state->renderer.descriptorPool,
			.descriptorSetCount = 1,
			.pSetLayouts = &state->renderer.textureSetLayout
		};

		PANIC(
			vkAllocateDescriptorSets(state->context.device, &allocInfo, &tex.descriptorSet),
			"Failed to allocate texture descriptor set"
		);

		// For now, use the same image for all 5 bindings
		std::array<VkDescriptorImageInfo, 5> infos{};
		for (uint32_t i = 0; i < 5; ++i) {
			infos[i] = {
				.sampler = tex.textureSampler,
				.imageView = tex.textureImageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};
		}

		std::array<VkWriteDescriptorSet, 5> writes{};
		for (uint32_t i = 0; i < 5; ++i) {
			writes[i] = {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = tex.descriptorSet,
				.dstBinding = i,                         // 0..4
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &infos[i]
			};
		}

		vkUpdateDescriptorSets(
			state->context.device,
			static_cast<uint32_t>(writes.size()), writes.data(),
			0, nullptr
		);
	}
}


void drawMesh(State* state, VkCommandBuffer cmd, const Mesh& mesh) {
	const Material& mat = state->scene.materials[mesh.materialIndex];

	int texIndex = mat.baseColorTextureIndex;

	// If invalid, use fallback
	if (texIndex < 0 || texIndex >= state->scene.textures.size()) {
		texIndex = state->scene.defaultTextureIndex;
	}

	const Texture& tex = state->scene.textures[texIndex];

	vkCmdBindDescriptorSets(
		cmd,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		state->renderer.pipelineLayout,
		1, // set = 1
		1,
		&tex.descriptorSet,
		0,
		nullptr
	);


	// Push constants for material factors
	PushConstantBlock pcb{};
	pcb.baseColorFactor = mat.baseColorFactor;
	pcb.metallicFactor = mat.metallicFactor;
	pcb.roughnessFactor = mat.roughnessFactor;

	vkCmdPushConstants(
		cmd,
		state->renderer.pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PushConstantBlock),
		&pcb
	);

	// Bind vertex + index buffers
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(cmd, 0, 1, &mesh.vertexBuffer, offsets);
	vkCmdBindIndexBuffer(cmd, mesh.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

	// Draw
	vkCmdDrawIndexed(cmd, mesh.indices.size(), 1, 0, 0, 0);
}



void drawNode(State* state, VkCommandBuffer cmd, const Node* node) {
	PushConstantBlock pushConstantBlock{
			.baseColorFactor = {1.0f, 1.0f, 1.0f, 1.0f},
			.metallicFactor = 1.0f,
			.roughnessFactor = 0.5f,
			.baseColorTextureSet = 0,
			.physicalDescriptorTextureSet = 1,
			.normalTextureSet = 2,
			.occlusionTextureSet = 3,
			.emissiveTextureSet = 4,
			.alphaMask = 0.0f,
			.alphaMaskCutoff = 0.5f
	};
    // Push node transform (if you want)
    glm::mat4 modelMatrix = node->getGlobalMatrix();
	vkCmdPushConstants(cmd,
		state->renderer.pipelineLayout,
		VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
		0,
		sizeof(PushConstantBlock),
		&pushConstantBlock
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

void gatherDrawItems( const Node* root, const glm::vec3& camPos, const std::vector<Material>& materials, std::vector<DrawItem>& out){
	std::function<void(const Node*)> recurse = [&](const Node* node) {

		// FIX: extract translation from global matrix
		glm::mat4 M = node->getGlobalMatrix();
		glm::vec3 worldPos = glm::vec3(M[3]);
		float dist = glm::length(worldPos - camPos);

		for (const Mesh& mesh : node->meshes) {

			bool isTransparent = false;

			if (mesh.materialIndex >= 0 && mesh.materialIndex < (int)materials.size()) {
				const auto& mat = materials[mesh.materialIndex];

				// Simple transparency rule
				if (mat.baseColorFactor.a < 1.0f) {
					isTransparent = true;
				}
			}

			out.push_back({
				.node = node,
				.mesh = &mesh,
				.distanceToCamera = dist,
				.transparent = isTransparent
			});

		}

		for (const Node* child : node->children) {
			recurse(child);
		}
	};
	recurse(root);
}


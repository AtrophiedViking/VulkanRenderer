#define _CRT_SECURE_NO_WARNINGS
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <tiny_gltf.h>
#include "headers/models.h"
//utility
static void processNode(tinygltf::Model& model, tinygltf::Node& node, Node* parent, const std::string& baseDir)
{
	Node* newNode = new Node();
	newNode->name = node.name;

	// ─────────────────────────────────────────────
	// Node transform
	// ─────────────────────────────────────────────
	if (!node.matrix.empty()) {
		newNode->matrix = glm::make_mat4(node.matrix.data());
		newNode->translation = glm::vec3(0.0f);
		newNode->rotation = glm::quat(1, 0, 0, 0);
		newNode->scale = glm::vec3(1.0f);
	}
	else {
		if (!node.translation.empty())
			newNode->translation = glm::vec3(node.translation[0], node.translation[1], node.translation[2]);
		if (!node.rotation.empty())
			newNode->rotation = glm::quat(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
		if (!node.scale.empty())
			newNode->scale = glm::vec3(node.scale[0], node.scale[1], node.scale[2]);
	}

	if (parent) {
		newNode->parent = parent;
		parent->children.push_back(newNode);
	}

	// ─────────────────────────────────────────────
	// Helper: decode normalized integer → float
	// ─────────────────────────────────────────────
	auto readFloat = [](const unsigned char* src, int componentType) -> float {
		switch (componentType) {
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			return *reinterpret_cast<const float*>(src);

		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			return (*src) / 255.0f;

		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			return (*reinterpret_cast<const uint16_t*>(src)) / 65535.0f;

		default:
			throw std::runtime_error("Unsupported componentType for float conversion");
		}
		};

	// ─────────────────────────────────────────────
	// Process mesh
	// ─────────────────────────────────────────────
	if (node.mesh >= 0) {
		const tinygltf::Mesh& mesh = model.meshes[node.mesh];

		for (const auto& primitive : mesh.primitives) {
			Mesh newMesh;

			// ─────────────────────────────────────────────
			// Index buffer
			// ─────────────────────────────────────────────
			const auto& indexAccessor = model.accessors[primitive.indices];
			const auto& indexBufferView = model.bufferViews[indexAccessor.bufferView];
			const auto& indexBuffer = model.buffers[indexBufferView.buffer];

			size_t indexCount = indexAccessor.count;

			const unsigned char* indexData =
				&indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];

			size_t indexStride = 0;
			switch (indexAccessor.componentType) {
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: indexStride = 2; break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   indexStride = 4; break;
			case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  indexStride = 1; break;
			default: throw std::runtime_error("Unsupported index type");
			}

			// ─────────────────────────────────────────────
			// POSITION (float only)
			// ─────────────────────────────────────────────
			const auto& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
			const auto& posBufferView = model.bufferViews[posAccessor.bufferView];
			const auto& posBuffer = model.buffers[posBufferView.buffer];

			if (posAccessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
				throw std::runtime_error("POSITION must be FLOAT");

			size_t posStride = posBufferView.byteStride ?
				posBufferView.byteStride :
				3 * sizeof(float);

			// ─────────────────────────────────────────────
			// NORMAL (float only)
			// ─────────────────────────────────────────────
			bool hasNormals = primitive.attributes.count("NORMAL");
			const tinygltf::Accessor* normalAccessor = nullptr;
			const tinygltf::BufferView* normalBufferView = nullptr;
			const tinygltf::Buffer* normalBuffer = nullptr;
			size_t normalStride = 0;

			if (hasNormals) {
				normalAccessor = &model.accessors[primitive.attributes.at("NORMAL")];
				if (normalAccessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
					throw std::runtime_error("NORMAL must be FLOAT");

				normalBufferView = &model.bufferViews[normalAccessor->bufferView];
				normalBuffer = &model.buffers[normalBufferView->buffer];
				normalStride = normalBufferView->byteStride ?
					normalBufferView->byteStride :
					3 * sizeof(float);
			}
			// ─────────────────────────────────────────────
			// TEXCOORD_0 (float or normalized int)
			// ─────────────────────────────────────────────
			bool hasTexCoords = primitive.attributes.count("TEXCOORD_0") > 0;
			const tinygltf::Accessor* uvAccessor = nullptr;
			const tinygltf::BufferView* uvBufferView = nullptr;
			const tinygltf::Buffer* uvBuffer = nullptr;
			size_t uvStride = 0;

			if (hasTexCoords) {
				uvAccessor = &model.accessors[primitive.attributes.at("TEXCOORD_0")];
				uvBufferView = &model.bufferViews[uvAccessor->bufferView];
				uvBuffer = &model.buffers[uvBufferView->buffer];

				if (uvAccessor->type != TINYGLTF_TYPE_VEC2)
					throw std::runtime_error("TEXCOORD_0 must be VEC2");

				if (uvBufferView->byteStride != 0) {
					uvStride = uvBufferView->byteStride;
				}
				else {
					switch (uvAccessor->componentType) {
					case TINYGLTF_COMPONENT_TYPE_FLOAT:          uvStride = 2 * sizeof(float);   break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  uvStride = 2 * sizeof(uint8_t); break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: uvStride = 2 * sizeof(uint16_t); break;
					default:
						throw std::runtime_error("Unsupported TEXCOORD_0 componentType");
					}
				}
			}


			// ─────────────────────────────────────────────
			// COLOR_0 (float or normalized int)
			// ─────────────────────────────────────────────
			bool hasColors = primitive.attributes.count("COLOR_0");
			const tinygltf::Accessor* colorAccessor = nullptr;
			const tinygltf::BufferView* colorBufferView = nullptr;
			const tinygltf::Buffer* colorBuffer = nullptr;
			size_t colorStride = 0;

			if (hasColors) {
				colorAccessor = &model.accessors[primitive.attributes.at("COLOR_0")];
				colorBufferView = &model.bufferViews[colorAccessor->bufferView];
				colorBuffer = &model.buffers[colorBufferView->buffer];

				int comps = (colorAccessor->type == TINYGLTF_TYPE_VEC3 ? 3 :
					colorAccessor->type == TINYGLTF_TYPE_VEC4 ? 4 : 0);
				if (!comps) throw std::runtime_error("COLOR_0 must be VEC3 or VEC4");

				if (colorBufferView->byteStride)
					colorStride = colorBufferView->byteStride;
				else {
					switch (colorAccessor->componentType) {
					case TINYGLTF_COMPONENT_TYPE_FLOAT:          colorStride = comps * 4; break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  colorStride = comps * 1; break;
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: colorStride = comps * 2; break;
					default: throw std::runtime_error("Unsupported COLOR_0 componentType");
					}
				}
			}

			// ─────────────────────────────────────────────
			// TANGENT (float only)
			// ─────────────────────────────────────────────
			bool hasTangents = primitive.attributes.count("TANGENT");
			const tinygltf::Accessor* tanAccessor = nullptr;
			const tinygltf::BufferView* tanBufferView = nullptr;
			const tinygltf::Buffer* tanBuffer = nullptr;
			size_t tanStride = 0;

			if (hasTangents) {
				tanAccessor = &model.accessors[primitive.attributes.at("TANGENT")];
				if (tanAccessor->componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
					throw std::runtime_error("TANGENT must be FLOAT");

				tanBufferView = &model.bufferViews[tanAccessor->bufferView];
				tanBuffer = &model.buffers[tanBufferView->buffer];
				tanStride = tanBufferView->byteStride ?
					tanBufferView->byteStride :
					4 * sizeof(float);
			}

			// ─────────────────────────────────────────────
			// Vertex loop
			// ─────────────────────────────────────────────
			uint32_t baseVertex = (uint32_t)newMesh.vertices.size();
			newMesh.vertices.reserve(newMesh.vertices.size() + posAccessor.count);

			for (size_t i = 0; i < posAccessor.count; ++i) {
				Vertex v{};

				// POSITION
				const float* pos = reinterpret_cast<const float*>(
					&posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset + i * posStride]);
				v.pos = { pos[0], pos[2], -pos[1] };

				// NORMAL
				if (hasNormals) {
					const float* n = reinterpret_cast<const float*>(
						&normalBuffer->data[normalBufferView->byteOffset + normalAccessor->byteOffset + i * normalStride]);
					v.normal = { n[0], n[2], -n[1] };
				}

				// TEXCOORD_0
				if (hasTexCoords) {
					const unsigned char* base = &uvBuffer->data[
						uvBufferView->byteOffset +
							uvAccessor->byteOffset +
							i * uvStride
					];

					switch (uvAccessor->componentType) {
					case TINYGLTF_COMPONENT_TYPE_FLOAT: {
						const float* uv = reinterpret_cast<const float*>(base);
						v.texCoord = { uv[0], uv[1] };
						break;
					}
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
						const uint8_t* uv = reinterpret_cast<const uint8_t*>(base);
						v.texCoord = {
							uv[0] / 255.0f,
							uv[1] / 255.0f
						};
						break;
					}
					case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
						const uint16_t* uv = reinterpret_cast<const uint16_t*>(base);
						v.texCoord = {
							uv[0] / 65535.0f,
							uv[1] / 65535.0f
						};
						break;
					}
					default:
						v.texCoord = { 0.0f, 0.0f }; // should never hit due to earlier check
						break;
					}
				}
				else {
					v.texCoord = { 0.0f, 0.0f };
				}


				// COLOR_0
				if (hasColors) {
					const unsigned char* base = &colorBuffer->data[
						colorBufferView->byteOffset + colorAccessor->byteOffset + i * colorStride];

					v.color.r = readFloat(base + 0 * (colorStride / (colorAccessor->type == TINYGLTF_TYPE_VEC4 ? 4 : 3)), colorAccessor->componentType);
					v.color.g = readFloat(base + 1 * (colorStride / (colorAccessor->type == TINYGLTF_TYPE_VEC4 ? 4 : 3)), colorAccessor->componentType);
					v.color.b = readFloat(base + 2 * (colorStride / (colorAccessor->type == TINYGLTF_TYPE_VEC4 ? 4 : 3)), colorAccessor->componentType);
				}
				else {
					v.color = { 1,1,1 };
				}

				// TANGENT
				if (hasTangents) {
					const float* t = reinterpret_cast<const float*>(
						&tanBuffer->data[tanBufferView->byteOffset + tanAccessor->byteOffset + i * tanStride]);
					v.tangent = { t[0], t[2], -t[1], t[3] };
				}

				newMesh.vertices.push_back(v);
			}

			// ─────────────────────────────────────────────
			// Index loop
			// ─────────────────────────────────────────────
			newMesh.indices.reserve(newMesh.indices.size() + indexCount);

			for (size_t i = 0; i < indexCount; ++i) {
				uint32_t idx = 0;
				const unsigned char* src = indexData + i * indexStride;

				switch (indexAccessor.componentType) {
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: idx = *reinterpret_cast<const uint16_t*>(src); break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:   idx = *reinterpret_cast<const uint32_t*>(src); break;
				case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:  idx = *reinterpret_cast<const uint8_t*>(src);  break;
				}

				newMesh.indices.push_back(baseVertex + idx);
			}

			if (primitive.material >= 0)
				newMesh.materialIndex = primitive.material;

			newNode->meshes.push_back(newMesh);
		}
	}

	// ─────────────────────────────────────────────
	// Recurse
	// ─────────────────────────────────────────────
	for (int child : node.children)
		processNode(model, model.nodes[child], newNode, baseDir);
}


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

// Base type
static void readTextureTransform(
	const tinygltf::TextureInfo& info,
	TextureTransform& out)
{
	out.texCoord = info.texCoord;

	auto it = info.extensions.find("KHR_texture_transform");
	if (it == info.extensions.end()) return;

	const tinygltf::Value& ext = it->second;

	if (ext.Has("offset")) {
		const auto& arr = ext.Get("offset").Get<tinygltf::Value::Array>();
		out.offset = glm::vec2(arr[0].GetNumberAsDouble(), arr[1].GetNumberAsDouble());
	}
	if (ext.Has("scale")) {
		const auto& arr = ext.Get("scale").Get<tinygltf::Value::Array>();
		out.scale = glm::vec2(arr[0].GetNumberAsDouble(), arr[1].GetNumberAsDouble());
	}
	if (ext.Has("rotation")) {
		out.rotation = float(ext.Get("rotation").GetNumberAsDouble());
	}
	if (ext.Has("center")) {
		const auto& arr = ext.Get("center").Get<tinygltf::Value::Array>();
		out.center = glm::vec2(arr[0].GetNumberAsDouble(), arr[1].GetNumberAsDouble());
	}
	if (ext.Has("texCoord")) {
		out.texCoord = int(ext.Get("texCoord").GetNumberAsInt());
	}
}

// NormalTextureInfo overload
static void readTextureTransform(
	const tinygltf::NormalTextureInfo& info,
	TextureTransform& out)
{
	// NormalTextureInfo has .texCoord too
	out.texCoord = info.texCoord;

	auto it = info.extensions.find("KHR_texture_transform");
	if (it == info.extensions.end()) return;

	const tinygltf::Value& ext = it->second;

	if (ext.Has("offset")) {
		const auto& arr = ext.Get("offset").Get<tinygltf::Value::Array>();
		out.offset = glm::vec2(arr[0].GetNumberAsDouble(), arr[1].GetNumberAsDouble());
	}
	if (ext.Has("scale")) {
		const auto& arr = ext.Get("scale").Get<tinygltf::Value::Array>();
		out.scale = glm::vec2(arr[0].GetNumberAsDouble(), arr[1].GetNumberAsDouble());
	}
	if (ext.Has("rotation")) {
		out.rotation = float(ext.Get("rotation").GetNumberAsDouble());
	}
	if (ext.Has("center")) {
		const auto& arr = ext.Get("center").Get<tinygltf::Value::Array>();
		out.center = glm::vec2(arr[0].GetNumberAsDouble(), arr[1].GetNumberAsDouble());
	}
	if (ext.Has("texCoord")) {
		out.texCoord = int(ext.Get("texCoord").GetNumberAsInt());
	}
}

// OcclusionTextureInfo overload
static void readTextureTransform(
	const tinygltf::OcclusionTextureInfo& info,
	TextureTransform& out)
{
	out.texCoord = info.texCoord;

	auto it = info.extensions.find("KHR_texture_transform");
	if (it == info.extensions.end()) return;

	const tinygltf::Value& ext = it->second;

	if (ext.Has("offset")) {
		const auto& arr = ext.Get("offset").Get<tinygltf::Value::Array>();
		out.offset = glm::vec2(arr[0].GetNumberAsDouble(), arr[1].GetNumberAsDouble());
	}
	if (ext.Has("scale")) {
		const auto& arr = ext.Get("scale").Get<tinygltf::Value::Array>();
		out.scale = glm::vec2(arr[0].GetNumberAsDouble(), arr[1].GetNumberAsDouble());
	}
	if (ext.Has("rotation")) {
		out.rotation = float(ext.Get("rotation").GetNumberAsDouble());
	}
	if (ext.Has("center")) {
		const auto& arr = ext.Get("center").Get<tinygltf::Value::Array>();
		out.center = glm::vec2(arr[0].GetNumberAsDouble(), arr[1].GetNumberAsDouble());
	}
	if (ext.Has("texCoord")) {
		out.texCoord = int(ext.Get("texCoord").GetNumberAsInt());
	}
}


//Loading
Model* modelLoad(State *state, std::string modelPath)
{
	// Use tinygltf to load the model instead of tinyobjloader
	Model* model = new Model();
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

	std::vector<int> textureToImage;
	textureToImage.reserve(gltfModel.textures.size());

	for (const auto& gltfTex : gltfModel.textures) {
		textureToImage.push_back(gltfTex.source);
	}

	std::string baseDir = "";
	size_t lastSlashPos = modelPath.find_last_of("/\\");
	if (lastSlashPos != std::string::npos) {
		baseDir = modelPath.substr(0, lastSlashPos + 1);
	};

	state->scene.materials.clear();
	state->scene.materials.reserve(gltfModel.materials.size());

	for (const auto& m : gltfModel.materials) {
		Material mat{};

		// Base color factor
		if (m.pbrMetallicRoughness.baseColorFactor.size() == 4) {
			mat.baseColorFactor = glm::vec4(
				m.pbrMetallicRoughness.baseColorFactor[0],
				m.pbrMetallicRoughness.baseColorFactor[1],
				m.pbrMetallicRoughness.baseColorFactor[2],
				m.pbrMetallicRoughness.baseColorFactor[3]
			);
		}
		// Alpha mode
		if (m.alphaMode == "MASK")
			mat.alphaMode = "MASK";
		else if (m.alphaMode == "BLEND")
			mat.alphaMode = "BLEND";
		else
			mat.alphaMode = "OPAQUE";

		// Alpha cutoff
		if (m.alphaCutoff > 0.0f)
			mat.alphaCutoff = (float)m.alphaCutoff;

		// Double-sided
		mat.doubleSided = m.doubleSided;

		// Base color texture
		if (m.pbrMetallicRoughness.baseColorTexture.index >= 0) {
			mat.baseColorTextureIndex =
				m.pbrMetallicRoughness.baseColorTexture.index;

			readTextureTransform(
				m.pbrMetallicRoughness.baseColorTexture,
				mat.baseColorTransform
			);
		}

		// Metallic-roughness texture
		if (m.pbrMetallicRoughness.metallicRoughnessTexture.index >= 0) {
			mat.metallicRoughnessTextureIndex =
				m.pbrMetallicRoughness.metallicRoughnessTexture.index;

			readTextureTransform(
				m.pbrMetallicRoughness.metallicRoughnessTexture,
				mat.metallicRoughnessTransform
			);
		}

		// Normal texture
		if (m.normalTexture.index >= 0) {
			mat.normalTextureIndex = m.normalTexture.index;

			readTextureTransform(
				m.normalTexture,
				mat.normalTransform
			);
		}

		// Occlusion texture
		if (m.occlusionTexture.index >= 0) {
			mat.occlusionTextureIndex = m.occlusionTexture.index;

			readTextureTransform(
				m.occlusionTexture,
				mat.occlusionTransform
			);
		}

		// Emissive texture
		if (m.emissiveTexture.index >= 0) {
			mat.emissiveTextureIndex = m.emissiveTexture.index;

			readTextureTransform(
				m.emissiveTexture,
				mat.emissiveTransform
			);
		}


		// Scalars
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
		}
	}
	else {
		// Fallback texture
		Texture tex{};
		textureImageCreate(state, state->config.KOBOLD_TEXTURE_PATH);
		textureImageViewCreate(state);
		textureSamplerCreate(state);

		tex.textureImageView = state->texture.textureImageView;
		tex.textureSampler = state->texture.textureSampler;

		state->scene.textures.push_back(tex);
	}

	state->scene.models.push_back(*model);
	return model;
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

void drawMesh(State* state, VkCommandBuffer cmd, const Mesh& mesh)
{
	const Material& mat = state->scene.materials[mesh.materialIndex];

	auto resolveTex = [&](int index) -> const Texture&
		{
			if (index >= 0 && index < state->scene.textures.size())
				return state->scene.textures[index];
			return state->scene.textures[state->scene.defaultTextureIndex];
		};

	const Texture& baseTex = resolveTex(mat.baseColorTextureIndex);

	// Bind descriptor set for this material (set = 1)
	vkCmdBindDescriptorSets(
		cmd,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		state->renderer.pipelineLayout,
		1, // set = 1
		1,
		&mat.descriptorSet,
		0,
		nullptr
	);

	// Push constants
	PushConstantBlock pcb{};
	pcb.baseColorFactor = mat.baseColorFactor;
	pcb.metallicFactor = mat.metallicFactor;
	pcb.roughnessFactor = mat.roughnessFactor;

	pcb.baseColorTextureSet = 0;
	pcb.physicalDescriptorTextureSet = 1;
	pcb.normalTextureSet = 2;
	pcb.occlusionTextureSet = 3;
	pcb.emissiveTextureSet = 4;
	pcb.alphaMask = (mat.alphaMode == "MASK") ? 1.0f : 0.0f;
	pcb.alphaMaskCutoff = mat.alphaCutoff;

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

void gatherDrawItems(const Node* root, const glm::vec3& camPos, const std::vector<Material>& materials, std::vector<DrawItem>& out) {
	std::function<void(const Node*)> recurse = [&](const Node* node) {

		// FIX: extract translation from global matrix
		glm::mat4 M = node->getGlobalMatrix();
		glm::vec3 worldPos = glm::vec3(M[3]);
		float dist = glm::length(worldPos - camPos);

		for (const Mesh& mesh : node->meshes) {

			bool isTransparent = false;

			if (mesh.materialIndex >= 0 && mesh.materialIndex < (int)materials.size()) {
				const auto& mat = materials[mesh.materialIndex];

				if (mat.alphaMode == "BLEND")
					isTransparent = true;
				else if (mat.alphaMode == "MASK")
					isTransparent = false; // still opaque, but alpha-tested
				else if (mat.baseColorFactor.a < 1.0f)
					isTransparent = true;
			};

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

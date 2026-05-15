#include "OgreHlsl2Glsl.h"

// OGRE
#include <OgreDataStream.h>
#include <OgreResourceGroupManager.h>
#include <OgreRootLayout.h>
#include <vao/OgreVertexElements.h>

// shaderc
#include <shaderc/shaderc.hpp>

// spirv-cross
#include <spirv_cross/spirv_glsl.hpp>

// std
#include <array>
#include <codecvt>
#include <locale>

namespace
{
	//////////////////////////////////////////////////////////////////////////
	class HlslIncluder : public shaderc::CompileOptions::IncluderInterface
	{
	public:
		struct IncludeResult
		{
			std::string m_FileName;
			std::string m_FileContent;
			shaderc_include_result m_Data;
		};

	public:
		HlslIncluder() = default;
		~HlslIncluder() override = default;

		shaderc_include_result* GetInclude(const char* requested_source, shaderc_include_type /*type*/, const char* /*requesting_source*/, size_t /*include_depth*/) override
		{
			auto found = m_Includes.find(requested_source);
			if (found == m_Includes.end())
			{
				// Load the include file.
				Ogre::DataStreamPtr pStream = Ogre::ResourceGroupManager::getSingleton().openResource(Ogre::String(requested_source));

				auto pIncludeResult = std::make_unique<IncludeResult>();
				pIncludeResult->m_FileName = requested_source;
				pIncludeResult->m_FileContent = pStream != nullptr ? pStream->getAsString() : Ogre::String();
				if (pIncludeResult->m_FileContent.empty())
				{
					// error
					pIncludeResult->m_FileName = Ogre::String();
					pIncludeResult->m_FileContent = Ogre::String("Cannot load the file ") + requested_source;
				}

				pIncludeResult->m_Data.source_name = pIncludeResult->m_FileName.c_str();
				pIncludeResult->m_Data.source_name_length = pIncludeResult->m_FileName.size();

				pIncludeResult->m_Data.content = pIncludeResult->m_FileContent.c_str();
				pIncludeResult->m_Data.content_length = pIncludeResult->m_FileContent.size();

				pIncludeResult->m_Data.user_data = nullptr;

				found = m_Includes.try_emplace(pIncludeResult->m_FileName, std::move(pIncludeResult)).first;
			}

			return &found->second->m_Data;
		}

		void ReleaseInclude(shaderc_include_result* data) override
		{
			m_Includes.erase(data->source_name);
		}

	private:
		std::unordered_map<Ogre::String, std::unique_ptr<IncludeResult>> m_Includes;
	};

	//////////////////////////////////////////////////////////////////////////
	/// Remap ShaderType to shaderc_shader_kind
	static_assert(Ogre::NumShaderTypes == 5, "ShaderType enum has been changed! Update the mapping.");
	constexpr std::array<shaderc_shader_kind, Ogre::NumShaderTypes> cShaderTypeToShaderKind = {
		shaderc_vertex_shader,         // Ogre::VertexShader
		shaderc_fragment_shader,       // Ogre::PixelShader
		shaderc_geometry_shader,       // Ogre::GeometryShader
		shaderc_tess_control_shader,   // Ogre::HullShader
		shaderc_tess_evaluation_shader // Ogre::DomainShader
	};

	//////////////////////////////////////////////////////////////////////////
	/// Remap VertexElementSemantic to the binding locations.
	/// note: The mapped values are copied from GL3PlusVaoManager::getAttributeIndexFor to avoid linking against RenderSystem_GL3Plus module.
	const std::unordered_map<std::string, uint32_t> cVertexElementSementicToLocation = {
		{"DRAWID", 15}, // DRAWID
		{"POSITION", 0}, // VES_POSITION
		{"BLENDWEIGHT", 3}, // VES_BLEND_WEIGHTS
		{"BLENDINDICES", 4}, // VES_BLEND_INDICES
		{"NORMAL", 1}, // VES_NORMAL
		{"COLOR", 5}, // VES_DIFFUSE
		{"COLOR0", 5}, // VES_DIFFUSE (just an alias in HLSL if 2 colors are used)
		{"COLOR1", 6}, // VES_SPECULAR
		{"TEXCOORD0", 7}, // VES_TEXTURE_COORDINATES + 0
		{"TEXCOORD1", 8}, // VES_TEXTURE_COORDINATES + 1
		{"TEXCOORD2", 9}, // VES_TEXTURE_COORDINATES + 2
		{"TEXCOORD3", 10}, // VES_TEXTURE_COORDINATES + 3
		{"TEXCOORD4", 11}, // VES_TEXTURE_COORDINATES + 4
		{"TEXCOORD5", 12}, // VES_TEXTURE_COORDINATES + 5
		{"TEXCOORD6", 13}, // VES_TEXTURE_COORDINATES + 6
		{"TEXCOORD7", 14}, // VES_TEXTURE_COORDINATES + 7
		{"BINORMAL", 16}, // VES_BINORMAL
		{"TANGENT", 2} // VES_TANGENT
	};
}

//////////////////////////////////////////////////////////////////////////
Ogre::String Ogre::convertHlsl2Glsl(const String& hlsl, const String& debugFilenameOutput, ShaderType shaderType, const RootLayout& rootLayout, Archive* pDataFolder)
{
	shaderc::Compiler compiler;
	if (!compiler.IsValid())
	{
		OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR, "Out of memory!", "Ogre::ConvertHlsl2Glsl");
	}

	shaderc::CompileOptions compilerOptions;
	compilerOptions.SetSourceLanguage(shaderc_source_language_hlsl);
	compilerOptions.SetTargetEnvironment(shaderc_target_env_opengl, shaderc_env_version_opengl_4_5);
	compilerOptions.SetIncluder(std::make_unique<HlslIncluder>());
	compilerOptions.SetHlslFunctionality1(true); // to access semantic information for the variables in spirv
	compilerOptions.SetAutoSampledTextures(true); // otherwise there is no 'layout(binding = xxx)' for the samplers
	compilerOptions.SetNanClamp(true);

	// Compile HLSL to SPIR-V (function has name GlslToSpv but it supports also HLSL as input through the compilerOptions).
	const auto result = compiler.CompileGlslToSpv(hlsl, cShaderTypeToShaderKind[shaderType], debugFilenameOutput.c_str(), "main", compilerOptions);
	if (result.GetCompilationStatus() != shaderc_compilation_status_success)
	{
		OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR, 
			"HLSL to Spir-V compilation error!\nFile: " + debugFilenameOutput + "\nOutput: " + result.GetErrorMessage(),
			"Ogre::ConvertHlsl2Glsl");
	}
	
	// Get the SPIR-V code.
	std::vector<uint32_t> spirv(result.begin(), result.end());

	// Convert SPIR-V to GLSL.
	auto pCompilerGLSL = std::make_unique<spirv_cross::CompilerGLSL>(std::move(spirv));

	// Remap input locations to reflect the settings from GLSLProgram::bindFixedAttributes
	if (shaderType == VertexShader)
	{
		spirv_cross::ShaderResources resources = pCompilerGLSL->get_shader_resources();
		for (auto& resource : resources.stage_inputs)
		{
			const auto& semantic = pCompilerGLSL->get_decoration_string(resource.id, spv::DecorationHlslSemanticGOOGLE);
			const auto found = cVertexElementSementicToLocation.find(semantic);
			if (found == cVertexElementSementicToLocation.end())
			{
				// Unknown semantic.
				OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
					"Spir-V parsing error!\nFile: " + debugFilenameOutput + "\nOutput: Unknown semantic " + semantic,
					"Ogre::ConvertHlsl2Glsl");
			}

			pCompilerGLSL->set_decoration(resource.id, spv::DecorationLocation, found->second);
		}
	}

	// Remap the bindings of UAVs - in HLSL the UAV buffers are bound after the render target's color buffers, but in GLSL they
	// just goes after all read-only buffers (as in GL3PlusRenderSystem they are created as SSBO which is the same type as used
	// for read/write UAVs).
	uint16_t uavOffset = 0;
	for (size_t k = 0; k < OGRE_MAX_NUM_BOUND_DESCRIPTOR_SETS; ++k)
	{
		const auto readOnlyMaxSlot = rootLayout.getDescBindingRanges(k)[DescBindingTypes::ReadOnlyBuffer].end;
		if (uavOffset < readOnlyMaxSlot)
			uavOffset = readOnlyMaxSlot;
	}

	if (shaderType == PixelShader)
	{
		const auto variables = pCompilerGLSL->get_active_interface_variables();
		/*for (const auto& variable : variables)
		{
			const auto storageClass = pCompilerGLSL->get_storage_class(variable);
			const auto variableType = pCompilerGLSL->get_type_from_variable(variable);
			if (variableType.basetype != spirv_cross::SPIRType::Struct)
				continue;
			const auto blockName = pCompilerGLSL->get_block_fallback_name(variable);
			const auto remappedBlockName = pCompilerGLSL->get_remapped_declared_block_name(variable);
			const auto flags = pCompilerGLSL->get_buffer_block_flags(variable);
			const bool isReadOnly = flags.get(spv::DecorationNonWritable);
			storageClass;

			int debug = 0;
			debug = 1;
		}*/

		spirv_cross::ShaderResources resources = pCompilerGLSL->get_shader_resources();
		const auto numColorBuffers = static_cast<uint16_t>(resources.stage_outputs.size());

		for (auto& resource : resources.storage_buffers)
		{
			const auto itVariable = variables.find(resource.id);
			const bool isReadOnly = itVariable != variables.end() ? pCompilerGLSL->get_buffer_block_flags(resource.id).get(spv::DecorationNonWritable) : false;
			if (!isReadOnly)
			{
				const auto binding = pCompilerGLSL->get_decoration(resource.id, spv::DecorationBinding);
				pCompilerGLSL->set_decoration(resource.id, spv::DecorationBinding, binding + uavOffset - numColorBuffers);
			}
		}
	}

	try
	{
		pCompilerGLSL->add_header_line("layout(std140) uniform;\n"); // only to avoid assertions in GLSLProgram::extractLayoutQualifiers()
		auto glsl = pCompilerGLSL->compile();
		return glsl;
	}
	catch (const std::exception& e)
	{
		OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
			"Spir-V to GLSL compilation error!\nFile: " + debugFilenameOutput + "\nOutput: " + e.what(),
			"Ogre::ConvertHlsl2Glsl");
	}
}

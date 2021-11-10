#include "Shader.h"

#include "FileSystem/FileSystem.h"
#include "Render/GLSLCompiler.h"
#include "Device.h"
#include <stack>

namespace Immortal
{
namespace Vulkan
{

static inline VkPipelineShaderStageCreateInfo CreateStage(VkShaderModule module, VkShaderStageFlagBits stage)
{
    VkPipelineShaderStageCreateInfo createInfo{};
    createInfo.sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    createInfo.stage  = stage;
    createInfo.module = module;
    createInfo.pName  = "main";
    return createInfo;
}

Shader::Shader(Device *d, const std::string &filename, Shader::Type type)
    : device{ d }
{   
    stages.reserve(2);
    if (type == Shader::Type::Graphics)
    {
        VkShaderModule vert = Load(filename + ".vert", Shader::Stage::Vertex);
        VkShaderModule frag = Load(filename + ".frag", Shader::Stage::Fragment);

        modules.push_back(vert);
        modules.push_back(frag);
        stages.resize(2);
        stages[0] = CreateStage(vert, VK_SHADER_STAGE_VERTEX_BIT);
        stages[1] = CreateStage(frag, VK_SHADER_STAGE_FRAGMENT_BIT);
    }
    else
    {
        VkShaderModule comp = Load(filename + ".comp", Shader::Stage::Compute);
        modules.push_back(comp);
        stages.resize(1);
        stages[0] = CreateStage(comp, VK_SHADER_STAGE_COMPUTE_BIT);
    }
}

Shader::~Shader()
{
    for (auto &m : modules)
    {
        device->Destory(m);
    }
}

VkShaderModule Shader::Load(const std::string &filename, Shader::Stage stage)
{
    GLSLCompiler compiler{};

    auto src = FileSystem::ReadString(filename);

    std::vector<uint32_t> spirv;
    std::string error;

    if (!GLSLCompiler::Src2Spirv(Shader::API::Vulkan, stage, src.size(), src.data(), "main", spirv, error))
    {
        LOG::FATAL("Failed to compiler Shader => {0}\n", error.c_str());
        return VK_NULL_HANDLE;
    }

    VkShaderModule shaderModule{ VK_NULL_HANDLE };
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spirv.size() * sizeof(uint32_t);
    createInfo.pCode    = spirv.data();

    Check(vkCreateShaderModule(device->Handle(), &createInfo, nullptr, &shaderModule));

    Reflect(src, resources);

    return shaderModule;
}

#define SKIP_IF(c, d, preprocess) if ((c) == (d)) \
    { \
        preprocess; \
        continue; \
    }

#define BREAK_IF(c, d, preprocess) if ((c) == (d)) \
    { \
        preprocess; \
        break; \
    }

inline size_t ParseKeyValue(const char *ptr, Shader::Resource &resource)
{
    char buffer[64] = { 0 };
    size_t i = 0, j = 0;

    while (ptr[i])
    {
        SKIP_IF(ptr[i], ' ', i++);

        if (ptr[i] == '=')
        {
            buffer[j++] = '\0';
            i++;
            continue;
        }

        if (ptr[i] == ',' || ptr[i] == ')')
        {
            buffer[j] = '\0';
            j = 0;
            if (Equals(buffer, "set"))
            {
                resource.set = std::atoi(buffer + 4);
            }
            else if (Equals(buffer, "location"))
            {
                resource.location = std::atoi(buffer + 9);
            }
            else if (Equals(buffer, "binding"))
            {
                resource.binding = std::atoi(buffer + 8);
            }
            else if (Equals(buffer, "push_constant"))
            {
                resource.type |= Shader::Resource::Type::PushConstant;
            }
            BREAK_IF(ptr[i], ')', );
            i++;
            continue;
        }

        buffer[j++] = ptr[i++];
    }

    return i;
}

inline size_t ParseStructure(const char *ptr, Shader::Resource &resource)
{
    size_t i = 0, j = 0;
    char buffer[64] = { 0 };

    std::vector<InputElement> elements;
    
    InputElement e;
    while (ptr[i] != '}')
    {
        SKIP_IF(ptr[i], '\t', i++);
        SKIP_IF(ptr[i], '\r', i++);
        SKIP_IF(ptr[i], '\n', i++);
        if (ptr[i] == ' ')
        {
            if (j > 0)
            {
                buffer[j] = '\0';
                if (e.format != Format::None)
                {
                    e.format = Shader::GetResourceFormat(buffer);
                }
                j = 0;
            }
            i++;
            continue;
        }
        if (ptr[i] == ';')
        {
            buffer[j] = '\0';
            e.name = buffer;
            elements.emplace_back(std::move(e));
            j = 0;
            i++;
            continue;
        }

        buffer[j++] = ptr[i++];
    }

    InputElementDescription desc{ std::move(elements) };
    resource.size = desc.Stride();

    return i;
}

inline void GetType(const char *word, Shader::Resource &resource)
{
    auto type = Shader::GetResourceType(std::string{ word });
    if (type != Shader::Resource::Type::None)
    {
        resource.type = type;
    }
}

inline void GetFormat(const char *word, Shader::Resource &resource)
{
    auto format = Shader::GetResourceFormat(std::string{ word });
    if (format != Format::None)
    {
        resource.format = format;
    }
}

inline size_t ParseLayout(const char *ptr, Shader::Resource &resource)
{
    size_t i = 0, j = 0;
    std::stack<uint8_t> tokens;
    char buffer[128] = { 0 };

    while (*ptr)
    {
        if (ptr[i] == ' ')
        {
            if (j > 0)
            {
                j = 0;
                GetType(buffer, resource);
                GetFormat(buffer, resource);
            }
            i++;
            continue;
        }
        SKIP_IF(ptr[i], '\n', i++);
        if (ptr[i] == '(')
        {
            tokens.push(ptr[i++]);
            i += ParseKeyValue(ptr + i, resource);
            continue;
        }
        if (ptr[i] == '{')
        {
            tokens.push(ptr[i++]);
            i += ParseStructure(ptr + i, resource);
            continue;
        }
        if (ptr[i] == ')')
        {
            if (tokens.top() == '(')
            {
                tokens.pop();
            }
            i++;
            continue;
        }
        if (ptr[i] == '}')
        {
            if (tokens.top() == '{')
            {
                tokens.pop();
            }
            i++;
            continue;
        }
        BREAK_IF(ptr[i], ';', buffer[j++] = '\0'; i++; resource.name = buffer);
        buffer[j++] = ptr[i++];
    }

    if (!tokens.empty())
    {
        LOG::ERR("Failed to reflect the shader resources");
    }

    return i;
}

void Shader::Reflect(const std::string &source, std::vector<Shader::Resource> &resources)
{
    const std::string layout = std::string{ "layout" };

    size_t i = 0;
    while ((i = source.find(layout, i)) != std::string::npos)
    {
        resources.resize(resources.size() + 1);
        i += layout.size();
        i += ParseLayout(source.data() + i, resources.back());
        continue;
    }
}

}
}

#include "impch.h"

#include "Shader.h"
#include "Render.h"

#include "Platform/OpenGL/OpenGLShader.h"

namespace Immortal
{
    Ref<Shader> Shader::Create(const std::string &filepath)
    {
        return InstantiateGrphicsPrimitive<Shader, OpenGLShader, OpenGLShader, OpenGLShader>(filepath);
    }

    Ref<Shader> Shader::Create(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc)
    {
        return InstantiateGrphicsPrimitive<Shader, OpenGLShader, OpenGLShader, OpenGLShader>(name, vertexSrc, fragmentSrc);
    }

    void ShaderMap::Add(const std::string &name, const Ref<Shader> &shader)
    {
        SLASSERT(!Exists(name) && "Shader already exists!");
        mShaders[name] = shader;
    }

    void ShaderMap::Add(const Ref<Shader> &shader)
    {
        auto name = shader->Name();
        SLASSERT(!Exists(name) && "Shader already exists!");
        mShaders[name] = shader;
    }

    Ref<Shader> ShaderMap::Load(const std::string &filepath)
    {
        auto shader = Shader::Create(filepath);
        Add(shader);
        return shader;
    }

    Ref<Shader> ShaderMap::Load(const std::string & name, const std::string & filepath)
    {
        auto shader = Shader::Create(filepath);
        Add(name, shader);
        return shader;
    }

    Ref<Shader> ShaderMap::Get(const std::string &name)
    {
        SLASSERT(Exists(name) && "Shader not found!");
        return mShaders[name];
    }

    bool ShaderMap::Exists(const std::string &name) const
    {
        return mShaders.find(name) != mShaders.end();
    }
}

#pragma once

#include "Entity.h"
#include "Interface/Delegate.h"

namespace Immortal
{

class __declspec(dllexport) ScriptableObject
{
public:
    virtual ~ScriptableObject() { }

    template <class T>
    T &GetComponent()
    {
        return mEntity.GetComponent<T>();
    }

    template <class T, class... Args>
    T &AddComponent(Args&&... args)
    {
        return mEntity.AddComponent<T>(std::forward<Args>(args)...);
    }

protected:
    virtual void OnStart() { }
    virtual void OnDestroy() { }
    virtual void OnUpdate() { }

public:
    Entity mEntity;
    friend class Scene;
    friend struct NativeScriptComponent;
};

using GameObject = ScriptableObject;

struct NativeScriptComponent : public Component
{
    enum class Status : int
    {
        Ready,
        NotLoaded
    };
    std::string Module;
    std::shared_ptr<Delegate<void()>> delegate;
    NativeScriptComponent::Status Status;

    void Map(Entity e, ScriptableObject *script)
    {
        script->mEntity = e;
        script->OnStart();
        delegate->Map<ScriptableObject, &ScriptableObject::OnUpdate>(script);
    }

    NativeScriptComponent() : Component(Component::Script), Status(NativeScriptComponent::Status::NotLoaded)
    {
        delegate = std::make_shared<Delegate<void()>>();
    }

    NativeScriptComponent(const std::string &module)
        : Component(Component::Script), Module(module), Status(NativeScriptComponent::Status::NotLoaded)
    {
        delegate = std::make_shared<Delegate<void()>>();
    }

    void OnRuntime()
    {
        delegate->Invoke();
    }

    ~NativeScriptComponent()
    {

    }
};
}

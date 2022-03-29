#pragma once
#include "Visitor.h"

class BaseCamera;
class EffectPSO;
class CommandList;

class SceneVisitor :
    public Visitor
{
public:
    SceneVisitor(CommandList& _commandList, const BaseCamera& _camera, EffectPSO& _pso, bool _transparent);

    virtual void Visit(Mesh& _mesh) override;
    virtual void Visit(SceneNode& sceneNode) override;
    virtual void Visit(Scene& scene) override;

private:
    CommandList& m_CommandList;
    const BaseCamera& m_Camera;
    EffectPSO& m_LightingPSO;
    bool m_Transparent;

};


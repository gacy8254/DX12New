#pragma once
#include "Visitor.h"

class Camera;
class EffectPSO;
class CommandList;

class SceneVisitor :
    public Visitor
{
public:
    SceneVisitor(CommandList& _commandList, const Camera& _camera, EffectPSO& _pso, bool _transparent);

    virtual void Visit(Mesh& _mesh) override;
    virtual void Visit(SceneNode& sceneNode) override;
    virtual void Visit(Scene& scene) override;

private:
    CommandList& m_CommandList;
    const Camera& m_Camera;
    EffectPSO& m_LightingPSO;
    bool m_Transparent;

};


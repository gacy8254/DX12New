#pragma once
#include "Visitor.h"

class BaseCamera;
class BasePSO;
class CommandList;

class SceneVisitor :
    public Visitor
{
public:
    SceneVisitor(CommandList& _commandList, const BaseCamera& _camera, BasePSO& _pso, bool _transparent);

    virtual void Visit(Actor& _actor) override;
    virtual void Visit(SceneNode& sceneNode) override;
    virtual void Visit(Scene& scene) override;

private:
    CommandList& m_CommandList;
    const BaseCamera& m_Camera;
    BasePSO& m_LightingPSO;
    bool m_Transparent;

};


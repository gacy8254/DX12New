#pragma once
#include "Visitor.h"
#include "BasePSO.h"

class BaseCamera;
class CommandList;
class Window;
class RenderTarget;

class SceneVisitor :
    public Visitor
{
public:
    SceneVisitor(CommandList& _commandList, const BaseCamera& _camera, BasePSO& _pso, Window& _window, RenderTarget& _rt, bool _transparent);
    virtual ~SceneVisitor();
    virtual void Visit(Actor& _actor) override;
    virtual void Visit(SceneNode& sceneNode) override;
    virtual void Visit(Scene& scene) override;

private:
    CommandList& m_CommandList;
    const BaseCamera& m_Camera;
    BasePSO& m_LightingPSO;
    Window& m_Window;
    RenderTarget& m_RenderTarget;
    bool m_Transparent;

	std::shared_ptr<ObjectCB> m_ObjectCB;
	std::shared_ptr<MainPass> m_MainPassCB;
};


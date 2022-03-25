#pragma once

class Scene;
class SceneNode;
class Mesh;


class Visitor
{
public:
	Visitor() = default;
	virtual ~Visitor() = default;

	virtual void Visit(Scene& _scene) = 0;
	virtual void Visit(SceneNode& _sceneNode) = 0;
	virtual void Visit(Mesh& _mesh) = 0;
};


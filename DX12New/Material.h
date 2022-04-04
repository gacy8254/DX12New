#pragma once
#include <DirectXMath.h>
#include <memory>
#include <map>
#include "Vector.h"

class Texture;

struct alignas(16) MaterialProperties
{
	// The Material properties must be aligned to a 16-byte boundary.
	// To guarantee alignment, the MaterialProperties structure will be allocated in aligned memory.
	MaterialProperties(
		const DirectX::XMFLOAT4 diffuse = { 1, 1, 1, 1 },
		const DirectX::XMFLOAT4 ORM = { 1, 1, 0, 1 },
		const float specularPower = 128.0f,
		const DirectX::XMFLOAT4 ambient = { 0, 0, 0, 1 },
		const DirectX::XMFLOAT4 emissive = { 0, 0, 0, 1 },
		const DirectX::XMFLOAT4 reflectance = { 0, 0, 0, 0 }, const float opacity = 1.0f,
		const float indexOfRefraction = 0.0f, const float bumpIntensity = 1.0f,
		const float alphaThreshold = 0.1f
	)
		: Diffuse(diffuse)
		, ORM(ORM)
		, Emissive(emissive)
		, Ambient(ambient)
		, Reflectance(reflectance)
		, Opacity(opacity)
		, SpecularPower(specularPower)
		, IndexOfRefraction(indexOfRefraction)
		, BumpIntensity(bumpIntensity)
		, HasAOTexture(false)
		, HasEmissiveTexture(false)
		, HasDiffuseTexture(false)
		, HasMetalticTexture(false)
		, HasRoughnessTexture(false)
		, HasNormalTexture(false)
		, HasBumpTexture(false)
		, HasOpacityTexture(false)
	{}

	DirectX::XMFLOAT4 Diffuse;
	//------------------------------------ ( 16 bytes )
	DirectX::XMFLOAT4 ORM;
	//------------------------------------ ( 16 bytes )
	DirectX::XMFLOAT4 Emissive;
	//------------------------------------ ( 16 bytes )
	DirectX::XMFLOAT4 Ambient;
	//------------------------------------ ( 16 bytes )
	DirectX::XMFLOAT4 Reflectance;
	//------------------------------------ ( 16 bytes )
	float Opacity;                       // If Opacity < 1, then the material is transparent.
	float SpecularPower;
	float IndexOfRefraction;             // For transparent materials, IOR > 0.
	float BumpIntensity;                 // When using bump textures (height maps) we need
										 // to scale the height values so the normals are visible.
	//------------------------------------ ( 16 bytes )
	uint32_t HasAOTexture;
	uint32_t HasEmissiveTexture;
	uint32_t HasDiffuseTexture;
	uint32_t HasMetalticTexture;
	//------------------------------------ ( 16 bytes )
	uint32_t HasRoughnessTexture;
	uint32_t HasNormalTexture;
	uint32_t HasBumpTexture;
	uint32_t HasOpacityTexture;
	//------------------------------------ ( 16 bytes )
	// Total:                              ( 16 * 8 = 128 bytes )
};

class Material
{
public:
	enum class TextureType
	{
		AO,
		Emissive,
		Diffuse,
		Metaltic,
		Roughness,
		Normal,
		Bump,
		Opacity,
		NumTypes,
	};

	Material(const MaterialProperties& _materialProperties = MaterialProperties());
	Material(const Material& _copy);

	~Material() = default;

	const Vector4& GetAmbientColor() const;
	void                     SetAmbientColor(Vector4 ambient);

	const Vector4& GetDiffuseColor() const;
	void                     SetDiffuseColor(Vector4 diffuse);

	const Vector4& GetEmissiveColor() const;
	void                     SetEmissiveColor( Vector4 emissive);

	const Vector4& GetORMColor() const;
	void                     SetORMColor( Vector4 specular);

	float GetSpecularPower() const;
	void  SetSpecularPower(float specularPower);

	const Vector4& GetReflectance() const;
	void                     SetReflectance(Vector4 reflectance);

	const float GetOpacity() const;
	void        SetOpacity(float opacity);

	float GetIndexOfRefraction() const;
	void  SetIndexOfRefraction(float indexOfRefraction);

	float GetBumpIntensity() const;
	void  SetBumpIntensity(float bumpIntensity);

	std::shared_ptr<Texture> GetTexture(TextureType ID) const;
	void                     SetTexture(TextureType type, std::shared_ptr<Texture> texture);

	// This material defines a transparent material
	// if the opacity value is < 1, or there is an opacity map, or the diffuse texture has an alpha channel.
	bool IsTransparent() const;

	const MaterialProperties& GetMaterialProperties() const;
	void                      SetMaterialProperties(const MaterialProperties& materialProperties);

	bool IsDirty() const { return m_MaterialDirty; }
	void SetDirty(bool _v) { m_MaterialDirty = _v; }

    // Define some interesting materials.
	static const MaterialProperties Zero;
	static const MaterialProperties Red;
	static const MaterialProperties Green;
	static const MaterialProperties Blue;
	static const MaterialProperties Cyan;
	static const MaterialProperties Magenta;
	static const MaterialProperties Yellow;
	static const MaterialProperties White;
	static const MaterialProperties WhiteDiffuse;
	static const MaterialProperties Black;
	static const MaterialProperties Emerald;
	static const MaterialProperties Jade;
	static const MaterialProperties Obsidian;
	static const MaterialProperties Pearl;
	static const MaterialProperties Ruby;
	static const MaterialProperties Turquoise;
	static const MaterialProperties Brass;
	static const MaterialProperties Bronze;
	static const MaterialProperties Chrome;
	static const MaterialProperties Copper;
	static const MaterialProperties Gold;
	static const MaterialProperties Silver;
	static const MaterialProperties BlackPlastic;
	static const MaterialProperties CyanPlastic;
	static const MaterialProperties GreenPlastic;
	static const MaterialProperties RedPlastic;
	static const MaterialProperties WhitePlastic;
	static const MaterialProperties YellowPlastic;
	static const MaterialProperties BlackRubber;
	static const MaterialProperties CyanRubber;
	static const MaterialProperties GreenRubber;
	static const MaterialProperties RedRubber;
	static const MaterialProperties WhiteRubber;
	static const MaterialProperties YellowRubber;

private:
	using TextureMap = std::map<TextureType, std::shared_ptr<Texture>>;
	// A unique pointer with a custom allocator/deallocator to ensure alignment.
	using MaterialPropertiesPtr = std::unique_ptr<MaterialProperties, void (*)(MaterialProperties*)>;

	MaterialPropertiesPtr m_MaterialProperties;
	TextureMap            m_Textures;

	bool m_MaterialDirty = true;
};
